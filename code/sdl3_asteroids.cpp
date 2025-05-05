#include <SDL3/SDL.h>

#include "asteroids.h"

#include "sdl3_asteroids.h"

global bool32 global_is_running;
global bool32 global_is_paused;

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

inline SDL_Time SDL3GetLastFileWriteTime(char *filename)
{
    SDL_Time last_write_time = {};

    SDL_PathInfo path_info;
    if (SDL_GetPathInfo(filename, &path_info))
    {
        last_write_time = path_info.modify_time;
    }

    return last_write_time;
}

internal SDL3GameCode SDL3LoadGameCode(char *source_dll_name, char *temp_dll_name)
{
    SDL3GameCode result = {};

    result.dll_last_write_time = SDL3GetLastFileWriteTime(source_dll_name);

    SDL_CopyFile(source_dll_name, temp_dll_name);

    result.game_code_dll = SDL_LoadObject(temp_dll_name);
    if (result.game_code_dll)
    {
        result.UpdateAndRender = (GameUpdateAndRenderFunc *)SDL_LoadFunction(result.game_code_dll, "GameUpdateAndRender");

        result.is_valid = (result.UpdateAndRender != 0);
    }

    if (!result.is_valid)
    {
        result.UpdateAndRender = 0;
    }

    return result;
}

internal void SDL3UnloadGameCode(SDL3GameCode *game_code)
{
    if (game_code->game_code_dll)
    {
        SDL_UnloadObject(game_code->game_code_dll);
        game_code->game_code_dll = 0;
    }

    game_code->is_valid = false;
    game_code->UpdateAndRender = 0;
}

// =================================================================================================
// INPUT PROCESSING
// =================================================================================================

internal void SDL3ProcessKeyboardMessage(GameButtonState *new_state, bool32 is_down)
{
    // Only process new state if it's actually different from the current state.
    if (new_state->ended_down != is_down)
    {
        new_state->ended_down = is_down;
        ++new_state->half_transition_count;
    }
}

// =================================================================================================
// TIMING
// =================================================================================================

inline uint64 SDL3GetTimeCounter()
{
    return SDL_GetPerformanceCounter();
}

inline float64 SDL3GetSecondsElapsed(float64 perf_count_frequency, uint64 start, uint64 end)
{
    return ((float64)(end - start) / perf_count_frequency);
}

// =================================================================================================
// WINDOW EVENT PROCESSING
// =================================================================================================

internal void SDL3ProcessPendingMessages(GameControllerInput *keyboard_input)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            global_is_running = false;
        }

        switch (event.type)
        {
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
            {
                SDL_Keycode key_code = event.key.key;
                bool32 was_down = event.key.repeat;
                bool32 is_down = event.key.down;

                if (was_down != is_down)
                {
                    if (key_code == SDLK_W)
                    {
                        SDL3ProcessKeyboardMessage(&keyboard_input->move_up, is_down);
                    }
                    else if (key_code == SDLK_A)
                    {
                        SDL3ProcessKeyboardMessage(&keyboard_input->move_left, is_down);
                    }
                    else if (key_code == SDLK_S)
                    {
                        SDL3ProcessKeyboardMessage(&keyboard_input->move_down, is_down);
                    }
                    else if (key_code == SDLK_D)
                    {
                        SDL3ProcessKeyboardMessage(&keyboard_input->move_right, is_down);
                    }
                    else if (key_code == SDLK_UP)
                    {
                        SDL3ProcessKeyboardMessage(&keyboard_input->move_up, is_down);
                    }
                    else if (key_code == SDLK_LEFT)
                    {
                        SDL3ProcessKeyboardMessage(&keyboard_input->move_left, is_down);
                    }
                    else if (key_code == SDLK_DOWN)
                    {
                        SDL3ProcessKeyboardMessage(&keyboard_input->move_down, is_down);
                    }
                    else if (key_code == SDLK_RIGHT)
                    {
                        SDL3ProcessKeyboardMessage(&keyboard_input->move_right, is_down);
                    }
                    else if (key_code == SDLK_ESCAPE) // Escape is the menu button.
                    {
                        SDL3ProcessKeyboardMessage(&keyboard_input->start, is_down);
                    }
                    else if (key_code == SDLK_SPACE) // Space is fire.
                    {
                        SDL3ProcessKeyboardMessage(&keyboard_input->action_down, is_down);
                    }
#if ASTEROIDS_DEBUG
                    else if (key_code == SDLK_P && is_down)
                    {
                        global_is_paused = !global_is_paused;
                    }
#endif
                }

                // Handle ALT-F4 case. Might not be necessary but I think it's good practice to have
                // the engine be aware of and handle this case deliberately, in case we need to do
                // specific things for it at some point.
                if (key_code == SDLK_F4 && event.key.mod == SDL_KMOD_ALT)
                {
                    global_is_running = false;
                }
            } break;
            default:
            {
                // Currently nothing...
            } break;
        }
    }
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
        float64 perf_count_frequency = (float64)perf_count_frequency_result;

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

            global_is_running = true;

            if (/*samples && */game_memory.permanent_storage && game_memory.transient_storage)
            {
                GameInput input[2] = {};
                GameInput *new_input = &input[0];
                GameInput *old_input = &input[1];

                GameTime game_time = {};
                uint64 last_time_counter = SDL3GetTimeCounter();
                uint64 flip_time_counter = SDL3GetTimeCounter();
                game_time.delta_time = target_seconds_per_frame; // We should actually calculate this.

                SDL3GameCode game_code = SDL3LoadGameCode(source_game_code_dll_full_path,
                                                          temp_game_code_dll_full_path);

                uint64 last_cycle_count = SDL3GetTimeCounter();
                while (global_is_running)
                {
                    SDL_Time new_dll_write_time = SDL3GetLastFileWriteTime(source_game_code_dll_full_path);
                    if (new_dll_write_time != game_code.dll_last_write_time)
                    {
                        SDL3UnloadGameCode(&game_code);
                        game_code = SDL3LoadGameCode(source_game_code_dll_full_path, temp_game_code_dll_full_path);
                    }

                    GameControllerInput *old_keyboard_controller = GetController(old_input, 0);
                    GameControllerInput *new_keyboard_controller = GetController(new_input, 0);
                    *new_keyboard_controller = {};
                    new_keyboard_controller->is_connected = true;
                    for (int button_index = 0;
                         button_index < ArrayCount(new_keyboard_controller->buttons);
                         ++button_index)
                    {
                        new_keyboard_controller->buttons[button_index].ended_down =
                            old_keyboard_controller->buttons[button_index].ended_down;
                    }

                    SDL3ProcessPendingMessages(new_keyboard_controller);

                    if (!global_is_paused)
                    {
                        // Process mouse state.
                        float32 x, y;
                        SDL_MouseButtonFlags mouse_state = SDL_GetMouseState(&x, &y);
                        new_input->mouse_x = (int32)x;
                        new_input->mouse_y = (int32)y;
                        SDL3ProcessKeyboardMessage(&new_input->mouse_buttons[0],
                                                   mouse_state & SDL_BUTTON_LMASK);
                        SDL3ProcessKeyboardMessage(&new_input->mouse_buttons[0],
                                                   mouse_state & SDL_BUTTON_MMASK);
                        SDL3ProcessKeyboardMessage(&new_input->mouse_buttons[0],
                                                   mouse_state & SDL_BUTTON_RMASK);
                        SDL3ProcessKeyboardMessage(&new_input->mouse_buttons[0],
                                                   mouse_state & SDL_BUTTON_X1MASK);
                        SDL3ProcessKeyboardMessage(&new_input->mouse_buttons[0],
                                                   mouse_state & SDL_BUTTON_X2MASK);

                        //TODO(mara): YOU ARE HERE
                    }

                    SDL3OffscreenBuffer offscreen_buffer = {};

                    SDL_Texture *texture = SDL_CreateTexture(renderer,
                                                             SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
                                                             WINDOW_WIDTH, WINDOW_HEIGHT);

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
