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

#include "code-fixer.h"

namespace z8
{
    using lol::array;
    using lol::array2d;
    using lol::ivec2;
    using lol::msg;
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

    class cartridge : public lol::LuaLoader
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

            // Fix code
            code_fixer fixer(m_code);
            m_code = fixer.fix();
            //msg::info("Fixed cartridge code:\n%s\n", m_code.C());
            //printf("%s", m_code.C());
        }

        class lua_object : public lol::LuaObject
        {
        public:
            lua_object(lol::String const& name) {}
            virtual ~lua_object() {}

            static lua_object* New(lol::LuaState* l, int arg_nb)
            {
            }

            static const lol::LuaObjectLib* GetLib()
            {
                static const lol::LuaObjectLib lib = lol::LuaObjectLib(
                    "_z8",

                    // Statics
                    {
                        { "btn",    &lua_object::btn },
                        { "btnp",   &lua_object::btnp },
                        { "cls",    &lua_object::cls },
                        { "cos",    &lua_object::cos },
                        { "cursor", &lua_object::cursor },
                        { "flr",    &lua_object::flr },
                        { "music",  &lua_object::music },
                        { "pget",   &lua_object::pget },
                        { "pset",   &lua_object::pset },
                        { "rnd",    &lua_object::rnd },
                        { "sget",   &lua_object::sget },
                        { "sset",   &lua_object::sset },
                        { "sin",    &lua_object::sin },
                        { "spr",    &lua_object::spr },
                        { "sspr",   &lua_object::sspr },
                        { nullptr, nullptr }
                    },

                    // Methods
                    {
                        { nullptr, nullptr },
                    },

                    // Variables
                    {
                        { nullptr, nullptr, nullptr },
                    });

                return &lib;
            }

            static int btn(lol::LuaState *l)
            {
                lol::LuaStack s(l);
                lol::LuaFloat x;
                lol::LuaBool ret;
                s >> x;
                ret = false;
                msg::info("z8:stub:btn(%d)\n", (int)x.GetValue());
                return s << ret;
            }

            static int btnp(lol::LuaState *l)
            {
                lol::LuaStack s(l);
                lol::LuaFloat x;
                lol::LuaBool ret;
                s >> x;
                ret = false;
                msg::info("z8:stub:btnp(%d)\n", (int)x.GetValue());
                return s << ret;
            }

            static int cls(lol::LuaState *l)
            {
                lol::LuaStack s(l);
                msg::info("z8:stub:cls()\n");
                return 0;
            }

            static int cos(lol::LuaState *l)
            {
                lol::LuaStack s(l);
                lol::LuaFloat x, ret;
                s >> x;
                ret = lol::cos(x.GetValue() / lol::F_TAU);
                return s << ret;
            }

            static int cursor(lol::LuaState *l)
            {
                lol::LuaStack s(l);
                lol::LuaFloat x, y;
                s >> x >> y;
                msg::info("z8:stub:cursor(%f,%f)\n", x.GetValue(), y.GetValue());
                return 0;
            }

            static int flr(lol::LuaState *l)
            {
                lol::LuaStack s(l);
                lol::LuaFloat x, ret;
                s >> x;
                ret = lol::floor(x.GetValue());
                return s << ret;
            }

            static int music(lol::LuaState *l)
            {
                lol::LuaStack s(l);
                msg::info("z8:stub:music()\n");
                return 0;
            }

            static int pget(lol::LuaState *l)
            {
                lol::LuaStack s(l);
                lol::LuaFloat x, y, ret;
                s >> x >> y;
                ret = 0;
                msg::info("z8:stub:pget(%d, %d)\n", (int)x.GetValue(), (int)y.GetValue());
                return s << ret;
            }

            static int pset(lol::LuaState *l)
            {
                lol::LuaStack s(l);
                lol::LuaFloat x, y;
                s >> x >> y;
                msg::info("z8:stub:pset(%d, %d...)\n", (int)x.GetValue(), (int)y.GetValue());
                return 0;
            }

            static int rnd(lol::LuaState *l)
            {
                lol::LuaStack s(l);
                lol::LuaFloat x, ret;
                s >> x;
                ret = lol::rand(x.GetValue());
                return s << ret;
            }

            static int sget(lol::LuaState *l)
            {
                lol::LuaStack s(l);
                lol::LuaFloat x, y, ret;
                s >> x >> y;
                ret = 0;
                msg::info("z8:stub:sget(%d, %d)\n", (int)x.GetValue(), (int)y.GetValue());
                return s << ret;
            }

            static int sset(lol::LuaState *l)
            {
                lol::LuaStack s(l);
                lol::LuaFloat x, y;
                s >> x >> y;
                msg::info("z8:stub:sset(%d, %d...)\n", (int)x.GetValue(), (int)y.GetValue());
                return 0;
            }

            static int sin(lol::LuaState *l)
            {
                lol::LuaStack s(l);
                lol::LuaFloat x, ret;
                s >> x;
                ret = lol::sin(x.GetValue() / -lol::F_TAU);
                return s << ret;
            }

            static int spr(lol::LuaState *l)
            {
                lol::LuaStack s(l);
                lol::LuaFloat n, x, y, w, h, flip_x, flip_y;
                s >> n >> x >> y >> w >> h >> flip_x >> flip_y;
                msg::info("z8:stub:spr(%d, %d, %d...)\n", (int)n, (int)x, (int)y);
                return 0;
            }

            static int sspr(lol::LuaState *l)
            {
                lol::LuaStack s(l);
                lol::LuaFloat sx, sy, sw, sh, dx, dy, dw, dh, flip_x, flip_y;
                s >> sx >> sy >> sw >> sh >> dx >> dy;
                msg::info("z8:stub:sspr(%d, %d, %d, %d, %d, %d...)\n", (int)sx, (int)sy, (int)sw, (int)sh, (int)dx, (int)dy);
                return 0;
            }

            static int test_stuff(lol::LuaState* l)
            {
                lol::LuaStack s(l);
                return 0;
            }
        };

        void run()
        {
            // FIXME: not required yet because we inherit from LuaLoader
            //lol::LuaLoader lua;

            // Register our Lua module
            lol::LuaObjectDef::Register<lua_object>(GetLuaState());
            ExecLuaFile("zepto8.lua");

            // Execute cartridge code
            ExecLuaCode(m_code.C());

            // Initialize cartridge code
            ExecLuaCode("_init()");
            ExecLuaCode("_draw()");
            ExecLuaCode("_update()");
        }

    private:
        array<uint8_t> m_data;
        lol::String m_code;
    };
}

