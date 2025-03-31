#ifndef ASTEROIDS_RANDOM_H
#define ASTEROIDS_RANDOM_H

#include <stdlib.h>
#include <time.h>

struct RandomLCGState
{
    int64 seed;
};

inline void SeedRandomLCG(RandomLCGState *rand, int64 seed)
{
    rand->seed = seed;
}

inline int64 RandomInt64LCG(RandomLCGState *rand)
{
    return rand->seed = (rand->seed * 1103515245 + 12345) & RAND_MAX;
}

inline int32 RandomInt32LCG(RandomLCGState *rand)
{
    return (int32)RandomInt64LCG(rand);
}

internal int64 RandomInt64InRangeLCG(RandomLCGState *rand, int64 min_inc, int64 max_inc)
{
    int64 result = RandomInt64LCG(rand) % (max_inc - min_inc + 1) + min_inc;
    return result;
}

internal int32 RandomInt32InRangeLCG(RandomLCGState *rand, int32 min_inc, int32 max_inc)
{
    return (int32)RandomInt64InRangeLCG(rand, (int64)min_inc, (int64)max_inc);
}

internal int32 RandomInt32InRangeSTDLib(int32 min_inc, int32 max_inc)
{
    srand((uint32)time(0));
    int32 result = rand() % (max_inc - min_inc + 1) + min_inc;
    return result;
}

/*
#define RANDOM_MAX = 100
#define RANDOM_MIN = 1

global uint32 random_number_table[] = {
    73,  21,  31,  48,  23,
    27,  4,   70,  94,  93,
    100, 28,  69,  16,  97,
    10,  24,  71,  27,  96,
    85,  47,  84,  64,  77,
    95,  16,  7,   80,  20,
    88,  1,   4,   13,  48,
    9,   12,  78,  2,   70,
    74,  33,  98,  32,  48,
    71,  77,  52,  55,  86,
    74,  54,  76,  97,  39,
    61,  40,  100, 72,  71,
    44,  93,  72,  11,  87,
    68,  95,  97,  76,  55,
    66,  52,  52,  78,  22,
    91,  53,  13,  56,  62,
    9,   9,   85,  81,  91,
    96,  28,  18,  4,   86,
    17,  66,  12,  42,  41,
    38,  86,  56,  72,  61,
    39,  6,   58,  91,  79,
    14,  20,  24,  80,  88,
    94,  61,  85,  73,  70,
    7,   65,  32,  57,  73,
    12,  60,  40,  52,  23,
    65,  84,  12,  69,  31,
    83,  51,  80,  4,   72,
    43,  39,  67,  51,  26,
    20,  80,  85,  87,  58,
    78,  88,  50,  14,  6,
    10,  88,  75,  33,  88,
    75,  48,  25,  21,  22,
    58,  47,  85,  2,   1,
    39,  24,  96,  38,  27,
    37,  53,  81,  56,  86,
    27,  32,  15,  91,  2,
    42,  96,  10,  46,  89,
    68,  87,  61,  88,  22,
    98,  33,  9,   19,  81,
    95,  20,  4,   81,  18,
    27,  2,   72,  56,  88,
    9,   50,  8,   65,  38,
    65,  38,  19,  7,   37,
    81,  74,  73,  4,   75,
    63,  45,  9,   11,  16,
    39,  60,  16,  93,  90,
    98,  52,  76,  26,  64,
    35,  31,  3,   32,  80,
    30,  94,  9,   76,  75,
    1,   32,  78,  23,  26,
    80,  37,  94,  70,  60,
    38,  99,  10,  29,  81,
    7,   52,  31,  41,  96,
    62,  91,  80,  12,  14,
    94,  61,  26,  12,  35,
    81,  36,  49,  39,  13,
    67,  30,  66,  19,  98,
    94,  76,  31,  61,  92,
    91,  38,  82,  94,  39,
    15,  92,  67,  90,  56,
    4,   30,  47,  34,  58,
    72,  77,  7,   53,  84,
    73,  40,  38,  13,  26,
    85,  85,  88,  31,  48,
    89,  75,  17,  84,  24,
    40,  10,  61,  64,  67,
    65,  11,  74,  97,  63,
    89,  15,  15,  31,  82,
    15,  94,  28,  4,   20,
    98,  59,  78,  33,  59,
    4,   5,   24,  97,  50,
    3,   29,  9,   2,   76,
    64,  1,   45,  41,  37,
    13,  74,  27,  75,  15,
    35,  87,  6,   62,  84,
    75,  85,  28,  6,   42,
    80,  64,  70,  68,  44,
    18,  77,  61,  65,  70,
    80,  22,  71,  77,  81,
    63,  94,  16,  73,  93,
    26,  61,  10,  85,  95,
    97,  43,  30,  10,  31,
    81,  21,  92,  5,   29,
    61,  31,  40,  22,  24,
    70,  74,  68,  34,  72,
    72,  36,  2,   21,  26,
    28,  89,  80,  26,  50,
    8,   77,  17,  50,  8,
    21,  15,  29,  60,  69,
    34,  11,  52,  55,  47,
    22,  7,   57,  42,  20,
    64,  24,  77,  75,  7,
    29,  42,  57,  96,  7,
    43,  94,  75,  19,  76,
    77,  100, 62,  99,  56,
    15,  88,  93,  77,  10,
    10,  99,  55,  26,  87,
    1,   9,   90,  58,  21,
    28,  73,  51,  62,  22,
    33,  3,   13,  50,  48,
    86,  94,  37,  87,  31,
    96,  84,  19,  89,  39,
    4,   12,  37,  53,  76,
    29,  75,  31,  68,  29,
    92,  23,  64,  18,  51,
    19,  14,  53,  20,  84,
    8,   26,  15,  77,  36,
    2,   96,  92,  23,  96,
    15,  85,  75,  39,  43,
    40,  41,  91,  26,  50,
    2,   20,  35,  16,  55,
    87,  8,   98,  73,  25,
    58,  43,  62,  69,  27,
    17,  53,  39,  31,  21,
    74,  13,  70,  69,  37,
    62,  95,  63,  44,  92,
    8,   35,  47,  3,   79,
    56,  86,  19,  22,  28,
    31,  63,  32,  41,  25,
    65,  89,  31,  70,  25,
    32,  91,  16,  69,  64,
    83,  94,  11,  52,  23,
    10,  75,  72,  85,  35,
    50,  78,  77,  26,  50,
    69,  79,  98,  99,  16,
    10,  31,  37,  4,   8,
    7,   19,  44,  33,  57,
    17,  61,  41,  2,   99,
    42,  10,  85,  78,  85,
    88,  65,  61,  29,  50,
    35,  19,  37,  29,  19,
    90,  4,   54,  32,  10,
    50,  36,  26,  22,  8,
    61,  6,   62,  10,  17,
    72,  73,  78,  70,  9,
    7,   57,  70,  4,   90,
    81,  48,  29,  22,  65,
    17,  48,  42,  88,  93,
    45,  4,   53,  94,  90,
    29,  18,  16,  46,  74,
    18,  1,   55,  9,   44,
    69,  2,   44,  23,  24,
    41,  33,  32,  17,  14,
    53,  82,  87,  36,  26,
    75,  71,  86,  54,  60,
    96,  26,  18,  60,  66,
    17,  56,  2,   99,  93,
    1,   83,  79,  91,  41,
    40,  97,  61,  17,  19,
    42,  11,  74,  88,  49,
    40,  24,  99,  78,  6,
    19,  16,  3,   60,  80,
    8,   61,  27,  9,   49,
    76,  9,   90,  84,  31,
    69,  4,   12,  98,  68,
    63,  14,  90,  25,  22,
    69,  53,  37,  55,  1,
    49,  38,  53,  42,  65,
    32,  54,  37,  46,  57,
    24,  66,  31,  17,  48,
    21,  44,  19,  26,  44,
    64,  36,  50,  54,  89,
    21,  50,  1,   62,  40,
    36,  35,  11,  66,  9,
    79,  5,   72,  90,  63,
    92,  31,  7,   9,   11,
    9,   60,  4,   8,   81,
    39,  32,  22,  84,  99,
    44,  43,  92,  88,  5,
    43,  2,   9,   91,  3,
    51,  31,  46,  71,  20,
    79,  67,  23,  7,   31,
    54,  99,  21,  2,   54,
    43,  25,  89,  96,  17,
    16,  75,  68,  38,  50,
    74,  53,  5,   100, 43,
    59,  4,   59,  9,   19,
    77,  36,  26,  87,  44,
    54,  58,  12,  60,  10,
    13,  6,   96,  66,  6,
    78,  91,  14,  86,  21,
    27,  62,  34,  57,  40,
    13,  12,  53,  88,  89,
    73,  20,  99,  92,  88,
    65,  57,  89,  83,  57,
    92,  95,  44,  7,   14,
    37,  52,  7,   91,  89,
    22,  71,  14,  56,  77,
    42,  36,  14,  60,  10,
    21,  43,  80,  24,  87,
    87,  37,  35,  17,  92,
    100, 17,  19,  78,  97,
    68,  34,  27,  65,  9,
    40,  94,  65,  15,  93,
    62,  56,  70,  22,  32,
    57,  87,  29,  22,  63,
    54,  55,  19,  100, 71,
    55,  100, 76,  48,  100,
    5,   86,  61,  82,  44,
    34,  44,  88,  3,   84
}
*/

#endif
