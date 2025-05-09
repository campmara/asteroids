#ifndef SDL3_ASTEROIDS_H
#define SDL3_ASTEROIDS_H

#define WINDOW_WIDTH 1366
#define WINDOW_HEIGHT 768

#define SDL3_STATE_FILE_NAME_COUNT 260

#define SDL3_GAMEPAD_AXIS_DEADZONE 7849

struct SDL3OffscreenBuffer
{
    SDL_Renderer *sdl_renderer;
    SDL_Texture *sdl_texture;
    void *memory;
    int32 width;
    int32 height;
    int32 pitch;
    int32 bytes_per_pixel;
};

struct SDL3WindowDimensions
{
    int width;
    int height;
};

struct SDL3SoundOutput
{
    float32 volume;
    int32 bytes_per_sample;
    uint32 samples_per_second;
    uint32 buffer_size; // Audio buffer size in bytes.
};

struct SDL3GameCode
{
    SDL_SharedObject *game_code_dll;
    SDL_Time dll_last_write_time;

    GameUpdateAndRenderFunc *UpdateAndRender;

    bool32 is_valid;
};

struct SDL3State
{
    uint64 total_size;
    void *game_memory_block;

    char exe_filename[SDL3_STATE_FILE_NAME_COUNT];
    char *one_past_last_exe_filenmae_slash;
};

#endif
