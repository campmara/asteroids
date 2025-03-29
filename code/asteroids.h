#ifndef ASTEROIDS_H
#define ASTEROIDS_H

#include "asteroids_platform.h"
#include "asteroids_random.h"

#define MAX_ASTEROIDS 26
#define MAX_ASTEROID_POINTS 8
#define MAX_BULLETS 3

#define PI_32 3.14159265359f
#define TWO_PI_32 6.28318530718f

#define DEG2RAD 0.0174533
#define RAD2DEG 57.2958

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

struct Line
{
    Vector2 a;
    Vector2 b;
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

    bool32 is_active; // value that tracks whether or not this asteroid slot exists on the game screen.

};

struct GameState
{
    Player player;

    Bullet bullets[MAX_BULLETS];
    float32 bullet_speed;
    float32 bullet_lifespan_seconds;

    Asteroid asteroids[MAX_ASTEROIDS];
    int32 num_asteroids;
    int32 asteroid_size_tier_one;
    int32 asteroid_size_tier_two;
    int32 asteroid_size_tier_three;
    int32 asteroid_size_tier_four;

    RandomLCGState random;
};

#endif
