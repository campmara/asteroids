#include "asteroids.h"

#include <windows.h>

global bool32 global_is_running;
global bool32 global_is_paused;

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
        } break;

        default:
        {
            result = DefWindowProcA(window, message, w_param, l_param);
            OutputDebugStringA("We're getting where we're supposed to!\n");
        } break;
    }

    return result;
}

internal void Win32ProcessPendingMessages()
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

int CALLBACK WinMain(HINSTANCE instance,
                     HINSTANCE prev_instance,
                     LPSTR     cmd_line,
                     int       show_code)
{
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
            global_is_running = true;

            while (global_is_running)
            {
                Win32ProcessPendingMessages();

                if (!global_is_paused)
                {
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

    return 0;
}
