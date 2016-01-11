#include <SDL2/SDL.h>
#include "Debug/Debug.h"
#include "Output.h"
#include "Physics.h"

// Variables
static SDL_Window *		sdlWindow = NULL;
static SDL_Renderer *	sdlRenderer = NULL;

// Functions
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
	DebugPrintF( "DisplayGameState was called!" );
	/* This needs to do the following:
	 * Use the predefined sdlWindow and sdlRenderer!
	 * The GameState should not be changed, therefore it is const.
	 * For rendering, have a look at the functions in Menu.c!
	 * 		I know you dislike them for the style, but they will be
	 *		refactored later and already work.
	 * 1. You need to get the window resolution
	 * 2. Take the minimum of window height and window width
	 * 		and find the largest square that fits centered onto the screen.
	 *		This is an internal definition, SDL does not do this for you.
	 * 		The coordinates on this square go from (-1,1) in the top left corner
	 *		to (1,-1) in the bottom right corner. You need these because in the
	 *		GameState, everything is saved on a coordinate system from -1 to 1 on 
	 *		the x and y axes.
	 * 3. Clear the renderer.
	 * 4. (Optional) Draw the background.
	 * 5. For each player, the paddle should be drawn like this:
	 * 		a) Get the player line using the GetPlayerLine function from Physics.
	 *			This line contains one point on the coordinate system and a vector
	 *			to the end of the line.
	 *		b) The inner edge of the paddle should be exactly on this line, and the
	 *			corners of the paddle should go from
	 *				AddVectorToPoint2D( GetPlayerLine(...).point, ScaleVector2D( GetPlayerLine(...).vector, state->player[i].position ) )
	 *			to
	 *				AddVectorToPoint2D( GetPlayerLine(...).point, ScaleVector2D( GetPlayerLine(...).vector, state->player[i].position + PADDLE_SIZE ) )
	 *			. Refactor this code as much as you like, but this gives you two corner
	 *			points of the paddle.
	 *		c) Now draw a colored line from the first point to the second point.
	 *			Remember that you should translate the points to screen coordinates
	 *			using the definition in step 2 before you draw.
	 * 6. Draw a circle to represent the ball. Also translate the state->ball.position
	 *		before you actually draw this.
	 * 7. Present and be done!
	 * P.S.: You can test this function for ten seconds by running the program, going into Host game mode and pressing enter in the lobby!
	*/
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
