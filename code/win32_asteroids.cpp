// IMPORTANT(mara): Remember! We want to put as much of our code as possible BEFORE the inclusion
// of the windows header!
#include "asteroids.h"

#include <windows.h>

#include "win32_asteroids.h"

global bool32 global_is_running;
global bool32 global_is_paused;
global Win32OffscreenBuffer global_backbuffer;

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
            global_is_running = true;

            while (global_is_running)
            {
                Win32ProcessPendingMessages();

                if (!global_is_paused)
                {
                    Win32WindowDimensions dimensions = GetWindowDimensions(window);
                    HDC device_context = GetDC(window);
                    Win32DisplayBufferInWindow(&global_backbuffer, device_context,
                                               dimensions.width, dimensions.height);
                    ReleaseDC(window, device_context);
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
