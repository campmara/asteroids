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
};

struct Win32XAudio2VoiceCallback : public IXAudio2VoiceCallback
{
    bool32 is_completed;

    void OnStreamEnd()
    {
        is_completed = true;
    }

    void OnBufferStart(void *buffer_context)
    {
        is_completed = false;
    }

    void OnVoiceProcessingPassEnd() {}
    void OnVoiceProcessingPassStart(UINT32 samples_required) {}
    void OnBufferEnd(void *buffer_context) {}
    void OnLoopEnd(void *buffer_context) {}
    void OnVoiceError(void *buffer_context, HRESULT error) {}
};

struct Win32AudioVoice
{
    SoundStream *playing_sound;

    IXAudio2SourceVoice *source_voice;
    Win32XAudio2VoiceCallback voice_callback;
};

struct Win32XAudio2Container
{
    IXAudio2 *xaudio2;
    IXAudio2MasteringVoice *mastering_voice;

    Win32AudioVoice audio_voices[MAX_CONCURRENT_SOUNDS];
};

struct Win32GameCode
{
    HMODULE game_code_dll;
    FILETIME dll_last_write_time;

    // Function Pointers
    // IMPORTANT(mara): These could be 0! You have to check before calling them.
    GameUpdateAndRenderFunc *UpdateAndRender;

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
