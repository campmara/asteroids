#ifndef ASTEROIDS_H
#define ASTEROIDS_H

#include "asteroids_platform.h"
global PlatformAPI global_platform;

#include "asteroids_random.h"
#include "asteroids_font.h"

#define BITMAP_BYTES_PER_PIXEL 4

#define NUM_GRID_SPACES_H 16
#define NUM_GRID_SPACES_V 9

#define MAX_ASTEROIDS 26
#define MAX_ASTEROID_POINTS 8

#define MAX_BULLETS 3

#define PI_32 3.14159265359f
#define TWO_PI_32 6.28318530718f

#define DEG2RAD 0.0174533
#define RAD2DEG 57.2958

#define EPSILON 0.0000001

#if ASSERTIONS_ENABLED
// NOTE(mara): This instruction might be different on other CPUs.
#define ASSERT(expression) if (!(expression)) { *(int *)0 = 0; }
#else
#define ASSERT(expression) // Evaluates to nothing.
#endif

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

#define SWAP(type, a, b) do { type tmp = (a); (a) = (b); (b) = tmp; } while (0)

#define KILOBYTES(value) ((value) * 1024)
#define MEGABYTES(value) (KILOBYTES(value) * 1024)
#define GIGABYTES(value) (MEGABYTES(value) * 1024)
#define TERABYTES(value) (GIGABYTES(value) * 1024)

// =================================================================================================
// HELPERS
// =================================================================================================

inline uint32 SafeTruncateUInt64(uint64 value)
{
    ASSERT(value <= 0xFFFFFFFF);
    uint32 result = (uint32)value;
    return result;
}

inline int GetStringLength(char *string)
{
    int length = 0;
    while (*string++)
    {
        ++length;
    }
    return length;
}

// TODO(mara): Step through this function and make sure the lengths of these
// loops are set up properly.
void ConcatenateStrings(size_t src_a_count, char *src_a,
                        size_t src_b_count, char *src_b,
                        size_t dest_count, char *dest)
{
    // NOTE(mara): This function will only concatenate the two strings given
    // up to the dest_count limit, after which it will truncate the result.
    for (int i = 0; i < src_a_count && dest_count > 0; ++i)
    {
        *dest++ = *src_a++;

        --dest_count;
    }

    for (int i = 0; i < src_b_count && dest_count > 0; ++i)
    {
        *dest++ = *src_b++;

        --dest_count;
    }

    *dest++ = 0; // Insert the null terminator.
}

inline GameControllerInput *GetController(GameInput *input, uint32 controller_index)
{
    ASSERT(controller_index < ARRAY_COUNT(input->controllers));
    GameControllerInput *result = &input->controllers[controller_index];
    return result;
}

// =================================================================================================
// GAME CODE STATE
// =================================================================================================

struct Vector2
{
    float32 x;
    float32 y;
};

inline Vector2 operator+(Vector2 &a, const Vector2 &b)
{
    return { a.x + b.x, a.y + b.y };
}

inline Vector2 operator-(Vector2 &a, const Vector2 &b)
{
    return { a.x - b.x, a.y - b.y };
}

struct Line
{
    Vector2 a;
    Vector2 b;
};

struct GridSpace
{
    GridSpace *north;
    GridSpace *east;
    GridSpace *south;
    GridSpace *west;

    // TODO(mara): There's a way we can further optimize the collision testing with this
    // array. Construct one of these for every space every frame and just query that
    // when grid-testing to get the list of asteroid indices in the game state array
    // we need to test against.
    uint32 asteroid_indices[MAX_ASTEROIDS];

    uint32 num_asteroid_line_points;
    uint32 num_bullets;
    bool32 has_player;
};

struct Grid
{
    float32 space_width;
    float32 space_height;

    GridSpace spaces[NUM_GRID_SPACES_H * NUM_GRID_SPACES_V];
};

struct Player
{
    Vector2 position;
    Vector2 forward;
    Vector2 right;
    Vector2 velocity;

    float32 rotation;
    float32 rotation_speed;

    float32 maximum_velocity;
    float32 thrust_factor;
    float32 acceleration;
    float32 speed_damping_factor;

    float32 color_r;
    float32 color_g;
    float32 color_b;

    int32 lives;

    Line lines[3];
};

struct Bullet
{
    Vector2 position;
    Vector2 forward;

    float32 time_remaining;

    bool32 is_active;
};

struct Asteroid
{
    Vector2 position;
    Vector2 forward;

    Vector2 points[MAX_ASTEROID_POINTS]; // Local asteroid vertex points.
    Line lines[MAX_ASTEROID_POINTS];

    float32 speed;

    float32 color_r;
    float32 color_g;
    float32 color_b;

    int32 phase_index; // large = 2, medium = 1, small = 0

    bool32 is_active; // value that tracks whether or not this asteroid slot exists on the game screen.
};

struct GameState
{
    Grid grid;

    Player player;

    Bullet bullets[MAX_BULLETS];
    float32 bullet_speed;
    float32 bullet_lifespan_seconds;
    float32 bullet_size;

    float32 asteroid_player_min_spawn_distance;

    Asteroid asteroids[MAX_ASTEROIDS];
    int32 asteroid_phases[4];
    int32 phase_score_amounts[3];

    int32 num_lives_at_start;
    int32 num_asteroids_at_start;

    int32 score;

    RandomLCGState random;

    FontData font;
};

#endif
