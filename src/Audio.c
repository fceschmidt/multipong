#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <string.h>
#include "Audio.h"
#include "Physics.h"
#include "Debug/Debug.h"

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
	
	Mix_Music *theme = NULL;
	theme = Mix_LoadMUS( PATH "MainTheme.ogg" );
	
	if( !theme ) {
		DebugPrintF( "Mix_LoadMUS( \"Musik\" ): %s", Mix_GetError() );
		theme = Mix_LoadMUS( PATH "MainTheme.wav" );
		if( !theme ) {
			DebugPrintF( "Mix_LoadMUS( \"Musik\" ): %s", Mix_GetError() );
		}
	}

	if( Mix_PlayMusic( theme, -1 ) < 0 ) {
		DebugPrintF( "Mix_PlayMusic: %s", Mix_GetError() );
	}
	
	AtRegisterHit( PlaySoundHit );
	AtRegisterPoint( PlaySoundPoint );
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
