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
    SOUND_SONG,

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
    int16 *samples;
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

// =================================================================================================
// .WAV FILE HANDLING
// =================================================================================================

#pragma pack(push, 1)

struct WAVESoundData
{
    ReadFileResult wav_file;

    uint32 sample_count;
    uint32 channel_count;
    int16 *samples[2];
};

struct WAVEHeader
{
    uint32 riff_id;
    uint32 size;
    uint32 wave_id;
};

#define RIFF_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))
enum
{
    WAVE_CHUNK_ID_FMT = RIFF_CODE('f', 'm', 't', ' '),
    WAVE_CHUNK_ID_DATA = RIFF_CODE('d', 'a', 't', 'a'),
    WAVE_CHUNK_ID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
    WAVE_CHUNK_ID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
};

struct WAVEChunk
{
    uint32 id;
    uint32 size;
};

struct WAVEFmt
{
    uint16 w_format_tag;            // 2-byte Format code
    uint16 n_channels;              // 2-byte Number of interleaved channels
    uint32 n_samples_per_sec;       // 4-byte Sampling rate (blocks per second)
    uint32 n_avg_bytes_per_sec;     // 4-byte Data rate
    uint16 n_block_align;           // 2-byte Data block size (bytes)
    uint16 w_bits_per_sample;       // 2-byte Bits per sample
    uint16 cb_size;                 // 2-byte Size of the extension (0 or 22)
    uint16 w_valid_bits_per_sample; // 2-byte Number of valid bits
    uint32 dw_channel_mask;         // 4-byte Speaker position mask
    uint8  sub_format[16];          // 16-byte GUID, including the data format code
};

struct RIFFIterator
{
    uint8 *at;
    uint8 *stop;
};

inline RIFFIterator ParseChunkAt(void *at, void *stop)
{
    RIFFIterator iter;

    iter.at = (uint8 *)at;
    iter.stop = (uint8 *)stop;

    return iter;
}

inline RIFFIterator NextChunk(RIFFIterator iter)
{
    WAVEChunk *chunk = (WAVEChunk *)iter.at;
    uint32 size = (chunk->size + 1) & ~1;
    iter.at += sizeof(WAVEChunk) + size;

    return iter;
}

inline bool32 IsValid(RIFFIterator iter)
{
    bool32 result = (iter.at < iter.stop);

    return result;
}

inline void *GetChunkData(RIFFIterator iter)
{
    void *result = (iter.at + sizeof(WAVEChunk));

    return result;
}

inline uint32 GetType(RIFFIterator iter)
{
    WAVEChunk *chunk = (WAVEChunk *)iter.at;
    uint32 result = chunk->id;

    return result;
}

inline uint32 GetChunkDataSize(RIFFIterator iter)
{
    WAVEChunk *chunk = (WAVEChunk *)iter.at;
    uint32 result = chunk->size;

    return result;
}

internal WAVESoundData LoadWAV(MemoryArena *sound_arena, char *filename)
{
    WAVESoundData result = {};

    result.wav_file = global_platform.ReadEntireFile(filename);
    if (result.wav_file.content_size != 0)
    {
        WAVEHeader *header = (WAVEHeader *)result.wav_file.content;
        Assert(header->riff_id == WAVE_CHUNK_ID_RIFF);
        Assert(header->wave_id == WAVE_CHUNK_ID_WAVE);

        uint32 channel_count = 0;
        uint32 sample_data_size = 0;
        int16 *sample_data = 0;
        for (RIFFIterator iter = ParseChunkAt(header + 1, (uint8 *)(header + 1) + header->size - 4);
             IsValid(iter);
             iter = NextChunk(iter))
        {
            switch(GetType(iter))
            {
                case WAVE_CHUNK_ID_FMT:
                {
                    WAVEFmt *fmt = (WAVEFmt *)GetChunkData(iter);
                    Assert(fmt->w_format_tag == 1); // NOTE(mara): Only support PCM.
                    Assert(fmt->n_samples_per_sec == 48000)
                    Assert(fmt->w_bits_per_sample == 16);
                    Assert(fmt->n_block_align = fmt->n_channels * sizeof(int16));
                    channel_count = fmt->n_channels;
                } break;

                case WAVE_CHUNK_ID_DATA:
                {
                    sample_data = (int16 *)GetChunkData(iter);
                    sample_data_size = GetChunkDataSize(iter);
                } break;
            }
        }

        Assert(channel_count && sample_data && sample_data_size);

        result.sample_count = sample_data_size / (channel_count * sizeof(int16));
        result.channel_count = channel_count;

        //result.samples[0] = (int16 *)PushSize(sound_arena, sample_data_size / channel_count);
        //result.samples[1] = (int16 *)PushSize(sound_arena, sample_data_size / channel_count);

        if (channel_count == 1)
        {
            result.samples[0] = sample_data;
            result.samples[1] = 0;
        }
        else if (channel_count == 2)
        {
            result.samples[0] = sample_data;
            result.samples[1] = sample_data + result.sample_count;

#if 0
            for (uint32 sample_index = 0; sample_index < result.sample_count; ++sample_index)
            {
                sample_data[sample_index * 2 + 0] = (int16)sample_index;
                sample_data[sample_index * 2 + 1] = (int16)sample_index;
            }
#endif

            // Swizzle the interleaved sample data in-place.
            for (uint32 sample_index = 0; sample_index < result.sample_count; ++sample_index)
            {
                int16 source = sample_data[sample_index * 2];
                sample_data[sample_index * 2] = sample_data[sample_index];
                sample_data[sample_index] = source;
            }
        }
        else
        {
            Assert(!"Invalid channel count in WAV file.");
        }
    }

    // TODO(mara): Load right channels!
    result.channel_count = 1;

    return result;
}

#pragma pack(pop)

#endif
