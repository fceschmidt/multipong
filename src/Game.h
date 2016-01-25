#ifndef _GAME_H
#define _GAME_H

/*
==========================================================

A movement vector in two-dimensional euclidean space.

==========================================================
*/
struct Vector2D {
	float dx;
	float dy;
};

/*
==========================================================

A point in two-dimensional euclidean space.

==========================================================
*/
struct Point2D {
	float x;
	float y;
};

/*
==========================================================

Information about the pong ball.

==========================================================
*/
struct Ball {
	struct Point2D	position;
	struct Vector2D	direction;
};

/*
==========================================================

Information about a player in the game.

==========================================================
*/
struct Player {
	char *	name;
	float	position;
	int		score;
};

/*
==========================================================

Indicates the current state of the game.

==========================================================
*/
struct GameState {
	int 			numPlayers;
	struct Player *	players;
	struct Ball		ball;
};

enum ProgramState	RunGame( void );

#endif
