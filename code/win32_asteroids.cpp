// IMPORTANT(mara): Remember! We want to put as much of our code as possible BEFORE the inclusion
// of the windows header!
#include "asteroids.h"

#include <windows.h>
#include <xinput.h>

#include "win32_asteroids.h"

global bool32 global_is_running;
global bool32 global_is_paused;
global Win32OffscreenBuffer global_backbuffer;

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
    buffer->bytes_per_pixel = 4;

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
            ASSERT(!"Keyboard input came out the wrong end!");
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

    Win32LoadXInput();
    Win32ResizeDIBSection(&global_backbuffer, 1280, 720);

    WNDCLASSA window_class = {};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = Win32WindowCallback;
    window_class.hInstance = instance;
    window_class.lpszClassName = "AsteroidsWindowClass";

    if (RegisterClassA(&window_class))
    {
        HWND window = CreateWindowExA(0,
                                      window_class.lpszClassName,
                                      "Marasteroids",
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      0, 0, instance, 0);

        if (window)
        {
            // TODO(mara): Try building paths for the game code dll here.
            // Win32GetExeFilename(&win32_state);
            char source_game_code_dll_full_path[WIN32_STATE_FILE_NAME_COUNT];
            // Win32BuildExePathFilename

            char temp_game_code_dll_full_path[WIN32_STATE_FILE_NAME_COUNT];
            // Win32BuildExePathFilename

            // Query the monitor refresh rate and use that to determine our update rates.

            // NOTE(mara): Specifying 0 for the lpszDeviceName field of the EnumDisplaySettingsA
            // function tells windows we just want to query the display settings of the monitor the
            // program is launched from.
            DEVMODEA device_mode = {};
            device_mode.dmSize = sizeof(DEVMODEA);
            if (EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &device_mode))
            {

            }

#if ASTEROIDS_DEBUG
            LPVOID base_address = (LPVOID)GIGABYTES((uint64)512);
#else
            LPVOID base_address = 0;
#endif

            // Allocate all our memory for the game upfront!
            GameMemory game_memory = {};
            // TODO(mara): Figure out exactly how much memory we need, and allocate no more than that.
            game_memory.permanent_storage_size = MEGABYTES(64);
            game_memory.transient_storage_size = MEGABYTES(64);

            win32_state.total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
            win32_state.game_memory_block = VirtualAlloc(base_address, (size_t)win32_state.total_size,
                                                         MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            game_memory.permanent_storage = win32_state.game_memory_block;
            game_memory.transient_storage = ((uint8 *)game_memory.permanent_storage + game_memory.permanent_storage_size);

            global_is_running = true;

            if (game_memory.permanent_storage && game_memory.transient_storage)
            {
                GameInput input[2] = {};
                GameInput *new_input = &input[0];
                GameInput *old_input = &input[1];

                Win32GameCode game = Win32LoadGameCode(source_game_code_dll_full_path,
                                                       temp_game_code_dll_full_path);

                while (global_is_running)
                {
                    // TODO(mara): Set delta time here once we get the right values from the monitor.
                    // new_input->delta_time = target_seconds_per_frame;

                    // TODO(mara): Check here for the last game code dll write time and reload the
                    // dll if it's changed.

                    GameControllerInput *old_keyboard_controller = GetController(old_input, 0);
                    GameControllerInput *new_keyboard_controller = GetController(new_input, 0);
                    *new_keyboard_controller = {};
                    new_keyboard_controller->is_connected = true;
                    for (int button_index = 0;
                         button_index < ARRAY_COUNT(new_keyboard_controller->buttons);
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

                        GameOffscreenBuffer offscreen_buffer = {};
                        offscreen_buffer.memory = global_backbuffer.memory;
                        offscreen_buffer.width = global_backbuffer.width;
                        offscreen_buffer.height = global_backbuffer.height;
                        offscreen_buffer.pitch = global_backbuffer.pitch;
                        offscreen_buffer.bytes_per_pixel = global_backbuffer.bytes_per_pixel;

                        if (game.UpdateAndRender)
                        {
                            game.UpdateAndRender(&game_memory, new_input, &offscreen_buffer);
                        }

                        Win32WindowDimensions dimensions = GetWindowDimensions(window);
                        HDC device_context = GetDC(window);
                        Win32DisplayBufferInWindow(&global_backbuffer, device_context,
                                                   dimensions.width, dimensions.height);
                        ReleaseDC(window, device_context);

                        GameInput *temp = new_input;
                        new_input = old_input;
                        old_input = temp;
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
