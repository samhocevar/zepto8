//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2020 Sam Hocevar <sam@hocevar.net>
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

#include "pico8/vm.h"
#include "pico8/pico8.h"

#include <lol/vector> // lol::u8vec4

namespace z8::pico8
{

void vm::render(lol::u8vec4 *screen) const
{
    // Cannot use a 256-value LUT because data access will be
    // very random due to rotation, flip, stretch etc.
    lol::u8vec4 lut[128 + 16];
    for (int c = 0; c < 16; ++c)
    {
        lut[c] = palette::get8(c);
        lut[128 + c] = palette::get8(16 + c);
    }

    for (int y = 0; y < 128; ++y)
    for (int x = 0; x < 128; ++x)
        *screen++ = lut[m_ram.pixel(x, y)];
}

int vm::get_ansi_color(uint8_t c) const
{
    static int const ansi_palette[] =
    {
         16, // 000000 → 000000
         17, // 1d2b53 → 00005f
         89, // 7e2553 → 87005f
         29, // 008751 → 00875f
        131, // ab5236 → ab5236
        240, // 5f574f → 5f5f5f
        251, // c2c3c7 → c6c6c6
        230, // fff1e8 → ffffdf
        197, // ff004d → ff005f
        214, // ffa300 → ffaf00
        220, // ffec27 → ffdf00
         47, // 00e436 → 00ff5f
         39, // 29adff → 00afff
        103, // 83769c → 8787af
        211, // ff77a8 → f787af
        223, // ffccaa → ffdfaf
    };

    // FIXME: support the extended palette!
    return ansi_palette[m_ram.draw_state.screen_palette[c & 0xf] & 0xf];
}

} // namespace z8::pico8

