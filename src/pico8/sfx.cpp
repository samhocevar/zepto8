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

void vm::update_sfx_state(state::sfx_state& cur_sfx, state::synth_param &new_synth, float freq_factor, float length, bool is_music, bool can_loop, bool half_rate, double inv_frames_per_second)
{
    using std::fabs, std::fmod, std::floor, std::max;

    if (cur_sfx.sfx == -1) return;

    int const index = cur_sfx.sfx;
    assert(index >= 0 && index < 64);
    sfx_t const& sfx = m_ram.sfx[index];

    // Speed must be 1—255 otherwise the SFX is invalid
    int const speed = max(1, (int)sfx.speed);

    double const offset = cur_sfx.offset;
    double const time = cur_sfx.time;

    // PICO-8 exports instruments as 22050 Hz WAV files with 183 samples
    // per speed unit per note, so this is how much we should advance
    double const offset_per_second = 22050.0 / (183.0 * speed);
    double const offset_per_frame = offset_per_second * inv_frames_per_second;
    double next_offset = offset + offset_per_frame;
    double next_time = time + offset_per_frame;

    // Handle SFX loops. From the documentation: “Looping is turned
    // off when the start index >= end index”.
    float const loop_range = float(sfx.loop_end - sfx.loop_start);
    if (loop_range > 0.f && next_offset >= sfx.loop_end
        && can_loop)
    {
        next_offset = fmod(next_offset - sfx.loop_start, loop_range)
            + sfx.loop_start;
    }

    bool has_end = false;
    float end_time = 32.f;
    if (length > 0.0f)
    {
        has_end = true;
        end_time = length;
    }
    // in pico 8, strangely, len is not applyed to musical sfx except for pattern len calculation
    // it's probably a bug
    if (!is_music && sfx.loop_end == 0 && sfx.loop_start > 0)
    {
        has_end = true;
        end_time = std::min<float>(end_time, sfx.loop_start);
    }
    // if there is no loop, we end after the length
    if (loop_range <= 0.f)
    {
        has_end = true;
        // if not a music sfx, check where is the last note to early stop
        if (!is_music)
        {
            int last_note = 0;
            // todo: maybe cache this info so we don't need to compute it for each sample (but sfx could change from frame to frame so we don't know when to recompute it)
            for (int n = 0; n < 32; ++n)
            {
                if (sfx.notes[n].volume > 0)
                {
                    last_note = std::min(32, n + 1);
                }
            }
            end_time = std::min(end_time, float(last_note));
        }
    }

    if (offset < 32)
    {
        int const note_id = (int)floor(offset);
        int const next_note_id = (int)floor(next_offset);

        uint8_t key = sfx.notes[note_id].key;
        float volume = sfx.notes[note_id].volume / 7.f;
        float freq = key_to_freq(key) * freq_factor;

        if (volume > 0.f)
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
                freq = lol::mix(key_to_freq(cur_sfx.prev_key), freq, t);
                if (cur_sfx.prev_vol > 0.f)
                    volume = lol::mix(cur_sfx.prev_vol, volume, t);
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

            if (half_rate)  freq *= 0.5f;

            new_synth.key = key;
            new_synth.freq = freq;
            new_synth.instrument = sfx.notes[note_id].instrument;
            new_synth.custom = sfx.notes[note_id].custom;
            new_synth.filters = sfx.filters;
            new_synth.volume = volume;
            new_synth.is_music = is_music;

            new_synth.phi = new_synth.phi + freq * inv_frames_per_second;
        }

        if (next_note_id != note_id)
        {
            cur_sfx.prev_key = sfx.notes[note_id].key;
            cur_sfx.prev_vol = sfx.notes[note_id].volume / 7.f;
        }
    }

    cur_sfx.offset = next_offset;
    cur_sfx.time = next_time;

    if (has_end && next_time >= end_time)
    {
        cur_sfx.sfx = -1;
    }
}

void vm::get_audio(void *inbuffer, size_t in_bytes)
{
    int16_t* buffer = (int16_t*)inbuffer;
    int const in_frames = in_bytes / 2;

    using std::fabs, std::fmod, std::floor, std::max;

    bool is_pause = m_ram.draw_state.pause_music==1 || (m_in_pause && m_ram.draw_state.pause_music != 2);

    for (size_t i = 0; i < in_frames; ++i)
    {
    float channel_mix = 0.0;
    for (int chan = 0; chan < 4; ++chan)
    {
        double inv_frames_per_second = ((m_ram.hw_state.half_rate & (1 << chan)) ? 0.5 : 1.0) / 22050.0;

        // Advance music using the first channel
        //
        // FIXME: not actually great, as channels after 0 will directly jump to next pattern
        // instead of having half of the previous one
        // also fading will not work correctly for channel > 0
        // maybe we should only update current channel's sfx
        // also doing this on a separate thread from rendering means lua can sometimes show sfx = -1 for a frame
        if (chan == 0 && m_state.music.pattern != -1 && !is_pause)
        {
            double const offset_per_second = 22050.0 / 183.0;
            double const offset_per_frame = offset_per_second * inv_frames_per_second;
            m_state.music.offset += offset_per_frame;
            m_state.music.fade_volume += m_state.music.fade_volume_step * inv_frames_per_second;
            m_state.music.fade_volume = lol::clamp(m_state.music.fade_volume, 0.f, 1.f);

            if (m_state.music.fade_volume_step < 0 && m_state.music.fade_volume <= 0)
            {
                set_music_pattern(-1);
            }
            else if (m_state.music.offset >= m_state.music.length)
            {
                int16_t next_pattern = m_state.music.pattern + 1;
                int16_t next_count = m_state.music.count + 1;
                if (m_ram.song[m_state.music.pattern].stop)
                {
                    next_pattern = -1;
                    next_count = -1;
                }
                else if (m_ram.song[m_state.music.pattern].loop)
                    while (--next_pattern > 0 && !m_ram.song[next_pattern].start)
                        ;

                m_state.music.count = next_count;
                set_music_pattern(next_pattern);
            }
        }

        state::channel &channel_state = m_state.channels[chan];

        // a no sfx is playing and there is a music sfx stored
        if (channel_state.main_sfx.sfx == -1 && channel_state.sfx_music != -1 && !is_pause)
        {
            int const index = channel_state.sfx_music;
            assert(index >= 0 && index < 64);
            sfx_t const& sfx = m_ram.sfx[index];

            // compute offset to start the sfx to
            bool want_play = true;
            int const speed = max(1, (int)sfx.speed);
            double new_offset = m_state.music.offset / speed;

            float const loop_range = float(sfx.loop_end - sfx.loop_start);
            if (loop_range > 0.f && channel_state.can_loop)
            {
                if (new_offset > sfx.loop_start)
                    new_offset = fmod(new_offset - sfx.loop_start, loop_range) + sfx.loop_start;
            }
            else
            {
                if (new_offset > 32.f)
                    want_play = false;
            }

            if (want_play)
            {
                launch_sfx(index, chan, new_offset, 0, true);
            }
            channel_state.sfx_music = -1;
        }

        state::synth_param& last_synth = channel_state.last_synth;
        state::synth_param new_synth;
        new_synth.phi = last_synth.phi;
        new_synth.last_advance = last_synth.last_advance;
        new_synth.last_sample = last_synth.last_sample;
        float value = 0.0f;

        if (!is_pause)
        {
            double main_sfx_base_offset = channel_state.main_sfx.offset;
            bool half_rate = m_ram.hw_state.half_rate & (1 << (chan + 4));
            // update main sfx
            update_sfx_state(channel_state.main_sfx, new_synth, 1.0f, channel_state.length, channel_state.is_music, channel_state.can_loop, half_rate, inv_frames_per_second);

            bool restart_custom = new_synth.instrument != channel_state.last_main_instrument || new_synth.key != channel_state.last_main_key;
            channel_state.last_main_instrument = new_synth.instrument;
            channel_state.last_main_key = new_synth.key;

            if (new_synth.volume > 0.0f)
            {
                if (new_synth.custom)
                {
                    // also need to restart if main_sfx loops (new offset is before base offset)
                    if (channel_state.main_sfx.offset < main_sfx_base_offset) restart_custom = true;
                    // also need to restart if custom_sfx.sfx == -1 (it has ended) and main_sfx.offset is changing integer
                    if (channel_state.custom_sfx.sfx == -1 && floor(main_sfx_base_offset) != floor(channel_state.main_sfx.offset)) restart_custom = true;

                    if (restart_custom)
                    {
                        channel_state.custom_sfx.sfx = new_synth.instrument;
                        channel_state.custom_sfx.offset = 0.0f;
                        channel_state.custom_sfx.time = 0.0f;
                    }
                    new_synth.phi = last_synth.phi;
                    float const freq_base = key_to_freq(24); // C2
                    float freq_factor = new_synth.freq / freq_base;
                    float main_sfx_volume = new_synth.volume;
                    update_sfx_state(channel_state.custom_sfx, new_synth, freq_factor, 0.0f, false, true, half_rate, inv_frames_per_second);
                    new_synth.volume *= main_sfx_volume;
                }
                value = get_synth_sample(new_synth);
            }
        }

        // detect harsh changes of states, and do a small fade
        float freq_threshold = std::min(new_synth.freq, last_synth.freq) * 0.01f;
        if (abs(new_synth.volume - last_synth.volume) > 0.1
            || abs(new_synth.freq - last_synth.freq) > freq_threshold
            || new_synth.instrument != last_synth.instrument)
        {
            if (channel_state.fade <= 0.0f) // avoid continious fades, it messes with noise algo
            {
                channel_state.fade_synth = last_synth;
            }
            channel_state.fade = 1.0f;
            // reset phi between notes so we dont get very big values that would loose precision
            // todo: should also reset phi if it's a very very long series of notes with same instrument?
            new_synth.phi = fmod(new_synth.phi, 1.0f);
        }
        last_synth = new_synth;

        uint8_t reverb = (last_synth.filters / 24) % 3;
        uint8_t dampen = (last_synth.filters / 72) % 3;
        float chan_reverb1_value = reverb == 1 ? 1.0f : 0.0f;
        float chan_reverb2_value = reverb == 2 ? 1.0f : 0.0f;
        float chan_damp1_value = dampen == 1 ? 1.0f : 0.0f;
        float chan_damp2_value = dampen == 2 ? 1.0f : 0.0f;

        if (channel_state.fade > 0.0f)
        {
            channel_state.fade_synth.phi = channel_state.fade_synth.phi + channel_state.fade_synth.freq * inv_frames_per_second;
            float value_fade = get_synth_sample(channel_state.fade_synth);
            
            value = lol::mix(value, value_fade, channel_state.fade);

            // TODO: factoryze this
            uint8_t fade_reverb = (channel_state.fade_synth.filters / 24) % 3;
            uint8_t fade_dampen = (channel_state.fade_synth.filters / 72) % 3;
            chan_reverb1_value = lol::mix(chan_reverb1_value, fade_reverb == 1 ? 1.0f : 0.0f, channel_state.fade);
            chan_reverb2_value = lol::mix(chan_reverb2_value, fade_reverb == 2 ? 1.0f : 0.0f, channel_state.fade);
            chan_damp1_value = lol::mix(chan_damp1_value, fade_dampen == 1 ? 1.0f : 0.0f, channel_state.fade);
            chan_damp2_value = lol::mix(chan_damp2_value, fade_dampen == 2 ? 1.0f : 0.0f, channel_state.fade);

            channel_state.fade -= 130.0f * inv_frames_per_second;
        }

        // hw can force fx passes
        if (m_ram.hw_state.reverb & (1 << (chan + 4))) chan_reverb1_value = 1.0f;
        if (m_ram.hw_state.reverb & (1 << chan)) chan_reverb2_value = 1.0f;
        if (m_ram.hw_state.lowpass & (1 << (chan + 4))) chan_damp1_value = 1.0f;
        if (m_ram.hw_state.lowpass & (1 << chan)) chan_damp2_value = 1.0f;
        
        if (chan_reverb1_value > 0.0f) value += chan_reverb1_value * channel_state.reverb_2[channel_state.reverb_index % 366] * 0.5f;
        if (chan_reverb2_value > 0.0f) value += chan_reverb1_value * channel_state.reverb_4[channel_state.reverb_index % 732] * 0.5f;

        channel_state.reverb_2[channel_state.reverb_index % 366] = value;
        channel_state.reverb_4[channel_state.reverb_index % 732] = value;
        ++channel_state.reverb_index;

        float value_damp1 = channel_state.damp1.run(value);
        if (chan_damp1_value > 0.0f) value = lol::mix(value, value_damp1, chan_damp1_value);

        float value_damp2 = channel_state.damp2.run(value);
        if (chan_damp2_value > 0.0f) value = lol::mix(value, value_damp2, chan_damp2_value);

        int16_t sample = (int16_t)(32767.99f * std::clamp(value,-0.99f,0.99f));

        // Apply hardware distort
        if (m_ram.hw_state.distort & (1 << chan))
        {
            sample = sample / 0x1000 * 0x1249;
        }
        else if (m_ram.hw_state.distort & (1 << (chan + 4)))
        {
            sample = (sample - (sample < 0 ? 0x1000: 0)) / 0x1000 * 0x1249;
        }
        channel_mix += sample;
    }
        
        buffer[i] = (int16_t)(std::clamp(channel_mix, -32767.9f, 32767.9f));
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
    // FIXME: this is no longer correct because the 4 tracks are interleaved
    fwrite(buffer, in_samples, 1, exports[chan]);
#endif
}

float vm::get_synth_sample(state::synth_param &params)
{
    // Play note
    float waveform = synth::waveform(params);

    uint8_t detune = (params.filters / 8) % 3;
    if (detune != 0 && params.instrument != synth::INST_NOISE)
    {
        // detune is a second wave slightly offset
        float factor = 1.0f;
        if (params.instrument == synth::INST_TRIANGLE) factor = (detune == 1) ? 3.0f / 4.0f : 3.0f / 2.0f; // triangle detune adds a fourth or a fifth
        else if (params.instrument == synth::INST_ORGAN) factor = (detune == 1) ? 200.0f / 199.0f : 800.0f / 199.0f; // slight offset, detune 2 at 2 octave above
        else if (params.instrument == synth::INST_PHASER) factor = (detune == 1) ? 49.0f / 50.0f : 400.0f / 199.0f;
        else factor = (detune == 1) ? 200.0f / 199.0f : 400.0f / 199.0f; // others are slight offset, detune 2 at 1 octave above

        state::synth_param second_wave = params;
        second_wave.phi *= factor;
        if (detune == 2 && params.instrument == synth::INST_ORGAN) second_wave.instrument = synth::INST_TRIANGLE; // organ second wave seems to be simpler
        waveform += synth::waveform(second_wave) * 0.5f;
    }

    float volume = params.volume;

    // Apply master music volume from fade in/out
    // FIXME: check whether this should be done after distortion
    if (params.is_music)
    {
        volume *= m_state.music.fade_volume * m_state.music.volume_music;
    }
    else
    {
        volume *= m_state.music.volume_sfx;
    }

    return std::clamp(waveform * volume, -1.0f, 1.0f);
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
        m_state.music.fade_volume_step = fade_len <= 0 ? -FLT_MAX
                                  : -m_state.music.fade_volume * (1000.f / fade_len);
        return;
    }

    // Initialise music state for the whole song
    m_state.music.count = 0;
    m_state.music.mask = mask & 0xf;

    m_state.music.fade_volume = 1.f;
    m_state.music.fade_volume_step = 0.f;
    if (fade_len > 0)
    {
        m_state.music.fade_volume = 0.f;
        m_state.music.fade_volume_step = 1000.f / fade_len;
    }

    set_music_pattern(pattern);
}

void vm::set_music_pattern(int pattern)
{
    using std::max, std::min;

    // stop all previously playing music sounds
    for (int n = 0; n < 4; ++n)
        if (m_state.channels[n].is_music)
        {
            m_state.channels[n].main_sfx.sfx = -1;
            m_state.channels[n].sfx_music = -1;
        }

    if (pattern < 0 || pattern > 63)
    {
        m_state.music.pattern = -1;
        m_state.music.count = -1;
        m_state.music.offset = -1;
        m_state.music.mask = 0;
        m_state.music.length = 0.0f;
        return;
    }

    // Find music duration
    // if there is at least one non-looping channel:
    // length of the first non-looping channel
    // if not (all channels are looping):
    // length of slowest channel (so it stops when all channels have reached at least 32 steps)

    int16_t duration_looping = -1;
    int16_t duration_no_loop = -1;
    for (int i = 0; i < 4; ++i)
    {
        int n = m_ram.song[pattern].sfx(i);
        if (n & 0x40)
            continue;

        auto &sfx = m_ram.sfx[n & 0x3f];
        bool has_loop = sfx.loop_end > 0 && sfx.loop_end > sfx.loop_start;
        if (has_loop)
        {
            int16_t sfx_duration = 32 * sfx.speed;
            duration_looping = max(duration_looping, sfx_duration);
        }
        else
        {
            // take duration of first non_looping channel
            int16_t end_time = 32;
            if (sfx.loop_end == 0 && sfx.loop_start > 0)
            {
                end_time = min<int16_t>(end_time, sfx.loop_start);
            }
            duration_no_loop = end_time * sfx.speed;
            break;
        }
    }

    int16_t duration = duration_no_loop > 0 ? duration_no_loop : duration_looping;
    if (duration <= 0)
    {
        // stop music
        m_state.music.pattern = -1;
        m_state.music.offset = -1;
        m_state.music.count = -1;
        m_state.music.mask = 0;
        m_state.music.length = 0.0f;
        return;
    }

    // Initialise music state for the current pattern
    // todo: mutex to avoid broken samples if data is not sync?
    m_state.music.pattern = pattern;
    m_state.music.offset = 0;
    m_state.music.length = duration;

    // Play music sfx on active channels
    for (int i = 0; i < 4; ++i)
    {
        int n = m_ram.song[pattern].sfx(i);
        if (n & 0x40)
            continue;

        if (m_state.channels[i].main_sfx.sfx == -1)
        {
            launch_sfx(n, i, 0, 0, true);
        }
        else
        {
            // if there is already a sfx playing, we store the music one to be played later, when the current sfx stop
            m_state.channels[i].sfx_music = n;
        }
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
        // Stop playing if sfx is a non musical channel
        if (chan != -1)
        {
            if (!m_state.channels[chan].is_music)
                m_state.channels[chan].main_sfx.sfx = -1;
        }
        else
        {
            // stop playing all non musical channels
            for (int i = 0; i < 4; ++i)
            {
                if (!m_state.channels[i].is_music)
                    m_state.channels[i].main_sfx.sfx = -1;
            }
        }
        return;
    }

    if (sfx == -2)
    {
        // Stop looping if sfx is a non musical channel
        if (chan != -1)
        {
            if (!m_state.channels[chan].is_music)
                m_state.channels[chan].can_loop = false;
        }
        else
        {
            // stop looping all non musical channels
            for (int i = 0; i < 4; ++i)
            {
                if (!m_state.channels[i].is_music)
                    m_state.channels[i].can_loop = false;
            }
        }
        return;
    }

    // Find the first available channel: either a channel that plays
    // nothing, or a channel that is already playing this sample (in
    // this case PICO-8 decides to forcibly reuse that channel, which
    // is reasonable)
    if (chan == -1)
    {
        for (int i = 0; i < 4; ++i)
        {
            if (((1 << i) & m_state.music.mask) != 0)
                continue;

            if (m_state.channels[i].main_sfx.sfx == -1 ||
                m_state.channels[i].main_sfx.sfx == sfx)
            {
                chan = i;
                break;
            }
        }
    }

    // TODO: quirk of pico 8, if a music play several times the same sfx, it can be interupted
        
    // if no free channel is found, stop music's first interuptable channel
    if (chan == -1)
    {
        for (int i = 0; i < 4; ++i)
        {
            if (((1 << i) & m_state.music.mask) != 0)
                continue;

            if (m_state.channels[i].is_music)
            {
                chan = i;
                break;
            }
        }
    }

    // If still no channel found, the PICO-8 strategy seems to be to
    // stop the channel with fastest speed (if there are several, take the latest one)
    if (chan == -1)
    {
        uint8_t fastest_speed = 255;
        for (int i = 0; i < 4; ++i)
        {
            if (((1 << i) & m_state.music.mask) != 0)
                continue;

            int const index = m_state.channels[i].main_sfx.sfx;
            if (index < 0 || index >= 64)
                continue;
                
            sfx_t const& sfx = m_ram.sfx[index];
            if (sfx.speed <= fastest_speed)
            {
                chan = i;
                fastest_speed = sfx.speed;
            }
        }
    }

    // still no channel found, the sfx is ignored
    if (chan == -1)
        return;

    // Stop any channel playing the same sfx
    for (int i = 0; i < 4; ++i)
        if (m_state.channels[i].main_sfx.sfx == sfx)
            m_state.channels[i].main_sfx.sfx = -1;

    // if there is already a music playing sfx, store it to be picked back up later before it's replaced
    if (m_state.channels[chan].main_sfx.sfx != -1 && m_state.channels[chan].is_music)
    {
        m_state.channels[chan].sfx_music = m_state.channels[chan].main_sfx.sfx;
    }

    // Play this sound!
    launch_sfx(sfx, chan, offset, length, false);
}

void vm::launch_sfx(int16_t sfx, int16_t chan, float offset, float length, bool is_music)
{
    m_state.channels[chan].main_sfx.sfx = sfx;
    m_state.channels[chan].main_sfx.offset = std::max(0.f, offset);
    m_state.channels[chan].main_sfx.time = 0.f;
    m_state.channels[chan].length = std::max(0.f, length);
    m_state.channels[chan].can_loop = true;
    m_state.channels[chan].is_music = is_music;
    m_state.channels[chan].last_main_instrument = 0xff;
    m_state.channels[chan].last_main_key = 0xff;
    // Playing an instrument starting with the note C-2 and the
    // slide effect causes no noticeable pitch variation in PICO-8,
    // so I assume this is the default value for “previous key”.
    m_state.channels[chan].main_sfx.prev_key = 24;
    // There is no default value for “previous volume”.
    m_state.channels[chan].main_sfx.prev_vol = 0.f;
}


} // namespace z8::pico8