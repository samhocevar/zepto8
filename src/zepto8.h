//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2018 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#pragma once

#include <lol/engine.h>
#include <cstddef>

#define DEBUG_EXPORT_WAV 0

// The ZEPTO-8 types
// —————————————————
// Various types and enums that describe ZEPTO-8.

namespace z8
{

enum
{
    EXPORT_VERSION = 15,
};

enum
{
    // FIXME: this should be a runtime setting
    DISPLAY_SCALE = 3,
};

enum
{
    SCREEN_WIDTH = 128 * DISPLAY_SCALE,
    SCREEN_HEIGHT = 128 * DISPLAY_SCALE,

    WINDOW_WIDTH = 144 * DISPLAY_SCALE,
    WINDOW_HEIGHT = 144 * DISPLAY_SCALE,
};

enum
{
    LABEL_WIDTH = 128,
    LABEL_HEIGHT = 128,
    LABEL_X = 16,
    LABEL_Y = 24,
};

struct palette
{
    static lol::u8vec4 get(int n)
    {
        static lol::u8vec4 const pal[] =
        {
            lol::u8vec4(0x00, 0x00, 0x00, 0xff), // black
            lol::u8vec4(0x1d, 0x2b, 0x53, 0xff), // dark_blue
            lol::u8vec4(0x7e, 0x25, 0x53, 0xff), // dark_purple
            lol::u8vec4(0x00, 0x87, 0x51, 0xff), // dark_green
            lol::u8vec4(0xab, 0x52, 0x36, 0xff), // brown
            lol::u8vec4(0x5f, 0x57, 0x4f, 0xff), // dark_gray
            lol::u8vec4(0xc2, 0xc3, 0xc7, 0xff), // light_gray
            lol::u8vec4(0xff, 0xf1, 0xe8, 0xff), // white
            lol::u8vec4(0xff, 0x00, 0x4d, 0xff), // red
            lol::u8vec4(0xff, 0xa3, 0x00, 0xff), // orange
            lol::u8vec4(0xff, 0xec, 0x27, 0xff), // yellow
            lol::u8vec4(0x00, 0xe4, 0x36, 0xff), // green
            lol::u8vec4(0x29, 0xad, 0xff, 0xff), // blue
            lol::u8vec4(0x83, 0x76, 0x9c, 0xff), // indigo
            lol::u8vec4(0xff, 0x77, 0xa8, 0xff), // pink
            lol::u8vec4(0xff, 0xcc, 0xaa, 0xff), // peach
        };

        return pal[n];
    }

    static int best(lol::u8vec4 c)
    {
        lol::ivec3 icol = lol::ivec3(c.rgb);
        int ret = 0, dist = lol::dot(icol, icol);
        for (int i = 1; i < 16; ++i)
        {
            lol::ivec3 delta = icol - lol::ivec3(get(i).rgb);
            int newdist = lol::dot(delta, delta);
            if (newdist < dist)
            {
                dist = newdist;
                ret = i;
            }
        }
        return ret;
    }
};

}

