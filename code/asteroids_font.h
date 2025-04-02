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

internal FontData LoadFont()
{
    FontData font_data = {};
    font_data.ttf_file = global_platform.ReadEntireFile("fonts/MapleMono-Regular.ttf");

    font_data.stb_font_info = {};
    stbtt_InitFont(&font_data.stb_font_info, (uchar8 *)font_data.ttf_file.content,
                   stbtt_GetFontOffsetForIndex((uchar8 *)font_data.ttf_file.content, 0));

    stbtt_GetFontVMetrics(&font_data.stb_font_info,
                          &font_data.ascent,
                          &font_data.descent,
                          &font_data.line_gap);

    return font_data;
}

#endif
