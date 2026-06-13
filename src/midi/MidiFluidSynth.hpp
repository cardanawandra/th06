#pragma once

#include "ZunResult.hpp"
#include "inttypes.hpp"

#include <fluidsynth.h>

struct MidiDevice
{
public:
    MidiDevice();
    ~MidiDevice();

    bool OpenDevice(const char* soundFontPath);
    ZunResult Close();

    bool SendShortMsg(u8 midiStatus,
                      u8 firstByte,
                      u8 secondByte);

    bool SendLongMsg(const u8* buf,
                     u32 len);

private:
    void Reset();

    fluid_settings_t* settings;
    fluid_synth_t* synth;
    fluid_audio_driver_t* audioDriver;

    int soundFontId;
    bool initialized;
};