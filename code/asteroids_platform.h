#ifndef ASTEROIDS_PLATFORM_H
#define ASTEROIDS_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

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

/*
  ==================================================================================================
  NOTE(mara): Services that the game provides to the platform layer.
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
} GameMemory;

// #define an API that the platform layer will call in order to run the game.
#define GAME_UPDATE_AND_RENDER(name) void name(GameMemory *memory, GameTime *time, GameInput *input, GameOffscreenBuffer *buffer)
typedef GAME_UPDATE_AND_RENDER(GameUpdateAndRenderFunc);

#ifdef __cplusplus
}
#endif

#endif
