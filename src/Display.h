#ifndef _OUTPUT_H
#define _OUTPUT_H

#include "Game.h"

int InitializeGraphics( void );
int DisplayGameState( struct GameState *state );
void CloseDisplay( void );

#endif
