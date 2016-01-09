#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include "Debug.h"

static FILE *fp;

int InitializeDebug() {
	fp = fopen("Debug.log", "at");
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

	va_start( args, Formatstring );
	vprintf( Formatstring, args );
	va_end( args );
	return 0;
}

int CloseDebug( void ) {
	fclose( fp );
	return 0;
}

