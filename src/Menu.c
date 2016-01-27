#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_net.h>
#include "Network.h"
#include "Main.h"
#include "Debug/Debug.h"
#include "Audio.h"
#include "Menu.h"

/*
==========================================================

A struct for a button in the main menu

==========================================================
*/
typedef struct Button {
	char *			text;
	int				x;
	int				y;
	int				height;
	int				width;
	SDL_Texture*	texNotSelected;
	SDL_Texture*	texSelected;
	struct Button*       succButton ;
	struct Button*       predButton ;
} Button_t;

/*
==========================================================

An enumeration for the states of the menu.

==========================================================
*/
enum MenuState {
	MS_MAIN_MENU,
	MS_HOST_GAME,
	MS_JOIN_GAME,
	MS_OPTIONS
};

static SDL_Texture *backgroundTexture;
static SDL_Texture *titleTexture;
static SDL_Texture *frameTexture;
static Button_t		startButton;
static char *		username = "multipong\0";
static enum Side	side = SI_UNDECIDED;
static TTF_Font *	sans;
static int          numWindowResolutions;
static int			VolumeMeter;
static int*			CW;
static int*			CH;
static int			markedOptions;
static int			resolutionCounter;

static void			InitializeMenuElements( Button_t *tabOrder , SDL_Renderer *renderer , SDL_Window* sdlWindow );
static void			TextInput( const char *description, char *text );
static int			EventCheckMainMenu( int *marked, enum MenuState *menuState );
static int			EventCheckLobby( int *marked, enum MenuState *menuState );
static int			EventCheck( int *marked , enum MenuState *menuState );
static int			RenderMainMenu( Button_t *tabOrder, int *marked, SDL_Renderer *renderer, SDL_Window *sdlWindow, enum MenuState *menuState );
static int			RenderLobby( Button_t *tabOrder , int *marked , SDL_Renderer *renderer , SDL_Window *sdlWindow , enum MenuState *menuState );
static int			Render ( Button_t *tabOrder, int *marked, SDL_Renderer *renderer, SDL_Window *sdlWindow, enum MenuState *menuState );
static int			Menu( SDL_Window *sdlWindow, SDL_Renderer* renderer, int *marked , enum MenuState *menuState, Button_t *tabOrder );
static void			GoDown( int *marked );
static void			GoUp( int *marked );
static int			EventCheckOptions( enum MenuState *menuState );
static int			RenderOptions( SDL_Renderer *renderer, SDL_Window *sdlWindow, enum MenuState *menuState ) ;
static void			HostGame( void );
static void			JoinGame( void );
static void			ChooseSide( void );
static void			GoOptions( void );

extern SDL_Window *			GetSdlWindow( void );
extern SDL_Renderer *		GetSdlRenderer( void );

// extern because this is defined in Network.c
// const because it should never be changed from here
// volatile because it might be changed from elsewhere (so the compiler doesn't strip the access code)
extern const volatile int	clientGameStarted;

void GetUserName( char* Name );

/*
====================
GoOptions

Simple tab order for the options submenu
====================
*/
static void GoOptions ( void ) {
    if ( markedOptions != 0 ){
        markedOptions = 0;
	} else {
		markedOptions = 1;
	}
}

/*
====================
GetSide

Returns the side value
====================
*/
enum Side GetSide( void ) {
	return side;
}

/*
====================
ChooseSide

Lets you choose sides in the beginning of the game.
====================
*/
static void ChooseSide ( void ){
    int				windowWidth, windowHeight, done;
	enum Side		choice = SI_GOOD;
    SDL_Window *	sdlWindow = GetSdlWindow();
    SDL_Renderer *	sdlRenderer = GetSdlRenderer();
    SDL_Event		event ;
    SDL_Rect		evil_rect;
    SDL_Rect		good_rect;
    SDL_Surface*	temp;
    SDL_Texture*	good;
    SDL_Texture*	evil;
    SDL_Texture*	goodSelected;
    SDL_Texture*	evilSelected;

	// TODO: Fix this.
	// Load everything
    done = 0;
    SDL_GetWindowSize( sdlWindow, &windowWidth, &windowHeight );
    temp = IMG_Load( ASSET_FOLDER "BadChooseSide.png" );
	evil = SDL_CreateTextureFromSurface( sdlRenderer, temp );
    temp = IMG_Load( ASSET_FOLDER "GoodChooseSide.png" );
	good = SDL_CreateTextureFromSurface( sdlRenderer, temp );
    temp = IMG_Load( ASSET_FOLDER "BadChooseSide(Selected).png" );
	evilSelected = SDL_CreateTextureFromSurface( sdlRenderer, temp );
    temp = IMG_Load( ASSET_FOLDER "GoodChooseSide(Selected).png" );
	goodSelected = SDL_CreateTextureFromSurface( sdlRenderer, temp );

	// Render loop all-in-one with event loop!
    while ( !done ) {
		if ( SDL_PollEvent( &event ) ) {
			if ( event.type == SDL_KEYDOWN ) {
                switch (event.key.keysym.sym){
                    case SDLK_LEFT:
                        choice = SI_EVIL;
                        break;
                    case SDLK_RIGHT:
                        choice = SI_GOOD;
                        break;
                    case SDLK_RETURN:
                        side = choice;
                        printf( "Chose Side : %d\n", ( int )side );
						PlayMusic();
                        done = 1;
                        break;
                }
			}
		}

		SDL_RenderClear( sdlRenderer ) ;
	    good_rect.x = windowWidth / 2 ;
	    good_rect.y = 0 ;
	    evil_rect.x = 0 ;
	    good_rect.w = windowWidth/2 ;
	    good_rect.h = windowHeight ;
	    evil_rect.y = 0 ;
	    evil_rect.w = windowWidth/2 ;
	    evil_rect.h = windowHeight ;

		if( choice == SI_EVIL ){
			SDL_RenderCopy( sdlRenderer, good, NULL, &good_rect );
	        SDL_RenderCopy( sdlRenderer, evilSelected, NULL, &evil_rect );
	    } else {
	        SDL_RenderCopy( sdlRenderer, goodSelected, NULL, &good_rect );
	        SDL_RenderCopy( sdlRenderer, evil, NULL, &evil_rect );
	    }
		SDL_RenderPresent( sdlRenderer );
    }

	// Destroy everything
    SDL_FreeSurface( temp );
    SDL_DestroyTexture( good );
    SDL_DestroyTexture( evil );
    SDL_DestroyTexture( goodSelected );
    SDL_DestroyTexture( evilSelected );
}

/*
====================
TextInput

Given an allocated text buffer and a description, prompts the user for text input and writes the input to the allocated text buffer using the SDL interface provided by GetSdlWindow() and GetSdlRenderer().
====================
*/
static void TextInput( const char *description, char *text ) {
	int				windowWidth, windowHeight, widthText, heightText;
	int				done = 0;
	char			array[40];
	SDL_Rect		inputRect;
	TTF_Font *		sans = TTF_OpenFont( SANS_FONT_FILE, 256 );
	SDL_Color		color = { 0, 255, 0 };
	SDL_Window *	sdlWindow = GetSdlWindow();
	SDL_Renderer *	sdlRenderer = GetSdlRenderer();
	SDL_Event		event;

	SDL_GetWindowSize( sdlWindow, &windowWidth, &windowHeight );
	SDL_StartTextInput();
	DebugPrintF( "Started text input." );
	while ( !done ) {
		if ( SDL_PollEvent( &event ) ) {
			switch ( event.type ) {
				case SDL_KEYDOWN:
					/* Quit */
					if ( event.key.keysym.sym == SDLK_RETURN){
						done = 1;
					}
					if (event.key.keysym.sym == SDLK_BACKSPACE){
                        text[strlen(text)-1] = '\0';
					}
					break;
				case SDL_TEXTINPUT:
					/* Add new text onto the end of our text */
					strcat( text, event.text.text );
					break;
			}
		}

		SDL_RenderClear( sdlRenderer );
		strcpy( array, description );
		SDL_Surface* surfaceMessage = TTF_RenderText_Solid( sans, strcat( array, text ), color );
		SDL_Texture* message = SDL_CreateTextureFromSurface( sdlRenderer, surfaceMessage );
		TTF_SizeText(sans,array,&widthText,&heightText);
		inputRect.w = 70*widthText/heightText;
		inputRect.h = 70 ;
		inputRect.x = windowWidth / 2 - inputRect.w / 2;
		inputRect.y = windowHeight / 2 - inputRect.h / 2;
		SDL_RenderCopy( sdlRenderer, message, NULL, &inputRect );
		SDL_RenderPresent( sdlRenderer );
		SDL_DestroyTexture( message );
		SDL_FreeSurface( surfaceMessage );
	}

	DebugPrintF( "The user input was \"%s\".", text );
}

/*
====================
InitializeMenu

Performs initialization tasks for the menu component.
====================
*/
int InitializeMenu( void ) {
	// Initialize SDL_ttf for font output.
	// THIS HAS BEEN MOVED TO OUTPUT.C
    side = SI_UNDECIDED;
	// Initialize the username variable.
	DebugAssert( username = malloc( sizeof( char ) * 30 ) );
	username[0] = '\0';

	return 0;
}

/*
====================
GetWindowResolutions

Returns an array of possible full-screen window resolutions after filtering the duplicates.
====================
*/
static int GetWindowResolutions( int *harray , int *warray ) {
	// A lot of variables.
    int				counter = 0;
	int				oldw = 0;
	int				oldh = 0;
	int				display_index = 0;
	int				display_count = SDL_GetNumDisplayModes( display_index );
    int				i;
	int				cw = 0;
	int				ch = 0;
    SDL_DisplayMode mode1 = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };

	// Make sure both arrays are actually there
	DebugAssert( harray );
	DebugAssert( warray );

    // Goes through the list and writes everything to the arrays.
	for( i = 0; i < display_count; i++ ) {
		if( SDL_GetDisplayMode( display_index, i, &mode1 ) != 0 ) {
	        DebugPrintF( "SDL_GetDisplayMode failed: %s", SDL_GetError() );
	        return 1;
	    }
	    if( ( oldw != mode1.w ) || ( oldh != mode1.h ) ) {
			harray[ch] = mode1.h;
			warray[cw] = mode1.w;
			oldw = mode1.w;
			oldh = mode1.h;
			ch++;
			cw++;
			counter ++;
		}
	}
	return counter;
}

/*
====================
ShowMenu

Renders the menu on the window. This call blocks until the user decides to leave the application (return value PS_QUIT) or start a game (return value PS_GAME).
====================
*/
int ShowMenu( void ) {
	enum ProgramState	mode = PS_MENU;
	int					marked = 0;
	enum MenuState		menuState = MS_MAIN_MENU;

	int					i;
	int					displayCount = SDL_GetNumDisplayModes( 0 );	// The amount of display modes.
	SDL_Window *		sdlWindow = GetSdlWindow();
	SDL_Renderer *		renderer = GetSdlRenderer();
	Button_t			tabOrder[4];								// The four buttons of the main menu.

	// The arrays that will
    CW = malloc( sizeof(int) * displayCount );
    CH = malloc( sizeof(int) * displayCount );

	// Prepare the menu output.
	DebugPrintF( "Running the menu." );

	// Get the user name
	while( strlen( username ) < 1 ) {
        TextInput( "Username: ", username );
	}

	// Let the user choose sides.
	if( side == SI_UNDECIDED ) {
		ChooseSide();
	}

	// Load textures etc.
	InitializeMenuElements( tabOrder, renderer, sdlWindow );

	// Retrieve and print all available full-screen display resolutions.
	numWindowResolutions = GetWindowResolutions( CH, CW );
    DebugPrintF( "Got Avaible Display Resolutions" );
	for ( i = 0 ; i < numWindowResolutions; i++ ) {
        DebugPrintF( "Resolution #%d: %d x %d", i, CW[i], CH[i] );
	}

	// Show the menu
	while( 1 ) {
		mode = Menu( sdlWindow, renderer, &marked, &menuState, tabOrder );
		if( mode != PS_MENU ) {
			return mode;
		}
	}
	return PS_QUIT;
}

/*
====================
Menu

Runs the event check and renders the menu, independently of the current menu state.
====================
*/
static int Menu ( SDL_Window *sdlWindow, SDL_Renderer* renderer, int *marked, enum MenuState *menuState, Button_t *tabOrder  ) {
	enum ProgramState mode = EventCheck( marked, menuState );
	if ( mode != PS_MENU ) {
		return mode;
	}
	Render( tabOrder, marked, renderer, sdlWindow, menuState );
	return PS_MENU;
}

/*
==========================================================
Performs the event check for MS_MAIN_MENU.
==========================================================
*/
static int EventCheckMainMenu( int *marked, enum MenuState *menuState ) {
	SDL_Event event;
	while( SDL_PollEvent( &event ) ){
		if( event.type == SDL_KEYDOWN ) {
			switch ( event.key.keysym.sym ){
				case SDLK_UP:
					GoUp( marked );
					break;
				case SDLK_DOWN:
					GoDown( marked );
					break;
				case SDLK_RETURN:
					switch ( *marked ){
						case 0:
							*menuState = MS_HOST_GAME;
							 HostGame();
							break;
						case 1:
							*menuState = MS_JOIN_GAME;
							JoinGame();
							break;
						case 2:
							*menuState = MS_OPTIONS ;
							break;
						case 3:
							return PS_QUIT;
							break;
					}
					break;
			}
		}
	}
	return PS_MENU;
}

/*
====================
EventCheckLobby

Performs the input event check for the menu states MS_HOST_GAME and MS_JOIN_GAME.
====================
*/
static int EventCheckLobby( int *marked, enum MenuState *menuState ) {
	if( clientGameStarted ) {
		return PS_GAME;
	}
	SDL_Event event;
	while( SDL_PollEvent( &event ) ){
		switch( event.type ){
			case SDL_KEYDOWN:
				switch ( event.key.keysym.sym ){
					case SDLK_BACKSPACE:
						*menuState = MS_MAIN_MENU;
						Disconnect() ;
						break;
					case SDLK_RETURN:
						// Filter menu state client lobby
						if( !( *menuState == MS_HOST_GAME ) ) {
							break;
						}
						return PS_GAME;
					default:
						break;
				}
		}
	}
	return PS_MENU;
}

/*
====================
EventCheckOptions

The event loop for the options submenu. Checks key states etc.
====================
*/
static int EventCheckOptions( enum MenuState *menuState ) {
	SDL_Event event;
	while( SDL_PollEvent( &event ) ){
		switch( event.type ){
			case SDL_KEYDOWN:
				switch ( event.key.keysym.sym ){
					case SDLK_BACKSPACE:
						*menuState = MS_MAIN_MENU;
						break;
					case SDLK_RETURN:
						*menuState = MS_MAIN_MENU ;
						break;
                    case  SDLK_UP:
                        GoOptions();
                        break;
                    case  SDLK_DOWN:
                        GoOptions();
                        break;
                    case SDLK_RIGHT:
                        if (markedOptions== 0){
                           if (resolutionCounter == numWindowResolutions-1){
                                resolutionCounter = 0 ;
                           } else {
                                resolutionCounter ++ ;
                           }
                        } else {
                            if (VolumeMeter < 100){
                                VolumeMeter ++ ;
                            }

                        }
                        break;
                    case SDLK_LEFT:
                        if (markedOptions == 0){
                            if (resolutionCounter == 0){
                                resolutionCounter = numWindowResolutions-1 ;
                           } else {
                                resolutionCounter -- ;
                           }
                        } else {
                            if (VolumeMeter > 0){
                                VolumeMeter -- ;
                            }
                        }
                        break;
					default:
						break;
				}
		}
	}
	return PS_MENU;
}

/*
====================
EventCheck

Preforms the state-independent event check.
====================
*/
static int EventCheck( int *marked, enum MenuState *menuState ) {
	switch( *menuState ){
		case MS_MAIN_MENU:
			return EventCheckMainMenu( marked, menuState );
			break;
		case MS_HOST_GAME:
		case MS_JOIN_GAME:
			return EventCheckLobby( marked, menuState );
			break;
		case MS_OPTIONS:
			return EventCheckOptions(menuState);
			break;
	}
	return PS_MENU;
}

/*
====================
HostGame

Calls the network component so that it creates a local server.
====================
*/
static void HostGame( void ) {
	Connect( 1, 0, NETWORK_STANDARD_SERVER_PORT );
}

/*
====================
JoinGame

Calls the network component so that it creates a remote client.
====================
*/
static void JoinGame( void ) {
	char input[40] = "";
	TextInput( "IP: ", input );
	Connect( 0, input, NETWORK_STANDARD_SERVER_PORT );
}

/*
====================
GetUserName

Writes the current user name into the Name argument.
====================
*/
void GetUserName( char* Name ){
	strcpy( Name, username );
}

/*
==========================================================
Renders all the buttons
==========================================================
*/
static int RenderLobby( Button_t *tabOrder, int *marked, SDL_Renderer *renderer, SDL_Window *sdlWindow, enum MenuState *menuState ) {
	int			w, h, i, n, wt, ht;
	char *		playerNames[6];
	SDL_Color	white = {255, 255, 255};
	SDL_Rect	startRect;
	SDL_Rect	frameRect;
	SDL_Rect	backgroundRect;
	SDL_Rect	Player_rect;

	// Calls the network component to go through the lobby code.
	ProcessLobby();

	// Get the current window size and set the rectangles for the images accordingly.
	SDL_GetWindowSize( sdlWindow, &w, &h );

	startRect.w = 0.2 *h;
	startRect.h = 0.2 *h;
	startRect.x = w - startRect.w -30;
	startRect.y = h - startRect.w -30;

	frameRect.h = h;
	frameRect.w = w/2;
	frameRect.x = w/2 - frameRect.w;
	frameRect.y = 0;

	backgroundRect.w = w;
	backgroundRect.h = h;
	backgroundRect.x = 0;
	backgroundRect.y = 0;

	// Get the list of currently logged-in players from the network component.
	GetPlayerList( playerNames, &n );

	// Prepare rendering
	SDL_RenderClear( renderer );

	// Draw the background :)
	SDL_RenderCopy( renderer, backgroundTexture, NULL, &backgroundRect );

	// Go through the players and draw every name
	for ( i = 0; i < n; i++ ){
		// Render text to surface and convert surface to texture
		SDL_Surface* surfaceMessage = TTF_RenderText_Solid( sans, playerNames[i], white );
		SDL_Texture* Message = SDL_CreateTextureFromSurface( renderer, surfaceMessage );

		// Get text size for scaling and set the player rectangle accordingly.
        TTF_SizeText( sans , playerNames[i] , &wt , &ht );

		// The constant 0.1125 was found out by experimentation.
		Player_rect.h = frameRect.h * 0.1125f;

		// The width of the player name rectangle is the minimum of
		//   a) What SDL_ttf says should be right for proper scaling
		//   b) What fits into our background rectangle, so we make sure we don't overrun the bounds of it.
		Player_rect.w = fminf( Player_rect.h * wt / ht, frameRect.w / 2 );

		// Position the rectangle
		Player_rect.x = ( frameRect.w / 2 ) - Player_rect.w / 2;
		Player_rect.y = ( i * Player_rect.h * 1 )  + h*0.15 + frameRect.y;

		// Draw the rectangle and clean up.
		SDL_RenderCopy( renderer, Message, NULL, &Player_rect );
		SDL_DestroyTexture( Message );
		SDL_FreeSurface( surfaceMessage );
	}

	// Render the frame for the player list
	SDL_RenderCopy( renderer, frameTexture, NULL, &frameRect );

	// If we're host, render the start button as selected, otherwise render it as unselected.
	SDL_RenderCopy( renderer, *menuState == MS_HOST_GAME ? startButton.texSelected : startButton.texNotSelected, NULL, &startRect );

	SDL_RenderPresent( renderer );
	return 0;
}

/*
====================
RenderOptions

TODO: Supposed to render the options submenu. Doesn't do much as of yet.
====================
*/
static int RenderOptions( SDL_Renderer *renderer, SDL_Window *sdlWindow, enum MenuState *menuState ) {
	return 0;
}

/*
====================
RenderMainMenu

Renders the main menu which you see after choosing sides in the game.
====================
*/
static int RenderMainMenu( Button_t *tabOrder, int *marked, SDL_Renderer *renderer, SDL_Window *sdlWindow, enum MenuState *menuState ) {
	int			w, h, i;
	SDL_Rect	backgroundRect;
	SDL_Rect	buttonRect;
	SDL_Rect	titleRect;

	// Get the window size and set rectangles for background and title accordingly.
	SDL_GetWindowSize( sdlWindow, &w, &h );

	backgroundRect.w = w;
	backgroundRect.h = h;
	backgroundRect.x = 0;
	backgroundRect.y = 0;

	titleRect.w = w * 0.7;
	titleRect.h = h / 5;
	titleRect.x = ( w / 2 ) - titleRect.w / 2;
	titleRect.y = 0;

	// Prepare the rendering with the background and title textures.
	SDL_RenderClear( renderer );
	SDL_RenderCopy( renderer, backgroundTexture, NULL, &backgroundRect );
	SDL_RenderCopy( renderer, titleTexture, NULL, &titleRect );

	// Go through the array and draw all four buttons (selected or unselected depending on state):
	//	1	Host Game
	//	2	Join Game
	//	3	Options
	//	4	Quit
	for ( i = 0; i < 4; i++ ) {
		buttonRect.x = tabOrder[i].x;
		buttonRect.y = tabOrder[i].y;
		buttonRect.w = tabOrder[i].width;
		buttonRect.h = tabOrder[i].height;
		SDL_RenderCopy( renderer, *marked == i ? tabOrder[i].texSelected : tabOrder[i].texNotSelected, NULL, &buttonRect );
	}

	SDL_RenderPresent( renderer );
	return 0;
}

/*
====================
Render

You call this when you don't know what to render. Renders whatever has to be rendered.
====================
*/
static int Render( Button_t *tabOrder, int *marked, SDL_Renderer *renderer, SDL_Window *sdlWindow, enum MenuState *menuState ) {
	int result = 0;

	// Well, this should explain itself.
	switch( *menuState ) {
		case MS_MAIN_MENU:
			result = RenderMainMenu(tabOrder,marked,renderer,sdlWindow,menuState);
			break;
		case MS_HOST_GAME:
		case MS_JOIN_GAME:
			result = RenderLobby(tabOrder,marked,renderer,sdlWindow,menuState);
			break;
		case MS_OPTIONS:
			result = RenderOptions(renderer,sdlWindow,menuState);
			break;
	}

	return result;
}


/*
==========================================================
Initializes the elements of the main menu.
==========================================================
*/
static void InitializeMenuElements( Button_t *tabOrder , SDL_Renderer *renderer , SDL_Window* sdlWindow ) {
	int				w,h;
	int				i;
	SDL_Surface *	temp;

	// Prepare rendering
	sans = TTF_OpenFont( SANS_FONT_FILE, 500 );
	SDL_GetWindowSize( sdlWindow, &w, &h );
	DebugPrintF( "SDL_GetWindowSize returned %d x %d pixels.", w, h );

	// Load the texutures for the start button, the text frame, title, background, and the four main menu buttons,
	// according to the side chosen (Evil or Good).
	if( side == SI_EVIL ) {
		temp = IMG_Load( ASSET_FOLDER "Evil/Start.png" );
		startButton.texSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Evil/Start(Disabled).png" );
		startButton.texNotSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Evil/Frame.png" );
		frameTexture = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Evil/Titel.png" );
		titleTexture = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Evil/Background.png" );
		backgroundTexture = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Evil/HostGame(Selected).png" );
		tabOrder[0].texSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Evil/HostGame(Unselected).png" );
		tabOrder[0].texNotSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Evil/JoinGame(Selected).png" );
		tabOrder[1].texSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Evil/JoinGame(Unselected).png" );
		tabOrder[1].texNotSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Evil/Options(Selected).png" );
		tabOrder[2].texSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Evil/Options(Unselected).png" );
		tabOrder[2].texNotSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Evil/Exit(Selected).png" );
		tabOrder[3].texSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Evil/Exit(Unselected).png" );
		tabOrder[3].texNotSelected = SDL_CreateTextureFromSurface( renderer, temp );
		DebugPrintF( "Chosen Sith" );
	} else {
		temp = IMG_Load( ASSET_FOLDER "Good/Start.png" );
		startButton.texSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Good/Start(Disabled).png" );
		startButton.texNotSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Good/Frame.png" );
		frameTexture = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Good/Titel.png" );
		titleTexture = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Good/Background.png" );
		backgroundTexture = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Good/HostGame(Selected).png" );
		tabOrder[0].texSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Good/HostGame(Unselected).png" );
		tabOrder[0].texNotSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Good/JoinGame(Selected).png" );
		tabOrder[1].texSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Good/JoinGame(Unselected).png" );
		tabOrder[1].texNotSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Good/Options(Selected).png" );
		tabOrder[2].texSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Good/Options(Unselected).png" );
		tabOrder[2].texNotSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Good/Exit(Selected).png" );
		tabOrder[3].texSelected = SDL_CreateTextureFromSurface( renderer, temp );
		temp = IMG_Load( ASSET_FOLDER "Good/Exit(Unselected).png" );
		tabOrder[3].texNotSelected = SDL_CreateTextureFromSurface( renderer, temp );
		DebugPrintF("Chosen Jedi");
	}

	// Check whether everything has been loaded nicely or if there were any errors, then print debug statements.
	DebugPrintF( "Done loading menu images." );
	DebugAssert( startButton.texSelected );
	DebugAssert( startButton.texNotSelected );
	DebugAssert( frameTexture );
	DebugAssert( titleTexture );
	DebugAssert( backgroundTexture );
	DebugAssert( tabOrder[0].texSelected );
	DebugAssert( tabOrder[0].texNotSelected );
	DebugAssert( tabOrder[1].texSelected );
	DebugAssert( tabOrder[1].texNotSelected );
	DebugAssert( tabOrder[2].texSelected );
	DebugAssert( tabOrder[2].texNotSelected );
	DebugAssert( tabOrder[3].texSelected );
	DebugAssert( tabOrder[3].texNotSelected );

	// Set the button rectangles..
	for( i = 0; i < 4; i++ ) {
		tabOrder[i].height = 0.17f * h;
		tabOrder[i].width = 0.5f * w;
		tabOrder[i].x = ( w / 2 ) - ( tabOrder[i].width / 2 );
		tabOrder[i].y = ( ( i + 1 ) * h ) / 5;
	}
	DebugPrintF( "Assigned all button positions." );
}

/*
==========================================================
Function to GoUp one Button. Wraps if the uppermost button is selected.
==========================================================
*/
static void GoUp( int *marked ) {
	if ( *marked > 0 ){
		( *marked )--;
	} else {
		*marked = 3;
	}
}

/*
==========================================================
Function to GoDown one Button. Wraps if the lowermost button is selected.
==========================================================
*/
static void GoDown( int *marked) {
	if ( *marked < 3 ){
		( *marked )++;
	} else {
		*marked = 0;
	}
}
