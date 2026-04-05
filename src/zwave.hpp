//-----------------------------------------------------------------------------
// File: zwave.hpp - SDL2_mixer based replacement for DSUtil
//-----------------------------------------------------------------------------
#ifndef DSUTIL_H
#define DSUTIL_H

#include "inttypes.hpp"
#include "sdl2_compat.hpp"
#include <SDL_mixer.h>

namespace th06
{

class CSoundManager;
class CStreamingSound;
class CWaveFile;

#define WAVEFILE_READ 1
#define WAVEFILE_WRITE 2

#define DSUtil_StopSound(s)                                                                                            \
    {                                                                                                                  \
        if (s)                                                                                                         \
            s->Stop();                                                                                                 \
    }
#define DSUtil_PlaySound(s)                                                                                            \
    {                                                                                                                  \
        if (s)                                                                                                         \
            s->Play(0, 0);                                                                                             \
    }
#define DSUtil_PlaySoundLooping(s)                                                                                     \
    {                                                                                                                  \
        if (s)                                                                                                         \
            s->Play(0, 1);                                                                                             \
    }

class CSoundManager
{
  public:
    CSoundManager();
    ~CSoundManager();

    HRESULT Initialize(HWND hWnd, DWORD dwCoopLevel, DWORD dwPrimaryChannels, DWORD dwPrimaryFreq,
                       DWORD dwPrimaryBitRate);

    HRESULT CreateStreaming(CStreamingSound **ppStreamingSound, char *strWaveFileName, DWORD dwCreationFlags,
                            GUID guid3DAlgorithm, DWORD dwNotifyCount, DWORD dwNotifySize, HANDLE hNotifyEvent);

    i32 m_initialized;
};

class CStreamingSound
{
  public:
    CStreamingSound();
    ~CStreamingSound();

    HRESULT Play(DWORD dwPriority, DWORD dwFlags);
    HRESULT Stop();
    HRESULT Reset();

    HRESULT UpdateFadeOut();
    HRESULT HandleWaveStreamNotification(BOOL bLoopedPlay);

    HRESULT FillBufferWithSound(void *unused, BOOL bRepeatWavIfBufferLarger);
    void *GetBuffer(DWORD dwIndex);

    CWaveFile *m_pWaveFile;

    i32 m_dwCurFadeoutProgress;
    i32 m_dwTotalFadeout;
    DWORD m_dwIsFadingOut;

    u8 *m_pcmData;
    u32 m_pcmDataSize;
    u32 m_playPosition;
    i32 m_isPlaying;

    static void MusicHookCallback(void *udata, u8 *stream, int len);
};

class CWaveFile
{
  public:
    WAVEFORMATEX *m_pwfx;
    DWORD m_dwSize;
    i32 m_loopStartPoint;
    i32 m_loopEndPoint;

    u8 *m_fileData;
    u8 *m_pcmData;
    u32 m_pcmDataSize;
    WAVEFORMATEX m_wfxStorage;

    CWaveFile();
    ~CWaveFile();

    HRESULT Open(char *strFileName, WAVEFORMATEX *pwfx, DWORD dwFlags);
    HRESULT Close();
    DWORD GetSize();
    HRESULT ResetFile(bool loop);
    WAVEFORMATEX *GetFormat()
    {
        return m_pwfx;
    };
};
}; // namespace th06

#endif // DSUTIL_H
