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

#pragma once

#include <lol/engine.h>

#include "zepto8.h"

namespace z8
{

struct memory
{
    uint8_t gfx[0x1000];

    union
    {
        uint8_t gfx2[0x1000];
        uint8_t map2[0x1000];
    };

    uint8_t map[0x1000];

    uint8_t gfx_props[0x100];

    uint8_t song[0x100];

    uint8_t sfx[0x1100];

    union
    {
        uint8_t user_data[0x1b00];
        uint8_t code[0x1b00];
    };

    uint8_t persistent[0x100];

    // Draw state
    struct
    {
        // Palette information (draw palette, screen palette)
        uint8_t pal[2][16];

        // Clipping information
        struct
        {
            uint8_t x1, y1, x2, y2;
        }
        clip;

        uint8_t undocumented1[1];

        // Pen colors:
        //  0x0f — default colour
        //  0xf0 — alternate collor for fillp
        uint8_t pen;

        // Text cursor coordinates
        struct
        {
            uint8_t x, y;
        }
        cursor;

        uint8_t camera[4];
        uint8_t undocumented3[1];
        uint8_t mouse_flag;
        uint8_t undocumented4[3];
        uint8_t fillp[2], fillp_trans, fillp_flag;
        uint8_t undocumented5[11];
    }
    draw_state;

    // Hardware state (mostly undocumented)
    uint8_t hw_state[0x40];

    uint8_t gpio_pins[0x80];

    // The screen, as an array of bytes
    uint8_t screen[0x2000];

    // Standard accessors
    inline uint8_t &operator[](int n)
    {
        return ((uint8_t *)this)[n];
    }

    inline uint8_t const &operator[](int n) const
    {
        return ((uint8_t const *)this)[n];
    }
};

// Check all section offsets and sizes
#define static_check_section(name, offset, size) \
    static_assert(offsetof(memory, name) == offset, \
                  "memory::"#name" should have offset "#offset); \
    static_assert(sizeof(memory::name) == size, \
                  "memory::"#name" should have size "#size);

static_check_section(gfx,        0x0000, 0x1000);
static_check_section(gfx2,       0x1000, 0x1000);
static_check_section(map2,       0x1000, 0x1000);
static_check_section(map,        0x2000, 0x1000);
static_check_section(gfx_props,  0x3000,  0x100);
static_check_section(song,       0x3100,  0x100);
static_check_section(sfx,        0x3200, 0x1100);
static_check_section(user_data,  0x4300, 0x1b00);
static_check_section(persistent, 0x5e00,  0x100);
static_check_section(draw_state, 0x5f00,   0x40);
static_check_section(hw_state,   0x5f40,   0x40);
static_check_section(gpio_pins,  0x5f80,   0x80);
static_check_section(screen,     0x6000, 0x2000);
#undef static_check_section

// Final sanity check
static_assert(sizeof(memory) == 0x8000);

}

