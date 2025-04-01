#ifndef ASTEROIDS_FONT_H
#define ASTEROIDS_FONT_H

#define STB_TRUETYPE_IMPLEMENTATION
#include "include/stb_truetype.h"

struct FontData
{
    ReadFileResult ttf_file;
    stbtt_fontinfo font;
};

internal FontData LoadFont()
{
    FontData font_data = {};
    font_data.ttf_file = global_platform.ReadEntireFile("fonts/MapleMono-Regular.ttf");

    font_data.font = {};
    stbtt_InitFont(&font_data.font, (uchar8 *)font_data.ttf_file.content,
                   stbtt_GetFontOffsetForIndex((uchar8 *)font_data.ttf_file.content, 0));

    return font_data;
}

#endif
