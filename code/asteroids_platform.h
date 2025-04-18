#ifndef ASTEROIDS_PLATFORM_H
#define ASTEROIDS_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#define local_persist static
#define internal      static
#define global        static

#define PI_32 3.14159265359f
#define TWO_PI_32 6.28318530718f

#define DEG2RAD 0.0174533
#define RAD2DEG 57.2958

#define EPSILON 0.0000001

#if ASSERTIONS_ENABLED
// NOTE(mara): This instruction might be different on other CPUs.
#define Assert(expression) if (!(expression)) { *(int *)0 = 0; }
#else
#define Assert(expression) // Evaluates to nothing.
#endif

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

#define Swap(type, a, b) do { type tmp = (a); (a) = (b); (b) = tmp; } while (0)

#define KILOBYTES(value) ((value) * 1024)
#define MEGABYTES(value) (KILOBYTES(value) * 1024)
#define GIGABYTES(value) (MEGABYTES(value) * 1024)
#define TERABYTES(value) (GIGABYTES(value) * 1024)

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

// Unsigned Char.
typedef unsigned char uchar8;

typedef size_t memsize;

inline uint32 SafeTruncateUInt64(uint64 value)
{
    Assert(value <= 0xFFFFFFFF);
    uint32 result = (uint32)value;
    return result;
}

/*
  ==================================================================================================
  NOTE(mara): Services that the platform provides to the game layer.
  ==================================================================================================
 */

typedef struct ReadFileResult
{
    uint32 content_size;
    void *content;
} ReadFileResult;

#define PLATFORM_FREE_FILE_MEMORY(name) void name(void *memory)
typedef PLATFORM_FREE_FILE_MEMORY(PlatformFreeFileMemoryFunc);

#define PLATFORM_READ_ENTIRE_FILE(name) ReadFileResult name(char *filename)
typedef PLATFORM_READ_ENTIRE_FILE(PlatformReadEntireFileFunc);

#define PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(char *filename, uint32 memory_size, void *memory)
typedef PLATFORM_WRITE_ENTIRE_FILE(PlatformWriteEntireFileFunc);

#define PLATFORM_PLAY_SOUND_SAMPLES(name) void name(int16 *samples)
typedef PLATFORM_PLAY_SOUND_SAMPLES(PlatformPlaySoundSamplesFunc);

typedef struct PlatformAPI
{
    PlatformFreeFileMemoryFunc *FreeFileMemory;
    PlatformReadEntireFileFunc *ReadEntireFile;
    PlatformWriteEntireFileFunc *WriteEntireFile;

    PlatformPlaySoundSamplesFunc *PlaySoundSamples;
} PlatformAPI;

/*
  ==================================================================================================
  NOTE(mara): An API that the platform layer will call in order to run the game.
  ==================================================================================================
 */

typedef struct GameOffscreenBuffer
{
    void *memory;
    int32 width;
    int32 height;
    int32 pitch;
    int32 bytes_per_pixel;
} GameOffscreenBuffer;

typedef struct GameSoundOutputBuffer
{
    int32 sample_count;
    int32 samples_per_second;
    int16 *samples;
} GameSoundOutputBuffer;

typedef struct GameButtonState
{
    bool32 ended_down;
    int32 half_transition_count;
} GameButtonState;

typedef struct GameControllerInput
{
    bool32 is_connected;
    bool32 is_analog;
    float32 stick_average_x;
    float32 stick_average_y;

    union
    {
        GameButtonState buttons[10];
        struct
        {
            GameButtonState move_up;
            GameButtonState move_down;
            GameButtonState move_left;
            GameButtonState move_right;

            GameButtonState action_up;
            GameButtonState action_down;
            GameButtonState action_left;
            GameButtonState action_right;

            GameButtonState start;

            // NOTE(mara): All buttons must be added above this line.

            GameButtonState terminator;
        };
    };
} GameControllerInput;

typedef struct GameInput
{
    GameButtonState mouse_buttons[5];
    int32 mouse_x, mouse_y, mouse_wheel;

    // 1 Keyboard, 1 Controller, shared input.
    GameControllerInput controllers[2];
} GameInput;

typedef struct GameTime
{
    float64 delta_time; // Time delta for THIS FRAME.
    float64 total_time; // Time elapsed since start.
} GameTime;

typedef struct GameMemory
{
    bool32 is_initialized;

    uint64 permanent_storage_size;
    void *permanent_storage; // NOTE(mara): REQUIRED to be cleared to zero at startup.

    uint64 transient_storage_size;
    void *transient_storage; // NOTE(mara): REQUIRED to be cleared to zero at startup.

    PlatformAPI platform_api;
} GameMemory;

#define GAME_UPDATE_AND_RENDER(name) void name(GameMemory *memory, GameTime *time, GameInput *input, GameOffscreenBuffer *buffer)
typedef GAME_UPDATE_AND_RENDER(GameUpdateAndRenderFunc);

#define GAME_GET_SOUND_SAMPLES(name) void name(GameMemory *memory, GameSoundOutputBuffer *buffer)
typedef GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesFunc);

#ifdef __cplusplus
}
#endif

#endif
