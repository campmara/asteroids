#ifndef ASTEROIDS_MATH_H
#define ASTEROIDS_MATH_H

#include <xmmintrin.h>
#include <math.h>

#include "asteroids_platform.h"

union Vector2
{
    struct
    {
        float32 x, y;
    };
    struct
    {
        float32 u, v;
    };
    float32 e[2];
};

inline Vector2 operator+(Vector2 &a, const Vector2 &b)
{
    return { a.x + b.x, a.y + b.y };
}

inline Vector2 operator-(Vector2 &a, const Vector2 &b)
{
    return { a.x - b.x, a.y - b.y };
}

inline float32 Abs(float32 value)
{
    return value > 0 ? value : -value;
}

inline float32 Sqrt(float32 value)
{
    // TODO(mara): Test the speed of this function w/ SIMD vs. sqrtf. See if it's actually working
    // properly.
    __m128 in = _mm_set_ss(value);
    __m128 out = _mm_sqrt_ss(in);
    return _mm_cvtss_f32(out);

    //return sqrtf(value);
}

inline float32 Cos(float32 radians)
{
    return cosf(radians);
}

inline float32 Sin(float32 radians)
{
    return sinf(radians);
}

inline float32 SqrMagnitude(Vector2 vector)
{
    return (vector.x * vector.x) + (vector.y * vector.y);
}

inline float32 Magnitude(Vector2 vector)
{
    return Sqrt(SqrMagnitude(vector));
}

inline Vector2 Normalize(Vector2 vector)
{
    float mag = Magnitude(vector);
    Vector2 result = vector;
    result.x /= mag;
    result.y /= mag;
    return result;
}

inline float32 Dot(Vector2 a, Vector2 b)
{
    return a.x * b.x + a.y * b.y;
}

inline int32 RoundFloat32ToInt32(float32 value)
{
    return (int32)(value + 0.5f);
}

inline uint32 RoundFloat32ToUInt32(float32 value)
{
    return (uint32)(value + 0.5f);
}

inline int32 FloorFloat32ToInt32(float32 value)
{
    return (int32)floorf(value);
}

inline void ClampAngleRadians(float32 *radians)
{
    if (*radians < 0)
    {
        *radians += TWO_PI_32;
    }
    else if (*radians > TWO_PI_32)
    {
        *radians -= TWO_PI_32;
    }
}

inline void WrapInt32PointAroundBuffer(GameOffscreenBuffer *buffer, int32 *x, int32 *y)
{
    if (*x < 0)
    {
        *x += buffer->width;
    }
    else if (*x >= buffer->width)
    {
        *x -= buffer->width;
    }
    if (*y < 0)
    {
        *y += buffer->height;
    }
    else if (*y >= buffer->height)
    {
        *y -= buffer->height;
    }
}

inline void WrapFloat32PointAroundBuffer(GameOffscreenBuffer *buffer, float32 *x, float32 *y)
{
    float32 width = (float32)buffer->width;
    float32 height = (float32)buffer->height;

    if (*x < 0)
    {
        *x += width;
    }
    else if (*x >= width)
    {
        *x -= width;
    }
    if (*y < 0)
    {
        *y += height;
    }
    else if (*y >= height)
    {
        *y -= height;
    }
}

#endif
