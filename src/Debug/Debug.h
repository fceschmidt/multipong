#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED


int InitializeDebug();
int DebugPrintF (const char *format , ... ) ;
int CloseDebug ();

#define DebugAssert( x ) if ( !( x ) ) { \
	DebugPrintF( "----[FAIL]----: %s.", #x );\
} else { \
	DebugPrintF( "++++[PASS]++++: %s", #x );\
}

#endif // DEBUG_H_INCLUDED
