#include "SoundPlayer.hpp"

#include "FileSystem.hpp"
#include "Supervisor.hpp"
#include "i18n.hpp"
#include "utils.hpp"
#include "SDLCompat.hpp"
#include "GamePaths.hpp"

#include <SDL.h>
#include <SDL_timer.h>
#include "SDLCompat.hpp"

#ifdef WIN98
#define DISABLE_SOUNDPLAYER_ZUN return ZUN_SUCCESS
#define DISABLE_SOUNDPLAYER_BGM return 0
#define DISABLE_SOUNDPLAYER return
#else
#define DISABLE_SOUNDPLAYER_ZUN
#define DISABLE_SOUNDPLAYER_BGM
#define DISABLE_SOUNDPLAYER
#endif

#include <math.h>
#include <cstring>
#define BACKGROUND_MUSIC_WAV_NUM_CHANNELS 2
#define BACKGROUND_MUSIC_WAV_SAMPLE_RATE 44100
#define BACKGROUND_MUSIC_WAV_BITS_PER_SAMPLE 16
#define BACKGROUND_MUSIC_WAV_BLOCK_ALIGN (BACKGROUND_MUSIC_WAV_BITS_PER_SAMPLE / 8 * BACKGROUND_MUSIC_WAV_NUM_CHANNELS)
#define BACKGROUND_MUSIC_WAV_BYTE_RATE (BACKGROUND_MUSIC_WAV_BLOCK_ALIGN * BACKGROUND_MUSIC_WAV_SAMPLE_RATE)
#define SDL_LOG_COMPAT SDL_Log

static i16 g_audioBuffer[44100 * 4];
static volatile u32 g_audioWritePos = 0;
static volatile u32 g_audioReadPos = 0;

static const SoundBufferIdxVolume g_SoundBufferIdxVol[32] = {
    {0, -1500}, {0, -2000}, {1, -1200}, {1, -1400}, {2, -1000},  {3, -500},   {4, -500},   {5, -1700},
    {6, -1700}, {7, -1700}, {8, -1000}, {9, -1000}, {10, -1900}, {11, -1200}, {12, -900},  {5, -1500},
    {13, -900}, {14, -900}, {15, -600}, {16, -400}, {17, -1100}, {18, -900},  {5, -1800},  {6, -1800},
    {7, -1800}, {19, -300}, {20, -600}, {21, -800}, {22, -100},  {23, -500},  {24, -1000}, {25, -1000},
};
static const char *const g_SFXList[26] = {
    "data/wav/plst00.wav", "data/wav/enep00.wav",   "data/wav/pldead00.wav", "data/wav/power0.wav",
    "data/wav/power1.wav", "data/wav/tan00.wav",    "data/wav/tan01.wav",    "data/wav/tan02.wav",
    "data/wav/ok00.wav",   "data/wav/cancel00.wav", "data/wav/select00.wav", "data/wav/gun00.wav",
    "data/wav/cat00.wav",  "data/wav/lazer00.wav",  "data/wav/lazer01.wav",  "data/wav/enep01.wav",
    "data/wav/nep00.wav",  "data/wav/damage00.wav", "data/wav/item00.wav",   "data/wav/kira00.wav",
    "data/wav/kira01.wav", "data/wav/kira02.wav",   "data/wav/extend.wav",   "data/wav/timeout.wav",
    "data/wav/graze.wav",  "data/wav/powerup.wav",
};
SoundPlayer g_SoundPlayer;

SoundPlayer::SoundPlayer()
{
    terminateFlag = false;
}

void SoundPlayer::AudioCallback(void* userdata, Uint8* stream, int len)
{
    i16* out = (i16*)stream;
    u32 samples = len / 2;

    for (u32 i = 0; i < samples; i++)
    {
        if (g_audioReadPos != g_audioWritePos)
        {
            out[i] = g_audioBuffer[g_audioReadPos];
            g_audioReadPos =
                (g_audioReadPos + 1) %
                ARRAY_SIZE_SIGNED(g_audioBuffer);
        }
        else
        {
            out[i] = 0;
        }
    }
}

ZunResult SoundPlayer::InitializeDSound()
{
    DISABLE_SOUNDPLAYER_ZUN;
    SDL_AudioSpec desired;
    memset(&desired, 0, sizeof(desired));
    SDL_AudioSpec obtained;
    memset(&obtained, 0, sizeof(obtained));

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
        return ZUN_ERROR;

    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    // desired.samples = 2048;
    // desired.callback = SoundPlayer::AudioCallback;
    this->audioDev = SDL_OPEN_AUDIO_COMPAT(&desired, &obtained);
    if (this->audioDev == 0){
        SDL_LOG_COMPAT("NO AUDIO DEVICE\n");
        return ZUN_ERROR;
    }
    this->stream = SDL_CREATE_AUDIO_STREAM_COMPAT(&desired, &desired);
    if (!stream){
        SDL_LOG_COMPAT("CreateAudioStream failed: %s", SDL_GetError());
        return ZUN_ERROR;
    }
    if (!SDL_BIND_AUDIO_STREAM_COMPAT(this->audioDev, this->stream)){
        SDL_LOG_COMPAT("BindAudioStream failed: %s", SDL_GetError());
        return ZUN_ERROR;
    }

    terminateFlag = false;
    backgroundMusicThreadHandle = SDL_CREATE_THREAD_COMPAT(&SoundPlayer::BackgroundMusicPlayerThread, "bgm", this);

    // Start playback
    SDL_RESUME_AUDIO_COMPAT(this->audioDev);
    return ZUN_SUCCESS;
}

ZunResult SoundPlayer::Release()
{
    DISABLE_SOUNDPLAYER_ZUN;
    terminateFlag = true;

    // if (backgroundMusicThreadHandle.joinable())
    //     backgroundMusicThreadHandle.join();

    StopBGM();

    // for (auto& s : soundBuffers)
    // {
    //     delete[] s.samples;
    //     s.samples = NULL;
    // }
    SDL_DESTROY_AUDIO_STREAM(this->stream);
    SDL_CLOSE_AUDIO_COMPAT(this->audioDev);

    return ZUN_SUCCESS;
}

void SoundPlayer::StopBGM()
{
    DISABLE_SOUNDPLAYER;
    if (backgroundMusic.srcWav.fileStream)
    {
        SDL_RWCLOSE_COMPAT(backgroundMusic.srcWav.fileStream);
        backgroundMusic.srcWav.fileStream = NULL;
    }
}

void SoundPlayer::FadeOut(f32 seconds)
{
    DISABLE_SOUNDPLAYER;
    if (backgroundMusic.srcWav.fileStream)
    {
        backgroundMusic.fadeoutLen = seconds * 44100;
        backgroundMusic.fadeoutProgress = 0;
    }
}

ZunResult SoundPlayer::LoadWav(const char *path)
{
    DISABLE_SOUNDPLAYER_ZUN;
    SDL_RWOPS_COMPAT *fileStream;
    char idBuf[4];
    u32 riffSize;
    u32 wavDataSize;

    if (g_Supervisor.cfg.playSounds == 0)
    {
        return ZUN_ERROR;
    }

    this->StopBGM();

    SDL_LOG_COMPAT("load BGM\n");

#ifdef __ANDROID__
    char resolvedPath[512];
    snprintf(resolvedPath, sizeof(resolvedPath), "%s%s",
            GamePaths::GetUserPath(), path);
    fileStream = SDL_RWFROMFILE_COMPAT(resolvedPath, "r");
#else
    fileStream = SDL_RWFROMFILE_COMPAT(path, "r");
#endif
    if (fileStream == NULL)
    {
        SDL_LOG_COMPAT("error : wav file load error %s\n", path);
        return ZUN_ERROR;
    }

    // Minimum size of RIFF header and chunk info preceeding the sample data
    if (SDL_RW_SIZE_COMPAT(fileStream) < 44)
    {
        SDL_LOG_COMPAT("load sound fail 1\n"); return ZUN_ERROR;
    }

    if (SDL_RWREAD_COMPAT(fileStream, idBuf, 4, 1) != 1 || strncmp(idBuf, "RIFF", 4) != 0)
    {
        SDL_LOG_COMPAT("load sound fail 2\n"); return ZUN_ERROR;
    }

    riffSize = SDL_READLE32_COMPAT(fileStream);

    // Same bounds check done earlier on the total filesize
    if (riffSize < 36 || riffSize > SDL_RW_SIZE_COMPAT(fileStream) - 8)
    {
        SDL_LOG_COMPAT("load sound fail 3\n"); return ZUN_ERROR;
    }

    if (SDL_RWREAD_COMPAT(fileStream, idBuf, 4, 1) != 1 || strncmp(idBuf, "WAVE", 4) != 0)
    {
        SDL_LOG_COMPAT("load sound fail 4\n"); return ZUN_ERROR;
    }

    // Checks here are quite a bit less flexible than what WAV can represent. EoSD uses 44.1 kHz, stereo, 16-bit PCM
    //   so that's what we handle. We also assume that fmt and data are the only subchunks, which is definitely not
    //   a general guarantee, but it'll work fine with EoSD's WAV files.

    if (SDL_RWREAD_COMPAT(fileStream, idBuf, 4, 1) != 1 || strncmp(idBuf, "fmt ", 4) != 0)
    {
        SDL_LOG_COMPAT("load sound fail 5\n"); return ZUN_ERROR;
    }

    // Format subchunk size. Guaranteed 16 for PCM data
    if (SDL_READLE32_COMPAT(fileStream) != 16)
    {
        SDL_LOG_COMPAT("load sound fail 6\n"); return ZUN_ERROR;
    }

    // Audio format. 1 represents raw PCM samples
    if (SDL_READLE16_COMPAT(fileStream) != 1)
    {
        SDL_LOG_COMPAT("load sound fail 7\n"); return ZUN_ERROR;
    }

    // Number of channels. We expect stereo
    if (SDL_READLE16_COMPAT(fileStream) != BACKGROUND_MUSIC_WAV_NUM_CHANNELS)
    {
        SDL_LOG_COMPAT("load sound fail 8\n"); return ZUN_ERROR;
    }

    // Sample frequency rate
    if (SDL_READLE32_COMPAT(fileStream) != BACKGROUND_MUSIC_WAV_SAMPLE_RATE)
    {
        SDL_LOG_COMPAT("load sound fail 9\n"); return ZUN_ERROR;
    }

    // Byte rate
    if (SDL_READLE32_COMPAT(fileStream) != BACKGROUND_MUSIC_WAV_BYTE_RATE)
    {
        SDL_LOG_COMPAT("load sound fail 10\n"); return ZUN_ERROR;
    }

    // Block alignment
    if (SDL_READLE16_COMPAT(fileStream) != BACKGROUND_MUSIC_WAV_BLOCK_ALIGN)
    {
        SDL_LOG_COMPAT("load sound fail 11\n"); return ZUN_ERROR;
    }

    // Bits per sample
    if (SDL_READLE16_COMPAT(fileStream) != BACKGROUND_MUSIC_WAV_BITS_PER_SAMPLE)
    {
        SDL_LOG_COMPAT("load sound fail 12\n"); return ZUN_ERROR;
    }

    if (SDL_RWREAD_COMPAT(fileStream, idBuf, 4, 1) != 1 || strncmp(idBuf, "data", 4) != 0)
    {
        SDL_LOG_COMPAT("load sound fail 13\n"); return ZUN_ERROR;
    }

    wavDataSize = SDL_READLE32_COMPAT(fileStream);

    if (wavDataSize > riffSize - 44)
    {
        SDL_LOG_COMPAT("load sound fail 14\n"); return ZUN_ERROR;
    }

    this->backgroundMusic.srcWav.samples = wavDataSize / BACKGROUND_MUSIC_WAV_BLOCK_ALIGN;

    if (this->backgroundMusic.srcWav.samples == 0)
    {
        SDL_LOG_COMPAT("load sound fail 15\n"); return ZUN_ERROR;
    }

    this->backgroundMusic.srcWav.fileStream = fileStream;
    this->backgroundMusic.srcWav.dataStartOffset = SDL_RWTELL_COMPAT(fileStream);
    this->backgroundMusic.loopStart = 0;
    this->backgroundMusic.loopEnd = this->backgroundMusic.srcWav.samples;
    this->backgroundMusic.fadeoutLen = 0;
    this->backgroundMusic.fadeoutProgress = 0;
    this->backgroundMusic.pos = 0;

    return ZUN_SUCCESS;
}

ZunResult SoundPlayer::LoadPos(const char *path)
{
    DISABLE_SOUNDPLAYER_ZUN;
    u8 *fileData;

    if (g_Supervisor.cfg.playSounds == 0 || backgroundMusic.srcWav.fileStream == NULL)
    {
        return ZUN_ERROR;
    }

    fileData = FileSystem::OpenPath(path, 0);

    if (fileData == NULL)
    {
        return ZUN_ERROR;
    }

    this->backgroundMusic.loopStart = SDL_SwapLE32(*((u32 *)fileData));
    this->backgroundMusic.loopEnd = SDL_SwapLE32(*(u32 *)(fileData + 4));

    free(fileData);

    if (this->backgroundMusic.loopStart >= this->backgroundMusic.loopEnd ||
        this->backgroundMusic.loopEnd > this->backgroundMusic.srcWav.samples)
    {
        this->backgroundMusic.loopStart = 0;
        this->backgroundMusic.loopEnd = this->backgroundMusic.srcWav.samples;

        return ZUN_ERROR;
    }

    return ZUN_SUCCESS;
}

ZunResult SoundPlayer::InitSoundBuffers()
{
    DISABLE_SOUNDPLAYER_ZUN;
    SDL_LOG_COMPAT("init sound buffer\n");

    SDL_LOG_COMPAT("fill_n\n");
    for (int i = 0; i < ARRAY_SIZE_SIGNED(this->soundBuffersToPlay); i++) {
        this->soundBuffersToPlay[i] = -1;
    }

    SDL_LOG_COMPAT("for loop\n");
    for (int idx = 0; idx < ARRAY_SIZE_SIGNED(g_SoundBufferIdxVol); idx++)
    {
        if (this->LoadSound(idx, g_SFXList[g_SoundBufferIdxVol[idx].bufferIdx],
                            1.0f / ZUN_POWF(10.0f, (float)g_SoundBufferIdxVol[idx].volume / -2000)) != ZUN_SUCCESS)
        {
            GameErrorContext::Log(&g_GameErrorContext, TH_ERR_SOUNDPLAYER_FAILED_TO_LOAD_SOUND_FILE, g_SFXList[idx]);
            SDL_LOG_COMPAT("failed\n");
            return ZUN_ERROR;
        }

        this->soundBuffers[idx].isPlaying = false;
        this->soundBuffers[idx].pos = 0;
    }
    SDL_LOG_COMPAT("finish\n");
    return ZUN_SUCCESS;
}


ZunResult SoundPlayer::LoadSound(i32 idx, const char *path, f32 volumeMultiplier)
{
    DISABLE_SOUNDPLAYER_ZUN;
    SDL_LOG_COMPAT("load sound 1\n");
    SDL_AudioCVT sampleConversionDesc;
    SDL_AudioSpec wavFormat;
    u8 *wavRawData;
    u8 *wavRawSamples;
    u32 wavRawSampleByteCount;

    SDL_LOG_COMPAT("load sound 2\n");
    // soundBufMutex.lock();

    if (this->soundBuffers[idx].samples != NULL)
    {
        delete[] this->soundBuffers[idx].samples;
        this->soundBuffers[idx].samples = NULL;
    }

    SDL_LOG_COMPAT("load sound 3\n");
    wavRawData = (u8 *)FileSystem::OpenPath(path, 0);

    if (wavRawData == NULL)
    {
        SDL_LOG_COMPAT("load sound fail 16\n"); return ZUN_ERROR;
    }

    SDL_LOG_COMPAT("load sound 4\n");
    if (SDL_LoadWAV_RW(SDL_RWFROMCONSTMEM_COMPAT(wavRawData, g_LastFileSize), 1, &wavFormat, &wavRawSamples,
                       &wavRawSampleByteCount) == NULL)
    {
        GameErrorContext::Log(&g_GameErrorContext, TH_ERR_NOT_A_WAV_FILE, path);
        SDL_LOG_COMPAT("load sound fail 17\n"); return ZUN_ERROR;
    }

    // EoSD's sound files are all 22050 Hz, and some even use 8-bit samples. Converting them
    //   here only uses a few hundred extra kilobytes of RAM compared to the original code,
    //   but it might be worth looking into avoiding it for especially RAM-limited systems

    SDL_LOG_COMPAT("load sound 5\n");

    #if SDL_MAJOR_VERSION >= 3
    SDL_AudioSpec dstSpec;
    dstSpec.format = AUDIO_S16SYS;
    dstSpec.channels = 1;
    dstSpec.freq = 44100;

    u8 *convertedData = NULL;
    int convertedLen = 0;

    if (!SDL_ConvertAudioSamples(
            &wavFormat,
            wavRawSamples,
            wavRawSampleByteCount,
            &dstSpec,
            &convertedData,
            &convertedLen))
    {
        SDL_LOG_COMPAT(
            "audio conversion failed: %s\n",
            SDL_GetError());

        SDL_FreeWAV(wavRawSamples);

        return ZUN_ERROR;
    }

    this->soundBuffers[idx].len =
        convertedLen / sizeof(i16);

    this->soundBuffers[idx].samples =
        new i16[this->soundBuffers[idx].len];

    memcpy(
        this->soundBuffers[idx].samples,
        convertedData,
        convertedLen);
    SDL_free(convertedData);

    #else
    if (SDL_BuildAudioCVT(&sampleConversionDesc, wavFormat.format, wavFormat.channels, wavFormat.freq, AUDIO_S16SYS, 1,
                          44100) == 1)
    {
        sampleConversionDesc.len = wavRawSampleByteCount;
        sampleConversionDesc.buf = new u8[wavRawSampleByteCount * sampleConversionDesc.len_mult];
        memcpy(sampleConversionDesc.buf, wavRawSamples, wavRawSampleByteCount);

        SDL_ConvertAudio(&sampleConversionDesc);

        this->soundBuffers[idx].len = sampleConversionDesc.len_cvt / 2;
        this->soundBuffers[idx].samples = new i16[this->soundBuffers[idx].len];
        memcpy(this->soundBuffers[idx].samples, sampleConversionDesc.buf, sampleConversionDesc.len_cvt);

        delete[] sampleConversionDesc.buf;
    }
    else
    {
        this->soundBuffers[idx].len = wavRawSampleByteCount / 2;
        this->soundBuffers[idx].samples = new i16[this->soundBuffers[idx].len];
        memcpy(this->soundBuffers[idx].samples, wavRawSamples, wavRawSampleByteCount);
    }
    #endif

    SDL_LOG_COMPAT("load sound 6\n");
    SDL_FreeWAV(wavRawSamples);

    for (u32 i = 0; i < this->soundBuffers[idx].len; i++)
    {
        this->soundBuffers[idx].samples[i] *= volumeMultiplier;
    }

    this->soundBuffers[idx].pos = 0;
    this->soundBuffers[idx].isPlaying = false;

    SDL_LOG_COMPAT("load sound success\n");
    // soundBufMutex.unlock();
    return ZUN_SUCCESS;
}

ZunResult SoundPlayer::PlayBGM(bool isLooping)
{
    DISABLE_SOUNDPLAYER_ZUN;
    SDL_LOG_COMPAT("play BGM\n");

    if (this->backgroundMusic.srcWav.fileStream == NULL)
    {
        SDL_LOG_COMPAT("error lagi\n");
        return ZUN_ERROR;
    }
    SDL_LOG_COMPAT("comp\n");
    this->isLooping = isLooping;
    return ZUN_SUCCESS;
}

void SoundPlayer::PlaySounds()
{
    DISABLE_SOUNDPLAYER;
    for (int i = 0; i < 3; i++)
    {
        int idx = soundBuffersToPlay[i];
        if (idx < 0) continue;

        soundBuffers[idx].pos = 0;
        soundBuffers[idx].isPlaying = true;

        soundBuffersToPlay[i] = -1;
    }
}

void SoundPlayer::PlaySoundByIdx(SoundIdx idx)
{
    DISABLE_SOUNDPLAYER;
    SDL_LOG_COMPAT("play Sound by index\n");
    for (int i = 0; i < 3; i++)
    {
        if (soundBuffersToPlay[i] < 0)
        {
            soundBuffersToPlay[i] = idx;
            return;
        }
    }
}

void SoundPlayer::MixAudio(u32 samples)
{
    DISABLE_SOUNDPLAYER;
    // Allocate raw buffers (PC-98 safer than std::vector in many toolchains)
    i32* mixBuffer   = new i32[samples];

    u32 i;
    // Clear mix buffer
    for (i = 0; i < samples; i++)
    {
        mixBuffer[i]   = 0;
    }

    u8 playingChannels = 0;

    // this->soundBufMutex.lock();

    // =========================
    // Sound effects mixing
    // =========================
    for (i = 0; i < ARRAY_SIZE_SIGNED(this->soundBuffers); i++)
    {
        SoundData* snd = &this->soundBuffers[i];

        if (!snd->isPlaying)
            continue;

        playingChannels++;

        u32 remaining = snd->len - snd->pos;

        u32 samplesToMix =
            ((samples >> 1) < remaining)
                ? (samples >> 1)
                : remaining;

        i16* src = snd->samples + snd->pos;
        i32* dst = mixBuffer;

        for (u32 j = 0; j < samplesToMix; j++)
        {
            i16 s = *src++;

            *dst++ += s;
            *dst++ += s;
        }

        snd->pos += samplesToMix;

        if (snd->pos >= snd->len)
        {
            snd->isPlaying = false;
        }
    }
    // =========================
    // Background music mixing
    // =========================
    if (this->backgroundMusic.srcWav.fileStream != NULL)
    {
        u32 samplesMixed = 0;
        f32 fadeoutMult;

        if (this->backgroundMusic.fadeoutLen != 0)
        {
            f32 fadeoutInterp =
                mapRange(this->backgroundMusic.fadeoutProgress,
                         0,
                         this->backgroundMusic.fadeoutLen,
                         0,
                         5);

            fadeoutMult = 1.0f / ZUN_POWF(10.0f, fadeoutInterp / 2.0f);
        }
        else
        {
            fadeoutMult = 1.0f;
        }

        while (samplesMixed < samples / 2)
        {
            const u32 samplesToMix =
                ((samples / 2 - samplesMixed) <
                (this->backgroundMusic.loopEnd - this->backgroundMusic.pos))
                    ? (samples / 2 - samplesMixed)
                    : (this->backgroundMusic.loopEnd - this->backgroundMusic.pos);

            for (u32 j = 0; j < samplesToMix; j++)
            {
                i16 left =
                    (i16)SDL_READLE16_COMPAT(this->backgroundMusic.srcWav.fileStream);

                i16 right =
                    (i16)SDL_READLE16_COMPAT(this->backgroundMusic.srcWav.fileStream);

                u32 outPos = (samplesMixed + j) * 2;

                mixBuffer[outPos]     += (i32)(left * fadeoutMult);
                mixBuffer[outPos + 1] += (i32)(right * fadeoutMult);
            }

            this->backgroundMusic.pos += samplesToMix;
            samplesMixed += samplesToMix;

            if (this->backgroundMusic.pos == this->backgroundMusic.loopEnd)
            {
                if (this->isLooping)
                {
                    this->backgroundMusic.pos = this->backgroundMusic.loopStart;

                    SDL_RWSEEK_COMPAT(
                        this->backgroundMusic.srcWav.fileStream,
                        this->backgroundMusic.srcWav.dataStartOffset +
                            this->backgroundMusic.pos * 4,
                        RW_SEEK_SET);
                }
                else
                {
                    SDL_RWCLOSE_COMPAT(this->backgroundMusic.srcWav.fileStream);
                    this->backgroundMusic.srcWav.fileStream = NULL;
                    break;
                }
            }
        }
        
        if (this->backgroundMusic.fadeoutLen != 0)
        {
            this->backgroundMusic.fadeoutProgress += samplesMixed;

            if (this->backgroundMusic.fadeoutProgress >= this->backgroundMusic.fadeoutLen)
            {
                SDL_RWCLOSE_COMPAT(this->backgroundMusic.srcWav.fileStream);
                this->backgroundMusic.srcWav.fileStream = NULL;
            }
        }

        playingChannels++;
    }

    // this->soundBufMutex.unlock();

    // =========================
    // Final mix down
    // =========================
    int channels = (int)playingChannels;

    const int mixDivisor = (8 > channels) ? 8 : channels;

    #if SDL_MAJOR_VERSION == 1
    for (i = 0; i < samples; i++)
    {
        g_audioBuffer[g_audioWritePos] = (i16)(mixBuffer[i] / mixDivisor);

        g_audioWritePos =
            (g_audioWritePos + 1) %
            ARRAY_SIZE_SIGNED(g_audioBuffer);
    }
    #else

    i16* finalBuffer = new i16[samples];
    for (i = 0; i < samples; i++)
    {
        finalBuffer[i] = (i16)(mixBuffer[i] / mixDivisor);
    }

    SDL_QUEUE_AUDIO_COMPAT(this->audioDev, this->stream, finalBuffer, samples * 2);
    #endif
    delete[] mixBuffer;
}

int SoundPlayer::BackgroundMusicPlayerThread(void* data)
{
    DISABLE_SOUNDPLAYER_BGM;
    SoundPlayer* self = (SoundPlayer*)data;

    u32 latencyLimit = 14700; // ~5 frames
    u64 samplesSent = 0;
    u64 startTick = SDL_GetTicks();

    while (1)
    {
        u64 curTicks = SDL_GetTicks();

        // Keep slightly more than 1 frame's worth of samples in the audio buffer at all times
        i32 targetSamples =
            (i32)(((i32)(curTicks - startTick)) * 44.100)
            - samplesSent
            + 1024;
        // Quick and dirty checks to keep audio latency low
        //   Can probably be horribly broken, but I don't have weaker hardware to test on
        if (targetSamples > 1024)
        {
            samplesSent += targetSamples - 1024;
            targetSamples = 1024;
        }

        if (targetSamples > 0)
        {
            self->MixAudio(targetSamples * 2);
            samplesSent += targetSamples;
        }

        if (self->terminateFlag)
        {
            return 0;
        }
    }
    return 0;
}