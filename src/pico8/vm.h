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

#include <optional>

#include "zepto8.h"
#include "bios.h"
#include "cart.h"
#include "memory.h"
#include "pico8/z8lua.h"

namespace z8
{

class player;

namespace pico8
{

using lol::u8vec4;

class vm : z8::vm_base
{
    friend class z8::player;

public:
    vm();
    virtual ~vm();

    virtual void load(char const *name);
    virtual void run();
    virtual bool step(float seconds);

    virtual void render(lol::u8vec4 *screen) const;

    virtual std::function<void(void *, int)> get_streamer(int channel);

    virtual void button(int index, int state);
    virtual void mouse(lol::ivec2 coords, int buttons);
    virtual void keyboard(char ch);

    virtual std::tuple<uint8_t *, size_t> ram();
    virtual std::tuple<uint8_t *, size_t> rom();

    void print_ansi(lol::ivec2 term_size = lol::ivec2(128, 128),
                    uint8_t const *prev_screen = nullptr) const;

private:
    static int panic_hook(lua_State *l);
    static void instruction_hook(lua_State *l, lua_Debug *ar);

    // Private methods (hidden from the user)
    int private_cartdata(lua_State *l);
    void private_stub(std::string str);

    // System
    void api_run();
    void api_menuitem();
    int api_reload(lua_State *l);
    int16_t api_peek(int16_t addr);
    int16_t api_peek2(int16_t addr);
    fix32 api_peek4(int16_t addr);
    int api_poke(lua_State *l);
    int api_poke2(lua_State *l);
    int api_poke4(lua_State *l);
    int api_memcpy(lua_State *l);
    int api_memset(lua_State *l);
    int api_stat(lua_State *l);
    static int api_printh(lua_State *l);
    int api_extcmd(lua_State *l);

    // I/O
    void api_update_buttons();
    int api_btn(lua_State *l);
    int api_btnp(lua_State *l);

    // Text
    void api_cursor(uint8_t x, uint8_t y,
                    std::optional<uint8_t> c);
    void api_print(std::optional<std::string> str,
                   std::optional<fix32> x,
                   std::optional<fix32> y,
                   std::optional<fix32> c);

    // Graphics
    void api_camera(int16_t x, int16_t y);
    void api_circ(int16_t x, int16_t y, int16_t r, std::optional<fix32> c);
    void api_circfill(int16_t x, int16_t y, int16_t r, std::optional<fix32> c);
    void api_clip(int16_t x, int16_t y, int16_t w, std::optional<int16_t> h);
    void api_cls(uint8_t c);
    void api_color(uint8_t c);
    void api_fillp(fix32 fillp);
    int api_fget(lua_State *l);
    void api_fset(std::optional<int16_t> n, std::optional<int16_t> f,
                  std::optional<bool> b);
    void api_line(int16_t x0, std::optional<int16_t> opt_y0,
                  std::optional<int16_t> opt_x1, int16_t y1,
                  std::optional<fix32> c);
    void api_map(int16_t cel_x, int16_t cel_y, int16_t sx, int16_t sy,
                 std::optional<int16_t> in_cel_w,
                 std::optional<int16_t> in_cel_h, int16_t layer);
    fix32 api_mget(int16_t x, int16_t y);
    void api_mset(int16_t x, int16_t y, uint8_t n);
    void api_pal(std::optional<int16_t> c0,
                 std::optional<int16_t> c1, uint8_t p);
    void api_palt(std::optional<int16_t> c, std::optional<uint8_t> t);
    fix32 api_pget(int16_t x, int16_t y);
    void api_pset(int16_t x, int16_t y, std::optional<fix32> c);
    void api_rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                  std::optional<fix32> c);
    void api_rectfill(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                      std::optional<fix32> c);
    int16_t api_sget(int16_t x, int16_t y);
    void api_sset(int16_t x, int16_t y, std::optional<int16_t> c);
    int api_spr(lua_State *l);
    int api_sspr(lua_State *l);

    // Sound
    void api_music(int16_t pattern, int16_t fade_len, int16_t mask);
    void api_sfx(int16_t sfx, std::optional<int16_t> in_chan, int16_t offset);

    // Deprecated
    fix32 api_time();

private:
    void install_lua_api();

    uint8_t get_pixel(int16_t x, int16_t y) const;

    uint32_t to_color_bits(std::optional<fix32> c);

    void set_pixel(int16_t x, int16_t y, uint32_t color_bits);

    void hline(int16_t x1, int16_t x2, int16_t y, uint32_t color_bits);
    void vline(int16_t x, int16_t y1, int16_t y2, uint32_t color_bits);

    uint8_t getspixel(int16_t x, int16_t y);
    void setspixel(int16_t x, int16_t y, uint8_t color);

    void getaudio(int channel, void *buffer, int bytes);

private:
    lua_State *m_lua;
    cart m_cart;
    memory m_ram;

    // Files
    std::string m_cartdata;

    // Input
    int m_buttons[2][64];
    struct { fix32 x, y, b; } m_mouse;
    struct { int start = 0, stop = 0; char chars[256]; } m_keyboard;

    // Audio
    struct music
    {
        int m_pattern = -1;
        uint8_t m_mask = -1;
    }
    m_music;

    struct channel
    {
        channel();

        int16_t m_sfx = -1;
        float m_offset = 0;
        float m_phi = 0;
        bool m_can_loop = true;

        int8_t m_prev_key = 0;
        float m_prev_vol = 0;
    }
    m_channels[4];

    lol::timer m_timer;
    int m_instructions;
};

} // namespace pico8

} // namespace z8

