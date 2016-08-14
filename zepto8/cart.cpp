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

#include "zepto8.h"
#include "cart.h"

namespace z8
{

using lol::array;
using lol::ivec2;
using lol::msg;
using lol::u8vec4;
using lol::PixelFormat;

void cart::load_png(char const *filename)
{
    // Open cartridge as PNG image
    lol::Image img;
    img.Load(filename);
    ivec2 size = img.GetSize();
    int count = size.x * size.y;
    m_data.resize(count);

    // Retrieve cartridge data from lower image bits
    u8vec4 const *pixels = img.Lock<PixelFormat::RGBA_8>();
    for (int n = 0; n < count; ++n)
    {
        u8vec4 p = pixels[n] * 64;
        m_data[n] = p.a + p.r / 4 + p.g / 16 + p.b / 64;
    }
    img.Unlock(pixels);

    // Retrieve code, with optional decompression
    int version = m_data[OFFSET_VERSION];
    msg::info("Found cartridge version %d\n", version);
    if (version == 0 || m_data[OFFSET_CODE] != ':'
                     || m_data[OFFSET_CODE + 1] != 'c'
                     || m_data[OFFSET_CODE + 2] != ':'
                     || m_data[OFFSET_CODE + 3] != '\0')
    {
        int length = 0;
        while (OFFSET_CODE + length < OFFSET_VERSION
                && m_data[OFFSET_CODE + length] != '\0')
            ++length;

        m_code.resize(length);
        memcpy(m_code.C(), m_data.data() + OFFSET_CODE, length);
        m_code[length] = '\0';
    }
    else if (version == 1 || version >= 5)
    {
        // Expected data length
        int length = m_data[OFFSET_CODE + 4] * 256
                   + m_data[OFFSET_CODE + 5];

        m_code.resize(0);
        for (int i = OFFSET_CODE + 8; i < m_data.count() && m_code.count() < length; ++i)
        {
            if(m_data[i] >= 0x3c)
            {
                int a = (m_data[i] - 0x3c) * 16 + (m_data[i + 1] & 0xf);
                int b = m_data[i + 1] / 16 + 2;
                while (b--)
                    m_code += m_code[m_code.count() - a];
                ++i;
            }
            else
            {
                static char const *lut = "#\n 0123456789abcdef"
                    "ghijklmnopqrstuvwxyz!#%(){}[]<>+=/*:;.,~_";

                m_code += m_data[i] ? lut[m_data[i]] : m_data[++i];
            }
        }
        msg::info("Expected %d bytes, got %d\n", length, (int)m_code.count());
    }

    // Dump code to stdout
    //msg::info("Cartridge code:\n");
    //printf("%s", m_code.C());
}

} // namespace z8

