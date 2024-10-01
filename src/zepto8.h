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

#include <lol/vector> // lol::ivec2
#include <string>     // std::string
#include <tuple>      // std::tuple
#include <functional> // std::function
#include <cassert>    // assert()
#include <cstddef>
#include <memory>     // std::unique_ptr

// The ZEPTO-8 types
// —————————————————
// Various types and enums that describe ZEPTO-8.

namespace z8
{

namespace pico8
{
    class bios; // TODO: get rid of this
}

//
// A simple 4-bit 2D array
//

template<int W, int H>
class u4mat2
{
public:
    inline uint8_t safe_get(int x, int y) const
    {
        return (x >= 0 && y >= 0 && x < W && y < H) ? get(x,y) : 0;
    }

    inline void safe_set(int x, int y, uint8_t c)
    {
        if (x >= 0 && y >= 0 && x < W && y < H)
            set(x, y, c);
    }

    inline uint8_t get(int x, int y) const
    {
        assert(x >= 0 && x < W && y >= 0 && y < H);
        uint8_t const p = data[y][x / 2];
        return x & 1 ? p >> 4 : p & 0xf;
    }

    inline void set(int x, int y, uint8_t c)
    {
        assert(x >= 0 && x < W && y >= 0 && y < H);
        uint8_t &p = data[y][x / 2];
        p = (p & (x & 1 ? 0x0f : 0xf0)) | (x & 1 ? c << 4 : c & 0x0f);
    }

    uint8_t data[H][W / 2];
};

//
// The generic VM interface
//

class vm_base
{
    friend class player;

public:
    vm_base() = default;
    virtual ~vm_base() = default;

    virtual void load(std::string const &name) = 0;
    virtual void run() = 0;
    virtual bool step(float seconds) = 0;

    // Rendering
    virtual void render(lol::u8vec4 *screen) const = 0;
    virtual u4mat2<128, 128> const &get_front_screen() const = 0;
    virtual int get_ansi_color(uint8_t c) const = 0;
    // FIXME: get_ansi_color() should be get_rgb(), and render()
    // should be removed in favour of a generic function that
    // uses get_rgb() too.

    void print_ansi(lol::ivec2 term_size = lol::ivec2(128, 64),
                    uint8_t const *prev_screen = nullptr) const;

    // Code
    virtual std::string const &get_code() const = 0;

    // Audio streaming
    virtual std::function<void(void *, int)> get_streamer(int channel) = 0;

    // IO
    virtual void button(int index, int state) = 0;
    virtual void mouse(lol::ivec2 coords, int buttons) = 0;
    virtual void text(char ch) = 0;

    // Memory (TODO: switch to std::span one day…)
    virtual std::tuple<uint8_t *, size_t> ram() = 0;
    virtual std::tuple<uint8_t *, size_t> rom() = 0;

protected:
    std::unique_ptr<pico8::bios> m_bios; // TODO: get rid of this
};

enum
{
    SCREEN_WIDTH = 128,
    SCREEN_HEIGHT = 128,
};

} // namespace z8

