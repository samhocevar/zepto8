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

namespace z8
{
    using lol::array;
    using lol::array2d;
    using lol::ivec2;
    using lol::u8vec4;
    using lol::PixelFormat;

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
        OFFSET_VERSION    = 0x8000,
    };

    class cartridge
    {
    public:
        cartridge(char const *filename)
        {
            load_png(filename);
        }

        void load_png(char const *filename)
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
                static char const *lut = "#\n 0123456789abcdef"
                    "ghijklmnopqrstuvwxyz!#%(){}[]<>+=/*:;.,~_";

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
                        m_code += m_data[i] ? lut[m_data[i]] : m_data[++i];
                    }
                }
                m_code += '\n';
            }

            // Dump code to stdout
            printf(m_code.C());

            // Execute code
            lol::LuaLoader lua;
            lua.ExecLuaCode(m_code.C());
        }

    private:
        array<uint8_t> m_data;
        lol::String m_code;
    };
}

