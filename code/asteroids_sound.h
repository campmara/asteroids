
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

    int32 buffer_size;
    int32 sample_count;
    int32 channel_count;
    int16 *samples;
};

internal SoundData LoadSoundOGG(MemoryArena *sound_arena, char *filename)
{
    SoundData result = {};

    int32 error;
    result.ogg_stream = stb_vorbis_open_filename(filename, &error, 0);
    result.ogg_info = stb_vorbis_get_info(result.ogg_stream);

    uint32 num_samples = stb_vorbis_stream_length_in_samples(result.ogg_stream);
    result.sample_count = num_samples;
    result.channel_count = result.ogg_info.channels;

    result.buffer_size = num_samples * sizeof(int16) * result.channel_count;
    result.samples = (int16 *)PushSize(sound_arena, result.buffer_size);

    int32 num_samples_filled = stb_vorbis_get_samples_short_interleaved(result.ogg_stream,
                                                                        result.channel_count,
                                                                        result.samples,
                                                                        result.sample_count * result.channel_count);

    Assert(num_samples_filled == result.sample_count);

    return result;
}

// =================================================================================================
// .WAV FILE HANDLING
// =================================================================================================

#pragma pack(push, 1)

struct WAVESoundData
{
    ReadFileResult wav_file;

    uint32 buffer_size;
    uint32 sample_count;
    uint32 channel_count;
    int16 *samples;
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

internal WAVESoundData LoadSoundWAV(MemoryArena *sound_arena, char *filename)
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

        result.buffer_size = sample_data_size;
        result.sample_count = sample_data_size / (channel_count * sizeof(int16));
        result.channel_count = channel_count;

        //result.samples = (int16 *)PushSize(sound_arena, sample_data_size / channel_count);
        result.samples = sample_data;
    }

    return result;
}

#pragma pack(pop)

#endif
