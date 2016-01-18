#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "Debug/Debug.h"
#include "Output.h"
#include "Physics.h"
#include "Game.h"

// Variables
static SDL_Window *		sdlWindow = NULL;
static SDL_Renderer *	sdlRenderer = NULL;
int						outputFullscreen = 0;
static SDL_Texture*     paddleTexture ;
static SDL_Texture*     ballTexture ;
static SDL_Texture*     backgroundTexture;
static SDL_Surface*     temp;

static void	CalculatePaddleCoordinates(  struct GameState *state, int playerId, struct Point2D *start, struct Point2D *end );
static void CalculateBallCoordinates( struct Ball ball, struct Point2D *point, int *radius );

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
	sdlWindow = SDL_CreateWindow( "multipong", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, outputFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_SHOWN );
	DebugAssert( sdlWindow );
	sdlRenderer = SDL_CreateRenderer( sdlWindow, -1, SDL_RENDERER_ACCELERATED );
	DebugAssert( sdlRenderer );
	temp = IMG_Load("Assets/Ball.png" );
    ballTexture = SDL_CreateTextureFromSurface( sdlRenderer, temp );
    //temp = IMG_Load("Assets/Paddle.png" );
    //paddleTexture = SDL_CreateTextureFromSurface( renderer, temp );

	//DebugAssert( SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) );
	atexit( SDL_Quit );
	return !( sdlWindow && sdlRenderer );
}


void SetWindowResolution (int  Width , int Height , int Fullscreen) {
    SDL_DestroyWindow(sdlWindow);
    SDL_DestroyRenderer(sdlRenderer);
    sdlWindow = SDL_CreateWindow( "multipong", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, Width, Height, Fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_SHOWN );
    sdlRenderer = SDL_CreateRenderer( sdlWindow, -1, SDL_RENDERER_ACCELERATED );
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
int DisplayGameState( struct GameState *state ) {
	// TODO: Code for rendering!
	/* This needs to do the following:
	 * Use the predefined sdlWindow and sdlRenderer!
	 * The GameState should not be changed, therefore it is .
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

    int            	windowWidth, windowHeight, i;
    SDL_Rect       	ball_rect;
    struct Point2D  paddleStart;
    struct Point2D  paddleEnd;
    struct Point2D  pointBall;
    int				radius;

    SDL_GetWindowSize( sdlWindow, &windowHeight, &windowHeight );
    
	//Set all corresponding Rect Values
    CalculateBallCoordinates( state->ball, &pointBall, &radius );
    ball_rect.w = radius*2 ;
    ball_rect.h = radius*2 ;
    ball_rect.x = (pointBall.x - radius) ;
    ball_rect.y = (pointBall.y - radius) ;

    //copy everything in the renderer
	SDL_RenderClear( sdlRenderer );

	for (i = 0 ; i < state->numPlayers ; i++){
        SDL_SetRenderDrawColor( sdlRenderer, 0x00, 0x00, 0xFF, 0xFF );
        CalculatePaddleCoordinates( state, i, &paddleStart, &paddleEnd );
        SDL_RenderDrawLine( sdlRenderer, paddleStart.x, paddleStart.y, paddleEnd.x, paddleEnd.y );
	}

	SDL_SetRenderDrawColor( sdlRenderer, 0x00, 0x00, 0x00, 0x00 );

	SDL_RenderCopy( sdlRenderer, ballTexture, NULL, &ball_rect );
	SDL_RenderPresent( sdlRenderer );

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

/*
====================
GetScreenRectangle

Returns the biggest possible square in the center of the screen.
====================
*/
static void GetScreenSquare( struct Line2D *rectangle ) {
	DebugAssert( rectangle );

	int sWidth, sHeight;
	SDL_GetWindowSize( sdlWindow, &sWidth, &sHeight );

	if( sWidth < sHeight ) {
		rectangle->point.x = 0.0f;
		rectangle->point.y = ( sWidth - sHeight ) / 2;
		rectangle->vector.dx = sWidth;
		rectangle->vector.dy = sWidth;
	} else {
		rectangle->point.x = ( sHeight - sWidth ) / 2;
		rectangle->point.y = 0.0f;
		rectangle->vector.dx = sHeight;
		rectangle->vector.dy = sHeight;
	}

	return;
}

/*
====================
GameToScreenCoordinates

Calculates on-screen coordinates from a set of game coordinates.
====================
*/
static struct Point2D GameToScreenCoordinates( struct Point2D point ) {
	struct Point2D result;
	struct Line2D rect;

	GetScreenSquare( &rect );
	result.x = rect.point.x + rect.vector.dx * ( point.x + 1.0f ) / 2.0f;
	result.y = rect.point.y + rect.vector.dy * ( -point.y + 1.0f ) / 2.0f;

	return result;
}

/*
====================
CalculatePaddleCoordinate

Given a game state
====================
*/
static void	CalculatePaddleCoordinates( struct GameState *state, int playerId, struct Point2D *start, struct Point2D *end ) {
	if( start ) {
		*start = GameToScreenCoordinates( AddVectorToPoint2D( GetPlayerLine( playerId, state->numPlayers ).point, ScaleVector2D( GetPlayerLine( playerId, state->numPlayers ).vector, state->players[playerId].position ) ) );
	}
	if( end ) {
		*end = GameToScreenCoordinates( AddVectorToPoint2D( GetPlayerLine( playerId, state->numPlayers ).point, ScaleVector2D( GetPlayerLine( playerId, state->numPlayers ).vector, state->players[playerId].position + PADDLE_SIZE ) ) );
	}
}

/*
====================
CalculateBallCoordinates

Given a ball, returns the on-screen point of the ball and its radius.
====================
*/
static void CalculateBallCoordinates( struct Ball ball, struct Point2D *point, int *radius ) {
	if( point ) {
		*point = GameToScreenCoordinates( ball.position );
	}
	if( radius ) {
		struct Line2D rect;

		GetScreenSquare( &rect );
		*radius = ( int )( DEFAULT_BALL_RADIUS * rect.vector.dx / 2.0f );
	}
}
