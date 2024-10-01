//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016–2024 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <format>    // std::format
#include <lol/math>  // lol::clamp, lol::mix
#include <algorithm> // std::max
#include <cmath>     // std::fabs, std::fmod, std::floor
#include <cassert>   // assert

#include "pico8/vm.h"
#include "synth.h"

namespace z8::pico8
{

#define DEBUG_EXPORT_WAV 0

enum
{
    FX_NO_EFFECT = 0,
    FX_SLIDE =     1,
    FX_VIBRATO =   2,
    FX_DROP =      3,
    FX_FADE_IN =   4,
    FX_FADE_OUT =  5,
    FX_ARP_FAST =  6,
    FX_ARP_SLOW =  7,
};

static float key_to_freq(float key)
{
    using std::exp2;
    return 440.f * exp2((key - 33.f) / 12.f);
}

#if DEBUG_STUFF
static std::string key_to_name(float key)
{
    static char const *lut = "C-\0C#\0D-\0D#\0E-\0F-\0F#\0G-\0G#\0A-\0A#\0B-";

    char const *note = lut[(int)key % 12 * 3];
    int const octave = (int)key / 12;
    return std::format("{}{}", note, octave);
}
#endif

uint8_t song_t::sfx(int n) const
{
    assert(n >= 0 && n <= 3);
    return data[n] & 0x7f;
}

#if DEBUG_EXPORT_WAV
std::map<int, FILE *> exports;
#endif

std::function<void(void *, int)> vm::get_streamer(int ch)
{
    using namespace std::placeholders;
    // Return a function that calls getaudio() with channel as first arg
    return std::bind(&vm::getaudio, this, ch, _1, _2);
}

// FIXME: there is a problem with the per-channel approach; if a channel
// advances the music, then all the other channels will reference the
// new music chunk. Be careful when implementing music.
void vm::getaudio(int chan, void *in_buffer, int in_bytes)
{
    using std::fabs, std::fmod, std::floor, std::max;

    int const samples_per_second = 22050;
    int const bytes_per_sample = 2; // mono S16 for now

    int16_t *buffer = (int16_t *)in_buffer;
    int const samples = in_bytes / bytes_per_sample;

    for (int i = 0; i < samples; ++i)
    {
        // Advance music using the master channel
        if (chan == m_state.music.master && m_state.music.pattern != -1)
        {
            float const offset_per_second = 22050.f / (183.f * m_state.music.speed);
            float const offset_per_sample = offset_per_second / samples_per_second;
            m_state.music.offset += offset_per_sample;
            m_state.music.volume += m_state.music.volume_step / samples_per_second;
            m_state.music.volume = lol::clamp(m_state.music.volume, 0.f, 1.f);

            if (m_state.music.volume_step < 0 && m_state.music.volume <= 0)
            {
                // Fade out is finished, stop playing the current song
                for (int n = 0; n < 4; ++n)
                    if (m_state.channels[n].is_music)
                        m_state.channels[n].sfx = -1;
                m_state.music.pattern = -1;
            }
            else if (m_state.music.offset >= 32.f)
            {
                int16_t next_pattern = m_state.music.pattern + 1;
                int16_t next_count = m_state.music.count + 1;
                if (m_ram.song[m_state.music.pattern].stop)
                {
                    next_pattern = -1;
                    next_count = m_state.music.count;
                }
                else if (m_ram.song[m_state.music.pattern].loop)
                    while (--next_pattern > 0 && !m_ram.song[next_pattern].start)
                        ;

                m_state.music.count = next_count;
                set_music_pattern(next_pattern);
            }
        }

        if (m_state.channels[chan].sfx == -1)
        {
            buffer[i] = 0;
            continue;
        }

        int const index = m_state.channels[chan].sfx;
        assert(index >= 0 && index < 64);
        sfx_t const &sfx = m_ram.sfx[index];

        // Speed must be 1—255 otherwise the SFX is invalid
        int const speed = max(1, (int)sfx.speed);

        float const offset = m_state.channels[chan].offset;
        float const phi = m_state.channels[chan].phi;

        // PICO-8 exports instruments as 22050 Hz WAV files with 183 samples
        // per speed unit per note, so this is how much we should advance
        float const offset_per_second = 22050.f / (183.f * speed);
        float const offset_per_sample = offset_per_second / samples_per_second;
        float next_offset = offset + offset_per_sample;

        // Handle SFX loops. From the documentation: “Looping is turned
        // off when the start index >= end index”.
        float const loop_range = float(sfx.loop_end - sfx.loop_start);
        if (loop_range > 0.f && next_offset >= sfx.loop_end
             && m_state.channels[chan].can_loop)
        {
            next_offset = fmod(next_offset - sfx.loop_start, loop_range)
                        + sfx.loop_start;
        }

        int const note_id = (int)floor(offset);
        int const next_note_id = (int)floor(next_offset);

        uint8_t key = sfx.notes[note_id].key;
        float volume = sfx.notes[note_id].volume / 7.f;
        float freq = key_to_freq(key);

        if (volume == 0.f)
        {
            // Play silence
            buffer[i] = 0;
        }
        else
        {
            int const fx = sfx.notes[note_id].effect;

            // Apply effect, if any
            switch (fx)
            {
                case FX_NO_EFFECT:
                    break;
                case FX_SLIDE:
                {
                    float t = fmod(offset, 1.f);
                    // From the documentation: “Slide to the next note and volume”,
                    // but it’s actually _from_ the _prev_ note and volume.
                    freq = lol::mix(key_to_freq(m_state.channels[chan].prev_key), freq, t);
                    if (m_state.channels[chan].prev_vol > 0.f)
                        volume = lol::mix(m_state.channels[chan].prev_vol, volume, t);
                    break;
                }
                case FX_VIBRATO:
                {
                    // 7.5f and 0.25f were found empirically by matching
                    // frequency graphs of PICO-8 instruments.
                    float t = fabs(fmod(7.5f * offset / offset_per_second, 1.0f) - 0.5f) - 0.25f;
                    // Vibrato half a semi-tone, so multiply by pow(2,1/12)
                    freq = lol::mix(freq, freq * 1.059463094359f, t);
                    break;
                }
                case FX_DROP:
                    freq *= 1.f - fmod(offset, 1.f);
                    break;
                case FX_FADE_IN:
                    volume *= fmod(offset, 1.f);
                    break;
                case FX_FADE_OUT:
                    volume *= 1.f - fmod(offset, 1.f);
                    break;
                case FX_ARP_FAST:
                case FX_ARP_SLOW:
                {
                    // From the documentation:
                    // “6 arpeggio fast  //  Iterate over groups of 4 notes at speed of 4
                    //  7 arpeggio slow  //  Iterate over groups of 4 notes at speed of 8”
                    // “If the SFX speed is <= 8, arpeggio speeds are halved to 2, 4”
                    int const m = (speed <= 8 ? 32 : 16) / (fx == FX_ARP_FAST ? 4 : 8);
                    int const n = (int)(m * 7.5f * offset / offset_per_second);
                    int const arp_note = (note_id & ~3) | (n & 3);
                    freq = key_to_freq(sfx.notes[arp_note].key);
                    break;
                }
            }

            // Play note
            float waveform = synth::waveform(sfx.notes[note_id].instrument, phi);

            // Apply master music volume from fade in/out
            // FIXME: check whether this should be done after distortion
            if (m_state.channels[chan].is_music)
                volume *= m_state.music.volume;

            int16_t sample = (int16_t)(32767.99f * volume * waveform);

            // Apply hardware effects
            if (m_ram.hw_state.distort & (1 << chan))
            {
                sample = sample / 0x1000 * 0x1249;
            }

            buffer[i] = sample;

            m_state.channels[chan].phi = phi + freq / samples_per_second;
        }

        m_state.channels[chan].offset = next_offset;

        if (next_offset >= 32.f)
        {
            m_state.channels[chan].sfx = -1;
        }
        else if (next_note_id != note_id)
        {
            m_state.channels[chan].prev_key = sfx.notes[note_id].key;
            m_state.channels[chan].prev_vol = sfx.notes[note_id].volume / 7.f;
        }
    }

#if DEBUG_EXPORT_WAV
    if (!exports[chan])
    {
        static char const *header =
            "RIFF" "\xe4\xc1\x08\0" /* chunk size */ "WAVEfmt "
            "\x10\0\0\0" /* subchunk size */ "\x01\0" /* format (PCM) */
            "\x01\0" /* channels (1) */ "\x22\x56\0\0" /* sample rate (22050) */
            "\x22\x56\0\0" /* byte rate */ "\x02\0" /* block align (2) */
            "\x10\0" /* bits per sample (16) */ "data"
            "\xc0\xc1\x08\00" /* bytes in data */;
        exports[chan] = fopen(std::format("/tmp/zepto8_{}.wav", chan).c_str(), "w+");
        fwrite(header, 44, 1, exports[chan]);
    }
    fwrite(buffer, samples, 1, exports[chan]);
#endif
}

//
// Sound
//

void vm::api_music(int16_t pattern, int16_t fade_len, int16_t mask)
{
    // pattern: 0..63, -1 to stop music.
    // fade_len: fade length in milliseconds (default 0)
    // mask: reserved channels

    if (pattern < -1 || pattern > 63)
        return;

    if (pattern == -1)
    {
        // Music will stop when fade out is finished
        m_state.music.volume_step = fade_len <= 0 ? -FLT_MAX
                                  : -m_state.music.volume * (1000.f / fade_len);
        return;
    }

    // Initialise music state for the whole song
    m_state.music.count = 0;
    m_state.music.mask = mask ? mask & 0xf : 0xf;

    m_state.music.volume = 1.f;
    m_state.music.volume_step = 0.f;
    if (fade_len > 0)
    {
        m_state.music.volume = 0.f;
        m_state.music.volume_step = 1000.f / fade_len;
    }

    set_music_pattern(pattern);
}

void vm::set_music_pattern(int pattern)
{
    using std::max;

    // Initialise music state for the current pattern
    m_state.music.pattern = pattern;
    m_state.music.offset = 0;

    // Find music speed; it’s the speed of the fastest sfx
    m_state.music.master = m_state.music.speed = -1;
    for (int i = 0; i < 4; ++i)
    {
        int n = m_ram.song[pattern].sfx(i);
        if (n & 0x40)
            continue;

        auto &sfx = m_ram.sfx[n & 0x3f];
        if (m_state.music.master == -1 || m_state.music.speed > sfx.speed)
        {
            m_state.music.master = i;
            m_state.music.speed = max(1, (int)sfx.speed);
        }
    }

    // Play music sfx on active channels
    for (int i = 0; i < 4; ++i)
    {
        if (((1 << i) & m_state.music.mask) == 0)
            continue;

        int n = m_ram.song[pattern].sfx(i);
        if (n & 0x40)
            continue;

        m_state.channels[i].sfx = n;
        m_state.channels[i].offset = 0.f;
        m_state.channels[i].phi = 0.f;
        m_state.channels[i].can_loop = false;
        m_state.channels[i].is_music = true;
        m_state.channels[i].prev_key = 24;
        m_state.channels[i].prev_vol = 0.f;
    }
}

void vm::api_sfx(int16_t sfx, opt<int16_t> in_chan, int16_t offset, int16_t length)
{
    // SFX index: valid values are 0..63 for actual samples,
    // -1 to stop sound on a channel, -2 to stop looping on a channel
    // Audio channel: valid values are 0..3 or -1 (autoselect)
    // Sound offset: valid values are 0..31, negative values act as 0,
    // and fractional values are ignored

    int chan = in_chan ? *in_chan : -1;

    if (sfx < -2 || sfx > 63 || chan < -1 || chan > 4 || offset > 31)
        return;

    if (sfx == -1)
    {
        // Stop playing the current channel
        if (chan != -1)
            m_state.channels[chan].sfx = -1;
    }
    else if (sfx == -2)
    {
        // Stop looping the current channel
        if (chan != -1)
            m_state.channels[chan].can_loop = false;
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
                if (m_state.channels[i].sfx == -1 ||
                    m_state.channels[i].sfx == sfx)
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
                    m_state.channels[i].sfx < m_state.channels[chan].sfx)
                   chan = i;
        }

        // Stop any channel playing the same sfx
        for (int i = 0; i < 4; ++i)
            if (m_state.channels[i].sfx == sfx)
                m_state.channels[i].sfx = -1;

        // Play this sound!
        m_state.channels[chan].sfx = sfx;
        m_state.channels[chan].offset = std::max(0.f, (float)offset);
        m_state.channels[chan].phi = 0.f;
        m_state.channels[chan].can_loop = true;
        m_state.channels[chan].is_music = false;
        // Playing an instrument starting with the note C-2 and the
        // slide effect causes no noticeable pitch variation in PICO-8,
        // so I assume this is the default value for “previous key”.
        m_state.channels[chan].prev_key = 24;
        // There is no default value for “previous volume”.
        m_state.channels[chan].prev_vol = 0.f;
    }
}

} // namespace z8::pico8

