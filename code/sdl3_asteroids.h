#ifndef SDL3_ASTEROIDS_H
#define SDL3_ASTEROIDS_H

#define WINDOW_WIDTH 1366
#define WINDOW_HEIGHT 768

#define SDL3_STATE_FILE_NAME_COUNT 260

struct SDL3Container
{
};

struct SDL3OffscreenBuffer
{
    SDL_Texture *sdl_texture;
    void *memory;
};



struct SDL3State
{
    uint64 total_size;
    void *game_memory_block;

    char exe_filename[SDL3_STATE_FILE_NAME_COUNT];
    char *one_past_last_exe_filenmae_slash;
};

#endif
