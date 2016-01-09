#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include "Debug.h"

static FILE *fp;

int InitializeDebug() {
	fp = fopen("Debug.log", "w");
	if( fp ) {
			fprintf( fp, "\n\n====================\nDebugger initialized.\n====================\n" );
	}
	return fp != NULL;
}

int DebugPrintF( const char *format , ... ){
	va_list args;
	time_t t;
	time( &t );
	fprintf( fp, "%s", ctime( &t ) );
	char *Formatstring;
	Formatstring = malloc( strlen( format ) + 3 );
	strcpy( Formatstring, format );
	strcat( Formatstring, "\n" );

	va_start( args, format );
	vfprintf( fp, Formatstring, args );
	va_end( args );

	va_start( args, format );
	vprintf( Formatstring, args );
	va_end( args );

	fflush( fp );
	return 0;
}

int CloseDebug( void ) {
	fclose( fp );
	return 0;
}

