#ifndef _MAIN_C
#define _MAIN_C

/*
========================================

An enumeration that describes the state of the program in the switch statement of the main function.

========================================
*/
enum ProgramState {
	PS_QUIT,
	PS_MENU,
	PS_GAME
};

#define ASSET_FOLDER "Assets/"
#define SANS_FONT_FILE ASSET_FOLDER "ocraextended.ttf"

#endif
