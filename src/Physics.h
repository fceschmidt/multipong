#ifndef _PHYSICS_H
#define _PHYSICS_H

#include "Game.h"

#define PADDLE_SIZE 0.1f

typedef void ( *registerHitHandler_t )( int player );
typedef void ( *registerPointHandler_t )( const struct GameState *state, int player );

/*
==========================================================

Describes a line in two-dimensional space.

==========================================================
*/
struct Line2D {
	struct Point2D	point;
	struct Vector2D	vector;	// The vector which we can add x times to point in order to get any point on the line.
};

int				InitializePhysics( void );
int				ProcessPhysics( struct GameState *state, float deltaSeconds );
void			AtRegisterHit( registerHitHandler_t handler );
void			RegisterHit( int player );
void			AtRegisterPoint( registerPointHandler_t handler );
int				LastHit( void );
struct Line2D	GetPlayerLine( int player, int numPlayers );
struct Vector2D	ScaleVector2D( struct Vector2D vector, float scalar );
struct Point2D	AddVectorToPoint2D( struct Point2D point, struct Vector2D vector );
void			InitializeBall( struct Ball *ball );

#endif
