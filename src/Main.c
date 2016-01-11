#include "Program.h"
#include "Menu.h"
#include "Main.h"

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

	// TODO: Implement the rest.
	while( mode != PS_QUIT ) {
		switch( mode ) {
			case PS_MENU:
				mode = ShowMenu();
				break;
			case PS_GAME:
				// TODO: Go to game loop...
				mode = PS_MENU;
				break;
			default:
				break;
		}
		
	}
	
	// Return 0 otherwise.
	CloseProgram( 0 );
	return 0;
}
