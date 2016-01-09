#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_net.h>
#include "Network.h"
#include "Debug/Debug.h"

#define SANS_FONT_FILE "Assets/OpenSans-Regular.ttf"

/*
==========================================================
A struct for a button in the main menu
==========================================================
*/
typedef struct {
	char*    		Text;
	int      		x;
	int      		y;
	int      		Hoehe;
	int		 		Breite;
	int      		RValue;
	int      		GValue;
	int      		BValue;
	SDL_Texture* 	NotSelected;
	SDL_Texture* 	Selected;
}Button_t;

static SDL_Texture *backgroundTexture;
static SDL_Texture *titleTexture;
static SDL_Texture *frameTexture;
static Button_t 	startButton;
static char * 		username;

static void InitializeMenuElements( Button_t *tabOrder , SDL_Renderer *renderer , SDL_Window* sdlWindow );

static void TextInput( char *description, char *text, SDL_Renderer *renderer, SDL_Window *sdlWindow );
static int EventCheckMainMenu( Button_t *tabOrder , int *marked , int *menuState );
static int EventCheckLobby( Button_t *tabOrder , int *marked , int *menuState );
static int EventCheck( Button_t *tabOrder , int *marked , int *menuState );
static int RenderMainMenu( Button_t *tabOrder, int *marked, SDL_Renderer *renderer, SDL_Window *sdlWindow, int *menuState );
static int RenderLobby( Button_t *tabOrder , int *marked , SDL_Renderer *renderer , SDL_Window *sdlWindow , int *menuState );
static int Render ( Button_t *tabOrder, int *marked, SDL_Renderer *renderer, SDL_Window *sdlWindow, int *menuState );
static int Menu( SDL_Window *sdlWindow, SDL_Renderer* renderer , int *marked , int *menuState, Button_t *tabOrder );
static void GoDown( int *marked );
static void GoUp( int *marked );

static void HostGame( void );
static void JoinGame( void );
static void Options( void );

/*
==========================================================
function that loops the menu. To be called in the main unit.
==========================================================
*/
static void TextInput( char *description, char *text, SDL_Renderer *renderer, SDL_Window *sdlWindow ) {
	int 		w, h, done = 0;
	char		array[40];
	SDL_Rect	inputRect;
	TTF_Font *	sans = TTF_OpenFont( SANS_FONT_FILE, 500 );
	SDL_Color 	color = { 0, 255, 0 };

	DebugPrintF( "TextInput( %s, %s, %d, %d ) called.", description, text, renderer, sdlWindow );
	SDL_GetWindowSize( sdlWindow, &w, &h );
	DebugPrintF( "SDL_GetWindowSize returned %d x %d pixels.", w, h );
	SDL_StartTextInput();
	DebugPrintF( "Started text input." );
	while ( !done ) {
		SDL_Event event;
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

		SDL_RenderClear( renderer );
		strcpy( array, description );
		SDL_Surface* surfaceMessage = TTF_RenderText_Solid( sans, strcat( array, text ), color );
		SDL_Texture* message = SDL_CreateTextureFromSurface( renderer, surfaceMessage );
		inputRect.w = 300;
		inputRect.h = 70 ;
		inputRect.x = w /2 - inputRect.w/2;
		inputRect.y = h /2 - inputRect.h/2;
		SDL_RenderCopy( renderer, message, NULL, &inputRect );
		SDL_RenderPresent( renderer );
		SDL_DestroyTexture( message );
		SDL_FreeSurface( surfaceMessage );
	}
	DebugPrintF( "Text input has finished." );
}

int ShowMenu( void ) {
	DebugPrintF( "ShowMenu called." );
	DebugAssert( !SDL_Init( SDL_INIT_VIDEO ) );
	DebugAssert( !TTF_Init() );
	username = malloc( sizeof( char ) * 30 );
	username[0] = '\0';
	int marked = 0;
	int menuState = 1;
	SDL_Window *sdlWindow = SDL_CreateWindow( "Hello World!", 100, 100, 1024, 768, SDL_WINDOW_SHOWN );
	DebugAssert( sdlWindow );
	SDL_Renderer* renderer = NULL;
	renderer = SDL_CreateRenderer( sdlWindow, -1, SDL_RENDERER_ACCELERATED );
	DebugAssert( renderer );
	Button_t tabOrder[4];
	InitializeMenuElements( tabOrder, renderer, sdlWindow );
	TextInput( "Username:", username, renderer, sdlWindow );
	while( 1 ) {
		if( Menu( sdlWindow, renderer, &marked, &menuState, tabOrder ) == 1 ) {
			break;
		}
	}
	return 1;
}

/*
==========================================================
Menu function. Checks for input applies the input then
renders the buttons.
==========================================================
*/
static int Menu ( SDL_Window *sdlWindow, SDL_Renderer* renderer , int *marked , int *menuState, Button_t *tabOrder  ) {
	if ( EventCheck( tabOrder, marked, menuState ) == 1 ) {
		return 1;
	}
	return Render( tabOrder, marked, renderer, sdlWindow, menuState );
}

/*
==========================================================
Checks for keyboard input and switches button states
==========================================================
*/
static int EventCheckMainMenu (Button_t *tabOrder , int *marked , int *menuState ) {
	SDL_Event event;
	while( SDL_PollEvent( &event ) ){
		if( event.type == SDL_KEYDOWN ) {
			switch ( event.key.keysym.sym ){
				case SDLK_UP:
					GoUp( marked );
					printf( "%d", *marked );
					break;
				case SDLK_DOWN:
					GoDown( marked );
					printf( "%d", *marked );
					break;
				case SDLK_RETURN:
					switch ( *marked ){
						case 0:
							*menuState = 2;
							 HostGame();
							break;
						case 1:
							JoinGame();
							break;
						case 2:
							Options();
							break;
						case 3:
							return 1;
							break;
					}
					break;
			}
		}
	}
	return 0;
}

static int EventCheckLobby (Button_t *tabOrder , int *marked , int *menuState ) {
	SDL_Event event;
	while( SDL_PollEvent( &event ) ){
		switch( event.type ){
			case SDL_KEYDOWN:
				switch ( event.key.keysym.sym ){
					case SDLK_BACKSPACE:
						*menuState = 1;
						break;
					case SDLK_RETURN:
						printf("StartGame");
						break;
					default:
						break;
				}
		}
	}
	return 0;
}

static int EventCheck( Button_t *tabOrder , int *marked , int *menuState ) {
	switch( *menuState ){
		case 1:
			return EventCheckMainMenu(tabOrder,marked,menuState);
			break;
		case 2:
			return EventCheckLobby(tabOrder,marked,menuState);
			break;
	}
	return 0;
}


static void Options( void ) {
	// TODO: Implement
}

static void HostGame( void ) {
	Connect( 1, 0, NETWORK_STANDARD_SERVER_PORT );
}

static void JoinGame( void ) {
	Connect( 0, "127.0.0.1", NETWORK_STANDARD_SERVER_PORT );
}

void GetUserName( char* Name ){
	strcpy( Name, username );
}

/*
==========================================================
Renders all the buttons
==========================================================
*/
static int RenderLobby( Button_t *tabOrder , int *marked , SDL_Renderer *renderer , SDL_Window *sdlWindow , int *menuState ) {
	int 		w, h, i, n;
	char *		playerNames[6];
	TTF_Font *	sans = TTF_OpenFont( "Verdana.ttf", 500 );
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

		printf( "%s :: %d \n", playerNames[i], n );
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
	SDL_RenderCopy( renderer, startButton.Selected, NULL, &startRect );
	SDL_RenderPresent( renderer );
	return 0;
}

static int RenderMainMenu( Button_t *tabOrder, int *marked, SDL_Renderer *renderer, SDL_Window *sdlWindow, int *menuState ) {
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
		buttonRect.w = tabOrder[i].Breite;
		buttonRect.h = tabOrder[i].Hoehe;
		if ( *marked == i ){
			SDL_RenderCopy( renderer, tabOrder[i].Selected, NULL, &buttonRect );
		} else {
			SDL_RenderCopy( renderer, tabOrder[i].NotSelected, NULL, &buttonRect );
		}
	}
	SDL_RenderPresent( renderer );
	return 0;
}


static int Render ( Button_t *tabOrder, int *marked, SDL_Renderer *renderer, SDL_Window *sdlWindow, int *menuState ) {
	switch( *menuState ) {
		case 1:
			return RenderMainMenu(tabOrder,marked,renderer,sdlWindow,menuState);
			break;
		case 2:
			return RenderLobby(tabOrder,marked,renderer,sdlWindow,menuState);
			break;
	}
	return 0;
}

/*
==========================================================
Sets the RGB values of a button
==========================================================
*/
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
	SDL_GetWindowSize( sdlWindow, &w, &h );
	DebugPrintF( "SDL_GetWindowSize returned %d x %d pixels.", w, h );
	SDL_Surface *temp;
	temp = IMG_Load( "ButtonImages/Start.png" );
	startButton.Selected = SDL_CreateTextureFromSurface( renderer, temp );
	temp = IMG_Load( "ButtonImages/Start(Disabled).png" );
	startButton.NotSelected = SDL_CreateTextureFromSurface( renderer, temp );
	temp = IMG_Load( "ButtonImages/Frame.png" );
	frameTexture = SDL_CreateTextureFromSurface( renderer, temp );
	temp = IMG_Load( "ButtonImages/Titel.png" );
	titleTexture = SDL_CreateTextureFromSurface( renderer, temp );
	temp = IMG_Load( "Background.png" );
	backgroundTexture = SDL_CreateTextureFromSurface( renderer, temp );
	temp = IMG_Load( "ButtonImages/HostGame(Selected).png" );
	tabOrder[0].Selected = SDL_CreateTextureFromSurface( renderer, temp );
	temp = IMG_Load( "ButtonImages/HostGame(Unselected).png" );
	tabOrder[0].NotSelected = SDL_CreateTextureFromSurface( renderer, temp );
	temp = IMG_Load( "ButtonImages/JoinGame(Selected).png" );
	tabOrder[1].Selected = SDL_CreateTextureFromSurface( renderer, temp );
	temp = IMG_Load( "ButtonImages/JoinGame(Unselected).png" );
	tabOrder[1].NotSelected = SDL_CreateTextureFromSurface( renderer, temp );
	temp = IMG_Load( "ButtonImages/Options(Selected).png" );
	tabOrder[2].Selected = SDL_CreateTextureFromSurface( renderer, temp );
	temp = IMG_Load( "ButtonImages/Options(Unselected).png" );
	tabOrder[2].NotSelected = SDL_CreateTextureFromSurface( renderer, temp );
	temp = IMG_Load( "ButtonImages/Exit(Selected).png" );
	tabOrder[3].Selected = SDL_CreateTextureFromSurface( renderer, temp );
	temp = IMG_Load( "ButtonImages/Exit(Unselected).png" );
	tabOrder[3].NotSelected = SDL_CreateTextureFromSurface( renderer, temp );
	DebugPrintF( "Loaded all menu images." );
	DebugAssert( startButton.Selected && startButton.NotSelected && frameTexture && titleTexture && backgroundTexture );

	tabOrder[0].Hoehe = 0.15f * h;
	tabOrder[1].Hoehe = 0.15f * h;
	tabOrder[2].Hoehe = 0.15f * h;
	tabOrder[3].Hoehe = 0.15f * h;
	tabOrder[0].Breite = 0.5f * w;
	tabOrder[1].Breite = 0.5f * w;
	tabOrder[2].Breite = 0.5f * w;
	tabOrder[3].Breite = 0.5f * w;
	tabOrder[0].x = (w / 2) - (tabOrder[0].Breite / 2);
	tabOrder[1].x = (w / 2) - (tabOrder[1].Breite / 2);
	tabOrder[2].x = (w / 2) - (tabOrder[2].Breite / 2);
	tabOrder[3].x = (w / 2) - (tabOrder[3].Breite / 2);
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
