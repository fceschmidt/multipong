#include "Program.h"

/*
====================
main

Entry point of the application. Starts the program.
====================
*/
int main( int argc, char *argv[] ) {
	int result;

	// Initialize and return if unsuccessful.
	result = InitializeProgram( argc, argv );
	if( result ) {
		return result;
	}

	// TODO: Implement the rest.
	
	// Return 0 otherwise.
	return 0;
}
