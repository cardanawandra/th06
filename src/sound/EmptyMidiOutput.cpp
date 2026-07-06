#include "../MidiOutput.hpp"
#include "../FileSystem.hpp"
#include "../Supervisor.hpp"
#include "../ZunMemory.hpp"
#include "../i18n.hpp"
#include "../inttypes.hpp"
#include "../utils.hpp"

#include "../compat/Compat.hpp"
#include <cstdlib>
#include <cstring>

//todo : midioutdev
void MidiOutput::StartTimer(u32 delay, COMPAT_TimerCallback cb, void *data)
{
    return;
}

i32 MidiOutput::StopTimer()
{
    return 1;
}

u32 CALLCOMPAT MidiOutput::DefaultTimerCallback(u32 interval, MidiOutput *timer)
{
    return 0;
}

u32 MidiOutput::ReadVariableLength(u8 **curTrackDataCursor)
{
    return 0;
}

MidiOutput::MidiOutput()
{
}

MidiOutput::~MidiOutput()
{
}

ZunResult MidiOutput::ReadFileData(u32 idx, const char *path)
{
    return ZUN_SUCCESS;
}

void MidiOutput::ReleaseFileData(u32 idx)
{
}

void MidiOutput::ClearTracks()
{
}

ZunResult MidiOutput::ParseFile(i32 fileIdx)
{
    return ZUN_SUCCESS;
}

ZunResult MidiOutput::LoadFile(const char *midiPath)
{
    return ZUN_SUCCESS;
}

void MidiOutput::LoadTracks()
{
}

ZunResult MidiOutput::Play()
{
    return ZUN_SUCCESS;
}

ZunResult MidiOutput::StopPlayback()
{
    return ZUN_SUCCESS;
}

u32 MidiOutput::SetFadeOut(u32 ms)
{
    return 0;
}

// Windows EoSD relies solely on the number of times this function is called for timing,
//   assuming that there is exactly 1 ms between calls. In my testing, the time between
//   calls with the SDL timer actually ends up averaging to 1.08 ms and the MIDI playback
//   ends up noticeably slow, so the timing mechanism has been replaced with getting a
//   delta from GET_TICKS instead.
void MidiOutput::OnTimerElapsed()
{
}

void MidiOutput::ProcessMsg(MidiTrack *track)
{
}

void MidiOutput::FadeOutSetVolume(i32 volume)
{
}