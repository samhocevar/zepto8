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

vm::channel::channel()
  : m_sfx(-1),
    m_offset(0)
{
}

void vm::getaudio(int chan, void *buffer, int bytes)
{
    UNUSED(chan);
    memset(buffer, 0, bytes);
}

int vm::sfx_speed(int sfx) const
{
    ASSERT(sfx >= 0 && sfx < 64);
    return *get_mem(OFFSET_SFX + 68 * sfx + 65);
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
        }
    }

    return 0;
}

} // namespace z8

