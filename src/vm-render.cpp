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

void vm::render(lol::u8vec4 *screen) const
{
    static lol::u8vec4 const palette[] =
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

    for (int n = 0; n < 128 * 128 / 2; ++n)
    {
        uint8_t data = m_memory[OFFSET_SCREEN + n];
        screen[2 * n] = palette[m_pal[1][data & 0xf]];
        screen[2 * n + 1] = palette[m_pal[1][data >> 4]];
    }
}

void vm::print_ansi() const
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

    int oldfg = -1, oldbg = -1;

    printf("\x1b[1;1H");

    for (int y = 0; y < 128; y += 2)
    {
        if (y)
            printf("\n");

        for (int x = 0; x < 128; ++x)
        {
            int offset = y * 64 + x / 2;
            int shift = 4 * (x & 1);
            uint8_t fg = (m_memory[OFFSET_SCREEN + offset] >> shift) & 0xf;
            uint8_t bg = (m_memory[OFFSET_SCREEN + offset + 64] >> shift) & 0xf;
            char const *glyph = "▀";

            if (fg < bg)
            {
                std::swap(fg, bg);
                glyph = "▄";
            }

            if (fg == oldfg)
            {
                if (bg != oldbg)
                    printf("\x1b[48;5;%dm", ansi_palette[m_pal[1][bg]]);
            }
            else
            {
                if (bg == oldbg)
                    printf("\x1b[38;5;%dm", ansi_palette[m_pal[1][fg]]);
                else
                    printf("\x1b[38;5;%d;48;5;%dm", ansi_palette[m_pal[1][fg]], ansi_palette[m_pal[1][bg]]);
            }

            printf(glyph);

            oldfg = fg;
            oldbg = bg;
        }
    }
}

} // namespace z8

