//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016–2024 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#pragma once

#include <any> // std::any
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
    virtual void reset() = 0;
    virtual bool step(float seconds) = 0;
    virtual float getTime() = 0;

    // Rendering
    virtual void render(lol::u8vec4 *screen) const = 0;
    virtual u4mat2<128, 128> const &get_front_screen() const = 0;
    virtual lol::ivec2 get_screen_resolution() const = 0;

    virtual int get_ansi_color(uint8_t c) const = 0;
    // FIXME: get_ansi_color() should be get_rgb(), and render()
    // should be removed in favour of a generic function that
    // uses get_rgb() too.

    void print_ansi(lol::ivec2 term_size = lol::ivec2(128, 64),
                    uint8_t const *prev_screen = nullptr) const;

    // Code
    virtual std::string const &get_code() const = 0;

    // Audio streaming
    virtual void get_audio(void* buffer, size_t frames) = 0;

    // IO
    virtual void button(int player, int index, int state) = 0;
    virtual void mouse(lol::ivec2 coords, lol::ivec2 relative, int buttons, int scroll) = 0;
    virtual void text(char ch) = 0;
    virtual void sixaxis(lol::vec3 angle) = 0;
    virtual void axis(int player, float valueX, float valueY) = 0;

    // Memory (TODO: switch to std::span one day…)
    virtual std::tuple<uint8_t *, size_t> ram() = 0;
    virtual std::tuple<uint8_t *, size_t> rom() = 0;

    virtual void request_exit() = 0;
    virtual bool is_running() = 0;

    virtual int get_filter_index() = 0;
    virtual int get_fullscreen() = 0;
    virtual void set_fullscreen(int value, bool save = true, bool runCallback = true) = 0;
    virtual void set_config_dir(std::string new_path_config_dir) = 0;
    virtual void use_default_carts_dir() = 0;

    std::function<int(int)> setfilter_callback;
    std::function<std::string(int)> getfiltername_callback;

    //full screen callbacks
    std::function<void(int)> setfullscreen_callback;
    std::function<std::string()> getfullscreen_callback;
    std::function<void(bool)> pointerLock_callback;

    void registerSetFilterCallback(std::function<int(int)> setfilterListener) {
        setfilter_callback = std::move(setfilterListener);
    }
    void registerGetFilterNameCallback(std::function<std::string(int)> getfilternameListener) {
        getfiltername_callback = std::move(getfilternameListener);
    }
    // Register SET fullscreen Callback
    void registerSetFullscreenCallback(std::function<void(int)> setFullscreenListener) {
        setfullscreen_callback = std::move(setFullscreenListener);
    }
    // Register GET fullscreen Callback
    void registerGetFullscreenCallback(std::function<std::string()> getFullscreenListener) {
        getfullscreen_callback = std::move(getFullscreenListener);
    }
    void registerPointerLockCallback(std::function<void(bool)> pointerLockListener) {
        pointerLock_callback = std::move(pointerLockListener);
    }

    // Extension commands
    virtual void add_extcmd(std::string const &, std::function<void(std::string const &)>) = 0;
    virtual void add_stat(int16_t, std::function<std::any()>) = 0;

protected:
    std::unique_ptr<pico8::bios> m_bios; // TODO: get rid of this
};

enum
{
    SCREEN_WIDTH = 128,
    SCREEN_HEIGHT = 128,
};

} // namespace z8
