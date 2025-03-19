#ifndef WIN32_ASTEROIDS_H
#define WIN32_ASTEROIDS_H

struct Win32OffscreenBuffer
{
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
    int bytes_per_pixel;
};

struct Win32WindowDimensions
{
    int width;
    int height;
};

#endif
