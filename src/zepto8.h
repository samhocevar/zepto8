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

#pragma once

#include <lol/engine.h>

#define DEBUG_EXPORT_WAV 0

namespace z8
{

enum
{
    EXPORT_VERSION = 8,
};

enum
{
    WINDOW_WIDTH = 576,
    WINDOW_HEIGHT = 576,
};

enum
{
    LABEL_WIDTH = 128,
    LABEL_HEIGHT = 128,
    LABEL_X = 16,
    LABEL_Y = 24,
};

enum
{
    OFFSET_GFX        = 0x0000,
    OFFSET_GFX2       = 0x1000,
    OFFSET_MAP2       = 0x1000,
    OFFSET_MAP        = 0x2000,
    OFFSET_GFX_PROPS  = 0x3000,
    OFFSET_SONG       = 0x3100,
    OFFSET_SFX        = 0x3200,
    OFFSET_USER_DATA  = 0x4300,
    OFFSET_CODE       = 0x4300,
    OFFSET_PERSISTENT = 0x5e00,
    OFFSET_DRAW_STATE = 0x5f00,
    OFFSET_HW_STATE   = 0x5f40,
    OFFSET_GPIO_PINS  = 0x5f80,
    OFFSET_SCREEN     = 0x6000,
    OFFSET_END        = 0x8000,

    OFFSET_CLIP       = 0x5f20,
    OFFSET_CURSOR_X   = 0x5f26,
    OFFSET_CURSOR_Y   = 0x5f27,
    OFFSET_CAMERA     = 0x5f28,

    SIZE_GFX        = OFFSET_GFX2       - OFFSET_GFX,
    SIZE_GFX2       = OFFSET_MAP        - OFFSET_GFX2,
    SIZE_MAP2       = SIZE_GFX2,
    SIZE_MAP        = OFFSET_GFX_PROPS  - OFFSET_MAP,
    SIZE_GFX_PROPS  = OFFSET_SONG       - OFFSET_GFX_PROPS,
    SIZE_SONG       = OFFSET_SFX        - OFFSET_SONG,
    SIZE_SFX        = OFFSET_USER_DATA  - OFFSET_SFX,
    SIZE_USER_DATA  = OFFSET_PERSISTENT - OFFSET_USER_DATA,
    SIZE_PERSISTENT = OFFSET_DRAW_STATE - OFFSET_PERSISTENT,
    SIZE_DRAW_STATE = OFFSET_HW_STATE   - OFFSET_DRAW_STATE,
    SIZE_HW_STATE   = OFFSET_GPIO_PINS  - OFFSET_HW_STATE,
    SIZE_GPIO_PINS  = OFFSET_SCREEN     - OFFSET_GPIO_PINS,
    SIZE_SCREEN     = OFFSET_END        - OFFSET_SCREEN,

    SIZE_MEMORY     = OFFSET_END,
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

