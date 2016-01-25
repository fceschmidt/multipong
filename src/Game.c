#include "Game.h"
#include "Display.h"
#include "Main.h"
#include "Network.h"
#include "Physics.h"
#include "Debug/Debug.h"
#include <time.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

static struct GameState	currentState;

static int InitializeGame( void );

/*
====================
InitializeGame

Resets the GameState.
====================
*/
static int InitializeGame( void ) {
	int 	i;
	char *	playerNames[6];

	GetPlayerList( playerNames, &currentState.numPlayers );
	currentState.players = malloc( sizeof( struct Player ) * currentState.numPlayers );
	for( i = 0; i < currentState.numPlayers; i++ ) {
		currentState.players[i].name = playerNames[i];
		currentState.players[i].position = 0.0f;
		currentState.players[i].score = 0;
	}
	InitializeBall( &currentState );
	return 0;
}

/*
====================
RunGame

Currently, displays the GameState for 10 seconds.
====================
*/
int RunGame( void ) {
	DebugPrintF( "RunGame called." );
	InitializeGame();
	NetworkStartGame( NETWORK_STANDARD_DATA_PORT );
	
	// For timekeeping...
	unsigned int lastFrame = SDL_GetTicks();

	while( 1 ) {
		ProcessPhysics( &currentState, ( float )( SDL_GetTicks() - lastFrame ) / 1000.0f );
		lastFrame = SDL_GetTicks();
		ProcessInGame( &currentState );
		DisplayGameState( &currentState );
		// Ensures acceptable time measurements (We don't want ~positive infinity FPS, that would break physics)
		SDL_Delay( 10 );
	}
	return PS_MENU;
}
