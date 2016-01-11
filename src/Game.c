#include "Game.h"
#include "Output.h"
#include "Main.h"
#include "Debug/Debug.h"
#include <time.h>
#include <stdlib.h>

static struct GameState	currentState;

static int InitializeGame( void );

/*
====================
InitializeGame

Resets the GameState.
====================
*/
static int InitializeGame( void ) {
	// TODO: Change this!
	int i;
	currentState.numPlayers = 6;
	currentState.players = malloc( sizeof( struct Player ) * 6 );
	for( i = 0; i < currentState.numPlayers; i++ ) {
		currentState.players[i].name = "pong";
		currentState.players[i].position = 0.0f;
		currentState.players[i].score = 0;
	}
	currentState.ball.position.x = 0.0f;
	currentState.ball.position.y = 0.0f;
	currentState.ball.direction.dx = 0.0f;
	currentState.ball.direction.dy = 0.0f;
	return 0;
}

/*
====================
RunGame

Currently, displays the GameState for 10 seconds. TODO: Change in the future so that network gets called too!
====================
*/
int RunGame( void ) {
	DebugPrintF( "RunGame called." );
	InitializeGame();
	time_t start = time( NULL );
	while( time( NULL ) < start + 10 ) {
		DisplayGameState( &currentState );
	}
	return PS_MENU;
}