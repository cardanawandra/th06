#include "SoundPlayer.hpp"

#include "FileSystem.hpp"
#include "Supervisor.hpp"
#include "i18n.hpp"
#include "utils.hpp"
#include "compat/Compat.hpp"
#include "GamePaths.hpp"

#define DISABLE_SOUNDPLAYER_ZUN return ZUN_SUCCESS
#define DISABLE_SOUNDPLAYER_BGM return 0
#define DISABLE_SOUNDPLAYER return

#include <math.h>
#include <cstring>
#define BACKGROUND_MUSIC_WAV_NUM_CHANNELS 2
#define BACKGROUND_MUSIC_WAV_SAMPLE_RATE 44100
#define BACKGROUND_MUSIC_WAV_BITS_PER_SAMPLE 16
#define BACKGROUND_MUSIC_WAV_BLOCK_ALIGN (BACKGROUND_MUSIC_WAV_BITS_PER_SAMPLE / 8 * BACKGROUND_MUSIC_WAV_NUM_CHANNELS)
#define BACKGROUND_MUSIC_WAV_BYTE_RATE (BACKGROUND_MUSIC_WAV_BLOCK_ALIGN * BACKGROUND_MUSIC_WAV_SAMPLE_RATE)

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

void SoundPlayer::AudioCallback(void* userdata, u8* stream, int len)
{
    DISABLE_SOUNDPLAYER;
}

ZunResult SoundPlayer::InitializeDSound()
{
    DISABLE_SOUNDPLAYER_ZUN;
}

ZunResult SoundPlayer::Release()
{
    DISABLE_SOUNDPLAYER_ZUN;
}

void SoundPlayer::StopBGM()
{
    DISABLE_SOUNDPLAYER;
}

void SoundPlayer::FadeOut(f32 seconds)
{
    DISABLE_SOUNDPLAYER;
}

ZunResult SoundPlayer::LoadWav(const char *path)
{
    DISABLE_SOUNDPLAYER_ZUN;
}

ZunResult SoundPlayer::LoadPos(const char *path)
{
    DISABLE_SOUNDPLAYER_ZUN;
}

ZunResult SoundPlayer::InitSoundBuffers()
{
    DISABLE_SOUNDPLAYER_ZUN;
}


ZunResult SoundPlayer::LoadSound(i32 idx, const char *path, f32 volumeMultiplier)
{
    DISABLE_SOUNDPLAYER_ZUN;
}

ZunResult SoundPlayer::PlayBGM(bool isLooping)
{
    DISABLE_SOUNDPLAYER_ZUN;
}

void SoundPlayer::PlaySounds()
{
    DISABLE_SOUNDPLAYER;
}

void SoundPlayer::PlaySoundByIdx(SoundIdx idx)
{
    DISABLE_SOUNDPLAYER;
}

void SoundPlayer::MixAudio(u32 samples)
{
    DISABLE_SOUNDPLAYER;
}

int CALLCOMPAT SoundPlayer::BackgroundMusicPlayerThread(void* data)
{
    DISABLE_SOUNDPLAYER_BGM;
}