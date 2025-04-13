#ifndef ASTEROIDS_FONT_H
#define ASTEROIDS_FONT_H

#define STB_TRUETYPE_IMPLEMENTATION
#include "include/stb_truetype.h"

struct FontData
{
    ReadFileResult ttf_file;
    stbtt_fontinfo stb_font_info;
    int32 ascent;
    int32 descent;
    int32 line_gap;
};

#endif
