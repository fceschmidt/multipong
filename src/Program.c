#include "Program.h"
#include "Physics.h"
#include "Network.h"
#include "Display.h"
#include "Menu.h"
#include "Debug/Debug.h"
#include "Audio.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static void DestroyResources( void );
static int ReadArguments( int argc, char *argv[] );

static int ArgumentHelp( void );
static int ArgumentFullscreen( void );
static int ArgumentWindowed( void );

/*
==========================================================

A function that is used to set a command line option.

==========================================================
*/
typedef int( *setArgumentFunction_t )( void );

/*
==========================================================

A couple of a name and the implementation function of a command line option.

==========================================================
*/
struct ArgumentNameFunctionCouple {
	const char *			name;
	setArgumentFunction_t 	function;
};

// The map of names and functions for the command line options parser.
// TODO: Add complete list of options HERE.
static struct ArgumentNameFunctionCouple argumentNameFunctionMap[] = {
	{ .name = "--help", .function = &ArgumentHelp },
	{ .name = "--fullscreen", .function = &ArgumentFullscreen },
	{ .name = "--windowed", .function = &ArgumentWindowed }
};

// Imported from Output.
extern int outputFullscreen;

/*
====================
InitializeProgram

Sets the execution settings for the program according to the command line arguments.
====================
*/
int InitializeProgram( int argc, char *argv[] ) {
	// Register DestroyResources with atexit.
	atexit( DestroyResources );

	// Initialize debug component.
	InitializeDebug();

	// Seed the randomizer.
	srand( time ( NULL ) );

	// Read command line arguments.
	ReadArguments( argc, argv );

	InitializeNetwork();
	InitializeGraphics();
	InitializePhysics();
	InitializeMenu();
	InitializeAudio();

	// Print hello message in debug.
	DebugPrintF( "Successfully started multipong." );
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
	CloseDisplay();
	CloseAudio();
	CloseDebug();
}

/*
====================
ReadArguments

Reads the arguments from the command line.
	--help		prints a help message
	--windowed	executes in windowed mode
For full list of options, see function pointer list above.
====================
*/
static int ReadArguments( int argc, char *argv[] ) {
	int i;
	int numImplementedArgs = sizeof( argumentNameFunctionMap ) / sizeof( struct ArgumentNameFunctionCouple );
	int j;
	int result;

	// Go through the arguments
	for( i = 0; i < argc; i++ ) {
		// Go through the implemented arguments map
		for( j = 0; j < numImplementedArgs; j++ ) {
			// If any entry's name matches the option:
			if( !strcmp( argumentNameFunctionMap[j].name, argv[i] ) ) {
				// Call the function from the map entry.
				result = argumentNameFunctionMap[j].function();
				// If it failed, return the failure.
				if( result ) {
					return result;
				}
			}
		}
	}

	return 0;
}

/*
====================
ArgumentHelp

Implements behaviour when the program is started with --help.
Prints a help message.
====================
*/
static int ArgumentHelp( void ) {
	printf( "Welcome to multipong. Your options:\n"
			"  --help         Prints this message\n"
			"  --fullscreeen  Executes in full-screen mode\n"
			"  --windowed     Executes in windowed mode\n" );
	return 0;
}

/*
====================
ArgumentFullscreen

Sets the fullscreen execution flag.
====================
*/
static int ArgumentFullscreen( void ) {
	outputFullscreen = 1;
	return 0;
}

/*
====================
ArgumentWindowed

Unsets the fullscreen execution flag.
====================
*/
static int ArgumentWindowed( void ) {
	outputFullscreen = 0;
	return 0;
}

