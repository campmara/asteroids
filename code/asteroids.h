#ifndef ASTEROIDS_H
#define ASTEROIDS_H

#include "asteroids_platform.h"

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

struct GameState
{
    float32 player_x;
    float32 player_y;

    float32 asteroid_x;
    float32 asteroid_y;
};

#endif
