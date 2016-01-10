#ifndef _PHYSICS_H
#define _PHYSICS_H

typedef void ( *registerHitHandler_t )( int player );

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
int				LastHit( void );
struct Line2D	GetPlayerLine( int player, int numPlayers );

#endif
