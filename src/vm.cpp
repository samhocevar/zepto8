//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2017 Sam Hocevar <sam@hocevar.net>
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

#include <lol/engine.h>

#include "zepto8.h"

namespace z8
{

void vm_base::print_ansi(lol::ivec2 term_size,
                         uint8_t const *prev_screen) const
{
    int palette[16];
    for (int i = 0; i < 16; ++i)
        palette[i] = get_ansi_color(i);

    printf("\x1b[?25l"); // hide cursor

    auto &screen = get_screen();

    for (int y = 0; y < 2 * lol::min(64, term_size.y); y += 2)
    {
        if (prev_screen && !memcmp(&screen.data[y],
                                   &prev_screen[y * 64], 128))
            continue;

        printf("\x1b[%d;1H", y / 2 + 1);

        int oldfg = -1, oldbg = -1;

        for (int x = 0; x < lol::min(128, term_size.x); ++x)
        {
            uint8_t fg = screen.get(x, y);
            uint8_t bg = screen.get(x, y + 1);
            char const *glyph = "▀";

            if (fg < bg)
            {
                std::swap(fg, bg);
                glyph = "▄";
            }

            if (fg == oldfg)
            {
                if (bg != oldbg)
                    printf("\x1b[48;5;%dm", palette[bg]);
            }
            else
            {
                if (bg == oldbg)
                    printf("\x1b[38;5;%dm", palette[fg]);
                else
                    printf("\x1b[38;5;%d;48;5;%dm", palette[fg], palette[bg]);
            }

            printf("%s", glyph);

            oldfg = fg;
            oldbg = bg;
        }

        printf("\x1b[0m\x1b[K"); // reset properties and clear to end of line
    }

    printf("\x1b[?25h"); // show cursor
    fflush(stdout);
}

} // namespace z8

