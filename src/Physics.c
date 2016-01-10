#include <math.h>
#include <SDL2/SDL.h>
#include "Physics.h"
#include "Game.h"
#include "Debug/Debug.h"

#define DEGREES_TO_RADIANS( x ) ( ( x ) * M_PI / 180.0f )
#define PADDLE_SIZE 0.1f
#define PADDLE_ACCELERATION 1.0f	// DISTANCE PER SECOND SQUARED
#define PADDLE_MAX_SPEED 0.5f		// DISTANCE PER SECOND
#define DEFAULT_BALL_SPEED 1.0f		// DISTANCE PER SECOND

/*
==========================================================

Describes a circle in two-dimensional euclidean space.

==========================================================
*/
struct Circle2D {
	struct Point2D	point;
	float			radius;
};

// VARIABLES

static registerHitHandler_t	rhHandler = NULL;
static int					lastHit = -1;
static const unsigned char *sdlKeyArray = NULL;
static int					clockwiseKey = SDLK_LEFT;
static int					counterclockwiseKey = SDLK_RIGHT;
static float				userPaddleSpeed = 0.0f;

// FUNCTIONS

static struct Vector2D 	ScaleVector2D( struct Vector2D vector, float scalar );
static struct Point2D	AddVectorToPoint2D( struct Point2D point, struct Vector2D vector );
static float			VectorNorm2D( struct Vector2D vector );
static int				GetPointSegment( struct Point2D point, int numPlayers );
static struct Vector2D	DeltaVector2D( struct Point2D point1, struct Point2D point2 );
static float			GetVectorAngle2D( struct Vector2D vector1, struct Vector2D vector2 );
static float			ScalarProduct2D( struct Vector2D vector1, struct Vector2D vector2 );
static void				LineCircleCollision2D( struct Circle2D circle, struct Line2D line, int *isRight, float *projection );
static void				HandleInput( struct GameState *state, float deltaSeconds );

// Imported from the network component.
extern int				IsServer( void );
extern int				ThisClient( void );

/*
====================
ScaleVector2D

Given a vector and a scalar, scales the vector by the scalar.
====================
*/
static struct Vector2D ScaleVector2D( struct Vector2D vector, float scaling ) {
	vector.dx *= scaling;
	vector.dy *= scaling;
	return vector;
}

/*
====================
AddVectorToPoint2D

Given a 2D point and vector, adds the dx component of the vector to the point's
x component, and the vector's dy component to the point's y component.
====================
*/
static struct Point2D AddVectorToPoint2D( struct Point2D point, struct Vector2D vector ) {
	point.x += vector.dx;
	point.y += vector.dy;
	return point;
}

/*
====================
GetPointSegment

Given a point on the plane and the amount of players, returns the player ID on whose segment the point is.
====================
*/
static int GetPointSegment( struct Point2D point, int numPlayers ) {
	DebugAssert( numPlayers >= 2 && numPlayers <= 6 );

	/* In the case of two players, everything that is
	 * on the left half belongs to player 0, everything else
	 * is player 1 terrain */
	if( numPlayers == 2 ) {
		return point.y <= 0.0f ? 0 : 1;
	}

	/* For the rest of the cases, simply start from the first player's clockwise left point
	 * and go 360/n degrees to the right and see in which segment the angle is. */
	float 			angle;
	struct Point2D	origin = { .x = 0.0f, .y = 0.0f };
	struct Vector2D playerZeroStartVector = DeltaVector2D( origin, GetPlayerLine( 0, numPlayers ).point );
	struct Vector2D	deltaVector = DeltaVector2D( origin, point );

	// Case where the point is the origin: player 0
	if( deltaVector.dx == 0.0f && deltaVector.dy == 0.0f ) {
		return 0;
	}

	angle = GetVectorAngle2D( playerZeroStartVector, deltaVector );
	return ( int )( floor( angle / DEGREES_TO_RADIANS( 360.0f / numPlayers ) ) );
}

/*
====================
DeltaVector2D

Given two points, returns the vector that leads to point2 when added to point1.
====================
*/
static struct Vector2D DeltaVector2D( struct Point2D point1, struct Point2D point2 ) {
	struct Vector2D result;
	result.dx = point2.x - point1.x;
	result.dy = point2.y - point1.y;
	return result;
}

/*
====================
VectorNorm2D

Calculates the norm of a 2-dimensional vector and returns it.
====================
*/
static float VectorNorm2D( struct Vector2D vector ) {
	return sqrt( ScalarProduct2D( vector, vector ) );
}

/*
====================
GetVectorAngle2D

Returns the angle in which vector2 differs from vector1. The range of output is [0, 2*PI).
Props to Escuti for this one.
====================
*/
static float GetVectorAngle2D( struct Vector2D vector1, struct Vector2D vector2 ) {
	// Find the vectors' norms.
	float norm1 = VectorNorm2D( vector1 );
	float norm2 = VectorNorm2D( vector2 );

	// First compute the cosine of the angle using the scalar product
	float cosine = ScalarProduct2D( vector1, vector2 ) / ( norm1 * norm2 );
	
	// Then compute the sine using the cross product
	float sine = ( vector1.dx * vector2.dy - vector1.dy * vector2.dx ) / ( norm1 * norm2 );

	// Then get the angle
	float angle = atan2( sine, cosine );
	if( angle < 0.0f ) {
		angle = 2 * M_PI + angle;
	}

	// and return the result.
	return angle;
}

/*
====================
ScalarProduct2D

Returns the scalar product of vector1 and vector2.
====================
*/
static float ScalarProduct2D( struct Vector2D vector1, struct Vector2D vector2 ) {
	return vector1.dx * vector2.dx + vector1.dy * vector2.dy;
}

/*
====================
GetPlayerLine

Returns a line element containing the clockwise start of
the player's paddle movement range (which goes from 0 to 
1 on the vector) to its end.
====================
*/
struct Line2D GetPlayerLine( int player, int numPlayers ) {
	DebugAssert( numPlayers >= 2 && numPlayers <= 6 );
	DebugAssert( player < numPlayers );

	struct Line2D result = { .point = { .x = 0.0f, .y = 0.0f }, .vector = { .dx = 0.0f, .dy = 0.0f } };

	static struct Line2D lConstants2[] = { 	{ .point = { .x = -0.9f, .y = -1.0f }, 	.vector = { .dx = 0.0f, .dy = 2.0f } },
											{ .point = { .x = 0.9f,  .y = 1.0f }, 	.vector = { .dx = 0.0f, .dy = -2.0f } } };

	// These are all the points on the regular n-gons, yay.
	static struct Point2D pConstants3[] = {	{ .x = 0.0f,	.y = 0.7794228634f }, 
											{ .x = 0.9f,	.y = -0.7794228634f }, 
											{ .x = -0.9f,	.y = -0.7794228634f } };

	static struct Point2D pConstants4[] = { { .x = -0.9f,	.y = 0.9f },
											{ .x = 0.9f,	.y = 0.9f },
											{ .x = 0.9f,	.y = -0.9f },
											{ .x = -0.9f,	.y = -0.9f } };

	static struct Point2D pConstants5[] = {	{ .x = 0.0f,			.y = 0.8559508646f },
											{ .x = 0.9f,			.y = 0.2020625895f },
											{ .x = 0.5562305899f, 	.y = 0.8559508646f },
											{ .x = -0.5562305899f, 	.y = 0.8559508646f },
											{ .x = -0.9f,			.y = 0.2020625895f } };
	
	static struct Point2D pConstants6[] = { { .x = -0.45f,	.y = 0.7794228634f },
											{ .x = 0.45f,	.y = 0.7794228634f },
											{ .x = 0.9f,	.y = 0.0f },
											{ .x = 0.45f,	.y = -0.7794228634f },
											{ .x = -0.45f,	.y = -0.7794228634f },
											{ .x = -0.9f,	.y = 0.0f } };	

	switch( numPlayers ) {
		case 2:
			return lConstants2[player];
		case 3:
			result.point = pConstants3[player];
			result.vector = DeltaVector2D( pConstants3[player], pConstants3[( player + 1 ) % 3] );
			break;
		case 4:
			result.point = pConstants4[player];
			result.vector = DeltaVector2D( pConstants4[player], pConstants4[( player + 1 ) % 4] );
			break;
		case 5:
			result.point = pConstants5[player];
			result.vector = DeltaVector2D( pConstants5[player], pConstants5[( player + 1 ) % 5] );
			break;
		case 6:
			result.point = pConstants6[player];
			result.vector = DeltaVector2D( pConstants6[player], pConstants6[( player + 1 ) % 6] );
			break;
	}
	return result;
}

/*
====================
AtRegsiterHit

Register a function which should be called when a hit gets registered.
Only one function can be registered at a time.
====================
*/
void AtRegisterHit( registerHitHandler_t handler ) {
	rhHandler = handler;
}

/*
====================
RegisterHit

Registers when a ball gets hit by a paddle. Also calls the hit handler.
====================
*/
void RegisterHit( int player ) {
	lastHit = player;
	rhHandler( player );
}

/*
====================
LastHit

Returns the ID of the last player that hit the ball.
====================
*/
int LastHit( void ) {
	return lastHit;
}

/*
====================
LineCircleCollision2D

Given a line and a circle:
	1. Finds out if the circle is right or centered on the line and writes this value into isRight;
	2. Determines the projection of the circle center onto the line, scaled by the norm of the line.
Also: F**k all this linear algebra shit.
====================
*/
static void LineCircleCollision2D( struct Circle2D circle, struct Line2D line, int *isRight, float *projection ) {
	if( isRight ) {
		// Firstly, calculate a line that is offset 90° right by the radius of the circle.
		struct Line2D 	offsetLine;
		struct Vector2D	offsetVector;
		offsetVector.dx = line.vector.dy;
		offsetVector.dy = -line.vector.dx;
		offsetLine.point = AddVectorToPoint2D( line.point, ScaleVector2D( offsetVector, circle.radius / VectorNorm2D( offsetVector ) ) );

		// Now write the value to the variable.
		struct Point2D lineEndPoint = AddVectorToPoint2D( offsetLine.point, offsetLine.vector );
		*isRight = ( lineEndPoint.x - offsetLine.point.x ) * ( circle.point.y - offsetLine.point.y ) < ( lineEndPoint.y - offsetLine.point.y ) * ( circle.point.x - offsetLine.point.x );
	}

	if( projection ) {
		// Secondly, calculate the projection. If the circle is on the origin of the line, this should be 0.0f, and if it is at the end, it should be 1.0f.
		struct Vector2D	circleDirection = DeltaVector2D( line.point, circle.point );
		*projection = ScalarProduct2D( line.vector, circleDirection ) / ScalarProduct2D( line.vector, line.vector );
	}
}

/*
====================
ProcessPhysics

Writes changed values to the state variable.
====================
*/
int ProcessPhysics( struct GameState *state, float deltaSeconds ) {
	DebugAssert( deltaSeconds > 0.0f && state );

	if( deltaSeconds <= 0.0f || !state ) {
		return -1;
	}

	HandleInput( state, deltaSeconds );	// Checks arrow keys and applies paddle speed (not paddle displacement).
	
	// Displace the paddle.
}

/*
====================
InitializePhysics

Performs some initialization tasks for the physics component. Should be called after initializing SDL.
====================
*/
int InitializePhysics( void ) {
	sdlKeyArray = SDL_GetKeyboardState( NULL );
}

/*
====================
HandleInput

Handles input from the keyboard that is relevant for the physics component.
====================
*/
static void	HandleInput( struct GameState *state, float deltaSeconds ) {
	// Check for keys.
	SDL_PumpEvents();

	if( sdlKeyArray[clockwiseKey] ) {
		userPaddleSpeed += PADDLE_ACCELERATION * deltaSeconds;
		if( userPaddleSpeed > PADDLE_MAX_SPEED ) {
			userPaddleSpeed = PADDLE_MAX_SPEED;
		}
	}
	
	if( sdlKeyArray[counterclockwiseKey] ) {
		userPaddleSpeed -= PADDLE_ACCELERATION * deltaSeconds;
		if( userPaddleSpeed < -PADDLE_MAX_SPEED ) {
			userPaddleSpeed = -PADDLE_MAX_SPEED;
		}
	}
}
