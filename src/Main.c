#include "Program.h"
#include "Menu.h"
#include "Main.h"
#include "Game.h"

/*
====================
main

Entry point of the application. Starts the program.
====================
*/
int main( int argc, char *argv[] ) {
	int result;
	enum ProgramState mode = PS_MENU;

	// Initialize and return if unsuccessful.
	result = InitializeProgram( argc, argv );
	if( result ) {
		return result;
	}

	// Control loop
	while( mode != PS_QUIT ) {
		switch( mode ) {
			case PS_MENU:
				mode = ShowMenu();
				break;
			case PS_GAME:
				// Go to game loop...
				mode = RunGame();
				break;
			default:
				// What is this? Someone broke our mode value. Print something and exit.
				DebugPrintF( "There exists no handler for mode = %d! Quitting.", ( int )mode );
				mode = PS_QUIT;
				break;
		}
	}

	// Return 0 otherwise.
	CloseProgram( 0 );
	return 0;
}
