#ifndef WIN32_ASTEROIDS_H
#define WIN32_ASTEROIDS_H

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH

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

struct Win32SoundOutput
{
    float32 volume;
    int32 bytes_per_sample;
    uint32 samples_per_second;
    uint32 buffer_size; // Audio buffer size in bytes.
    uint64 running_sample_index;

    IXAudio2 *xaudio2;
    IXAudio2MasteringVoice *mastering_voice;
    IXAudio2SourceVoice *source_voice;
};

struct Win32GameCode
{
    HMODULE game_code_dll;
    FILETIME dll_last_write_time;

    // Function Pointers
    // IMPORTANT(mara): These could be 0! You have to check before calling them.
    GameUpdateAndRenderFunc *UpdateAndRender;
    GameGetSoundSamplesFunc *GetSoundSamples;

    bool32 is_valid;
};

struct Win32State
{
    uint64 total_size;
    void *game_memory_block;

    char exe_filename[WIN32_STATE_FILE_NAME_COUNT];
    char *one_past_last_exe_filename_slash;
};

#endif
