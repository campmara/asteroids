#ifndef ASTEROIDS_PLATFORM_H
#define ASTEROIDS_PLATFORM_H

#define local_persist static
#define internal      static
#define global        static

#include <stdint.h>

// Unsigned integer types.
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

// Signed integer types.
typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

// Real-number types.
typedef float  float32;
typedef double float64;

// Booleans.
typedef int32 bool32;

#endif
