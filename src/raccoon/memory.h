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

#include "zepto8.h"

// The Raccoon memory class
// ————————————————————————
// These classes map 1-to-1 with the Raccoon memory layout and provide
// handful accessors for higher level information.
// For instance:
//  - "memory[x]" accesses the xth byte in memory
//  - "memory.palette[3].g" gets the green component of the 3rd palette colour

namespace z8::raccoon
{

struct memory
{
    uint8_t sprites[96][128 / 2];
    uint8_t map[64][128];

    lol::u8vec3 palette[16];

    uint8_t flags[192];
    uint8_t sound[64 * (8 + 8 + 32 * 16) / 8];
    uint8_t music[64 * (4 * 6 + 2 + 6) / 8];

    uint8_t padding1[0x155e];

    lol::i16vec2 camera;
    uint8_t soundreg[16];
    uint8_t palmod[16];

    uint8_t padding2[6];

    uint8_t gamepad[2 * 4];
    uint8_t screen[128][128 / 2];

    inline uint8_t &operator[](int n)
    {
        ASSERT(n >= 0 && n < (int)sizeof(memory));
        return ((uint8_t *)this)[n];
    }

    inline uint8_t const &operator[](int n) const
    {
        ASSERT(n >= 0 && n < (int)sizeof(memory));
        return ((uint8_t const *)this)[n];
    }
};

// Check all section offsets and sizes
#define static_check_section(name, offset, size) \
    static_assert(offsetof(memory, name) == offset, \
                  "z8::raccoon::memory::"#name" should have offset "#offset); \
    static_assert(sizeof(memory::name) == size, \
                  "z8::raccoon::memory::"#name" should have size "#size);

static_check_section(sprites,    0x0000, 0x1800);
static_check_section(map,        0x1800, 0x2000);
static_check_section(palette,    0x3800,   0x30);
static_check_section(flags,      0x3830,   0xc0);
static_check_section(sound,      0x38f0, 0x1080);
static_check_section(music,      0x4970,  0x100);
static_check_section(camera,     0x5fce,    0x4);
static_check_section(soundreg,   0x5fd2,   0x10);
static_check_section(palmod,     0x5fe2,   0x10);
static_check_section(gamepad,    0x5ff8,    0x8);
static_check_section(screen,     0x6000, 0x2000);
#undef static_check_section

// Final sanity check
static_assert(sizeof(memory) == 0x8000, "z8::raccoon::memory should have size 0x8000");

} // namespace z8::raccoon

