#include "Program.h"
#include <stdlib.h>
#include <stdio.h>

static void DestroyResources( void );

/*
====================
InitializeProgram

Sets the execution settings for the program according to the command line arguments.
====================
*/
int InitializeProgram( int argc, char *argv[] ) {
	// Register DestroyResources with atexit.
	atexit( DestroyResources );

	// TODO: Read command line arguments.
	
	// TODO: Remove this hello message and print it in debug.
	printf( "Successfully started multipong.\n" );
	return 0;
}

/*
====================
CloseProgram

Calls the destructors of all active components and returns the code passed.
====================
*/
void CloseProgram( int returnCode ) {
	exit( returnCode );
}

/*
====================
DestroyResources

Destroys all resources used by the program.
====================
*/
static void DestroyResources( void ) {
	// TODO: Call all destructors.
}
