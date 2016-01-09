#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_net.h>
#include "Network.h"

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

static void GoDown( int *Marked );
static void GoUp( int *Marked );

static SDL_Texture *Background;
static SDL_Texture *Titel;
static SDL_Texture *Frame;
static Button_t 	StartKnopf;
static char * 		UserName;

/*
==========================================================
function that loops the menu. To be called in the main unit.
==========================================================
*/
void TextInput( char *description, char *text, SDL_Renderer *renderer, SDL_Window *m_window ){
    int 		w, h, done = 0;
    int 		cursor;
    int 		selectionLength;
    char * 		composition;
    char		array[40];
    SDL_Rect	inputRect;
    TTF_Font *	sans = TTF_OpenFont( "Verdana.ttf", 500 );
    SDL_Color 	color = { 0, 255, 0 };

    SDL_GetWindowSize( m_window, &w, &h );
    SDL_StartTextInput();
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
                case SDL_TEXTEDITING:
                    /*
                    Update the composition text.
                    Update the cursor position.
                    Update the selection length (if any).
                    */
                    composition = event.edit.text;
                    cursor = event.edit.start;
                    selectionLength = event.edit.length;
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
        SDL_DestroyTexture( message );
        SDL_FreeSurface( surfaceMessage );
        SDL_RenderPresent( renderer );
    }
}


int ShowMenu () {
    InitializeNetwork();
    SDL_Init(SDL_INIT_VIDEO);
    UserName = malloc(sizeof(char)*30);
    UserName[0] = '\0';
    TTF_Init();
    int Marked,MenuState;
    Marked = 0;
    MenuState = 1;
    SDL_Window *m_window = SDL_CreateWindow("Hello World!", 100, 100, 1980, 1020, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = NULL;
    renderer = SDL_CreateRenderer( m_window, -1, SDL_RENDERER_ACCELERATED );
    Button_t TabOrder[4];
	InitializeMenuElements(TabOrder,renderer,m_window);
	TextInput("Username:",UserName,renderer,m_window);
	while(1){
		if ( Menu(m_window,renderer,&Marked,&MenuState,TabOrder) == 1 ){
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
int Menu ( SDL_Window *m_window, SDL_Renderer* renderer , int *Marked , int *MenuState, Button_t *TabOrder  ){


	if ( EventCheck(TabOrder,Marked,MenuState) == 1 ) {
 		return 1;
	}
	Render(TabOrder,Marked,renderer,m_window,MenuState,MenuState);
}







/*
==========================================================
Checks for keyboard input and switches button states
==========================================================
*/


int EventCheckMainMenu (Button_t *TabOrder , int *Marked , int *MenuState ){

 SDL_Event event;
	while (SDL_PollEvent( &event )){
		switch( event.type ){
			case SDL_KEYDOWN:
				switch ( event.key.keysym.sym ){
					case SDLK_UP:
						GoUp(Marked);
						printf("%d",*Marked);
						break;
					case SDLK_DOWN:
						GoDown(Marked);
						printf("%d",*Marked);
						break;
					case SDLK_RETURN:
						switch ( *Marked ){
							case 0:
                            *MenuState = 2;
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
					default:
						break;
				}
		}
	}
	return 0;
}

int EventCheckLobby (Button_t *TabOrder , int *Marked , int *MenuState ){
 SDL_Event event;
	while (SDL_PollEvent( &event )){
		switch( event.type ){
			case SDL_KEYDOWN:
				switch ( event.key.keysym.sym ){
					case SDLK_BACKSPACE:
                        *MenuState = 1;
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


int EventCheck ( Button_t *TabOrder , int *Marked , int *MenuState ) {
       switch(*MenuState){
        case 1:
        EventCheckMainMenu(TabOrder,Marked,MenuState);
        break;
        case 2:
        EventCheckLobby(TabOrder,Marked,MenuState);
        break;
        case 3:
        break;
        }
}


void Options() {

}

void HostGame () {
    Connect(1,0,NETWORK_STANDARD_SERVER_PORT);
}

void JoinGame() {
    Connect(0,"127.0.0.1",NETWORK_STANDARD_SERVER_PORT);
}

void GetUserName (char*Name){
    strcpy(Name,UserName);
}

/*
==========================================================
Renders all the buttons
==========================================================
*/

int RenderLobby(TabOrder,Marked,renderer,m_window,MenuState){
    int w,h,i,n;
    char *Playernames[6];
    GetPlayerList(Playernames,&n);
    ProcessLobby();
    TTF_Font* Sans = TTF_OpenFont("Verdana.ttf", 500);
    SDL_Color White = {255, 255, 255};


    SDL_Rect Start_rect;
    SDL_Rect Frame_rect;
    SDL_Rect Background_rect;
    SDL_GetWindowSize(m_window,&w,&h);

    Start_rect.w = 0.2 *h;
    Start_rect.h = 0.2 *h;
    Start_rect.x = w - Start_rect.w -30;
    Start_rect.y = h - Start_rect.w -30;
    Frame_rect.h = h;
    Frame_rect.w = w/2;
    Frame_rect.x = w/2 - Frame_rect.w;
    Frame_rect.y = 0;
    Background_rect.w = w;
    Background_rect.h = h;
    Background_rect.x = 0;
    Background_rect.y = 0;
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer,Background,NULL,&Background_rect);
    for (i = 0; i < n; i++){
        SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, Playernames[i], White);
        SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

        printf("%s :: %d \n",Playernames[i],n);
        SDL_Rect Player_rect;
        Player_rect.w = Frame_rect.w /2;
        Player_rect.h = Frame_rect.h * 0.05;
        Player_rect.x = (Frame_rect.w /2) - Player_rect.w /2;
        Player_rect.y = (i * Player_rect.h*1.8f)  + Player_rect.h *5;
        SDL_RenderCopy(renderer,Message,NULL,&Player_rect);
        SDL_DestroyTexture(Message);
        SDL_FreeSurface(surfaceMessage);


    }
    SDL_RenderCopy(renderer,Frame,NULL,&Frame_rect);
    SDL_RenderCopy(renderer,StartKnopf.Selected,NULL,&Start_rect);

    SDL_RenderPresent(renderer);




}
int RenderMainMenu(Button_t *TabOrder , int *Marked , SDL_Renderer * renderer , SDL_Window *m_window , int *MenuState) {
    int w,h,i;
    SDL_Rect Background_rect;
    SDL_Rect Button_rect;
    SDL_Rect Titel_rect;
    SDL_GetWindowSize(m_window,&w,&h);
    Background_rect.w = w;
    Background_rect.h = h;
    Background_rect.x = 0;
    Background_rect.y = 0;
    Titel_rect.w = w * 0.7;
    Titel_rect.h = h/5;
    Titel_rect.x = (w/2) - Titel_rect.w/2;
    Titel_rect.y = 0;

    SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer,Background,NULL,&Background_rect);
    SDL_RenderCopy(renderer,Titel,NULL,&Titel_rect);

    for ( i = 0; i<4; i++){
    Button_rect.x = TabOrder[i].x;
    Button_rect.y = TabOrder[i].y;
    Button_rect.w = TabOrder[i].Breite;
    Button_rect.h = TabOrder[i].Hoehe;
    if (*Marked == i){
    SDL_RenderCopy(renderer,TabOrder[i].Selected,NULL,&Button_rect);
    } else {
    SDL_RenderCopy(renderer,TabOrder[i].NotSelected,NULL,&Button_rect);
    }

    }


    SDL_RenderPresent(renderer);




}


int Render ( Button_t *TabOrder , int *Marked , SDL_Renderer * renderer , SDL_Window *m_window , int *MenuState ) {
    switch(*MenuState){
        case 1:
        RenderMainMenu(TabOrder,Marked,renderer,m_window,MenuState);
        break;
        case 2:
        RenderLobby(TabOrder,Marked,renderer,m_window,MenuState);
        break;
        case 3:
        break;

    }

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
void InitializeMenuElements ( Button_t *TabOrder , SDL_Renderer *renderer , SDL_Window* m_window ){
    int w,h;
    SDL_GetWindowSize(m_window,&w,&h);
    SDL_Surface *temp;
    temp = IMG_Load("ButtonImages/Start.png");
    StartKnopf.Selected = SDL_CreateTextureFromSurface(renderer,temp);
    temp = IMG_Load("ButtonImages/Start(Disabled).png");
    StartKnopf.NotSelected = SDL_CreateTextureFromSurface(renderer,temp);
    temp = IMG_Load("ButtonImages/Frame.png");
    Frame = SDL_CreateTextureFromSurface(renderer,temp);
    temp = IMG_Load("ButtonImages/Titel.png");
    Titel = SDL_CreateTextureFromSurface(renderer,temp);
    temp = IMG_Load("Background.png");
    Background = SDL_CreateTextureFromSurface(renderer,temp);
    temp = IMG_Load("ButtonImages/HostGame(Selected).png");
    TabOrder[0].Selected = SDL_CreateTextureFromSurface(renderer,temp);
    temp = IMG_Load("ButtonImages/HostGame(Unselected).png");
    TabOrder[0].NotSelected = SDL_CreateTextureFromSurface(renderer,temp);
    temp = IMG_Load("ButtonImages/JoinGame(Selected).png");
    TabOrder[1].Selected = SDL_CreateTextureFromSurface(renderer,temp);
    temp = IMG_Load("ButtonImages/JoinGame(Unselected).png");
    TabOrder[1].NotSelected = SDL_CreateTextureFromSurface(renderer,temp);
    temp = IMG_Load("ButtonImages/Options(Selected).png");
    TabOrder[2].Selected = SDL_CreateTextureFromSurface(renderer,temp);
    temp = IMG_Load("ButtonImages/Options(Unselected).png");
    TabOrder[2].NotSelected = SDL_CreateTextureFromSurface(renderer,temp);
    temp = IMG_Load("ButtonImages/Exit(Selected).png");
    TabOrder[3].Selected = SDL_CreateTextureFromSurface(renderer,temp);
    temp = IMG_Load("ButtonImages/Exit(Unselected).png");
    TabOrder[3].NotSelected = SDL_CreateTextureFromSurface(renderer,temp);

	TabOrder[0].Hoehe = 0.15f * h;
	printf("%d",h);
	TabOrder[1].Hoehe = 0.15f * h;
	TabOrder[2].Hoehe = 0.15f * h;
	TabOrder[3].Hoehe = 0.15f * h;
	TabOrder[0].Breite = 0.5f * w;
	TabOrder[1].Breite = 0.5f * w;
	TabOrder[2].Breite = 0.5f * w;
	TabOrder[3].Breite = 0.5f * w;
	TabOrder[0].x = (w / 2) - (TabOrder[0].Breite / 2);
	TabOrder[1].x = (w / 2) - (TabOrder[1].Breite / 2);
	TabOrder[2].x = (w / 2) - (TabOrder[2].Breite / 2);
	TabOrder[3].x = (w / 2) - (TabOrder[3].Breite / 2);
	TabOrder[0].y = h/5;
	TabOrder[1].y = (2*h)/5;
	TabOrder[2].y = (3*h)/5;
 	TabOrder[3].y = (4*h)/5;
}


/*
==========================================================
Function to GoUp one Button. Wraps if the uppermost button is selected.
==========================================================
*/
static void GoUp( int *Marked ) {
	if ( *Marked > 0 ){
		(*Marked)--;
	} else {
    	*Marked = 3;
	}
}

/*
==========================================================
Function to GoDown one Button. Wraps if the lowermost button is selected.
==========================================================
*/
static void GoDown( int *Marked) {
	if ( *Marked < 3 ){
		(*Marked)++;
	} else {
    	*Marked = 0;
	}

}

