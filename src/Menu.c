#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_net.h>
#include "Network.h"
#include "Main.h"
#include "Debug/Debug.h"

#define ASSET_FOLDER "Assets/"
#define SANS_FONT_FILE ASSET_FOLDER "OpenSans-Regular.ttf"

/*
==========================================================

A struct for a button in the main menu

==========================================================
*/
typedef struct {
	char*    		text;
	int      		x;
	int      		y;
	int      		height;
	int		 		width;
	int      		rValue;
	int      		gValue;
	int      		bValue;
	SDL_Texture* 	texNotSelected;
	SDL_Texture* 	texSelected;
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
static Button_t 	startButton;
static char * 		username = "multipong\0";
static TTF_Font *	sans;

static void InitializeMenuElements( Button_t *tabOrder , SDL_Renderer *renderer , SDL_Window* sdlWindow );

static void TextInput( const char *description, char *text );
static int EventCheckMainMenu( int *marked, enum MenuState *menuState );
static int EventCheckLobby( int *marked, enum MenuState *menuState );
static int EventCheck( int *marked , enum MenuState *menuState );
static int RenderMainMenu( Button_t *tabOrder, int *marked, SDL_Renderer *renderer, SDL_Window *sdlWindow, enum MenuState *menuState );
static int RenderLobby( Button_t *tabOrder , int *marked , SDL_Renderer *renderer , SDL_Window *sdlWindow , enum MenuState *menuState );
static int Render ( Button_t *tabOrder, int *marked, SDL_Renderer *renderer, SDL_Window *sdlWindow, enum MenuState *menuState );
static int Menu( SDL_Window *sdlWindow, SDL_Renderer* renderer, int *marked , enum MenuState *menuState, Button_t *tabOrder );
static void GoDown( int *marked );
static void GoUp( int *marked );

static void HostGame( void );
static void JoinGame( void );
static void Options( void );

extern SDL_Window *GetSdlWindow( void );
extern SDL_Renderer *GetSdlRenderer( void );

void GetUserName( char* Name );

/*
====================
TextInput

Given an allocated text buffer and a description, prompts the user for text input and writes the input to the allocated text buffer using the SDL interface provided by GetSdlWindow() and GetSdlRenderer().
====================
*/
static void TextInput( const char *description, char *text ) {
	int 			windowWidth, windowHeight;
	int				done = 0;
	char			array[40];
	SDL_Rect		inputRect;
	TTF_Font *		sans = TTF_OpenFont( SANS_FONT_FILE, 256 );
	SDL_Color 		color = { 0, 255, 0 };
	SDL_Window *	sdlWindow = GetSdlWindow();
	SDL_Renderer *	sdlRenderer = GetSdlRenderer();
	SDL_Event 		event;

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
		inputRect.w = 300;
		inputRect.h = 70 ;
		inputRect.x = windowWidth / 2 - inputRect.w / 2;
		inputRect.y = windowHeight / 2 - inputRect.h / 2;
		SDL_RenderCopy( sdlRenderer, message, NULL, &inputRect );
		SDL_RenderPresent( sdlRenderer );
		SDL_DestroyTexture( message );
		SDL_FreeSurface( surfaceMessage );
	}

	DebugPrintF( "The user input was \"%s\".", username );
}

/*
====================
InitializeMenu

Performs initialization tasks for the menu component.
====================
*/
int InitializeMenu( void ) {
	// Initialize SDL_ttf for font output.
	DebugAssert( !TTF_Init() );
	
	// Initialize the username variable.
	DebugAssert( username = malloc( sizeof( char ) * 30 ) );
	username[0] = '\0';

	return 0;
}

/*
====================
ShowMenu

Renders the menu on the window. This call blocks until the user decides to leave the application (return value PS_QUIT) or start a game (return value PS_GAME).
====================
*/
int ShowMenu( void ) {
	enum ProgramState 	mode = PS_MENU;
	int 				marked = 0;
	enum MenuState		menuState = MS_MAIN_MENU;
	SDL_Window *		sdlWindow = GetSdlWindow();
	SDL_Renderer *		renderer = GetSdlRenderer();

	DebugPrintF( "Running the menu." );
	Button_t tabOrder[4];
	InitializeMenuElements( tabOrder, renderer, sdlWindow );
	TextInput( "Username: ", username );
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
							Options();
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
	SDL_Event event;
	while( SDL_PollEvent( &event ) ){
		switch( event.type ){
			case SDL_KEYDOWN:
				switch ( event.key.keysym.sym ){
					case SDLK_BACKSPACE:
						*menuState = MS_MAIN_MENU;
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
			// TODO: Implement this.
			break;
	}
	return PS_MENU;
}


/*
====================
Options

Shows the option submenu.
====================
*/
static void Options( void ) {
	// TODO: Implement
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
	int 		w, h, i, n;
	char *		playerNames[6];
	SDL_Color 	white = {255, 255, 255};

	GetPlayerList( playerNames, &n );
	ProcessLobby();

	SDL_Rect startRect;
	SDL_Rect frameRect;
	SDL_Rect backgroundRect;
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

	SDL_RenderClear( renderer );
	SDL_RenderCopy( renderer, backgroundTexture, NULL, &backgroundRect );
	for ( i = 0; i < n; i++ ){
		SDL_Surface* surfaceMessage = TTF_RenderText_Solid( sans, playerNames[i], white );
		SDL_Texture* Message = SDL_CreateTextureFromSurface( renderer, surfaceMessage );

		//printf( "%s :: %d \n", playerNames[i], n );
		SDL_Rect Player_rect;
		Player_rect.w = frameRect.w /2;
		Player_rect.h = frameRect.h * 0.05;
		Player_rect.x = ( frameRect.w / 2 ) - Player_rect.w / 2;
		Player_rect.y = ( i * Player_rect.h * 1.8f )  + Player_rect.h * 5;
		SDL_RenderCopy( renderer, Message, NULL, &Player_rect );
		SDL_DestroyTexture( Message );
		SDL_FreeSurface( surfaceMessage );
	}
	SDL_RenderCopy( renderer, frameTexture, NULL, &frameRect );
	if( *menuState == MS_HOST_GAME ) {
		SDL_RenderCopy( renderer, startButton.texSelected, NULL, &startRect );
	} else {
		SDL_RenderCopy( renderer, startButton.texNotSelected , NULL , &startRect );
	}
	SDL_RenderPresent( renderer );
	return 0;
}

static int RenderMainMenu( Button_t *tabOrder, int *marked, SDL_Renderer *renderer, SDL_Window *sdlWindow, enum MenuState *menuState ) {
	int 		w, h, i;
	SDL_Rect 	backgroundRect;
	SDL_Rect 	buttonRect;
	SDL_Rect 	titleRect;
	SDL_GetWindowSize( sdlWindow, &w, &h );
	backgroundRect.w = w;
	backgroundRect.h = h;
	backgroundRect.x = 0;
	backgroundRect.y = 0;
	titleRect.w = w * 0.7;
	titleRect.h = h / 5;
	titleRect.x = ( w / 2 ) - titleRect.w / 2;
	titleRect.y = 0;

	SDL_RenderClear( renderer );

	SDL_RenderCopy( renderer, backgroundTexture, NULL, &backgroundRect );
	SDL_RenderCopy( renderer, titleTexture, NULL, &titleRect );

	for ( i = 0; i < 4; i++ ) {
		buttonRect.x = tabOrder[i].x;
		buttonRect.y = tabOrder[i].y;
		buttonRect.w = tabOrder[i].width;
		buttonRect.h = tabOrder[i].height;
		if ( *marked == i ){
			SDL_RenderCopy( renderer, tabOrder[i].texSelected, NULL, &buttonRect );
		} else {
			SDL_RenderCopy( renderer, tabOrder[i].texNotSelected, NULL, &buttonRect );
		}
	}
	SDL_RenderPresent( renderer );
	return 0;
}


static int Render ( Button_t *tabOrder, int *marked, SDL_Renderer *renderer, SDL_Window *sdlWindow, enum MenuState *menuState ) {
	int result = 0;

	switch( *menuState ) {
		case MS_MAIN_MENU:
			result = RenderMainMenu(tabOrder,marked,renderer,sdlWindow,menuState);
			break;
		case MS_HOST_GAME:
		case MS_JOIN_GAME:
			result = RenderLobby(tabOrder,marked,renderer,sdlWindow,menuState);
			break;
		case MS_OPTIONS:
			// TODO: Implement this
			break;
	}

	return result;
}

// The button RGB values (useless?)
#define BTN_R 255
#define BTN_G 255
#define BTN_B 255

/*
==========================================================
Initializes the elements of the main menu.
==========================================================
*/
static void InitializeMenuElements ( Button_t *tabOrder , SDL_Renderer *renderer , SDL_Window* sdlWindow ) {
	int w,h;
	int r = rand() % 2;

	sans = TTF_OpenFont( SANS_FONT_FILE, 500 );
	SDL_GetWindowSize( sdlWindow, &w, &h );
	DebugPrintF( "SDL_GetWindowSize returned %d x %d pixels.", w, h );
	SDL_Surface *temp;

	if( r ) {
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
	}
	DebugPrintF( "Loaded all menu images." );
	DebugAssert( startButton.texSelected && startButton.texNotSelected && frameTexture && titleTexture && backgroundTexture );

	tabOrder[0].height = 0.17f * h;
	tabOrder[1].height = 0.17f * h;
	tabOrder[2].height = 0.17f * h;
	tabOrder[3].height = 0.17f * h;
	tabOrder[0].width = 0.5f * w;
	tabOrder[1].width = 0.5f * w;
	tabOrder[2].width = 0.5f * w;
	tabOrder[3].width = 0.5f * w;
	tabOrder[0].x = (w / 2) - (tabOrder[0].width / 2);
	tabOrder[1].x = (w / 2) - (tabOrder[1].width / 2);
	tabOrder[2].x = (w / 2) - (tabOrder[2].width / 2);
	tabOrder[3].x = (w / 2) - (tabOrder[3].width / 2);
	tabOrder[0].y = h / 5;
	tabOrder[1].y = ( 2 * h ) / 5;
	tabOrder[2].y = ( 3 * h ) / 5;
	tabOrder[3].y = ( 4 * h ) / 5;
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
