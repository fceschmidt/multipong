#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <string.h>
#include "Audio.h"
#include "Physics.h"
#include "Debug/Debug.h"
#include "Menu.h"

#define PATH "Assets/Audio/"

/*
====================
InitializeAudio

Initializes SDL_mixer
====================
*/
void InitializeAudio() {
	if( SDL_Init( SDL_INIT_AUDIO ) < 0 ) {
		DebugPrintF( "SDL_mixer could not initialize! SDL_mixer Error: %s", Mix_GetError() );
	}
	
	if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0 ) {
		DebugPrintF( "SDL_mixer could not initialize! SDL_mixer Error: %s", Mix_GetError() );	
	}
	
	AtRegisterHit( PlaySoundHit );
	AtRegisterPoint( PlaySoundPoint );
}

/*
====================
PlayMusic

Plays the music file based on GetSide from the menu component.
====================
*/
void PlayMusic( void ) {
	Mix_Music *theme = NULL;
	char *Filename = malloc( strlen( PATH ) + 30 );
	strcpy( Filename, PATH );
	switch( GetSide() ) {
		case SI_GOOD:
			strcat( Filename, "Vodka.ogg" );
			break;
		case SI_EVIL:
			strcat( Filename, "Freedom.ogg" );
			break;
		default:
			DebugPrintF( "What went wrong here?" );
			return;
	}

	theme = Mix_LoadMUS( Filename );

	DebugAssert( theme );

	if( Mix_PlayMusic( theme, -1 ) < 0 ) {
		DebugPrintF( "Mix_PlayMusic: %s", Mix_GetError() );
	}
}

/*
====================
CloseAudio

Closes SDL_mixer
====================
*/
void CloseAudio() {
	Mix_Quit();
}

/*
====================
PlaySoundHit

Plays soundeffect for a hit
====================
*/
void PlaySoundHit( int player ) {
	Mix_Chunk *ping = NULL;
	ping = Mix_LoadWAV( PATH "ping.wav" );
	Mix_PlayChannel( -1, ping, 0 );
//	Mix_FreeChunk( ping );
//	ping = NULL;
}

/*
====================
PlaySoundPoint

Plays soundeffect for a scored point
====================
*/
void PlaySoundPoint( const struct GameState *state, int player ) {
	Mix_Chunk *success = NULL;
	success = Mix_LoadWAV( PATH "success.wav" );
	Mix_PlayChannel( -1, success, 0 );
//	Mix_FreeChunk( success );
//	success = NULL;
}
