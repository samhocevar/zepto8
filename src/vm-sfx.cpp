//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#include <lol/engine.h>

#include "vm.h"

namespace z8
{

using lol::msg;

lol::perlin_noise<1> noise;

struct sfx
{
    // Use uint8_t[2] instead of uint16_t so that 1-byte aligned storage
    // is still possible.
    uint8_t notes[32][2];
    // 0: editor mode
    // 1: speed (1-255)
    // 2: loop start
    // 3: loop end
    uint8_t flags[4];

    float frequency(int n) const
    {
        static float const lut[] =
        {
            0.0f,
            69.295658f, 73.416192f, 77.781746f, 82.406889f, 87.307058f,
            92.498606f, 97.998859f, 103.82617f, 110.00000f, 116.54094f,
            123.47082f, 130.81278f, 138.59131f, 146.83238f, 155.56349f,
            164.81377f, 174.61411f, 184.99721f, 195.99771f, 207.65234f,
            220.00000f, 233.08188f, 246.94165f, 261.62556f, 277.18263f,
            293.66476f, 311.12698f, 329.62755f, 349.22823f, 369.99442f,
            391.99543f, 415.30469f, 440.00000f, 466.16376f, 493.88330f,
            523.25113f, 554.36526f, 587.32953f, 622.25396f, 659.25511f,
            698.45646f, 739.98884f, 783.99087f, 830.60939f, 880.00000f,
            932.32752f, 987.76660f, 1046.5022f, 1108.7305f, 1174.6590f,
            1244.5079f, 1318.5102f, 1396.9129f, 1479.9776f, 1567.9817f,
            1661.2187f, 1760.0000f, 1864.6550f, 1975.5332f, 2093.0045f,
            2217.4610f, 2349.3181f, 2489.0158f,
        };

        ASSERT(n >= 0 && n <= 31);
        return lut[notes[n][0] & 0x3f];
    }

    float volume(int n) const
    {
        ASSERT(n >= 0 && n <= 31);
        return ((notes[n][1] >> 1) & 0x7) / 7.f;
    }

    int effect(int n) const
    {
        // FIXME: there is an actual extra bit for the effect but I don’t
        // know what it’s for: PICO-8 documentation says 0…7, not 0…15
        ASSERT(n >= 0 && n <= 31);
        return (notes[n][1] >> 4) & 0x7;
    }

    int instrument(int n) const
    {
        ASSERT(n >= 0 && n <= 31);
        return ((notes[n][1] << 2) & 0x4) | (notes[n][0] >> 6);
    }

    int speed() const { return flags[1]; }
    int loop_start() const { return flags[2]; }
    int loop_end() const { return flags[3]; }
};

static_assert(sizeof(sfx) == 68, "z8::sfx has incorrect size");


vm::channel::channel()
  : m_sfx(-1),
    m_offset(0),
    m_phi(0)
{
#if DEBUG_EXPORT_WAV
    char const *header =
        "\x52\x49\x46\x46\xe4\xc1\x08\x00\x57\x41\x56\x45\x66\x6d\x74\x20"
        "\x10\x00\x00\x00\x01\x00\x02\x00\x22\x56\x00\x00\x44\xac\x00\x00"
        "\x02\x00\x10\x00\x64\x61\x74\x61\xc0\xc1\x08\x00";
    m_fd = fopen(lol::String::format("/tmp/zepto8_%p.wav", this).C(), "w+");
    fwrite(header, 44, 1, m_fd);
#endif
}

void vm::getaudio(int chan, void *in_buffer, int in_bytes)
{
    int const samples_per_second = 22050;
    int const bytes_per_sample = 4; // stereo S16 for now

    int16_t *buffer = (int16_t *)in_buffer;
    int samples = in_bytes / bytes_per_sample;

    for (int i = 0; i < samples; ++i)
    {
        if (m_channels[chan].m_sfx == -1)
        {
            buffer[2 * i] = buffer[2 * i + 1] = 0;
            m_channels[chan].m_sfx = -1;
            continue;
        }

        // Speed must be 1—255 otherwise the SFX is invalid
        struct sfx const &sfx = get_sfx(m_channels[chan].m_sfx);
        int speed = lol::max(1, sfx.speed());

        float offset = m_channels[chan].m_offset;
        float phi = m_channels[chan].m_phi;

        // PICO-8 exports instruments as 22050 Hz WAV files with 183 samples
        // per speed unit per note, so this is how much we should advance
        float offset_per_second = 22050.f / (183.f * speed);
        float next_offset = offset + offset_per_second / samples_per_second;

        int note = (int)lol::floor(offset);
        int next_note = (int)lol::floor(next_offset);

        float freq = sfx.frequency(note);
        float volume = sfx.volume(note);

        if (freq == 0.f || volume == 0.f)
        {
            // Play silence
            buffer[2 * i] = buffer[2 * i + 1] = 0;
        }
        else
        {
            float t = lol::fmod(freq * offset / offset_per_second + phi, 1.f);
            float waveform = 0.f;

            // Multipliers were measured from WAV exports. Waveforms are
            // inferred from those exports by guessing what the original
            // equations could be.
            switch (sfx.instrument(note))
            {
            case 0:
                // Triangle signal
                waveform = 0.354f * (lol::abs(4.f * t - 2.0f) - 1.0f);
                break;
            case 1:
                // Slanted triangle
                {
                    static float const a = 0.9f;
                    waveform = t < a ? 2.f * t / a - 1.f
                             : 2.f * (1.f - t) / (1.f - a) - 1.f;
                    waveform *= 0.406f;
                }
                break;
            case 2:
                // Sawtooth
                waveform = 0.653f * (t < 0.5f ? t : t - 1.f);
                break;
            case 3:
                // Square signal
                waveform = t < 0.5f ? 0.25f : -0.25f;
                break;
            case 4:
                // Asymmetric square signal
                waveform = t < 0.33333333f ? 0.25f : -0.25f;
                break;
            case 5:
                // Some triangle stuff again
                waveform = t < 0.5f ? 3.f - lol::abs(24.f * t - 6.f)
                                    : 1.f - lol::abs(16.f * t - 12.f);
                waveform *= 0.111111111f;
                break;
            case 6:
                // Spectral analysis indicates this is some kind of
                // brown noise, but losing almost 10dB per octave.
                // This may help us create a correct filter:
                // http://www.firstpr.com.au/dsp/pink-noise/
                for (float mul = 1; mul <= 16; mul *= 2)
                    waveform += noise.eval(lol::vec_t<float, 1>(t * 32.f * mul)) / mul;
                waveform *= 0.8f;
                break;
            case 7:
                // This one has a subfrequency of freq/128 that appears
                // to modulate two signals using a triangle wave
                // FIXME: amplitude seems to be affected, too
                {
                    float k = lol::abs(2.f * lol::fmod(freq / 128.f * offset / offset_per_second, 1.f) - 1.f);
                    float u = lol::fmod(t + 0.5f * k, 1.0f);
                    waveform = lol::abs(4.f * u - 2.f) - lol::abs(8.f * t - 4.f);
                    waveform *= 0.166666666f;
                }
                break;
            }

            buffer[2 * i] = buffer[2 * i + 1]
                  = (int16_t)(32767.99f * volume * waveform);
        }

        // TODO: handle loops etc.
        m_channels[chan].m_offset = next_offset;

        if (next_offset >= 32.f)
        {
            m_channels[chan].m_sfx = -1;
        }
        else if (note != next_note)
        {
            float next_freq = sfx.frequency(next_note);
            //float next_volume = sfx.volume(next_note);

            phi += (freq * offset - next_freq * next_offset) / offset_per_second;
            phi = lol::fmod(phi, 1.f) + (phi >= 0.f ? 0.f : 1.f);
            m_channels[chan].m_phi = phi;
        }
    }

#if DEBUG_EXPORT_WAV
    fwrite(in_buffer, in_bytes, 1, m_channels[chan].m_fd);
#endif
}

struct sfx const &vm::get_sfx(int n) const
{
    ASSERT(n >= 0 && n < 64);
    return *reinterpret_cast<struct sfx const *>(get_mem(OFFSET_SFX + 68 * n));
}

//
// Sound
//

int vm::api::music(lua_State *l)
{
    UNUSED(l);
    msg::info("z8:stub:music\n");
    return 0;
}

int vm::api::sfx(lua_State *l)
{
    // SFX index: valid values are 0..63 for actual samples,
    // -1 to stop sound on a channel, -2 to stop looping on a channel
    int sfx = (int)lua_toclamp64(l, 1);
    // Audio channel: valid values are 0..3 or -1 (autoselect)
    int chan = lua_isnone(l, 2) ? -1 : (int)lua_toclamp64(l, 2);
    // Sound offset: valid values are 0..31, negative values act as 0,
    // and fractional values are ignored
    int offset = lol::max(0, (int)lua_toclamp64(l, 3));

    if (sfx < -2 || sfx > 63 || chan < -1 || chan > 4 || offset > 31)
        return 0;

    vm *that = get_this(l);
    if (sfx == -1)
    {
        // TODO: stop channel
    }
    else if (sfx == -2)
    {
        // TODO: stop looping
    }
    else
    {
        // Find the first available channel: either a channel that plays
        // nothing, or a channel that is already playing this sample (in
        // this case PICO-8 decides to forcibly reuse that channel, which
        // is reasonable)
        if (chan == -1)
        {
            for (int i = 0; i < 4; ++i)
                if (that->m_channels[i].m_sfx == -1 ||
                    that->m_channels[i].m_sfx == sfx)
                {
                    chan = i;
                    break;
                }
        }

        // If still no channel found, the PICO-8 strategy seems to be to
        // stop the sample with the lowest ID currently playing
        if (chan == -1)
        {
            for (int i = 0; i < 4; ++i)
               if (chan == -1 ||
                    that->m_channels[i].m_sfx < that->m_channels[chan].m_sfx)
                   chan = i;
        }

        // Play this sound!
        if (chan != -1)
        {
            // Stop any channel playing the same sfx
            for (int i = 0; i < 4; ++i)
                if (that->m_channels[i].m_sfx == sfx)
                    that->m_channels[i].m_sfx = -1;

            that->m_channels[chan].m_sfx = sfx;
            that->m_channels[chan].m_offset = offset;
            that->m_channels[chan].m_phi = 0.f;
        }
    }

    return 0;
}

} // namespace z8

