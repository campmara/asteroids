#include <windows.h>

LRESULT CALLBACK Win32WindowCallback(HWND window,
                                     UINT message,
                                     WPARAM w_param,
                                     LPARAM l_param)
{
    LRESULT result = 0;

    return result;
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
        /*
        HWND CreateWindowExA(
            [in]           DWORD     dwExStyle,
            [in, optional] LPCSTR    lpClassName,
            [in, optional] LPCSTR    lpWindowName,
            [in]           DWORD     dwStyle,
            [in]           int       X,
            [in]           int       Y,
            [in]           int       nWidth,
            [in]           int       nHeight,
            [in, optional] HWND      hWndParent,
            [in, optional] HMENU     hMenu,
            [in, optional] HINSTANCE hInstance,
            [in, optional] LPVOID    lpParam
                             );
        */
    }
}
