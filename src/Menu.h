#ifndef _MENU_H
#define _MENU_H

/*
==========================================================

An enumeration for the two sides that can be taken in the game.

==========================================================
*/
enum Side {
	SI_GOOD = 0,
	SI_EVIL = 1,
	SI_UNDECIDED
};

int ShowMenu( void );
int InitializeMenu( void );
enum Side GetSide( void );

#endif
