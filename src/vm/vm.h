//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2018 Sam Hocevar <sam@hocevar.net>
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
#include "bios.h"
#include "cart.h"
#include "memory.h"
#include "z8lua/lua.h"
#include "z8lua/lauxlib.h"

namespace z8
{

using lol::u8vec4;

class player;

class vm
{
    friend class z8::player;

public:
    vm();
    ~vm();

    void load(char const *name);
    void run();
    void step(float seconds);

    inline memory &get_ram() { return m_ram; }
    inline memory const &get_ram() const { return m_ram; }

    void render(lol::u8vec4 *screen) const;
    void print_ansi(lol::ivec2 term_size = lol::ivec2(128, 128),
                    uint8_t const *prev_screen = nullptr) const;

    void button(int index, int state);
    void mouse(lol::ivec2 coords, int buttons);

private:
    static int panic_hook(lua_State *l);
    static void instruction_hook(lua_State *l, lua_Debug *ar);

    // Private methods (hidden from the user)
    int private_cartdata(lua_State *l);

    // System
    int api_run(lua_State *l);
    int api_menuitem(lua_State *l);
    int api_reload(lua_State *l);
    int api_peek(lua_State *l);
    int api_peek4(lua_State *l);
    int api_poke(lua_State *l);
    int api_poke4(lua_State *l);
    int api_memcpy(lua_State *l);
    int api_memset(lua_State *l);
    int api_stat(lua_State *l);
    static int api_printh(lua_State *l);
    int api_extcmd(lua_State *l);

    // I/O
    int api_update_buttons(lua_State *l);
    int api_btn(lua_State *l);
    int api_btnp(lua_State *l);

    // Text
    int api_cursor(lua_State *l);
    int api_print(lua_State *l);

    // Graphics
    int api_camera(lua_State *l);
    int api_circ(lua_State *l);
    int api_circfill(lua_State *l);
    int api_clip(lua_State *l);
    int api_cls(lua_State *l);
    int api_color(lua_State *l);
    int api_fillp(lua_State *l);
    int api_fget(lua_State *l);
    int api_fset(lua_State *l);
    int api_line(lua_State *l);
    int api_map(lua_State *l);
    int api_mget(lua_State *l);
    int api_mset(lua_State *l);
    int api_pal(lua_State *l);
    int api_palt(lua_State *l);
    int api_pget(lua_State *l);
    int api_pset(lua_State *l);
    int api_rect(lua_State *l);
    int api_rectfill(lua_State *l);
    int api_sget(lua_State *l);
    int api_sset(lua_State *l);
    int api_spr(lua_State *l);
    int api_sspr(lua_State *l);

    // Sound
    int api_music(lua_State *l);
    int api_sfx(lua_State *l);

    // Deprecated
    int api_time(lua_State *l);

private:
    uint8_t get_pixel(int16_t x, int16_t y) const;

    uint32_t lua_to_color_bits(lua_State *l, int n);

    void set_pixel(int16_t x, int16_t y, uint32_t color_bits);

    void hline(int16_t x1, int16_t x2, int16_t y, uint32_t color_bits);
    void vline(int16_t x, int16_t y1, int16_t y2, uint32_t color_bits);

    uint8_t getspixel(int16_t x, int16_t y);
    void setspixel(int16_t x, int16_t y, uint8_t color);

    void getaudio(int channel, void *buffer, int bytes);

private:
    lua_State *m_lua;
    bios m_bios;
    cart m_cart;
    memory m_ram;

    // Files
    std::string m_cartdata;

    // Input
    int m_buttons[2][64];
    struct { fix32 x, y, b; } m_mouse;

    // Audio
    struct channel
    {
        channel();

        int16_t m_sfx = -1;
        float m_offset = 0;
        float m_phi = 0;
        bool m_can_loop = true;

        int8_t m_prev_key = 0;
    }
    m_channels[4];

    lol::timer m_timer;
    int m_instructions;
};

} // namespace z8

