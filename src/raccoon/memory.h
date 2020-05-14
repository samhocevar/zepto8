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

#pragma once

#include <lol/vector> // lol::u8vec3
#include <cassert>    // assert
#include <array>      // std::array

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

#pragma pack(push,1)
struct snd_state
{
    uint8_t index;
    uint8_t offset;
    uint8_t length;
    uint16_t time;
};
#pragma pack(pop)

#pragma pack(push,1)
struct mus_state
{
    uint8_t index   : 7;
    uint8_t playing : 1;
    uint16_t time;
    uint16_t max_time;
};
#pragma pack(pop)

#pragma pack(push,1)
struct pad_state
{
    std::array<uint8_t,4> buttons;
    std::array<uint8_t,4> prev_buttons;
    std::array<uint8_t,4> layouts;
};
#pragma pack(pop)

// Register: SPPPPPPP EEIIIIII OOTTTTTT __EEEVVV
struct snd_reg
{
    uint8_t period : 7;
    uint8_t on_off : 1;

    uint8_t instrument : 6;
    uint8_t envelope   : 2;

    uint8_t pitch  : 6;
    uint8_t offset : 2;

    uint8_t volume : 3;
    uint8_t effect : 3;
    uint8_t        : 2;
};

// Note: __TTTTTT __EEEVVV
struct note
{
    uint8_t pitch  : 6;
    uint8_t        : 2;

    uint8_t volume : 3;
    uint8_t effect : 3;
    uint8_t        : 2;
};

// Track: PPPPPPPPP EEIIIIII 64xNote
struct track
{
    uint8_t period     : 8;

    uint8_t instrument : 6;
    uint8_t envelope   : 2;

    struct note notes[32];
};

// Music: FUTTTTTT FUTTTTTT FUTTTTTTT _UTTTTTT
struct node
{
    uint8_t track : 6;
    uint8_t used  : 1;
    uint8_t flag  : 1;
};

#pragma pack(push,1)
struct memory
{
    // Spritesheet data is 128x96 pixels, arranged in 192 8x8 sprites from
    // left to right, top to bottom.
    u4mat2<128, 96> sprites;

    // Map data is 128x64 tiles, where each tile is a byte-sized sprite index.
    uint8_t map[64][128];

    // Palette data is 16 colors represented in 4 bytes each: 3 bytes for the
    // red, green and blue channels, and a fourth byte with extra data. The
    // fourth byte's 4 lowest bits are used as an indirection index (to render
    // another color instead of this one), and its highest bit is used for
    // transparency.
    struct
    {
        lol::u8vec3 color;
        uint8_t index : 4;
        uint8_t       : 3;
        uint8_t trans : 1;
    }
    palette[16];

    // Sprite flags are 192 8bits bitfields.
    uint8_t flags[192];

    // Sound data is 64 tracks.
    struct track sound[64];

    // Music data is 64 32bit nodes that connect tracks together via their indices.
    struct node music[64][4];

    uint8_t ____[0x0580];
    union
    {
        uint8_t end_of_rom;
        uint8_t _____[0x0fc7];
    };

    lol::i16vec2 camera;
    struct snd_state soundstate[4];
    struct mus_state musicstate;
    struct snd_reg soundreg[4];
    struct pad_state gamepad;
    u4mat2<128, 128> screen;

    inline uint8_t &operator[](int n)
    {
        assert(n >= 0 && n < (int)sizeof(memory));
        return ((uint8_t *)this)[n];
    }

    inline uint8_t const &operator[](int n) const
    {
        assert(n >= 0 && n < (int)sizeof(memory));
        return ((uint8_t const *)this)[n];
    }
};
#pragma pack(pop)

// Check all section offsets and sizes
#define static_check_section(name, offset, size) \
    static_assert(offsetof(memory, name) == offset, \
                  "z8::raccoon::memory::"#name" should have offset "#offset); \
    static_assert(sizeof(memory::name) == size, \
                  "z8::raccoon::memory::"#name" should have size "#size);

static_check_section(sprites,    0x0000, 0x1800);
static_check_section(map,        0x1800, 0x2000);
static_check_section(palette,    0x3800,   0x40);
static_check_section(flags,      0x3840,   0xc0);
static_check_section(sound,      0x3900, 0x1080);
static_check_section(music,      0x4980,  0x100);

static_check_section(end_of_rom, 0x5000,    0x1);

static_check_section(camera,     0x5fc7,    0x4);
static_check_section(soundstate, 0x5fcb,   0x14);
static_check_section(musicstate, 0x5fdf,   0x05);
static_check_section(soundreg,   0x5fe4,   0x10);
static_check_section(gamepad,    0x5ff4,    0xc);
static_check_section(screen,     0x6000, 0x2000);
#undef static_check_section

// Final sanity check
static_assert(sizeof(memory) == 0x8000, "z8::raccoon::memory should have size 0x8000");

} // namespace z8::raccoon

