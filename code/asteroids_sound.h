#ifndef ASTEROIDS_SOUND_H
#define ASTEROIDS_SOUND_H

#include "include/stb_vorbis.h"

struct SoundData
{
    stb_vorbis_info ogg_info;
    stb_vorbis *ogg_stream;

    int32 frame_size;
    float32 *samples;
};

#endif
