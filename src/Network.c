#include "Network.h"
#include <SDL2/SDL_net.h>
#include <string.h>
#include <stdio.h>

#define ERROR_TCP_SOCKET_CREATION_FAILED -1
#define PLAYER_NAME "player"

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
};

/*
==========================================================

A list of valid packet IDs with their explanations.

==========================================================
*/
enum PacketId {
	PID_QUIT,		// Client or server quits.
	PID_JOIN,		// New client has joined
	PID_YOUR_ID,	// Tells a new client their ID
	PID_MY_NAME,	// Tells the server the client's name
	PID_START_GAME,	// Starts the game (+udp_port)
	PID_BALL_HIT,	// Registers a ball hit (+player_id)
	PID_SCORE		// New score (+player_id+score)
};

// A boolean value which is 1 if this component is a server and 0 if it is a client.
static int isServer = 0;
// A boolean value which indicates whether the component is currently connected.
static int isConnected = 0;
// A boolean value indicating initialization state.
static int isInitialized = 0;

// A variable for the current socket.
static TCPsocket activeSocket = NULL;

static int ConnectLocalServer( uint16_t localPort );
static int ConnectRemoteServer( const char *remoteAddress, uint16_t remotePort );

static int ProcessLobbyServer( void );
static int ProcessLobbyClient( void );
static int BroadcastPacketToClients( const void *data, int length );

static int AddPlayer( struct NetworkClientInfo client );
static void RemovePlayer( int playerId );

// Functions for lobby state
static void ServerHandleClientQuit( int playerId );
static void ServerHandleClientMyName( int playerId, char *name );
static void IssueAllJoins( TCPsocket socket );

// Functions for client in lobby state
static void ClientHandleClientJoin( int playerId, char *name );
static void ClientHandleServerYourId( int newId );
static void ClientHandleServerStartGame( void );

static struct NetworkClientInfo clients[6];	// 6 players maximum, eh?!
static int						numClients = 0;
static int						thisClient = -1;

/*
====================
InitializeNetwork

Sets up the network component.
====================
*/
int InitializeNetwork( void ) {
	printf( "InitializeNetwork called.\n" );
	int result;

	// Initialize SDL_net and return on error.
	result = SDLNet_Init();
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
	printf( "Connect( %d, remoteAddress, %d ) called.\n", server, port );
	// If the network component is not initialized, return error.
	if( !isInitialized ) {
		return -1;
	}

	isServer = server;
	// Branch for server/client.
	if( server ) {
		return ConnectLocalServer( port );
	} else {
		return ConnectRemoteServer( remoteAddress, port );
	}
}

/*
====================
ConnectLocalServer

Creates a server to listen for incoming connections.
====================
*/
static int ConnectLocalServer( uint16_t localPort ) {
	printf( "ConnectLocalServer called.\n" );
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
	activeSocket = listener;

	struct NetworkClientInfo clientInfo;
	clientInfo.alias = PLAYER_NAME;
	clientInfo.socket = NULL;
	AddPlayer( clientInfo );

	return 0;
}

/*
====================
ConnectRemoteServer

Creates a socket that communicates with a remote server.
====================
*/
static int ConnectRemoteServer( const char *remoteAddress, uint16_t remotePort ) {
	printf( "ConnectRemoteServer called.\n" );
	IPaddress	ip;
	TCPsocket	client;
	int			result;

	result = SDLNet_ResolveHost( &ip, remoteAddress, remotePort );
	if( result ) {
		return result;
	}

	client = SDLNet_TCP_Open( &ip );
	if( !client ) {
		return ERROR_TCP_SOCKET_CREATION_FAILED;
	}

	isConnected = 1;
	activeSocket = client;

	return 0;
}

void Disconnect( void ) {
	// TODO: Implement
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
		printf( "New client!\n" );
		struct NetworkClientInfo clientInfo;
		clientInfo.alias = NULL;
		clientInfo.socketSet = SDLNet_AllocSocketSet( 1 );
		clientInfo.socket = newClient;
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
		if( clients[client].socket ) {
			// Reset endofstream and receive bytes from this client
			endOfStream = 0;
			if( SDLNet_CheckSockets( clients[client].socketSet, 1 ) <= 0 ) {
				continue;
			}
			if( !SDLNet_SocketReady( clients[client].socket ) ) {
				continue;
			}
			numBytes = SDLNet_TCP_Recv( clients[client].socket, bytes, sizeof( bytes ) );
			// while there is something to read from and the stream hasn't ended yet
			while( numBytes > 0 ) {
				while( !endOfStream ) {
					switch( SDLNet_Read32( &bytes[bReadPosition] ) ) {
						case PID_QUIT:
							// Handle the quit
							ServerHandleClientQuit( client );
							break;
						case PID_MY_NAME:
							printf( "Got PID_MY_NAME.\n" );
							// Determine length of name and call handler with the length.
							stringLength = SDLNet_Read32( &bytes[bReadPosition + 4] ) - 8;
							name = malloc( stringLength + 1 );
							strncpy( name, &bytes[bReadPosition + 8], stringLength );
							name[stringLength] = '\0';
							ServerHandleClientMyName( client, name );
							break;
					}
					bReadPosition += SDLNet_Read32( &bytes[bReadPosition + 4] );
					if( bReadPosition >= numBytes ) {
						endOfStream = 1;
					}
				}
				numBytes = SDLNet_TCP_Recv( clients[client].socket, bytes, sizeof( bytes ) );
				bReadPosition = 0;
			}
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
	char *packet = malloc( 12 + strlen( name ) + 1 );
	SDLNet_Write32( ( int )PID_MY_NAME,			&packet[0] );
	SDLNet_Write32( 12 + strlen( name ) + 1,	&packet[4] );
	SDLNet_Write32( playerId,					&packet[8] );
	strcpy( &packet[12], name );
	BroadcastPacketToClients( packet, 12 + strlen( name ) + 1 );
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
	int		numBytes = SDLNet_TCP_Recv( activeSocket, bytes, sizeof( bytes ) );
	int		endOfStream = 0;
	int		bReadPosition = 0;

	int 	playerId;
	int		stringLength;
	char *	alias;
	int		newId;

	while( numBytes > 0 ) {
		while( !endOfStream ) {
			switch( SDLNet_Read32( &bytes[bReadPosition] ) ) {
				case PID_JOIN:
					printf( "Got PID_JOIN.\n" );
					playerId = SDLNet_Read32( &bytes[bReadPosition + 8] );
					stringLength = SDLNet_Read32( &bytes[bReadPosition + 4] ) - 12;
					alias = malloc( stringLength + 1 );
					strncpy( alias, &bytes[bReadPosition + 12], stringLength );
					ClientHandleClientJoin( playerId, alias );
					break;
				case PID_YOUR_ID:
					newId = SDLNet_Read32( &bytes[bReadPosition + 8] );
					printf( "Got PID_YOUR_ID %d.\n", newId );
					ClientHandleServerYourId( newId );
					break;
				case PID_START_GAME:
					printf( "Got PID_START_GAME.\n" );
					ClientHandleServerStartGame();
					break;
			}
			bReadPosition += SDLNet_Read32( &bytes[bReadPosition + 4] );
			if( bReadPosition >= numBytes ) {
				endOfStream = 1;
			}
		}
		numBytes = SDLNet_TCP_Recv( activeSocket, bytes, sizeof( bytes ) );
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
}

/*
====================
ClientHandleServerStartGame

Handles when the server starts the game.
====================
*/
static void ClientHandleServerStartGame( void ) {
	// TODO: Implement this functionality.
}

/*
====================
ProcessInGame

Process packets inside the game.
====================
*/
// TODO: Implement
//int ProcessInGame( struct GameState *state );

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
		players[i] = clients[client].alias;
	}
	return 0;
}
