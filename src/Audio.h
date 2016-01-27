#ifndef AUDIO_H
#define AUDIO_H

#include "Game.h"

void InitializeAudio( void );
void CloseAudio ( void );
void PlayMusic( void );
void PlaySoundHit( int player );
void PlaySoundPoint( const struct GameState *state, int player );

#endif
