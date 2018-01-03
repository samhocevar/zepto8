//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2017 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#include <lol/engine.h>

#include <string>

#include "zepto8.h"
#include "splore.h"

namespace z8
{

using lol::array;
using lol::ivec2;
using lol::msg;
using lol::u8vec4;
using lol::PixelFormat;

bool splore::dump(char const *filename)
{
    // Open cartridge as PNG image
    lol::image img;
    img.load(filename);
    ivec2 size = img.size();

    if (size != ivec2(8 * 128, 4 * (128 + 8)))
        return false;

    auto const &pixels = img.lock2d<PixelFormat::RGBA_8>();

    for (int cart = 0; cart < 32; ++cart)
    {
        std::string info_lines[3];

        for (int line = 0; line < 3; ++line)
        {
            u8vec4 const *sol = &pixels[cart % 8 * 128][cart / 8 * (128 + 8) + 128 + line];
            for (int x = 0; x < 128; ++x)
            {
                u8vec4 p = sol[x];
                if (p.r == 0)
                    break;
                info_lines[line] += (char)p.r;
            }

            printf("%d: %s\n", cart, info_lines[line].c_str());
        }
    }

    img.unlock2d(pixels);

    return true;
}

} // namespace z8

