#ifndef _NETWORK_H
#define _NETWORK_H

#include <stdint.h>
#include "Game.h"

#define NETWORK_STANDARD_SERVER_PORT ( ( uint16_t )26228 )
#define STANDARD_UDP_SERVER_PORT ( ( uint16_t )26229 )
#define NETWORK_STANDARD_DATA_PORT STANDARD_UDP_SERVER_PORT
#define GAME_START 20

/*
==========================================================

A type that identifies players in the lobby.

==========================================================
*/
typedef char *playerInfo_t;

int InitializeNetwork( void );
void CloseNetwork( void );

int Connect( int server, const char *remoteAddress, uint16_t port );
void Disconnect( void );

int ProcessLobby( void );
int NetworkStartGame( uint16_t udpPort );
int ProcessInGame( struct GameState *state );

int GetLocalIP( const char **string );
int GetPlayerList( playerInfo_t *players, int *numPlayers );
#endif
