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

#define KILOBYTES(value) ((value) * 1024)
#define MEGABYTES(value) (KILOBYTES(value) * 1024)
#define GIGABYTES(value) (MEGABYTES(value) * 1024)
#define TERABYTES(value) (GIGABYTES(value) * 1024)

inline GameControllerInput *GetController(GameInput *input, uint32 controller_index)
{
    ASSERT(controller_index < ARRAY_COUNT(input->controllers));
    GameControllerInput *result = &input->controllers[controller_index];
    return result;
}

#endif
