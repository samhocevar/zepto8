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

#include "zepto8.h"
#include "cart.h"

namespace z8
{

using lol::array;
using lol::u8vec4;

class vm : public lol::LuaLoader,
           public lol::LuaObject
{
public:
    vm();
    ~vm();

    void load(char const *name);
    void run();
    void step(float seconds);

    uint8_t *get_mem(int offset = 0) { return &m_memory[offset]; }
    uint8_t const *get_mem(int offset = 0) const { return &m_memory[offset]; }

    void render(lol::u8vec4 *screen) const;
    void print_ansi(lol::ivec2 term_size = lol::ivec2(128, 128),
                    uint8_t const *prev_screen = nullptr) const;

    void button(int index, int state) { m_buttons[1][index] = state; }

    static const lol::LuaObjectLib* GetLib();
    static vm* New(lol::LuaState* l, int arg_nb);

private:
    void set_this(lol::LuaState *l);
    static vm* get_this(lol::LuaState *l);
    static void hook(lol::LuaState *l, lua_Debug *ar);

    // System
    static int run(lol::LuaState *l);
    static int menuitem(lol::LuaState *l);
    static int cartdata(lol::LuaState *l);
    static int reload(lol::LuaState *l);
    static int peek(lol::LuaState *l);
    static int poke(lol::LuaState *l);
    static int memcpy(lol::LuaState *l);
    static int memset(lol::LuaState *l);
    static int dget(lol::LuaState *l);
    static int dset(lol::LuaState *l);
    static int stat(lol::LuaState *l);
    static int printh(lol::LuaState *l);

    // I/O
    static int update_buttons(lol::LuaState *l);
    static int btn(lol::LuaState *l);
    static int btnp(lol::LuaState *l);

    // Text
    static int cursor(lol::LuaState *l);
    static int print(lol::LuaState *l);

    // Maths
    static int max(lol::LuaState *l);
    static int min(lol::LuaState *l);
    static int mid(lol::LuaState *l);
    static int flr(lol::LuaState *l);
    static int cos(lol::LuaState *l);
    static int sin(lol::LuaState *l);
    static int atan2(lol::LuaState *l);
    static int sqrt(lol::LuaState *l);
    static int abs(lol::LuaState *l);
    static int sgn(lol::LuaState *l);

    static int rnd(lol::LuaState *l);
    static int srand(lol::LuaState *l);

    static int band(lol::LuaState *l);
    static int bor(lol::LuaState *l);
    static int bxor(lol::LuaState *l);
    static int bnot(lol::LuaState *l);
    static int shl(lol::LuaState *l);
    static int shr(lol::LuaState *l);

    // Graphics
    static int camera(lol::LuaState *l);
    static int circ(lol::LuaState *l);
    static int circfill(lol::LuaState *l);
    static int clip(lol::LuaState *l);
    static int cls(lol::LuaState *l);
    static int color(lol::LuaState *l);
    static int fget(lol::LuaState *l);
    static int fset(lol::LuaState *l);
    static int line(lol::LuaState *l);
    static int map(lol::LuaState *l);
    static int mget(lol::LuaState *l);
    static int mset(lol::LuaState *l);
    static int pal(lol::LuaState *l);
    static int palt(lol::LuaState *l);
    static int pget(lol::LuaState *l);
    static int pset(lol::LuaState *l);
    static int rect(lol::LuaState *l);
    static int rectfill(lol::LuaState *l);
    static int sget(lol::LuaState *l);
    static int sset(lol::LuaState *l);
    static int spr(lol::LuaState *l);
    static int sspr(lol::LuaState *l);

    // Sound
    static int music(lol::LuaState *l);
    static int sfx(lol::LuaState *l);

    // Deprecated
    static int time(lol::LuaState *l);

private:
    int getpixel(int x, int y);
    void setpixel(int x, int y, int color);

    void hline(int x1, int x2, int y, int color);
    void vline(int x, int y1, int y2, int color);

    int getspixel(int x, int y);
    void setspixel(int x, int y, int color);

private:
    uint8_t m_memory[SIZE_MEMORY];
    lol::Image m_font;
    cart m_cart;

    uint32_t m_seed;
    uint8_t m_color;
    lol::ivec2 m_camera, m_cursor;
    lol::ibox2 m_clip;
    int m_buttons[2][64];
    uint8_t m_pal[2][16], m_palt[16];

    lol::Timer m_timer;
    int m_instructions;
};

// Clamp a double to the nearest value that can be represented as a 16:16
// fixed point number (the ones used in PICO-8).
static double const multiplier = 65536.0;

static inline int32_t double2fixed(double x)
{
    // This is more or less necessary because we use standard Lua with no
    // modifications, so we need a way to compute 1/0 or 0/0.
    if (std::isnan(x) || std::isinf(x))
        return x < 0 ? (int32_t)0x80000000 : (int32_t)0x7fffffff;
    return (int32_t)(int64_t)lol::round(x * multiplier);
}

static inline double fixed2double(int32_t x)
{
    return x / multiplier;
}

static inline double clamp64(double x)
{
    return fixed2double(double2fixed(x));
}

// Not a Lua function, but behaves like one
static inline double lua_toclamp64(lua_State *l, int index)
{
    return clamp64(lua_tonumber(l, index));
}

} // namespace z8

