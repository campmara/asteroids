// IMPORTANT(mara): Remember! We want to put as much of our code as possible BEFORE the inclusion
// of the windows header!
#include "asteroids.h"

#include <windows.h>
#include <stdio.h>
#include <xinput.h>
#include <xaudio2.h>

#include "win32_asteroids.h"

global bool32 global_is_running;
global bool32 global_is_paused;
global Win32OffscreenBuffer global_backbuffer;
global Win32XAudio2Container global_xaudio2_container;
global float64 global_perf_count_frequency;

// =================================================================================================
// PLATFORM FILE API
// =================================================================================================

// (void *memory)
PLATFORM_FREE_FILE_MEMORY(PlatformFreeFileMemory)
{
    if (memory)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

// (char *filename)
PLATFORM_READ_ENTIRE_FILE(PlatformReadEntireFile)
{
    ReadFileResult result = {};
    HANDLE file_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (file_handle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER file_size;
        if (GetFileSizeEx(file_handle, &file_size))
        {
            uint32 file_size_32 = SafeTruncateUInt64(file_size.QuadPart);
            result.content = VirtualAlloc(0, file_size_32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (result.content)
            {
                DWORD bytes_read;
                if (ReadFile(file_handle, result.content, file_size_32, &bytes_read, 0) &&
                    (file_size_32 == bytes_read))
                {
                    // NOTE(mara): File read successfully.
                    result.content_size = file_size_32;
                }
                else
                {
                    // TODO(mara): Logging
                    PlatformFreeFileMemory(result.content);
                    result.content = 0;
                }
            }
            else
            {
                // TODO(mara): Logging
            }
        }
        else
        {
            // TODO(mara): Logging
        }

        CloseHandle(file_handle);
    }
    else
    {
        // TODO(mara): Logging
    }

    return result;
}

// (char *filename, uint32 memory_size, void *memory)
PLATFORM_WRITE_ENTIRE_FILE(PlatformWriteEntireFile)
{
    bool32 result = false;

    HANDLE file_handle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

    if (file_handle != INVALID_HANDLE_VALUE)
    {
        DWORD bytes_written;
        if (WriteFile(file_handle, memory, memory_size, &bytes_written, 0))
        {
            // NOTE(mara): File written successfully.
            result = (bytes_written == memory_size);
        }
        else
        {
            // TODO(mara): Logging
        }

        CloseHandle(file_handle);
    }
    else
    {
        // TODO(mara): Logging
    }

    return result;
}

// (int16 *samples)
PLATFORM_PLAY_SOUND_SAMPLES(PlatformPlaySoundSamples)
{

}

// =================================================================================================
// GAME CODE HOT-RELOADING
// =================================================================================================

internal void Win32GetEXEFileName(Win32State *state)
{
    DWORD size_of_file_name = GetModuleFileNameA(0, state->exe_filename, sizeof(state->exe_filename));
    state->one_past_last_exe_filename_slash = state->exe_filename;
    for (char *scan = state->exe_filename; *scan; ++scan)
    {
        if (*scan == '\\')
        {
            state->one_past_last_exe_filename_slash = scan + 1;
        }
    }
}

internal void Win32BuildEXEPath(Win32State *state, char *filename, int dest_count, char *dest)
{
    ConcatenateStrings(state->one_past_last_exe_filename_slash - state->exe_filename, state->exe_filename,
                       GetStringLength(filename), filename,
                       dest_count, dest);
}

inline FILETIME Win32GetLastFileWriteTime(char *filename)
{
    FILETIME last_write_time = {};

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesExA(filename, GetFileExInfoStandard, &data))
    {
        last_write_time = data.ftLastWriteTime;
    }

    return last_write_time;
}

internal Win32GameCode Win32LoadGameCode(char *source_dll_name, char *temp_dll_name)
{
    Win32GameCode result = {};

    result.dll_last_write_time = Win32GetLastFileWriteTime(source_dll_name);

    CopyFile(source_dll_name, temp_dll_name, FALSE);

    result.game_code_dll = LoadLibraryA(temp_dll_name);
    if (result.game_code_dll)
    {
        result.UpdateAndRender = (GameUpdateAndRenderFunc *)GetProcAddress(result.game_code_dll, "GameUpdateAndRender");
        result.GetSoundSamples = (GameGetSoundSamplesFunc *)GetProcAddress(result.game_code_dll, "GameGetSoundSamples");

        result.is_valid = (result.UpdateAndRender != 0) && (result.GetSoundSamples != 0);
    }

    if (!result.is_valid)
    {
        result.UpdateAndRender = 0;
        result.GetSoundSamples = 0;
    }

    return result;
}

internal void Win32UnloadGameCode(Win32GameCode *game_code)
{
    if (game_code->game_code_dll)
    {
        FreeLibrary(game_code->game_code_dll);
        game_code->game_code_dll = 0;
    }

    game_code->is_valid = false;
    game_code->UpdateAndRender = 0;
    game_code->GetSoundSamples = 0;
}

// =================================================================================================
// XINPUT LIBRARY LOADING AND PROCESSING
// =================================================================================================

#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef XINPUT_GET_STATE(XInputGetStateFunc);
XINPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global XInputGetStateFunc *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef XINPUT_SET_STATE(XInputSetStateFunc);
XINPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global XInputSetStateFunc *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void Win32LoadXInput()
{
    HMODULE xinput_library = LoadLibraryA("xinput1_4.dll");
    if (!xinput_library)
    {
        xinput_library = LoadLibraryA("Xinput9_1_0.dll");
    }

    if (!xinput_library)
    {
        xinput_library = LoadLibraryA("xinput1_3.dll");
    }

    if (xinput_library)
    {
        XInputGetState = (XInputGetStateFunc *)GetProcAddress(xinput_library, "XInputGetState");
        if (!XInputGetState) { XInputGetState = XInputGetStateStub; }

        XInputSetState = (XInputSetStateFunc *)GetProcAddress(xinput_library, "XInputSetState");
        if (!XInputSetState) { XInputSetState = XInputSetStateStub; }
    }
    else
    {
        // TODO(mara): Maybe think about doing some proper logging here?
        OutputDebugStringA("Could not load XInput library.");
    }
}

internal void Win32ProcessKeyboardMessage(GameButtonState *new_state, bool32 is_down)
{
    // Only process new state if it's actually different from the current state.
    if (new_state->ended_down != is_down)
    {
        new_state->ended_down = is_down;
        ++new_state->half_transition_count;
    }
}

internal void Win32ProcessXInputDigitalButton(DWORD xinput_button_state, DWORD button_bit,
                                              GameButtonState *prev_state,
                                              GameButtonState *new_state)
{
    new_state->ended_down = ((xinput_button_state & button_bit) == button_bit);
    new_state->half_transition_count = (prev_state->ended_down != new_state->ended_down) ? 1 : 0;
}

internal float32 Win32ProcessXInputStickValue(SHORT value, SHORT deadzone_threshold)
{
    float32 result = 0;
    if (value < -deadzone_threshold)
    {
        // TODO(mara): Find out why 32768 instead of the actual max SHORT value, 32767...
        result = (float32)((value + deadzone_threshold) / (32768.0f - deadzone_threshold));
    }
    else if (value > deadzone_threshold)
    {
        result = (float32)((value - deadzone_threshold) / (32768.0f - deadzone_threshold));
    }
    return result;
}

// =================================================================================================
// XAUDIO2 LIBRARY LOADING AND PROCESSING
// =================================================================================================

#define XAUDIO2_CREATE(name) HRESULT name(IXAudio2 **ppXAudio2, UINT32 Flags, XAUDIO2_PROCESSOR XAudio2Processor)
typedef XAUDIO2_CREATE(XAudio2CreateFunc);

internal void Win32InitXAudio2(Win32SoundOutput *sound_output, Win32XAudio2Container *xaudio2_container)
{
    HMODULE xaudio2_library = LoadLibraryA("XAUDIO2_9.DLL");
    if (!xaudio2_library)
    {
        // TODO(mara): Logging
        xaudio2_library = LoadLibraryA("XAUDIO2_8.DLL");
    }

    if (!xaudio2_library)
    {
        // TODO(mara): Logging
        xaudio2_library = LoadLibraryA("XAUDIO2_7.DLL");
    }

    if (xaudio2_library)
    {
        XAudio2CreateFunc *XAudio2Create = (XAudio2CreateFunc *)GetProcAddress(xaudio2_library, "XAudio2Create");
        if (XAudio2Create && SUCCEEDED(XAudio2Create(&xaudio2_container->xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
        {
            if (xaudio2_container->xaudio2 && SUCCEEDED(xaudio2_container->xaudio2->CreateMasteringVoice(&xaudio2_container->mastering_voice)))
            {
                WAVEFORMATEX wave_format = {};
                wave_format.wFormatTag = WAVE_FORMAT_PCM;
                wave_format.nChannels = 2;
                wave_format.nSamplesPerSec = sound_output->samples_per_second;
                wave_format.wBitsPerSample = 16;
                wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
                wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
                wave_format.cbSize = 0;

                if (xaudio2_container->mastering_voice)
                {
                    if (SUCCEEDED(xaudio2_container->xaudio2->CreateSourceVoice(&xaudio2_container->source_voice, &wave_format)))
                    {
                        OutputDebugStringA("[Win32InitXAudio2] Succeeded in creating source voice.\n");
                    }
                    else
                    {
                        // TODO(mara): Logging
                    }
                }
                else
                {
                    // TODO(mara): Logging
                }
            }
            else
            {
                // TODO(mara): Logging
            }
        }
        else
        {
            // TODO(mara): Logging
        }
    }
    else
    {
        // TODO(mara): XAudio2 failed to load. Log here.
    }
}

void Win32FillSoundBuffer(Win32SoundOutput *sound_output, GameSoundOutputBuffer *source_buffer)
{
    XAUDIO2_BUFFER xaudio2_buffer = {};
    xaudio2_buffer.Flags = XAUDIO2_END_OF_STREAM;
    xaudio2_buffer.AudioBytes = source_buffer->sample_count * sound_output->bytes_per_sample;
    xaudio2_buffer.pAudioData = (byte *)source_buffer->samples;
    xaudio2_buffer.PlayBegin = 0;
    xaudio2_buffer.PlayLength = 0;
    xaudio2_buffer.LoopBegin = 0;
    xaudio2_buffer.LoopLength = 0;
    xaudio2_buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

    if (SUCCEEDED(global_xaudio2_container.source_voice->SubmitSourceBuffer(&xaudio2_buffer)))
    {
        if (SUCCEEDED(global_xaudio2_container.source_voice->Start(0)))
        {

        }
        else
        {
            // TODO(mara): Logging
        }
    }
    else
    {
        // TODO(mara): Logging
    }
}

// =================================================================================================
// DISPLAY
// =================================================================================================

internal Win32WindowDimensions GetWindowDimensions(HWND window)
{
    Win32WindowDimensions result;

    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.width = client_rect.right - client_rect.left;
    result.height = client_rect.bottom - client_rect.top;

    return result;
}

internal void Win32ResizeDIBSection(Win32OffscreenBuffer *buffer, int new_width, int new_height)
{
    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = new_width;
    buffer->height = new_height;
    buffer->bytes_per_pixel = BITMAP_BYTES_PER_PIXEL;

    // NOTE(mara): when the biHeight field is negative, this is the clue to Windows to treat this
    // bitmap as top-down, not bottom-up, meaning that the first three bytes of the image are the
    // color for the top left pixel in the bitmap, not the bottom left!
    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32; // 8 bits for each RGB, padding for DWORD-alignment.
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bitmap_memory_size = (buffer->width * buffer->height) * buffer->bytes_per_pixel;
    buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);

    buffer->pitch = buffer->width * buffer->bytes_per_pixel;
}

internal void Win32DisplayBufferInWindow(Win32OffscreenBuffer *buffer,
                                         HDC device_context,
                                         int window_width, int window_height)
{
    StretchDIBits(device_context,
                  0, 0, buffer->width, buffer->height, // dest
                  0, 0, buffer->width, buffer->height, // src
                  buffer->memory,
                  &buffer->info,
                  DIB_RGB_COLORS, SRCCOPY);

}

// =================================================================================================
// TIMING
// =================================================================================================

inline LARGE_INTEGER Win32GetTimeCounter()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

inline float64 Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    float64 result = ((float64)(end.QuadPart - start.QuadPart) / global_perf_count_frequency);
    return result;
}

// =================================================================================================
// WINDOW MESSAGE PROCESSING
// =================================================================================================

internal void Win32ProcessPendingMessages(GameControllerInput *keyboard_input)
{
    MSG message;
    while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT)
        {
            global_is_running = false;
        }

        switch (message.message)
        {
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                uint32 vk_code = (uint32)message.wParam; // Ok to truncate, VK Codes are not 64bit.

#define KEY_MESSAGE_WAS_DOWN_BIT (1 << 30)
#define KEY_MESSAGE_IS_DOWN_BIT (1 << 31)

                // Conversion to 0 or 1 values w/ bit tests.
                bool32 was_down = ((message.lParam & KEY_MESSAGE_WAS_DOWN_BIT) != 0);
                bool32 is_down = ((message.lParam & KEY_MESSAGE_IS_DOWN_BIT) == 0);
                if (was_down != is_down)
                {
                    if (vk_code == 'W') // WASD / Arrow Keys for movement.
                    {
                        Win32ProcessKeyboardMessage(&keyboard_input->move_up, is_down);
                    }
                    else if (vk_code == 'A')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_input->move_left, is_down);
                    }
                    else if (vk_code == 'S')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_input->move_down, is_down);
                    }
                    else if (vk_code == 'D')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_input->move_right, is_down);
                    }
                    else if (vk_code == VK_UP)
                    {
                        Win32ProcessKeyboardMessage(&keyboard_input->move_up, is_down);
                    }
                    else if (vk_code == VK_LEFT)
                    {
                        Win32ProcessKeyboardMessage(&keyboard_input->move_left, is_down);
                    }
                    else if (vk_code == VK_DOWN)
                    {
                        Win32ProcessKeyboardMessage(&keyboard_input->move_down, is_down);
                    }
                    else if (vk_code == VK_RIGHT)
                    {
                        Win32ProcessKeyboardMessage(&keyboard_input->move_right, is_down);
                    }
                    else if (vk_code == VK_ESCAPE) // Escape is the menu button.
                    {
                        Win32ProcessKeyboardMessage(&keyboard_input->start, is_down);
                    }
                    else if (vk_code == VK_SPACE) // Space is fire.
                    {
                        Win32ProcessKeyboardMessage(&keyboard_input->action_down, is_down);
                    }
#if ASTEROIDS_DEBUG
                    else if (vk_code == 'P' && is_down)
                    {
                        global_is_paused = !global_is_paused;
                    }
#endif
                }

                // Handle ALT-F4 case.
                bool32 alt_key_was_down = (message.lParam & (1 << 29));
                if (vk_code == VK_F4 && alt_key_was_down)
                {
                    global_is_running = false;
                }
            } break;
            default:
            {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            } break;
        }
    }
}

LRESULT CALLBACK Win32WindowCallback(HWND window,
                                     UINT message,
                                     WPARAM w_param,
                                     LPARAM l_param)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_SIZE:
        {
            uint32 width = LOWORD(l_param);
            uint32 height = HIWORD(l_param);
            Win32ResizeDIBSection(&global_backbuffer, width, height);
        } break;

        case WM_ACTIVATEAPP:
        {
        } break;

        case WM_CLOSE:
        case WM_DESTROY:
        {
            // TODO(mara): Handle this with a message to the user?
            global_is_running = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert(!"Keyboard input came out the wrong end!");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;

            HDC device_context = BeginPaint(window, &paint);
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;

            Win32WindowDimensions dimensions = GetWindowDimensions(window);

            Win32DisplayBufferInWindow(&global_backbuffer, device_context,
                                       dimensions.width, dimensions.height);
            EndPaint(window, &paint);
        } break;

        default:
        {
            // IMPORTANT(mara): You sometimes forget that this function absolutely MUST be called
            // somewhere in your event processing loop for windows, so here's another reminder that
            // this call does the work of telling windows to go ahead and process every window
            // message that you *haven't* processed in your program in the "default" Windows way.
            result = DefWindowProcA(window, message, w_param, l_param);
        } break;
    }

    return result;
}

int CALLBACK WinMain(HINSTANCE instance,
                     HINSTANCE prev_instance,
                     LPSTR     cmd_line,
                     int       show_code)
{
    Win32State win32_state = {};

    LARGE_INTEGER perf_count_frequency_result;
    QueryPerformanceFrequency(&perf_count_frequency_result);
    global_perf_count_frequency = (float32)perf_count_frequency_result.QuadPart;

    Win32LoadXInput();
    Win32ResizeDIBSection(&global_backbuffer, 1366, 768);

    WNDCLASSA window_class = {};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = Win32WindowCallback;
    window_class.hInstance = instance;
    window_class.lpszClassName = "AsteroidsWindowClass";

    if (RegisterClassA(&window_class))
    {
        HWND window = CreateWindowExA(0,
                                      window_class.lpszClassName,
                                      "Mara's Asteroids",
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      global_backbuffer.width, global_backbuffer.height,
                                      0, 0, instance, 0);

        if (window)
        {
            // NOTE(mara): Set the windows scheduler granularity to 1ms so that our sleep can be more granular.
            UINT desired_scheduler_ms = 1;
            bool32 sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);

            Win32GetEXEFileName(&win32_state);
            char source_game_code_dll_full_path[WIN32_STATE_FILE_NAME_COUNT];
            Win32BuildEXEPath(&win32_state, "asteroids.dll",
                              sizeof(source_game_code_dll_full_path), source_game_code_dll_full_path);
            char temp_game_code_dll_full_path[WIN32_STATE_FILE_NAME_COUNT];
            Win32BuildEXEPath(&win32_state, "asteroids_temp.dll",
                              sizeof(temp_game_code_dll_full_path), temp_game_code_dll_full_path);

            // NOTE(mara): Specifying 0 for the lpszDeviceName field of the EnumDisplaySettingsA
            // function tells windows we just want to query the display settings of the monitor the
            // program is launched from.
            int monitor_refresh_hz = 60;
            DEVMODEA device_mode = {};
            device_mode.dmSize = sizeof(DEVMODEA);
            if (EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &device_mode))
            {
                if (device_mode.dmDisplayFrequency > 1)
                {
                    monitor_refresh_hz = device_mode.dmDisplayFrequency;
                }
            }
            float32 game_update_hz = ((float32)monitor_refresh_hz/* / 2.0f*/);
            float32 target_seconds_per_frame = 1.0f / game_update_hz;

            Win32SoundOutput sound_output = {};
            sound_output.volume = 0.5f;
            sound_output.samples_per_second = SOUND_SAMPLES_PER_SECOND;
            sound_output.bytes_per_sample = sizeof(int16) * 2;
            sound_output.buffer_size = sound_output.samples_per_second * sound_output.bytes_per_sample;

            int16 *samples = (int16 *)VirtualAlloc(0, sound_output.buffer_size,
                                                   MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            global_xaudio2_container = {};
            Win32InitXAudio2(&sound_output, &global_xaudio2_container);

#if ASTEROIDS_DEBUG
            LPVOID base_address = (LPVOID)GIGABYTES((uint64)512);
#else
            LPVOID base_address = 0;
#endif

            // Allocate all our memory for the game upfront!
            GameMemory game_memory = {};
            // TODO(mara): Figure out exactly how much memory we need, and allocate no more than that.
            game_memory.permanent_storage_size = MEGABYTES(32);
            game_memory.transient_storage_size = MEGABYTES(32);

            win32_state.total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
            win32_state.game_memory_block = VirtualAlloc(base_address, (size_t)win32_state.total_size,
                                                         MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            game_memory.permanent_storage = win32_state.game_memory_block;
            game_memory.transient_storage = ((uint8 *)game_memory.permanent_storage + game_memory.permanent_storage_size);

            // Load the platform file API.
            game_memory.platform_api.FreeFileMemory = PlatformFreeFileMemory;
            game_memory.platform_api.ReadEntireFile = PlatformReadEntireFile;
            game_memory.platform_api.WriteEntireFile = PlatformWriteEntireFile;
            game_memory.platform_api.PlaySoundSamples = PlatformPlaySoundSamples;

            global_is_running = true;

            if (samples && game_memory.permanent_storage && game_memory.transient_storage)
            {
                GameInput input[2] = {};
                GameInput *new_input = &input[0];
                GameInput *old_input = &input[1];

                GameTime time = {};
                LARGE_INTEGER last_time_counter = Win32GetTimeCounter();
                LARGE_INTEGER flip_time_counter = Win32GetTimeCounter();
                time.delta_time = target_seconds_per_frame; // NOTE(mara): Intentionally fixed.

                bool32 sound_is_valid = false;

                Win32GameCode game = Win32LoadGameCode(source_game_code_dll_full_path,
                                                       temp_game_code_dll_full_path);

                uint64 last_cycle_count = __rdtsc();
                while (global_is_running)
                {
                    // Check for game code dll changes and reload the dll if necessary.
                    FILETIME new_dll_write_time = Win32GetLastFileWriteTime(source_game_code_dll_full_path);
                    if (CompareFileTime(&new_dll_write_time, &game.dll_last_write_time) != 0)
                    {
                        Win32UnloadGameCode(&game);
                        game = Win32LoadGameCode(source_game_code_dll_full_path, temp_game_code_dll_full_path);
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

                    Win32ProcessPendingMessages(new_keyboard_controller);

                    if (!global_is_paused)
                    {
                        POINT mouse_pos;
                        GetCursorPos(&mouse_pos);
                        ScreenToClient(window, &mouse_pos);
                        new_input->mouse_x = mouse_pos.x;
                        new_input->mouse_y = mouse_pos.y;
                        new_input->mouse_wheel = 0;
                        Win32ProcessKeyboardMessage(&new_input->mouse_buttons[0],
                                                    GetKeyState(VK_LBUTTON) & (1 << 15));
                        Win32ProcessKeyboardMessage(&new_input->mouse_buttons[1],
                                                    GetKeyState(VK_MBUTTON) & (1 << 15));
                        Win32ProcessKeyboardMessage(&new_input->mouse_buttons[2],
                                                    GetKeyState(VK_RBUTTON) & (1 << 15));
                        Win32ProcessKeyboardMessage(&new_input->mouse_buttons[3],
                                                    GetKeyState(VK_XBUTTON1) & (1 << 15));
                        Win32ProcessKeyboardMessage(&new_input->mouse_buttons[4],
                                                    GetKeyState(VK_XBUTTON2) & (1 << 15));

                        // We're just going to get input state for one possible controller. If we
                        // want more, then we just have to add more to our controllers array and
                        // make sure it's less than XUSER_MAX_COUNT.
#define CONTROLLER_INDEX 1
                        GameControllerInput *old_controller = GetController(old_input, CONTROLLER_INDEX);
                        GameControllerInput *new_controller = GetController(new_input, CONTROLLER_INDEX);

                        XINPUT_STATE controller_state;
                        if (XInputGetState(CONTROLLER_INDEX, &controller_state) == ERROR_SUCCESS)
                        {
                            new_controller->is_connected = true;
                            new_controller->is_analog = old_controller->is_analog;

                            XINPUT_GAMEPAD *pad = &controller_state.Gamepad;

                            new_controller->stick_average_x =
                                Win32ProcessXInputStickValue(pad->sThumbLX,
                                                             XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                            new_controller->stick_average_y =
                                Win32ProcessXInputStickValue(pad->sThumbLY,
                                                             XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                            if (new_controller->stick_average_x != 0 || new_controller->stick_average_y != 0)
                            {
                                new_controller->is_analog = true;
                            }
                            if (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                            {
                                new_controller->stick_average_y = 1.0f;
                                new_controller->is_analog = false;
                            }
                            if (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                            {
                                new_controller->stick_average_y = -1.0f;
                                new_controller->is_analog = false;
                            }
                            if (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                            {
                                new_controller->stick_average_x = -1.0f;
                                new_controller->is_analog = false;
                            }
                            if (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                            {
                                new_controller->stick_average_x = 1.0f;
                                new_controller->is_analog = false;
                            }

                            float32 threshold = 0.5f;
                            Win32ProcessXInputDigitalButton((new_controller->stick_average_y > threshold) ? 1 : 0,
                                                            1,
                                                            &old_controller->move_up,
                                                            &new_controller->move_up);
                            Win32ProcessXInputDigitalButton((new_controller->stick_average_y > threshold) ? 1 : 0,
                                                            1,
                                                            &old_controller->move_down,
                                                            &new_controller->move_down);
                            Win32ProcessXInputDigitalButton((new_controller->stick_average_x > threshold) ? 1 : 0,
                                                            1,
                                                            &old_controller->move_left,
                                                            &new_controller->move_left);
                            Win32ProcessXInputDigitalButton((new_controller->stick_average_x > threshold) ? 1 : 0,
                                                            1,
                                                            &old_controller->move_right,
                                                            &new_controller->move_right);

                            Win32ProcessXInputDigitalButton(pad->wButtons, XINPUT_GAMEPAD_A,
                                                            &old_controller->action_down,
                                                            &new_controller->action_down);
                            Win32ProcessXInputDigitalButton(pad->wButtons, XINPUT_GAMEPAD_B,
                                                            &old_controller->action_right,
                                                            &new_controller->action_right);
                            Win32ProcessXInputDigitalButton(pad->wButtons, XINPUT_GAMEPAD_X,
                                                            &old_controller->action_left,
                                                            &new_controller->action_left);
                            Win32ProcessXInputDigitalButton(pad->wButtons, XINPUT_GAMEPAD_Y,
                                                            &old_controller->action_up,
                                                            &new_controller->action_up);

                            Win32ProcessXInputDigitalButton(pad->wButtons, XINPUT_GAMEPAD_START,
                                                            &old_controller->start,
                                                            &new_controller->start);
                        }
                        else
                        {
                            new_controller->is_connected = false;
                        }

                        // Game update and render step.
                        GameOffscreenBuffer offscreen_buffer = {};
                        offscreen_buffer.memory = global_backbuffer.memory;
                        offscreen_buffer.width = global_backbuffer.width;
                        offscreen_buffer.height = global_backbuffer.height;
                        offscreen_buffer.pitch = global_backbuffer.pitch;
                        offscreen_buffer.bytes_per_pixel = global_backbuffer.bytes_per_pixel;

                        if (game.UpdateAndRender)
                        {
                            game.UpdateAndRender(&game_memory, &time, new_input, &offscreen_buffer);
                        }

                        // Game sound output step.
                        LARGE_INTEGER audio_time_counter = Win32GetTimeCounter();
                        float64 from_begin_to_audio_seconds = Win32GetSecondsElapsed(flip_time_counter, audio_time_counter);

                        XAUDIO2_VOICE_STATE voice_state = {};
                        // TODO(mara): Figure out if we need the buffer or the sampler state in the
                        // GetState call here.
                        global_xaudio2_container.source_voice->GetState(&voice_state, 0);
                        uint64 buffer_position = voice_state.SamplesPlayed % (uint64)sound_output.samples_per_second;

                        if (!sound_is_valid)
                        {
                            sound_output.running_sample_index = buffer_position;
                            sound_is_valid = true;
                        }

                        DWORD byte_position = ((sound_output.running_sample_index *
                                                sound_output.bytes_per_sample) %
                                               sound_output.buffer_size);
                        DWORD expected_sound_bytes_per_frame = (int32)((float32)(sound_output.samples_per_second *
                                                                                 sound_output.bytes_per_sample) / game_update_hz);
                        float64 seconds_left_until_flip = (target_seconds_per_frame - from_begin_to_audio_seconds);
                        DWORD expected_bytes_until_flip = (DWORD)((seconds_left_until_flip /
                                                                   target_seconds_per_frame) *
                                                                  (float32)expected_sound_bytes_per_frame);
                        uint64 expected_frame_boundary_byte = ((buffer_position * sound_output.bytes_per_sample) +
                                                               expected_bytes_until_flip);

                        uint64 target_position = expected_frame_boundary_byte + expected_sound_bytes_per_frame;
                        target_position = (target_position % sound_output.buffer_size);

                        uint64 bytes_to_write = 0;
                        bytes_to_write = (sound_output.buffer_size - target_position);
                        bytes_to_write += target_position;

                        GameSoundOutputBuffer sound_buffer = {};
                        sound_buffer.samples_per_second = sound_output.samples_per_second;
                        sound_buffer.sample_count = SafeTruncateUInt64(bytes_to_write) / sound_output.bytes_per_sample;
                        sound_buffer.samples = samples;
                        if (game.GetSoundSamples)
                        {
                            game.GetSoundSamples(&game_memory, &sound_buffer);
                        }

                        Win32FillSoundBuffer(&sound_output, &sound_buffer);

                        // Perform timing calculations and sleep.
                        LARGE_INTEGER time_now = Win32GetTimeCounter();
                        float64 frame_time = Win32GetSecondsElapsed(last_time_counter, time_now);

                        if (frame_time < target_seconds_per_frame)
                        {
                            if (sleep_is_granular)
                            {
                                DWORD sleep_ms = (DWORD)(1000.0f * (target_seconds_per_frame - frame_time));
                                if (sleep_ms > 0)
                                {
                                    Sleep(sleep_ms);
                                }
                            }

                            // Make sure that we slept here if we needed it this frame.
                            float64 test_frame_time = Win32GetSecondsElapsed(last_time_counter,
                                                                             Win32GetTimeCounter());
                            if (test_frame_time < target_seconds_per_frame)
                            {
                                // TODO(mara): We're always missing the sleep here because we're asking
                                // for too small a ms number to sleep for... Not sure what to do but
                                // maybe....do something?
                                // OutputDebugStringA("Missed sleep!\n");
                            }

                            // Finally, just spin out in a while loop if we missed the sleep.
                            while (frame_time < target_seconds_per_frame)
                            {
                                frame_time = Win32GetSecondsElapsed(last_time_counter, Win32GetTimeCounter());
                            }
                        }
                        else
                        {
                            // TODO(mara): Proper logging for the fact we missed the target fps!
                            OutputDebugStringA("Missed target FPS!\n");
                        }

                        time.total_time += frame_time;

                        LARGE_INTEGER end_time_counter = Win32GetTimeCounter();
                        float64 ms_per_frame = 1000.0 * Win32GetSecondsElapsed(last_time_counter, end_time_counter);
                        last_time_counter = end_time_counter;

                        // Blit the backbuffer to the window.
                        Win32WindowDimensions dimensions = GetWindowDimensions(window);
                        HDC device_context = GetDC(window);
                        Win32DisplayBufferInWindow(&global_backbuffer, device_context,
                                                   dimensions.width, dimensions.height);
                        ReleaseDC(window, device_context);

                        flip_time_counter = Win32GetTimeCounter();

                        GameInput *temp = new_input;
                        new_input = old_input;
                        old_input = temp;

#if 0
                        uint64 end_cycle_count = __rdtsc();
                        uint64 cycles_elapsed = end_cycle_count - last_cycle_count;
                        last_cycle_count = end_cycle_count;

                        float64 fps = global_perf_count_frequency / (ms_per_frame * 10000.0);
                        float64 mcpf = ((float64)cycles_elapsed / (1000.0 * 1000.0));

                        char fps_buffer[256];
                        _snprintf_s(fps_buffer, sizeof(fps_buffer),
                                    "| %.02fms/f | %.02ff/s | %.02fmc/f |\n", ms_per_frame, fps, mcpf);
                        OutputDebugStringA(fps_buffer);
#endif
                    }
                }
            }
            else
            {
                // TODO(mara): Logging.
            }
        }
        else
        {
            // TODO(mara): Logging.
        }
    }
    else
    {
        // TODO(mara): Logging.
    }

    return 0;
}
