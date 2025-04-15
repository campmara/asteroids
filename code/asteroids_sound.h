#ifndef ASTEROIDS_SOUND_H
#define ASTEROIDS_SOUND_H

#include "include/stb_vorbis.h"

enum SoundID
{
    SOUND_BANG_LARGE = 0,
    SOUND_BANG_MEDIUM,
    SOUND_BANG_SMALL,
    SOUND_BEAT_1,
    SOUND_BEAT_2,
    SOUND_EXTRA_SHIP,
    SOUND_FIRE,
    SOUND_SAUCER_BIG,
    SOUND_SAUCER_SMALL,
    SOUND_THRUST,

    //

    SOUND_ASSET_COUNT,
};

struct SoundData
{
    stb_vorbis_info ogg_info;
    stb_vorbis *ogg_stream;

    int32 full_buffer_size;
    int32 sample_count;
    int32 channel_count;
    float32 *samples;
};

struct SoundStream
{
    float32 volume[2]; // left / right speaker volumes

    uint32 loaded_sound_id;
    int32 samples_played;

    bool32 is_loop;

    // Simple linked-list so we can have an arbitrary amount of playing sounds.
    SoundStream *next;
};

#endif
