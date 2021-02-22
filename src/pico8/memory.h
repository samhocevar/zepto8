//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2021 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#pragma once

#include <lol/vector> // lol::u8vec2
#include <algorithm>  // std::swap
#include <array>      // std::array
#include <bitset>     // std::bitset
#include <cassert>    // assert

#include "zepto8.h"

// The PICO-8 memory classes: sfx, song, draw_state, memory
// ————————————————————————————————————————————————————————
// These classes map 1-to-1 with the PICO-8 memory layout and provide
// handful accessors for higher level information.
// For instance:
//  - "memory[x]" accesses the xth byte in memory
//  - "memory.sfx[3].notes[6].effect" gets the effect of the 6th note of the 3rd SFX
//  - "memory.gpio_pin[2] is the 2nd GPIO pin

namespace z8::pico8
{

// We use uint16_t instead of uint8_t because note::instrument overlaps two bytes.
struct note_t
{
    uint16_t key : 6;
    uint16_t instrument : 3;
    uint16_t volume : 3;
    // FIXME: there is an actual extra bit for the effect but I don’t
    // know what it’s for: PICO-8 documentation says 0…7, not 0…15
    // Update: maybe this is used for the new SFX instrument feature?
    uint16_t effect : 4;
};

struct sfx_t
{
    note_t notes[32];

    // 0: editor mode
    // 1: speed (1-255)
    // 2: loop start
    // 3: loop end
    uint8_t editor_mode;
    uint8_t speed;
    uint8_t loop_start;
    uint8_t loop_end;
};

struct song_t
{
    union
    {
        struct
        {
            uint8_t sfx0  : 7;
            uint8_t start : 1;
            uint8_t sfx1  : 7;
            uint8_t loop  : 1;
            uint8_t sfx2  : 7;
            uint8_t stop  : 1;
            uint8_t sfx3  : 7;
            uint8_t mode  : 1;
        };

        // The four song channels that should play, 0…63 (each msb holds a flag)
        uint8_t data[4];
    };

    // Accessor for data transformation
    uint8_t sfx(int n) const;
};

struct code_t : std::array<uint8_t, 0x3d00>
{
};

struct custom_font_t
{
    // 0x5600: character width in pixels (can be more than 8, but only 8 pixels are drawn)
    uint8_t width;

    // 0x5601: character width for character 128 and above
    uint8_t extended_width;

    // 0x5602: character height in pixels
    uint8_t height;

    // 0x5603—0x5605: draw offset x, draw offset y
    lol::u8vec2 offset;

    uint8_t padding[3];

    // 0x5608—0x5e00: glyph data
    uint8_t glyphs[255][8];
};

struct draw_state_t
{
    // 0x5f00—0x5f20: palette information (draw palette, screen palette)
    uint8_t draw_palette[16];
    uint8_t screen_palette[16];

    // 0x5f20—0x5f24: clipping information
    struct
    {
        uint8_t x1, y1, x2, y2;
    }
    clip;

    // 0x5f24: undocumented
    uint8_t undocumented1[1];

    // 0x5f25: pen colors
    //  0x0f — default colour
    //  0xf0 — alternate color for fillp
    uint8_t pen;

    // 0x5f26—0x5f28: text cursor coordinates
    lol::u8vec2 cursor;

    // 0x5f28—0x5f2c: camera coordinates
    lol::i16vec2 camera;

    // 0x5f2c: screen mode (double width, etc.)
    uint8_t screen_mode;

    // 0x5f2d: mouse flags
    struct
    {
        uint8_t enabled      : 1;
        uint8_t buttons      : 1;
        uint8_t locked       : 1;
        uint8_t undocumented : 5;
    }
    mouse_flags;

    // 0x5f2e: preserve flags
    struct
    {
        uint8_t palette       : 1;
        uint8_t undocumented1 : 4;
        uint8_t fillp         : 1;
        uint8_t undocumented2 : 2;
    }
    preserve_flags;

    // 0x5f2f: undocumented
    uint8_t undocumented2[1];

    // 0x5f30: block pause menu flag
    uint8_t pause_flag;

    // 0x5f31—0x5f35: fill pattern
    uint8_t fillp[2], fillp_trans, fillp_flag;

    // 0x5f35: next polyline will not draw (0x1)
    uint8_t polyline_flag;

    // 0x5f36: if 0x8, treat sprite 0 as opaque in tline() and map()
    uint8_t sprite_zero;

    // 0x5f37: if 0x1, do not call reload() when exiting any of the editor modes
    uint8_t disable_editor_reload;

    struct
    {
        // 0x5f38—0x5f3c: tline mask
        lol::u8vec2 mask;

        // 0x5f3a—0x5f3c: tline offset
        lol::u8vec2 offset;
    }
    tline;

    // 0x5f3c—0x5f40: polyline current point coordinates
    lol::i16vec2 polyline;
};

struct print_state_t
{
     uint8_t active  : 1;
     uint8_t padding : 1;
     uint8_t wide    : 1;
     uint8_t tall    : 1;
     uint8_t solid   : 1;
     uint8_t invert  : 1;
     uint8_t dotty   : 1;
     uint8_t custom  : 1;

     uint8_t char_w   : 4;
     uint8_t char_h   : 4;
     uint8_t char_w2  : 4;
     uint8_t tab_w    : 4;
     uint8_t offset_x : 4;
     uint8_t offset_y : 4;
};

struct hw_state_t
{
    // 0x5f40—0x5f44: sound channel effects
    uint8_t half_rate, reverb, distort, lowpass;

    // 0x5f44—0x5f4c: PRNG state
    struct { uint32_t a, b; } prng;

    // 0x5f4c—0x5f54: button state
    uint8_t btn_state[8];

    // 0x5f54—0x5f58: undocumented
    uint8_t undocumented2[4];

    // 0x5f58—0x5f5c: default attributes for print
    print_state_t print_state;

    // 0x5f5c—0x5f5e: btnp() autorepeat parameters
    uint8_t btnp_delay, btnp_rate;

    // 0x5f5e: bitplane selector
    uint8_t bit_mask;

#pragma pack(push,1)
    struct
    {
        // 0x5f5f: raster mode
        uint8_t mode;

        // 0x5f60—0x5f70: raster palette
        uint8_t palette[16];

        // 0x5f70—0x5f80: raster bits
        std::bitset<128> bits;
    }
    raster;
#pragma pack(pop)
};

struct memory
{
    // This union handles the gfx/map shared section
    union
    {
        // 0x0—0x1000: gfx
        u4mat2<128, 128> gfx;

        struct
        {
            uint8_t padding[0x1000];

            // 0x1000—0x2000: gfx2/map2
            uint8_t map2[0x1000];

            // 0x2000—0x3000: map
            struct
            {
                 // These accessors allow to access map2 as if it was
                 // located after map.
                 inline uint8_t &operator[](int n)
                 {
                     assert(n >= 0 && n < (int)(sizeof(memory::map) + sizeof(memory::map2)));
                     return b[(n ^ 0x1000) - 0x1000];
                 }

                 inline uint8_t const &operator[](int n) const
                 {
                     assert(n >= 0 && n < (int)(sizeof(memory::map) + sizeof(memory::map2)));
                     return b[(n ^ 0x1000) - 0x1000];
                 }

            private:
                uint8_t b[0x1000];
            }
            map;
        };
    };

    // 0x3000—0x3100: gfx flags
    uint8_t gfx_flags[0x100];

    // 0x3100—0x3200: song (64 songs)
    song_t song[0x40];

    // 0x3200—0x4300: sfx (64 samples)
    sfx_t sfx[0x40];

    // A cart will have the code section at the same place as user_data. Cannot use a
    // nameless union here because we’re aliased with hw_state which has constructors.

    // Runtime PICO-8 memory
    union
    {
        struct
        {
            inline auto operator()() { return reinterpret_cast<code_t &>(*this); }
            inline auto &operator()() const { return reinterpret_cast<code_t const &>(*this); }
        }
        code;

        struct
        {
            uint8_t user_data[0x1300];

            // 0x5600—0x5e00: custom font;
            custom_font_t custom_font;
        };
    };

    // 0x5e00—0x5f00: persistent cart data
    uint8_t persistent[0x100];

    // 0x5f00—0x5f40: draw state
    draw_state_t draw_state;

    // 0x5f40—0x5f80: hardware state
    hw_state_t hw_state;

    // 0x5f80—0x6000: GPIO pins
    uint8_t gpio_pins[0x80];

    // 0x6000—0x8000: screen (8k)
    u4mat2<128, 128> screen;

    // Standard accessors
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

    // Hardware pixel accessor
    uint8_t pixel(int x, int y) const
    {
        // Get screen mode
        uint8_t const &mode = draw_state.screen_mode;

        // Apply screen mode (rotation, mirror, flip…)
        if ((mode & 0xbc) == 0x84)
        {
            // Rotation modes (0x84 to 0x87)
            if (mode & 1)
                std::swap(x, y);
            x = mode & 2 ? 127 - x : x;
            y = ((mode + 1) & 2) ? 127 - y : y;
        }
        else
        {
            // Other modes
            x = (mode & 0xbd) == 0x05 ? std::min(x, 127 - x) // mirror
              : (mode & 0xbd) == 0x01 ? x / 2                // stretch
              : (mode & 0xbd) == 0x81 ? 127 - x : x;         // flip
            y = (mode & 0xbe) == 0x06 ? std::min(y, 127 - y) // mirror
              : (mode & 0xbe) == 0x02 ? y / 2                // stretch
              : (mode & 0xbe) == 0x82 ? 127 - y : y;         // flip
        }

        int c = screen.get(x, y);

        // Apply raster mode
        if (hw_state.raster.mode == 0x10)
        {
            // Raster mode: alternate palette
            if (hw_state.raster.bits[y])
                return hw_state.raster.palette[c];
        }
        else if ((hw_state.raster.mode & 0x30) == 0x30)
        {
            // Raster mode: gradient
            if ((hw_state.raster.mode & 0x0f) == c)
            {
                int c2 = (y / 8 + (hw_state.raster.bits[y] ? 1 : 0)) % 16;
                return hw_state.raster.palette[c2];
            }
        }

        // Apply screen palette
        return draw_state.screen_palette[c];
    }

    ~memory() = default;
};

// Check type sizes
static_assert(sizeof(sfx_t) == 68, "pico8::sfx has incorrect size");
static_assert(sizeof(code_t) == 0x3d00, "pico8::code_t has incorrect size");


// Check all section offsets and sizes
#define static_check_section(name, offset, size) \
    static_assert(offsetof(memory, name) == offset, \
                  "pico8::memory::"#name" should have offset "#offset); \
    static_assert((sizeof(memory::name) == size) || (size == -1), \
                  "pico8::memory::"#name" should have size "#size);

static_check_section(gfx,        0x0000, 0x2000);
static_check_section(map2,       0x1000, 0x1000);
static_check_section(map,        0x2000, 0x1000);
static_check_section(gfx_flags,  0x3000,  0x100);
static_check_section(song,       0x3100,  0x100);
static_check_section(sfx,        0x3200, 0x1100);
static_check_section(code,       0x4300,     -1);
static_check_section(user_data,  0x4300, 0x1300);
static_check_section(custom_font,0x5600,  0x800);
static_check_section(persistent, 0x5e00,  0x100);
static_check_section(draw_state, 0x5f00,   0x40);
static_check_section(hw_state,   0x5f40,   0x40);
static_check_section(gpio_pins,  0x5f80,   0x80);
static_check_section(screen,     0x6000, 0x2000);
#undef static_check_section

// Final sanity check
static_assert(sizeof(memory) == 0x8000, "pico8::memory should have size 0x8000");

}
