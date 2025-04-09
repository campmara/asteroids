#ifndef ASTEROIDS_RANDOM_H
#define ASTEROIDS_RANDOM_H

#include <stdlib.h>
#include <time.h>

// This is a pretty simple and straightforward LCG random number generator.

struct RandomState
{
    int64 seed;
};

inline void SeedRandom(RandomState *rand, int64 seed)
{
    rand->seed = seed;
}

inline int64 RandomInt64(RandomState *rand)
{
    return rand->seed = (rand->seed * 1103515245 + 12345) & RAND_MAX;
}

inline int32 RandomInt32(RandomState *rand)
{
    return (int32)RandomInt64(rand);
}

inline float32 RandomFloat32InRange(RandomState *rand, float32 min_inc, float32 max_inc)
{
    return min_inc + ((float32)RandomInt32(rand) / (float32)(RAND_MAX / (max_inc - min_inc)));
}

internal int64 RandomInt64InRange(RandomState *rand, int64 min_inc, int64 max_inc)
{
    int64 result = RandomInt64(rand) % (max_inc - min_inc + 1) + min_inc;
    return result;
}

internal int32 RandomInt32InRange(RandomState *rand, int32 min_inc, int32 max_inc)
{
    return (int32)RandomInt64InRange(rand, (int64)min_inc, (int64)max_inc);
}

#endif
