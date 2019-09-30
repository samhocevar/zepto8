//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2019 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#pragma once

#include <lol/engine.h>

#include <map>
#include <string_view>
#include <cuchar>

// The PICO-8 definitions
// ——————————————————————

namespace z8::pico8
{

struct charset
{
    // Map 8-bit PICO-8 characters to UTF-32 codepoints
    static std::u32string_view pico8_to_utf32[256];

    // Map 8-bit PICO-8 characters to UTF-8 string views
    static std::string_view pico8_to_utf8[256];

    static std::string encode(std::string const &str);

private:
    static int static_init();
    static int unused;
};

struct palette
{
    enum
    {
        black = 0,
        dark_blue,
        dark_purple,
        dark_green,
        brown,
        dark_gray,
        light_gray,
        white,
        red,
        orange,
        yellow,
        green,
        blue,
        indigo,
        pink,
        peach,
    };

    /* Get the nth palette element as a vector of floats in 0…1 */
    static lol::vec4 get(int n)
    {
        return lol::vec4(get8(n)) / 255.f;
    }

    /* Get the nth palette element as a vector of uint8_ts in 0…255 */
    static lol::u8vec4 get8(int n)
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

            lol::u8vec4(0x29, 0x18, 0x14, 0xff),
            lol::u8vec4(0x11, 0x1d, 0x35, 0xff),
            lol::u8vec4(0x42, 0x21, 0x36, 0xff),
            lol::u8vec4(0x12, 0x53, 0x59, 0xff),
            lol::u8vec4(0x74, 0x2f, 0x29, 0xff),
            lol::u8vec4(0x49, 0x33, 0x3b, 0xff),
            lol::u8vec4(0xa2, 0x88, 0x79, 0xff),
            lol::u8vec4(0xf3, 0xef, 0x7d, 0xff),
            lol::u8vec4(0xbe, 0x12, 0x50, 0xff),
            lol::u8vec4(0xff, 0x6c, 0x24, 0xff),
            lol::u8vec4(0xa8, 0xe7, 0x2e, 0xff),
            lol::u8vec4(0x00, 0xb5, 0x43, 0xff),
            lol::u8vec4(0x06, 0x5a, 0xb5, 0xff),
            lol::u8vec4(0x75, 0x46, 0x65, 0xff),
            lol::u8vec4(0xff, 0x6e, 0x59, 0xff),
            lol::u8vec4(0xff, 0x9d, 0x81, 0xff),
        };

        // Bit 0x80 activates the second half of the palette
        return pal[(n & 0xf) | ((n & 0x80) >> 3)];
    }

    /* Find the closest palette element to c (a vector of floats in 0…1) */
    static int best(lol::vec4 c)
    {
        int ret = 0;
        float dist = FLT_MAX;
        for (int i = 0; i < 16; ++i)
        {
            lol::vec3 delta = c.rgb - get(i).rgb;
            float newdist = lol::sqlength(delta);
            if (newdist < dist)
            {
                dist = newdist;
                ret = i;
            }
        }
        return ret;
    }

    /* Find the closest palette element to c (a vector of uint8_ts in 0…255) */
    static int best(lol::u8vec4 c)
    {
        return best(lol::vec4(c) / 255.f);
    }
};

} // namespace z8::pico8

