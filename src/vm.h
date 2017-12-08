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
#include "fix32.h"

namespace z8
{

using lol::array;
using lol::u8vec4;

class player;

class vm : public lol::LuaLoader,
           public lol::LuaObject
{
    friend class z8::player;

public:
    vm();
    ~vm();

    void load(char const *name);
    void run();
    void step(float seconds);

    inline uint8_t *get_mem(int offset = 0) { return &m_memory[offset]; }
    inline uint8_t const *get_mem(int offset = 0) const { return &m_memory[offset]; }

    void render(lol::u8vec4 *screen) const;
    void print_ansi(lol::ivec2 term_size = lol::ivec2(128, 128),
                    uint8_t const *prev_screen = nullptr) const;

    void button(int index, int state);
    void mouse(lol::ivec2 coords, int buttons);

    static const lol::LuaObjectLibrary* GetLib();
    static vm* New(lua_State* l, int arg_nb);

private:
    static void hook(lua_State *l, lua_Debug *ar);

    /* Helpers to dispatch C++ functions to Lua C bindings */
    typedef int (vm::*api_func)(lua_State *);

    template<api_func f>
    static int dispatch(lua_State *l)
    {
        vm *that = *static_cast<vm**>(lua_getextraspace(l));
        return ((*that).*f)(l);
    }

    // System
    int api_run(lua_State *l);
    int api_menuitem(lua_State *l);
    int api_cartdata(lua_State *l);
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
    static int api_tonum(lua_State *l);
    static int api_tostr(lua_State *l);

    // Maths
    static int api_max(lua_State *l);
    static int api_min(lua_State *l);
    static int api_mid(lua_State *l);
    static int api_ceil(lua_State *l);
    static int api_flr(lua_State *l);
    static int api_cos(lua_State *l);
    static int api_sin(lua_State *l);
    static int api_atan2(lua_State *l);
    static int api_sqrt(lua_State *l);
    static int api_abs(lua_State *l);
    static int api_sgn(lua_State *l);

    int api_rnd(lua_State *l);
    int api_srand(lua_State *l);

    static int api_band(lua_State *l);
    static int api_bor(lua_State *l);
    static int api_bxor(lua_State *l);
    static int api_bnot(lua_State *l);
    static int api_shl(lua_State *l);
    static int api_shr(lua_State *l);
    static int api_lshr(lua_State *l);
    static int api_rotl(lua_State *l);
    static int api_rotr(lua_State *l);

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
    uint32_t get_color_bits() const;

    void set_pixel(int16_t x, int16_t y, uint32_t color_bits);

    void hline(int16_t x1, int16_t x2, int16_t y, uint32_t color_bits);
    void vline(int16_t x, int16_t y1, int16_t y2, uint32_t color_bits);

    uint8_t getspixel(int16_t x, int16_t y);
    void setspixel(int16_t x, int16_t y, uint8_t color);

    void getaudio(int channel, void *buffer, int bytes);

private:
    uint8_t m_memory[SIZE_MEMORY];
    lol::image m_font;
    cart m_cart;

    // Graphics
    fix32 m_colors, m_fillp;
    struct { fix32 x, y; } m_camera, m_cursor;
    struct { lol::i16vec2 aa, bb; } m_clip;
    uint8_t m_pal[2][16], m_palt[16];

    // Input
    int m_buttons[2][64];
    struct { fix32 x, y, b; } m_mouse;

    // Audio
    struct channel
    {
        channel();

        int16_t m_sfx;
        float m_offset, m_phi;
#if DEBUG_EXPORT_WAV
        FILE *m_fd;
#endif
    }
    m_channels[4];

    struct sfx const &get_sfx(int n) const;

    lol::Timer m_timer;
    fix32 m_seed;
    int m_instructions;
};

// Clamp a double to the nearest value that can be represented as a 16:16
// fixed point number (the ones used in PICO-8).
static inline int32_t double2fixed(double x)
{
    // This is more or less necessary because we use standard Lua with no
    // modifications, so we need a way to compute 1/0 or 0/0.
    if (std::isnan(x) || std::isinf(x))
        return x < 0 ? (int32_t)0x80000001 : (int32_t)0x7fffffff;
    return (int32_t)(int64_t)lol::round(x * double(1 << 16));
}

// Not Lua functions, but behave like them
static inline fix32 lua_tofix32(lua_State *l, int index)
{
    return fix32::frombits(double2fixed(lua_tonumber(l, index)));
}

static inline void lua_pushfix32(lua_State *l, fix32 const &x)
{
    return lua_pushnumber(l, (double)x);
}

} // namespace z8

