#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <string.h>
#include "Audio.h"
#include "Physics.h"

#define PATH "Assets/Audio/"

/*
====================
InitializeAudio

Initializes SDL_mixer
====================
*/
void InitializeAudio() {
	if( SDL_Init( SDL_INIT_AUDIO ) < 0 )
		printf( "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError() );
	if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0 )
		printf( "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError() );	
	Mix_Music *theme = NULL;
	theme = Mix_LoadMUS( PATH "MainTheme.wav" );
	if( !theme )
		printf( "Mix_LoadMUS( \"Musik\" ): %s\n", Mix_GetError() );
	if( Mix_PlayMusic( theme, -1 ) < 0 )
		printf( "Mix_PlayMusic: %s\n", Mix_GetError() );
	AtRegisterHit( PlaySoundHit );
	AtRegisterPoint( PlaySoundPoint );
} //TODO: Implement into Program and tie into debugging?

/*
====================
CloseAudio

Closes SDL_mixer
====================
*/
void CloseAudio() {
	Mix_Quit();
} //TODO: Implement into Program

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
} //TODO: Implement into Physics probably?

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
} //TODO: Implement into Physics probably?
