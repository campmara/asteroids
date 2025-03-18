#ifndef ASTEROIDS_H
#define ASTEROIDS_H

#include "asteroids_platform.h"

#if ASSERTIONS_ENABLED
// NOTE(mara): This instruction might be different on other CPUs.
#define ASSERT(expression) if (!(expression)) { *(int *)0 = 0; }
#else
#define ASSERT(expression) // Evaluates to nothing.
#endif

#endif
