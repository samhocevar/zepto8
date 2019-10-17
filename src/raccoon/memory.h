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

struct snd_reg
{
    uint8_t on_off     : 1;
    uint8_t period     : 7;
    uint8_t envelope   : 2;
    uint8_t instrument : 6;
    uint8_t offset     : 2;
    uint8_t pitch      : 6;
    uint8_t effect     : 5;
    uint8_t volume     : 3;
};

struct sfx
{
    uint8_t period;
    uint8_t envelope   : 2;
    uint8_t instrument : 6;
    struct
    {
        uint8_t padding : 2;
        uint8_t pitch   : 6;
        uint8_t effect  : 5;
        uint8_t volume  : 3;
    }
    notes[32];
};

struct track
{
    uint8_t flag    : 1;
    uint8_t has_sfx : 1;
    uint8_t sfx     : 6;
};

struct memory
{
    u4mat2<128, 96> sprites;
    uint8_t map[64][128];

    lol::u8vec3 palette[16];

    uint8_t flags[192];
    sfx sound[64];
    track music[64][4];

    uint8_t padding1[0x0590];
    union
    {
        uint8_t end_of_rom;
        uint8_t padding2[0x0fce];
    };

    lol::i16vec2 camera;
    snd_reg soundreg[4];
    uint8_t palmod[16];

    uint8_t padding3[6];

    uint32_t gamepad[2];
    u4mat2<128, 128> screen;

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
static_check_section(end_of_rom, 0x5000,    0x1);
static_check_section(camera,     0x5fce,    0x4);
static_check_section(soundreg,   0x5fd2,   0x10);
static_check_section(palmod,     0x5fe2,   0x10);
static_check_section(gamepad,    0x5ff8,    0x8);
static_check_section(screen,     0x6000, 0x2000);
#undef static_check_section

// Final sanity check
static_assert(sizeof(memory) == 0x8000, "z8::raccoon::memory should have size 0x8000");

} // namespace z8::raccoon

