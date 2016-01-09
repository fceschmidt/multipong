#include <SDL2/SDL.h>
#include "Debug/Debug.h"
#include "Output.h"

SDL_Window *	sdlWindow = NULL;
SDL_Renderer *	sdlRenderer = NULL;

SDL_Window *GetSdlWindow( void );
SDL_Renderer *GetSdlRenderer( void );

/*
====================
InitializeGraphics

Creates the window and renderer.
====================
*/
int InitializeGraphics( void ) {
	DebugAssert( !SDL_Init( SDL_INIT_VIDEO ) );
	sdlWindow = SDL_CreateWindow( "multipong", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN );
	DebugAssert( sdlWindow );
	sdlRenderer = SDL_CreateRenderer( sdlWindow, -1, SDL_RENDERER_ACCELERATED );
	DebugAssert( sdlRenderer );
	DebugAssert( SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) );
	atexit( SDL_Quit );
	return !( sdlWindow && sdlRenderer );
}

/*
====================
GetSdlWindow

Returns the window.
====================
*/
SDL_Window *GetSdlWindow( void ) {
	return sdlWindow;
}

/*
====================
GetSdlRenderer

Returns the renderer.
====================
*/
SDL_Renderer *GetSdlRenderer( void ) {
	return sdlRenderer;
}

/*
====================
DisplayGameState

Renders the game state using information from the state variable.
====================
*/
int DisplayGameState( const struct GameState *state ) {
	// TODO: Code for rendering!
	return 0;
}

/*
====================
CloseDisplay

Gets rid of the window and renderer.
====================
*/
void CloseDisplay( void ) {
	SDL_DestroyRenderer( sdlRenderer );
	SDL_DestroyWindow( sdlWindow );
}