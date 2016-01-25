#include <math.h>
#include <SDL2/SDL.h>
#include "Physics.h"
#include "Game.h"
#include "Debug/Debug.h"

#define DEGREES_TO_RADIANS( x ) ( ( x ) * M_PI / 180.0f )
#define PADDLE_ACCELERATION 4.0f	// DISTANCE PER SECOND SQUARED
#define PADDLE_MAX_SPEED 1.5f		// DISTANCE PER SECOND
#define PADDLE_MAX_POS ( 1.0f - PADDLE_SIZE )
#define PADDLE_MIN_POS ( 0.0f )
#define DEFAULT_BALL_SPEED 1.0f		// DISTANCE PER SECOND
#define PADDLE_TOLERANCE ( DEFAULT_BALL_RADIUS / 2.0f )

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

static registerHitHandler_t *	rhHandler = NULL;
static registerPointHandler_t *	rpHandler = NULL;
static registerQuitHandler_t *	rqHandler = NULL;
int								numRhHandler = 0;
int								numRpHandler = 0;
int								numRqHandler = 0;
static int						lastHit = -1;
static const unsigned char *	sdlKeyArray = NULL;
static const int				clockwiseKey = SDL_SCANCODE_LEFT;
static const int				counterclockwiseKey = SDL_SCANCODE_RIGHT;
static float					userPaddleSpeed = 0.0f;

// FUNCTIONS

static struct Vector2D	AddVectors2D( struct Vector2D vector1, struct Vector2D vector2 );
static int				GetPointSegment( struct Point2D point, int numPlayers );
static struct Vector2D	DeltaVector2D( struct Point2D point1, struct Point2D point2 );
static float			GetVectorAngle2D( struct Vector2D vector1, struct Vector2D vector2 );
static float			ScalarProduct2D( struct Vector2D vector1, struct Vector2D vector2 );
static struct Vector2D	GetReflectionVector( struct Vector2D wall, struct Vector2D objectMovement, int random );
static void				LineCircleCollision2D( struct Circle2D circle, struct Line2D line, int *isRight, float *projection );
static int				HandleInput( float deltaSeconds );
static void				DisplaceUserPaddle( struct GameState *state, float deltaSeconds );
static void				BallLogic( struct GameState *state, float deltaSeconds );
static void				RegisterPoint( struct GameState *state );
static void				ResetBall( struct GameState *state );
static struct Vector2D	VectorFromPolar2D( float angle, float norm );
static void				RegisterQuit( void );

// Imported from the network component.
extern int				IsServer( void );
extern int				ThisClient( void );

/*
====================
ScaleVector2D

Given a vector and a scalar, scales the vector by the scalar.
====================
*/
struct Vector2D ScaleVector2D( struct Vector2D vector, float scaling ) {
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
struct Point2D AddVectorToPoint2D( struct Point2D point, struct Vector2D vector ) {
	point.x += vector.dx;
	point.y += vector.dy;
	return point;
}

/*
====================
AddVectors2D

Returns the result of vector addition of its two arguments.
====================
*/
static struct Vector2D AddVectors2D( struct Vector2D vector1, struct Vector2D vector2 ) {
	vector1.dx += vector2.dx;
	vector1.dy += vector2.dy;
	return vector1;
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
		return point.x <= 0.0f ? 0 : 1;
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

	angle = GetVectorAngle2D( deltaVector, playerZeroStartVector );
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

Calculates the euclidean norm of a 2-dimensional vector and returns it.
====================
*/
float VectorNorm2D( struct Vector2D vector ) {
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

Given the player ID (integer ranging from 0 to numPlayers - 1) and the total amount of players (ranging from 2 to 6).
Returns a line element containing the clockwise start of the player's paddle movement range (which goes from 0 to 1 on the vector) to its end.
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
	if( !rhHandler ) {
		numRhHandler = 1;
		rhHandler = malloc( numRhHandler * sizeof( registerHitHandler_t ) );
		rhHandler[0] = handler;
		return;
	}
	numRhHandler++;
	registerHitHandler_t *newArray = malloc( numRhHandler * sizeof( registerHitHandler_t ) );
	memcpy( newArray, rhHandler, ( numRhHandler - 1 ) * sizeof( registerHitHandler_t ) );
	free( rhHandler );
	newArray[numRhHandler - 1] = handler;
	rhHandler = newArray;
}

/*
====================
RegisterHit

Registers when a ball gets hit by a paddle. Also calls the hit handler.
====================
*/
void RegisterHit( int player ) {
	lastHit = player;
	if( rhHandler ) {
		int elem;
		for( elem = 0; elem < numRhHandler; elem++ ) {
			if( rhHandler[elem] ) {
				( rhHandler[elem] )( player );
			}
		}
	}
}

/*
====================
AtRegisterPoint

Register a function which should be called when some player gets a point.
====================
*/
void AtRegisterPoint( registerPointHandler_t handler ) {
	// If there is no point handler, initialize the array
	if( !rpHandler ) {
		numRpHandler = 1;
		rpHandler = malloc( numRpHandler * sizeof( registerPointHandler_t ) );
		rpHandler[0] = handler;
		return;
	}
	// Else, add the handler to the list.
	numRpHandler++;
	registerPointHandler_t *newArray = malloc( numRpHandler * sizeof( registerPointHandler_t ) );
	memcpy( newArray, rpHandler, ( numRpHandler - 1 ) * sizeof( registerPointHandler_t ) );
	free( rpHandler );
	newArray[numRpHandler - 1] = handler;
	rpHandler = newArray;
}

/*
====================
AtRegisterQuit

Registers a function so it gets called whenever the user hits the escape key during the game.
====================
*/
void AtRegisterQuit( registerQuitHandler_t handler ) {
	// If there is no quit handler, initialize the array.
	if( !rqHandler ) {
		numRqHandler = 1;
		rqHandler = malloc( numRqHandler * sizeof( registerQuitHandler_t ) );
		rqHandler[0] = handler;
		return;
	}
	// Else add the handler to the list
	numRqHandler++;
	registerQuitHandler_t *newArray = malloc( numRqHandler * sizeof( registerQuitHandler_t ) );
	memcpy( newArray, rqHandler, ( numRqHandler - 1 ) * sizeof( registerQuitHandler_t ) );
	free( rqHandler );
	newArray[numRqHandler - 1] = handler;
	rqHandler = newArray;
}

/*
====================
RegisterQuit

Calls all event handlers for quitting.
====================
*/
static void RegisterQuit( void ) {
	if( rqHandler ) {
		int elem;
		for( elem = 0; elem < numRqHandler; elem++ ) {
			if( rqHandler[elem] ) {
				( rqHandler[elem] )();
			}
		}
	}
}

/*
====================
RegisterPoint

Registers when the ball moves beyond the pitch and determines which player, if any, gets a point. If so, it calls the event handler registered with rpHandler.
====================
*/
static void RegisterPoint( struct GameState *state ) {
	// Check if any player got lastHit
	if( lastHit >= 0 && lastHit < state->numPlayers ) {
		// Increment that player's score and call the event handler.
		// Only if you're the server, because the clients get their scores from the server.
		if( IsServer() ) {
			state->players[lastHit].score++;
		}
		if( rpHandler ) {
			// Call all registered handlers for the RegisterPoint event.
			int elem;
			for( elem = 0; elem < numRpHandler; elem++ ) {
				if( rpHandler[elem] ) {
					( rpHandler[elem] )( state, lastHit );
				}
			}
		}
	}
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
		offsetLine.vector = line.vector;
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
RotateVector2D

Rotates a vector by the given angle.
====================
*/
static struct Vector2D RotateVector2D( struct Vector2D vector, float angle ) {
	struct Vector2D result;
	result.dx = vector.dx * cos( angle ) - vector.dy * sin( angle );
	result.dy = vector.dx * sin( angle ) + vector.dy * cos( angle );
	return result;
}

/*
====================
GetReflectionVector

Gets a reflection vector for the object which bounces off the wall.
====================
*/
static struct Vector2D GetReflectionVector( struct Vector2D wall, struct Vector2D objectMovement, int random ) {
	struct Vector2D wallNormal;
	wallNormal.dx = wall.dy;
	wallNormal.dy = -wall.dx;
	wallNormal = ScaleVector2D( wallNormal, 1.0f / VectorNorm2D( wallNormal ) );

	if( random ) {
		return RotateVector2D( AddVectors2D( objectMovement, ScaleVector2D( wallNormal, -2.0f * ScalarProduct2D( objectMovement, wallNormal ) ) ), ( float )DEGREES_TO_RADIANS( rand() % 40 - 20 ) );
	} else {
		return AddVectors2D( objectMovement, ScaleVector2D( wallNormal, -2.0f * ScalarProduct2D( objectMovement, wallNormal ) ) );
	}
}

/*
====================
DisplaceUserPaddle

Takes values from global variables and displaces the user paddle accordingly.
====================
*/
static void DisplaceUserPaddle( struct GameState *state, float deltaSeconds ) {
	float imminentDisplacement = userPaddleSpeed * deltaSeconds;
	float *currentPosition;
	if( !IsServer() ) {
		currentPosition = &state->players[ThisClient()].position;
	} else {
		currentPosition = &state->players[0].position;
	}
	*currentPosition += imminentDisplacement;
	// Some braking
	userPaddleSpeed -= ( userPaddleSpeed / PADDLE_MAX_SPEED ) * 2.0f * deltaSeconds;
	// Check for borders!
	if( *currentPosition > PADDLE_MAX_POS ) {
		*currentPosition = PADDLE_MAX_POS;
		userPaddleSpeed *= -0.5f;
	}
	if( *currentPosition < PADDLE_MIN_POS ) {
		*currentPosition = PADDLE_MIN_POS;
		userPaddleSpeed *= -0.5f;
	}
}

/*
====================
BallLogic

Calculates the new ball position and registers hits and misses according to the ball and paddle states.
====================
*/
static void BallLogic( struct GameState *state, float deltaSeconds ) {
	struct Ball *	ball = &state->ball;
	float *			currentPosition;
	int 			segment;
	struct Circle2D	ballCircle;
	struct Point2D 	newPosition = AddVectorToPoint2D( ball->position, ScaleVector2D( ball->direction, deltaSeconds ) );
	int				isRight;
	float			projection;
	ballCircle.point = newPosition;
	ballCircle.radius = DEFAULT_BALL_RADIUS;

	// Determine which player we have to check.
	segment = GetPointSegment( newPosition, state->numPlayers );

	// In 2-player mode, check collision with the lines y = -1.0 and y = 1.0.
	if( state->numPlayers == 2 ) {
		struct Line2D lowerLine = { .point = { .x = 1.0f, .y = -1.0f }, .vector = { .dx = -2.0f, .dy = 0.0f } };
		struct Line2D upperLine = { .point = { .x = -1.0f, .y = 1.0f }, .vector = { .dx = 2.0f, .dy = 0.0f } };
		LineCircleCollision2D( ballCircle, lowerLine, &isRight, &projection );
		if( !isRight ) {
			ball->direction = GetReflectionVector( lowerLine.vector, ball->direction, 0 );
			return;
		}
		LineCircleCollision2D( ballCircle, upperLine, &isRight, &projection );
		if( !isRight ) {
			ball->direction = GetReflectionVector( upperLine.vector, ball->direction, 0 );
			return;
		}
	}

	// Check collision with the player paddle.
	currentPosition = &state->players[segment].position;
	// This works. I considered doing some calculations for intermediate states of the ball rather than this,
	// but it seems like that is unnecessary.
	LineCircleCollision2D( ballCircle, GetPlayerLine( segment, state->numPlayers ), &isRight, &projection );
	if( isRight ) {
		// Normal displacement.
		ball->position = ballCircle.point;
	} else {
		// Check if the paddle hits the ball!
		if( projection >= *currentPosition - PADDLE_TOLERANCE && projection <= *currentPosition + PADDLE_SIZE + PADDLE_TOLERANCE ) {
			RegisterHit( segment );
			ball->direction = GetReflectionVector( GetPlayerLine( segment, state->numPlayers ).vector, ball->direction, 1 );
		} else {
			RegisterPoint( state );
			ResetBall( state );
		}
	}
}

/*
====================
ProcessPhysics

Calculates a new game state and writes changed values to the state variable.
====================
*/
int ProcessPhysics( struct GameState *state, float deltaSeconds ) {
	DebugAssert( deltaSeconds > 0.0f && state );

	if( deltaSeconds <= 0.0f || !state ) {
		return -1;
	}

	// Code for the user input. Returns -2 when the user hits escape.
	if( HandleInput( deltaSeconds ) == -2 ) {
		// Call all quit handlers
		RegisterQuit();
		return -2;
	}

	// Handles what happens to the paddle according to input.
	DisplaceUserPaddle( state, deltaSeconds );

	// Code for the ball and collisions
	BallLogic( state, deltaSeconds );

	return 0;
}

/*
====================
InitializePhysics

Performs some initialization tasks for the physics component. Should be called after initializing SDL.
====================
*/
int InitializePhysics( void ) {
	sdlKeyArray = SDL_GetKeyboardState( NULL );
	return 0;
}

/*
====================
HandleInput

Handles input from the keyboard that is relevant for the physics component.
====================
*/
static int	HandleInput( float deltaSeconds ) {
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

	if( sdlKeyArray[SDL_SCANCODE_ESCAPE] ) {
		return -2; // Code for user interruption.
	}

	return 0; // Normal exit code.
}

/*
====================
ResetBall

Resets the ball to the center of the pitch and assigns a new movement vector.
====================
*/
static void ResetBall( struct GameState *state ) {
	// Reset lastHit so that nobody gets a point until anybody actually hits the ball.
	lastHit = -1;

	// Reset ball position
	switch( state->numPlayers ) {
		case 2:
		case 4:
		case 5:
		case 6:
			state->ball.position.x = 0.0f;
			state->ball.position.y = 0.0f;
			break;
		case 3:
			state->ball.position.x = 0.0f;
			state->ball.position.y = -0.26f;
	}

	// New random movement vector.
	if( state->numPlayers == 2 ) {
		state->ball.direction = VectorFromPolar2D( DEGREES_TO_RADIANS( ( ( rand() % 2 ) * 180 ) + ( rand() % 90 - 45 ) ), DEFAULT_BALL_SPEED );
	} else {
		state->ball.direction = VectorFromPolar2D( DEGREES_TO_RADIANS( ( rand() % 360 ) ), DEFAULT_BALL_SPEED );
	}
}


/*
====================
VectorFromPolar2D

Given the polar form of a vector, returns a vector in coordinate form.
====================
*/
static struct Vector2D VectorFromPolar2D( float angle, float norm ) {
	struct Vector2D result;
	result.dx = norm * cos( angle );
	result.dy = norm * sin( angle );
	return result;
}

/*
====================
InitializeBall

Initializes a ball structure for a new game.
====================
*/
void InitializeBall( struct GameState *state ) {
	ResetBall( state );
}

