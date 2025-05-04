#include <SDL3/SDL.h>

#include "asteroids.h"

#include "sdl3_asteroids.h"

global bool32 global_is_running;
global float32 global_perf_count_frequency;

// =================================================================================================
// PLATFORM FILE API
// =================================================================================================

// (void *memory)
PLATFORM_FREE_FILE_MEMORY(PlatformFreeFileMemory)
{
    if (memory)
    {
        SDL_free(memory);
    }
}

// (char *filename)
PLATFORM_READ_ENTIRE_FILE(PlatformReadEntireFile)
{
    ReadFileResult result = {};
    size_t file_size;
    result.content = SDL_LoadFile(filename, &file_size);

    if (result.content)
    {
        uint32 file_size_32 = SafeTruncateSizeT(file_size);
        result.content_size = file_size_32;
    }
    else
    {
        // TODO(mara): Logging.
        PlatformFreeFileMemory(result.content);
        result.content = 0;
    }

    return result;
}

// (char *filename, uint32 memory_size, void *memory)
PLATFORM_WRITE_ENTIRE_FILE(PlatformWriteEntireFile)
{
    bool32 result = SDL_SaveFile(filename, memory, memory_size);
    return result;
}

// =================================================================================================
// GAME CODE HOT-RELOADING
// =================================================================================================

internal void SDL3GetEXEFileName(SDL3State *state)
{
    char exe_directory[SDL3_STATE_FILE_NAME_COUNT];
    sprintf_s(exe_directory, "%s", SDL_GetBasePath());
    int dir_length = GetStringLength(exe_directory);

    ConcatenateStrings(dir_length, exe_directory,
                       19, "sdl3_asteroids.exe\0",
                       dir_length + 19, state->exe_filename);

    state->one_past_last_exe_filenmae_slash = state->exe_filename;
    for (char *scan = state->exe_filename; *scan; ++scan)
    {
        if (*scan == '\\')
        {
            state->one_past_last_exe_filenmae_slash = scan + 1;
        }
    }
}

internal void SDL3BuildPathFromEXE(SDL3State *state, char *filename, int dest_count, char *dest)
{
    ConcatenateStrings(state->one_past_last_exe_filenmae_slash - state->exe_filename, state->exe_filename,
                       GetStringLength(filename), filename,
                       dest_count, dest);
}

// =================================================================================================
// int main()
// =================================================================================================

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        SDL3State sdl3_state = {};

        uint64 perf_count_frequency_result = SDL_GetPerformanceFrequency();
        global_perf_count_frequency = (float32)perf_count_frequency_result;

        SDL_Window *window;
        SDL_Renderer *renderer;

        if (SDL_CreateWindowAndRenderer("Mara's Asteroids",
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_RESIZABLE,
                                        &window, &renderer))
        {
            // Build paths for game dlls.
            SDL3GetEXEFileName(&sdl3_state);

            char source_game_code_dll_full_path[SDL3_STATE_FILE_NAME_COUNT];
            SDL3BuildPathFromEXE(&sdl3_state, "asteroids.dll",
                                 sizeof(source_game_code_dll_full_path),
                                 source_game_code_dll_full_path);

            char temp_game_code_dll_full_path[SDL3_STATE_FILE_NAME_COUNT];
            SDL3BuildPathFromEXE(&sdl3_state, "asteroids_temp.dll",
                                 sizeof(temp_game_code_dll_full_path),
                                 temp_game_code_dll_full_path);

            // Refresh rate and delta_time calculation.
            float32 monitor_refresh_hz = 60;
            SDL_DisplayID display_id = SDL_GetPrimaryDisplay();
            const SDL_DisplayMode *display_mode = SDL_GetCurrentDisplayMode(display_id);
            if (display_mode)
            {
                monitor_refresh_hz = display_mode->refresh_rate;
            }
            float32 game_update_hz = monitor_refresh_hz/* / 2.0f */;
            float32 target_seconds_per_frame = 1.0f / game_update_hz;

            // TODO(mara): Sound initialization.

            // Memory initialization.
            GameMemory game_memory = {};
            game_memory.permanent_storage_size = MEGABYTES(32);
            game_memory.transient_storage_size = MEGABYTES(32);

            sdl3_state.total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
            sdl3_state.game_memory_block = SDL_malloc((size_t)sdl3_state.total_size);

            game_memory.permanent_storage = sdl3_state.game_memory_block;
            game_memory.transient_storage = ((uint8 *)game_memory.permanent_storage + game_memory.permanent_storage_size);

            // Load the platform file API.
            game_memory.platform_api.FreeFileMemory = PlatformFreeFileMemory;
            game_memory.platform_api.ReadEntireFile = PlatformReadEntireFile;
            game_memory.platform_api.WriteEntireFile = PlatformWriteEntireFile;

            // TODO(mara): Input initialization.

            // TODO(mara): Game code dll code loading.

            SDL3OffscreenBuffer offscreen_buffer = {};

            SDL_Texture *texture = SDL_CreateTexture(renderer,
                                                     SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
                                                     WINDOW_WIDTH, WINDOW_HEIGHT);

            global_is_running = true;

            while (global_is_running)
            {
                SDL_Event event;

                while (SDL_PollEvent(&event))
                {
                    if (event.type == SDL_EVENT_QUIT)
                    {
                        global_is_running = false;
                    }

                    // TODO(mara): Input processing.

                    // TODO(mara): Rendering.
                    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
                    SDL_RenderClear(renderer);

                    // SDL_RenderTexture(renderer, texture, NULL, NULL);
                    // SDL_RenderPresent(renderer);\

                    // TODO(mara): Sound Processing.

                    // TODO(mara): Figure out FPS and output that.
                }
            }
        }
        else
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create window and renderer: %s\n", SDL_GetError());
            return 3;
        }
    }
    else
    {
        return 3;
    }

    return 0;
}
