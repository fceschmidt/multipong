#include "Network.h"
#include "Debug/Debug.h"
#include <SDL2/SDL_net.h>
#include <string.h>
#include <stdio.h>

// DEFINITIONS

#define ERROR_TCP_SOCKET_CREATION_FAILED -1
#define PLAYER_NAME "fabian"
#define MAX_PLAYER_NAME_LENGTH 40

// TYPES

/*
==========================================================

A struct which saves information about a player in the network context.
The socket is NULL for every player unless isServer is true. 

==========================================================
*/
struct NetworkClientInfo {
	char *				alias;
	TCPsocket			socket;
	SDLNet_SocketSet	socketSet;
	TCPsocket 			dataSocket;
	SDLNet_SocketSet 	dataSocketSet;
};

/*
==========================================================

A list of valid packet IDs with their explanations.
A packet on the active socket looks like this:
	Name	Length	Description
	PID		4		The type of packet
	Len		4		The length of the whole packet in bytes
	Args	Len-8	The arguments, depending on the packet
					type.
A packet on the data socket only contains either a serialized
GameState (when sent by the server) or a serialized paddle
position float (when sent by a client).

==========================================================
*/
enum PacketId {
	PID_QUIT,		// Client or server quits.
	PID_JOIN,		// New client has joined
	PID_YOUR_ID,	// Tells a new client their ID
	PID_MY_NAME,	// Tells the server the client's name
	PID_START_GAME,	// Starts the game (+data_port)
	PID_BALL_HIT,	// Registers a ball hit (+player_id)
	PID_SCORE		// New score (+player_id+score)
};

// VARIABLES

static int 						isServer = 0;			// A boolean value which is 1 if this component is a server and 0 if it is a client.
static int 						isConnected = 0;		// A boolean value which indicates whether the component is currently connected.
static int 						isInitialized = 0;		// A boolean value indicating initialization state.

static TCPsocket 				activeSocket = NULL;	// The current socket for state & meta information
static SDLNet_SocketSet			activeSocketSet = NULL;	// The associated socket set for asio
static TCPsocket 				dataSocket = NULL;		// The current socket for exchange of in-game information
static SDLNet_SocketSet 		dataSocketSet = NULL;	// The associated socket set for asio

static struct NetworkClientInfo clients[6];				// 6 players maximum, eh?!
static int						numClients = 0;			// The amount of filled in elements of the clients array
static int						thisClient = -1;		// The index of this client in the clients array

// FUNCTIONS

static int	ConnectLocalServer( TCPsocket *socket, uint16_t localPort );
static int	ConnectRemoteServer( TCPsocket *socket, SDLNet_SocketSet *socketSet, const char *remoteAddress, uint16_t remotePort );
static int	ConnectRemoteServerIp( TCPsocket *socket, SDLNet_SocketSet *socketSet, IPaddress *remoteAddress );
static int	NonBlockingRecv( TCPsocket socket, SDLNet_SocketSet set, char *data, int maxlen );

static int	BroadcastPacketToClients( const void *data, int length );

static int	AddPlayer( struct NetworkClientInfo client );
static void	RemovePlayer( int playerId );

static int	ProcessLobbyServer( void );
static void	ServerHandleClientQuit( int playerId );
static void	ServerHandleClientMyName( int playerId, char *name );
static void	IssueAllJoins( TCPsocket socket );

static int	ProcessLobbyClient( void );
static void	ClientHandleClientJoin( int playerId, char *name );
static void	ClientHandleServerYourId( int newId );
static void	ClientHandleServerStartGame();

static int	ProcessInGameServer( struct GameState *state );
static int	ProcessInGameClient( struct GameState *state );

// These two functions are accessed by the physics component... dont' make them static!
int			IsServer( void );
int			ThisClient( void );

extern void	GetUserName( char* Name );

/*
====================
InitializeNetwork

Sets up the network component.
====================
*/
int InitializeNetwork( void ) {
	DebugPrintF( "InitializeNetwork called." );	
	int result;

	// Initialize SDL_net and return on error.
	result = SDLNet_Init();
	DebugAssert( !result );
	if( result ) {
		return result;
	}

	// Set clients to zero.
	memset( clients, 0, sizeof( struct NetworkClientInfo ) * 6 );
	numClients = 0;

	// Set isInitialized to value representing true.
	isInitialized = 1;

	return 0;
}

/*
====================
CloseNetwork

Finally cleans up the network component.
====================
*/
void CloseNetwork( void ) {
	// Reset isInitialized to false.
	isInitialized = 0;

	// Destroy SDL_net.
	SDLNet_Quit();
}

/*
====================
Connect

Connects the network component.
	server			If this argument is TRUE, the component is initialized as a
					server on the port given by remotePort, while remoteAddress
					is ignored.
	remoteAddress	The IP or host name of the server to which the client should
					connect.
	port			The port on which the server should listen / to which the
					client should connect.
====================
*/
int Connect( int server, const char *remoteAddress, uint16_t port ) {
	DebugPrintF( "Connect( %d, %s, %d ) called.", server, remoteAddress ? remoteAddress : "", port );	
	// If the network component is not initialized, return error.
	if( !isInitialized ) {
		return -1;
	}

	isServer = server;
	// Branch for server/client.
	if( server ) {
		if( !ConnectLocalServer( &activeSocket, port ) ) {
			struct NetworkClientInfo clientInfo;
			clientInfo.alias = malloc( MAX_PLAYER_NAME_LENGTH );
			GetUserName( clientInfo.alias );
			clientInfo.socket = NULL;
			clientInfo.socketSet = NULL;
			clientInfo.dataSocket = NULL;
			clientInfo.dataSocketSet = NULL;
			AddPlayer( clientInfo );
			return 0;
		} else {
			return -1;
		}
	} else {
		return ConnectRemoteServer( &activeSocket, &activeSocketSet, remoteAddress, port );
	}
}

/*
====================
ConnectLocalServer

Creates a server to listen for incoming connections.
====================
*/
static int ConnectLocalServer( TCPsocket *socket, uint16_t localPort ) {
	// Create a listening TCP socket on localPort
	IPaddress 	ip;
	TCPsocket 	listener;
	int			result;

	result = SDLNet_ResolveHost( &ip, NULL, localPort );
	if( result ) {
		return result;
	}

	listener = SDLNet_TCP_Open( &ip );
	if( !listener ) {
		return ERROR_TCP_SOCKET_CREATION_FAILED;
	}

	isConnected = 1;
	*socket = listener;

	return 0;
}

/*
====================
ConnectRemoteServer

Creates a socket that communicates with a remote server.
====================
*/
static int ConnectRemoteServer( TCPsocket *socket, SDLNet_SocketSet *socketSet, const char *remoteAddress, uint16_t remotePort ) {
	IPaddress	ip;
	int			result;

	result = SDLNet_ResolveHost( &ip, remoteAddress, remotePort );
	DebugAssert( result );
	if( result ) {
		return result;
	}

	return ConnectRemoteServerIp( socket, socketSet, &ip );
}

/*
====================
ConnectRemoteServerIp

Connects to a remote server using an IPaddress pointer.
====================
*/
static int ConnectRemoteServerIp( TCPsocket *socket, SDLNet_SocketSet *socketSet, IPaddress *remoteAddress ) {
	TCPsocket	client;
	
	client = SDLNet_TCP_Open( remoteAddress );
	if( !client ) {
		return ERROR_TCP_SOCKET_CREATION_FAILED;
	}

	isConnected = 1;
	*socket = client;

	*socketSet = SDLNet_AllocSocketSet( 1 );
	SDLNet_TCP_AddSocket( *socketSet, client );

	return 0;
}

/*
====================
Disconnect

Disconnects the network component's sockets.
====================
*/
void Disconnect( void ) {
	if( activeSocket ) {
		SDLNet_TCP_Close( activeSocket );
	}
	isConnected = 0;
	memset( clients, 0, sizeof( struct NetworkClientInfo ) * 6 );
	numClients = 0;
}

/*
====================
ProcessLobby

Process packets in the lobby.
====================
*/
int ProcessLobby( void ) {
	if( !isInitialized ) {
		return -1;
	}
	if( !isConnected ) {
		return -1;
	}

	if( isServer ) {
		return ProcessLobbyServer();
	} else {
		return ProcessLobbyClient();
	}
}

/*
====================
ProcessInGame

Process packets inside the game.
====================
*/
int ProcessInGame( struct GameState *state ) {
	if( !isInitialized ) {
		return -1;
	}
	if( !isConnected ) {
		return -1;
	}

	if( isServer ) {
		return ProcessInGameServer( state );
	} else {
		return ProcessInGameClient( state );
	}
}

/*
====================
NonBlockingRecv

Receives from the socket in a non-blocking fashion.
====================
*/
static int NonBlockingRecv( TCPsocket socket, SDLNet_SocketSet set, char *data, int maxlen ) {
	int bPosition = 0;
	while( SDLNet_CheckSockets( set, 0 ) > 0 && bPosition < maxlen ) {
		if( SDLNet_SocketReady( socket ) ) {
			if( SDLNet_TCP_Recv( socket, &data[bPosition++], 1 ) == -1 ) {
				return -1;
			}
		}
	}
	return bPosition;
}

/*
====================
ProcessLobbyServer

Accepts incoming connections, receives information packets from clients and
informs all clients about the new client.
====================
*/
static int ProcessLobbyServer( void ) {
	// Accept new clients!
	TCPsocket newClient = NULL;
	newClient = SDLNet_TCP_Accept( activeSocket );

	if( newClient ) {
		DebugPrintF( "A new client has connected." );
		struct NetworkClientInfo clientInfo;
		clientInfo.alias = NULL;
		clientInfo.socketSet = SDLNet_AllocSocketSet( 1 );
		clientInfo.socket = newClient;
		clientInfo.dataSocket = NULL;
		clientInfo.dataSocketSet = NULL;
		SDLNet_TCP_AddSocket( clientInfo.socketSet, newClient );
		// Tell client about the rest of the world.
		IssueAllJoins( newClient );
		int newId = AddPlayer( clientInfo );
		int packet[3];
		SDLNet_Write32( ( int )PID_YOUR_ID,	&packet[0] );
		SDLNet_Write32( 12, 				&packet[1] );
		SDLNet_Write32( newId,				&packet[2] );
		SDLNet_TCP_Send( clients[newId].socket, packet, sizeof( packet ) );
	}

	// Process other clients' packets!
	int client;
	int numBytes;
	char bytes[1024];
	int bReadPosition = 0;
	int endOfStream;
	int stringLength;
	char *name;

	// Go through the clients
	for( client = 0; client < numClients; client++ ) {
		// If there is a socket (so, not this client)
		if( !clients[client].socket ) {
			continue;
		}
		// Reset endofstream and receive bytes from this client
		endOfStream = 0;
		numBytes = NonBlockingRecv( clients[client].socket, clients[client].socketSet, bytes, sizeof( bytes ) );
		// while there is something to read from and the stream hasn't ended yet
		while( numBytes > 0 ) {
			DebugPrintF( "Received %d bytes.", numBytes );
			while( !endOfStream ) {
				switch( SDLNet_Read32( &bytes[bReadPosition] ) ) {
					case PID_QUIT:
						// Handle the quit
						ServerHandleClientQuit( client );
						break;
					case PID_MY_NAME:
						// Determine length of name and call handler with the length.
						stringLength = SDLNet_Read32( &bytes[bReadPosition + 4] ) - 8;
						name = malloc( stringLength + 1 );
						strncpy( name, &bytes[bReadPosition + 8], stringLength );
						name[stringLength] = '\0';
						DebugPrintF( "Client #%d is now called %s.", client, name );
						ServerHandleClientMyName( client, name );
						//return 10;
						break;
				}
				bReadPosition += SDLNet_Read32( &bytes[bReadPosition + 4] );
				if( bReadPosition >= numBytes ) {
					endOfStream = 1;
				}
			}
			numBytes = NonBlockingRecv( clients[client].socket, clients[client].socketSet, bytes, sizeof( bytes ) );
			bReadPosition = 0;
		}
	}
	return 0;
}

/*
====================
ServerHandleClientQuit

Handles when a client quits the game.
====================
*/
static void ServerHandleClientQuit( int playerId ) {
	// Remove the player from the list and ask all other clients to do the same.
	RemovePlayer( playerId );
	int packet[3];
	SDLNet_Write32( ( int )PID_QUIT, 	&packet[0] );
	SDLNet_Write32( 12, 				&packet[1] );
	SDLNet_Write32( playerId,			&packet[2] );
	BroadcastPacketToClients( packet, sizeof( packet ) );
}

/*
====================
ServerHandleClientMyName

Handles when a client announces a new name. Adds the name to the client list and
broadcasts the packet to all other clients.
====================
*/
static void ServerHandleClientMyName( int playerId, char *name ) {
	clients[playerId].alias = name;
	char *packet = malloc( 12 + strlen( name ) );
	SDLNet_Write32( ( int )PID_JOIN,		&packet[0] );
	SDLNet_Write32( 12 + strlen( name ),	&packet[4] );
	SDLNet_Write32( playerId,				&packet[8] );
	strncpy( &packet[12], name, strlen( name ) );
	BroadcastPacketToClients( packet, 12 + strlen( name ) );
	free( packet );
}

/*
====================
IssueAllJoins

Sends all joins from the beginning of the client list to a specified TCP socket.
====================
*/
void IssueAllJoins( TCPsocket socket ) {
	int client;
	char *packet;
	for( client = 0; client < numClients; client++ ) {
		int packetSize = 12;
		packet = malloc( packetSize );
		if( clients[client].alias ) {
			free( packet );
			packetSize += strlen( clients[client].alias );
			packet = malloc( packetSize );
			strncpy( &packet[12], clients[client].alias, packetSize - 12 );
		}
		SDLNet_Write32( ( int )PID_JOIN,	&packet[0] );
		SDLNet_Write32( packetSize,			&packet[4] );
		SDLNet_Write32( client,				&packet[8] );
		
		SDLNet_TCP_Send( socket, packet, packetSize );

		free( packet );
	}
}

/*
====================
BroadcastPacketToClients

Takes a packet and broadcasts it to all connected clients (server only)
====================
*/
static int BroadcastPacketToClients( const void *data, int length ) {
	int client;
	for( client = 0; client < numClients; client++ ) {
		if( clients[client].socket ) {
			DebugPrintF( "Broadcasting a %d packet.", SDLNet_Read32( data ) );
			SDLNet_TCP_Send( clients[client].socket, data, length );
		}
	}
	return 0;
}

/*
====================
ProcessLobbyClient

Answers server packets with PID_MY_NAME, PID_QUIT and accepts packets like
PID_JOIN, PID_YOUR_ID and PID_START_GAME
====================
*/
static int ProcessLobbyClient( void ) {
	char 	bytes[1024];
	int		numBytes = NonBlockingRecv( activeSocket, activeSocketSet, bytes, sizeof( bytes ) );
	int		endOfStream = 0;
	int		bReadPosition = 0;

	int 	playerId;
	int		stringLength;
	char *	alias;
	int		newId;
	int		port;

	while( numBytes > 0 ) {
		while( !endOfStream ) {
			switch( SDLNet_Read32( &bytes[bReadPosition] ) ) {
				case PID_JOIN:
					playerId = SDLNet_Read32( &bytes[bReadPosition + 8] );
					stringLength = SDLNet_Read32( &bytes[bReadPosition + 4] ) - 12;
					alias = malloc( stringLength + 1 );
					strncpy( alias, &bytes[bReadPosition + 12], stringLength );
					alias[stringLength] = '\0';
					DebugPrintF( "Client #%d joined, his name is %s.", playerId, alias );
					ClientHandleClientJoin( playerId, alias );
					break;
				case PID_YOUR_ID:
					newId = SDLNet_Read32( &bytes[bReadPosition + 8] );
					DebugPrintF( "I am now client #%d.", newId );
					ClientHandleServerYourId( newId );
					break;
				case PID_START_GAME:
					port = SDLNet_Read32( &bytes[bReadPosition + 8] );
					DebugPrintF( "The game starts now on port %d.", port );
					ClientHandleServerStartGame( port );
					return GAME_START;
					break;
			}
			bReadPosition += SDLNet_Read32( &bytes[bReadPosition + 4] );
			if( bReadPosition >= numBytes ) {
				endOfStream = 1;
			}
		}
		numBytes = NonBlockingRecv( activeSocket, activeSocketSet, bytes, sizeof( bytes ) );
		bReadPosition = 0;
	}

	return 0;
}

/*
====================
ClientHandleClientJoin

Handles when another client joins the game
====================
*/
static void ClientHandleClientJoin( int playerId, char *name ) {
	if( numClients <= playerId ) {
		numClients = playerId + 1;
	}
	clients[playerId].alias = name;
}

/*
====================
ClientHandleServerYourId

Handles when a server tells this client its ID.
====================
*/
static void ClientHandleServerYourId( int newId ) {
	thisClient = newId;
	char *packet;
	char *alias = malloc( MAX_PLAYER_NAME_LENGTH );
	GetUserName( alias );
	packet = malloc( 8 + strlen( alias ) );
	SDLNet_Write32( ( int )PID_MY_NAME, 	&packet[0] );
	SDLNet_Write32( 8 + strlen( alias ), 	&packet[4] );
	strncpy( packet + 8, alias, strlen( alias ) );
	SDLNet_TCP_Send( activeSocket, packet, 8 + strlen( alias ) );
	DebugPrintF( "I gave the server my name." );
}

/*
====================
ClientHandleServerStartGame

Handles when the server starts the game.
====================
*/
static void ClientHandleServerStartGame() {
	// TODO: Implement this functionality.
	IPaddress *address = SDLNet_TCP_GetPeerAddress( activeSocket );
	DebugAssert( address );
	
	address->port = NETWORK_STANDARD_DATA_PORT;
	ConnectRemoteServerIp( &dataSocket, &dataSocketSet, address );
}

/*
====================
AddPlayer

Adds a NetworkClientInfo to the clients array.
====================
*/
static int AddPlayer( struct NetworkClientInfo client ) {
	int newIndex = numClients;
	if( numClients == 6 ) {
		return -1;
	}

	DebugPrintF( "Adding Player %s.", client.alias );

	if( numClients == -1 ) {
		numClients = 0;
	}
	clients[numClients++] = client;
	return newIndex;
}

/*
====================
RemovePlayer

Removes a player from the clients array.
====================
*/
static void RemovePlayer( int playerId ) {
	int n;
	for( n = playerId; n < numClients - 1; n++ ) {
		clients[n] = clients[n + 1];
	}
	numClients--;
}

/*
====================
GetPlayerList

Returns a list of player names into players.
====================
*/
int GetPlayerList( playerInfo_t *players, int *numPlayers ) {
	*numPlayers = numClients;
	if( !players ) {
		return -1;
	}

	int client;
	for( client = 0; client < numClients; client++ ) {
		players[client] = clients[client].alias;
	}
	return 0;
}

/*
====================
NetworkStartGame

For use by the server only, broadcasts the packet that starts the game.
====================
*/
int NetworkStartGame( uint16_t udpPort ) {
	// Prepare data port
	ConnectLocalServer( &dataSocket, NETWORK_STANDARD_DATA_PORT );
	
	// Broadcast connection info
	char packet[12];
	SDLNet_Write32( ( int )PID_START_GAME,	&packet[0] );
	SDLNet_Write32( sizeof( packet ),		&packet[4] );
	SDLNet_Write32( udpPort,				&packet[8] );
	BroadcastPacketToClients( packet, 12 );

	// Accept all clients for the data port now.
	// Wait until we have all clients.
	int 		dataClients = 1;
	int 		i;
	IPaddress *	info;
	IPaddress *	newInfo;
	while( dataClients < numClients ) {
		TCPsocket newClient = NULL;
		newClient = SDLNet_TCP_Accept( dataSocket );

		if( newClient ) {
			newInfo = SDLNet_TCP_GetPeerAddress( newClient );
			for( i = 0; i < numClients; i++ ) {
				info = SDLNet_TCP_GetPeerAddress( clients[i].socket );
				if( !info ) {
					continue;
				}
				if( newInfo->host != info->host ) {
					continue;
				}

				clients[i].dataSocketSet = SDLNet_AllocSocketSet( 1 );
				clients[i].dataSocket = newClient;
				SDLNet_TCP_AddSocket( clients[i].dataSocketSet, newClient );
				dataClients++;
			}
		}
	}

	return 0;
}

/*
====================
ProcessInGameClient

The in-game client loop.
====================
*/
static int ProcessInGameClient( struct GameState *state ) {
	// Null pointer check
	if( !state || !isInitialized || !isConnected  ) {
		return -1;
	}

	// TODO: Broadcast own information from state (dataSocket)
	// TODO: Update foreign and ball information into state (dataSocket)
	// TODO: Register hits and score lists from activeSocket
	
	return 0;
}

/*
====================
ProcessInGameServer

The in-game server loop.
====================
*/
static int ProcessInGameServer( struct GameState *state ) {
	// Null pointer check
	if( !state || !isInitialized || !isConnected ) {
		return -1;
	}

	// TODO: Update own information from clients (dataSocket)
	// TODO: Broadcast complete GameState information (dataSocket)
	// TODO: Broadcast hits and score lists (activeSocket)

	return 0;
}

/*
====================
IsServer

Determines whether the component is connected as a server.
====================
*/
int IsServer( void ) {
	return isServer;
}

/*
====================
ThisClient

Returns the index of this client in the GameState player array.
====================
*/
int ThisClient( void ) {
	return thisClient;
}
