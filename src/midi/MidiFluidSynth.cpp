#include "MidiFluidSynth.hpp"
#include "i18n.hpp"
#include "utils.hpp"

MidiDevice::MidiDevice()
{
    this->settings = nullptr;
    this->synth = nullptr;
    this->audioDriver = nullptr;
    this->soundFontId = FLUID_FAILED;
    this->initialized = false;
}

MidiDevice::~MidiDevice()
{
    this->Close();
}

bool MidiDevice::OpenDevice(const char* soundFontPath)
{
    this->Close();

    this->settings = new_fluid_settings();
    if (!this->settings)
    {
        goto fail;
    }

    this->synth = new_fluid_synth(this->settings);
    if (!this->synth)
    {
        goto fail;
    }

#ifndef __ANDROID__
    this->audioDriver =
        new_fluid_audio_driver(
            this->settings,
            this->synth);

    if (!this->audioDriver)
    {
        goto fail;
    }
#endif

    this->soundFontId =
        fluid_synth_sfload(
            this->synth,
            soundFontPath,
            1);

    if (this->soundFontId == FLUID_FAILED)
    {
        goto fail;
    }

    this->initialized = true;

    utils::DebugPrint2(
        "Loaded SoundFont: %s",
        soundFontPath);

    return true;

fail:
    this->Close();
    return false;
}

ZunResult MidiDevice::Close()
{
    this->Reset();

    if (this->audioDriver)
    {
        delete_fluid_audio_driver(
            this->audioDriver);
        this->audioDriver = nullptr;
    }

    if (this->synth)
    {
        delete_fluid_synth(
            this->synth);
        this->synth = nullptr;
    }

    if (this->settings)
    {
        delete_fluid_settings(
            this->settings);
        this->settings = nullptr;
    }

    this->soundFontId = FLUID_FAILED;
    this->initialized = false;

    return ZUN_SUCCESS;
}

bool MidiDevice::SendShortMsg(
    u8 midiStatus,
    u8 firstByte,
    u8 secondByte)
{
    if (!this->synth)
    {
        return false;
    }

    int channel = midiStatus & 0x0F;

    switch (midiStatus & 0xF0)
    {
        case 0x80:
            fluid_synth_noteoff(
                this->synth,
                channel,
                firstByte);
            return true;

        case 0x90:
            if (secondByte == 0)
            {
                fluid_synth_noteoff(
                    this->synth,
                    channel,
                    firstByte);
            }
            else
            {
                fluid_synth_noteon(
                    this->synth,
                    channel,
                    firstByte,
                    secondByte);
            }
            return true;

        case 0xA0:
            return true; // ignored

        case 0xB0:
            fluid_synth_cc(
                this->synth,
                channel,
                firstByte,
                secondByte);
            return true;

        case 0xC0:
            fluid_synth_program_change(
                this->synth,
                channel,
                firstByte);
            return true;

        case 0xD0:
            fluid_synth_channel_pressure(
                this->synth,
                channel,
                firstByte);
            return true;

        case 0xE0:
        {
            int bend =
                ((secondByte << 7) | firstByte) - 8192;

            fluid_synth_pitch_bend(
                this->synth,
                channel,
                bend);

            return true;
        }

        default:
            return false;
    }
}

bool MidiDevice::SendLongMsg(
    const u8* buf,
    u32 len)
{
    if (!this->synth || !buf || len == 0)
    {
        return false;
    }

    if (buf[0] != 0xF0)
    {
        return false;
    }

    int handled = 0;

    fluid_synth_sysex(
        this->synth,
        reinterpret_cast<const char*>(buf),
        len,
        nullptr,
        nullptr,
        nullptr,
        &handled,
        false);

    return handled != 0;
}

void MidiDevice::Reset()
{
    if (!this->synth)
    {
        return;
    }

    for (int channel = 0;
         channel < MIDI_CHANNELS;
         channel++)
    {
        fluid_synth_cc(
            this->synth,
            channel,
            MIDI_CTL_ALL_SOUNDS_OFF,
            0);

        fluid_synth_cc(
            this->synth,
            channel,
            MIDI_CTL_ALL_NOTES_OFF,
            0);
    }
}