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
#include <variant>

#include "zepto8.h"
#include "bios.h"
#include "pico8/cart.h"
#include "pico8/memory.h"
#include "z8lua/lua.h"

namespace z8 { class player; }

namespace z8::pico8
{

using lol::u8vec4;

template<typename T> using opt = std::optional<T>;
template<typename... T> using var = std::variant<T...>;
template<typename... T> using tup = std::tuple<T...>;

struct rich_string : public std::string {};

class vm : z8::vm_base
{
    friend class z8::player;

public:
    vm();
    virtual ~vm();

    virtual void load(std::string const &name);
    virtual void run();
    virtual bool step(float seconds);

    virtual std::string const &get_code() const;

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
    void runtime_error(std::string str);
    static int panic_hook(struct lua_State *l);
    static void instruction_hook(struct lua_State *l, struct lua_Debug *ar);

    // Private methods (hidden from the user)
    opt<bool> private_cartdata(opt<std::string> str);
    void private_stub(std::string str);

    // System
    void api_run();
    void api_menuitem();
    void api_reload(int16_t in_dst, int16_t in_src, opt<int16_t> in_size);
    int16_t api_peek(int16_t addr);
    int16_t api_peek2(int16_t addr);
    fix32 api_peek4(int16_t addr);
    void api_poke(int16_t addr, int16_t val);
    void api_poke2(int16_t addr, int16_t val);
    void api_poke4(int16_t addr, fix32 val);
    void api_memcpy(int16_t dst, int16_t src, int16_t size);
    void api_memset(int16_t dst, uint8_t val, int16_t size);
    var<bool, int16_t, fix32, std::string, std::nullptr_t> api_stat(int16_t id);
    void api_printh(rich_string str, opt<std::string> filename, opt<bool> overwrite);
    void api_extcmd(std::string cmd);

    // I/O
    void api_update_buttons();
    var<bool, int16_t> api_btn(opt<int16_t> n, int16_t p);
    var<bool, int16_t> api_btnp(opt<int16_t> n, int16_t p);

    // Text
    tup<uint8_t, uint8_t> api_cursor(uint8_t x, uint8_t y, opt<uint8_t> c);
    void api_print(opt<rich_string> str, opt<fix32> x, opt<fix32> y, opt<fix32> c);

    // Graphics
    tup<int16_t, int16_t> api_camera(int16_t x, int16_t y);
    void api_circ(int16_t x, int16_t y, int16_t r, opt<fix32> c);
    void api_circfill(int16_t x, int16_t y, int16_t r, opt<fix32> c);
    void api_clip(int16_t x, int16_t y, int16_t w, opt<int16_t> h);
    void api_cls(uint8_t c);
    uint8_t api_color(uint8_t c);
    fix32 api_fillp(fix32 fillp);
    opt<var<int16_t, bool>> api_fget(opt<int16_t> n, opt<int16_t> f);
    void api_fset(opt<int16_t> n, opt<int16_t> f, opt<bool> b);
    void api_line(int16_t x0, opt<int16_t> opt_y0, opt<int16_t> opt_x1,
                  int16_t y1, opt<fix32> c);
    void api_map(int16_t cel_x, int16_t cel_y, int16_t sx, int16_t sy,
                 opt<int16_t> in_cel_w, opt<int16_t> in_cel_h, int16_t layer);
    fix32 api_mget(int16_t x, int16_t y);
    void api_mset(int16_t x, int16_t y, uint8_t n);
    opt<uint8_t> api_pal(opt<uint8_t> c0, opt<uint8_t> c1, uint8_t p);
    opt<bool> api_palt(opt<uint8_t> c, opt<uint8_t> t);
    fix32 api_pget(int16_t x, int16_t y);
    void api_pset(int16_t x, int16_t y, opt<fix32> c);
    void api_rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, opt<fix32> c);
    void api_rectfill(int16_t x0, int16_t y0, int16_t x1, int16_t y1, opt<fix32> c);
    int16_t api_sget(int16_t x, int16_t y);
    void api_sset(int16_t x, int16_t y, opt<int16_t> c);
    void api_spr(int16_t n, int16_t x, int16_t y, opt<fix32> w,
                 opt<fix32> h, bool flip_x, bool flip_y);
    void api_sspr(int16_t sx, int16_t sy, int16_t sw, int16_t sh,
                  int16_t dx, int16_t dy, opt<int16_t> in_dw,
                  opt<int16_t> in_dh, bool flip_x, bool flip_y);

    // Sound
    void api_music(int16_t pattern, int16_t fade_len, int16_t mask);
    void api_sfx(int16_t sfx, opt<int16_t> in_chan, int16_t offset);

    // Deprecated
    fix32 api_time();

public:
    // The API we export to extension languages
    template<typename T> struct api
    {
        template<auto U> using bind = typename T::template bind<U>;

        std::vector<typename T::bind_desc> data =
        {
            { "run",      bind<&vm::api_run>() },
            { "menuitem", bind<&vm::api_menuitem>() },
            { "reload",   bind<&vm::api_reload>() },
            { "peek",     bind<&vm::api_peek>() },
            { "peek2",    bind<&vm::api_peek2>() },
            { "peek4",    bind<&vm::api_peek4>() },
            { "poke",     bind<&vm::api_poke>() },
            { "poke2",    bind<&vm::api_poke2>() },
            { "poke4",    bind<&vm::api_poke4>() },
            { "memcpy",   bind<&vm::api_memcpy>() },
            { "memset",   bind<&vm::api_memset>() },
            { "stat",     bind<&vm::api_stat>() },
            { "printh",   bind<&vm::api_printh>() },
            { "extcmd",   bind<&vm::api_extcmd>() },

            { "_update_buttons", bind<&vm::api_update_buttons>() },
            { "btn",  bind<&vm::api_btn>() },
            { "btnp", bind<&vm::api_btnp>() },

            { "cursor", bind<&vm::api_cursor>() },
            { "print",  bind<&vm::api_print>() },

            { "camera",   bind<&vm::api_camera>() },
            { "circ",     bind<&vm::api_circ>() },
            { "circfill", bind<&vm::api_circfill>() },
            { "clip",     bind<&vm::api_clip>() },
            { "cls",      bind<&vm::api_cls>() },
            { "color",    bind<&vm::api_color>() },
            { "fillp",    bind<&vm::api_fillp>() },
            { "fget",     bind<&vm::api_fget>() },
            { "fset",     bind<&vm::api_fset>() },
            { "line",     bind<&vm::api_line>() },
            { "map",      bind<&vm::api_map>() },
            { "mget",     bind<&vm::api_mget>() },
            { "mset",     bind<&vm::api_mset>() },
            { "pal",      bind<&vm::api_pal>() },
            { "palt",     bind<&vm::api_palt>() },
            { "pget",     bind<&vm::api_pget>() },
            { "pset",     bind<&vm::api_pset>() },
            { "rect",     bind<&vm::api_rect>() },
            { "rectfill", bind<&vm::api_rectfill>() },
            { "sget",     bind<&vm::api_sget>() },
            { "sset",     bind<&vm::api_sset>() },
            { "spr",      bind<&vm::api_spr>() },
            { "sspr",     bind<&vm::api_sspr>() },

            { "music", bind<&vm::api_music>() },
            { "sfx",   bind<&vm::api_sfx>() },

            { "time", bind<&vm::api_time>() },

            { "__cartdata", bind<&vm::private_cartdata>() },
            { "__stub",     bind<&vm::private_stub>() },
        };
    };

private:
    uint8_t get_pixel(int16_t x, int16_t y) const;

    uint32_t to_color_bits(opt<fix32> c);

    void set_pixel(int16_t x, int16_t y, uint32_t color_bits);

    void hline(int16_t x1, int16_t x2, int16_t y, uint32_t color_bits);
    void vline(int16_t x, int16_t y1, int16_t y2, uint32_t color_bits);

    void getaudio(int channel, void *buffer, int bytes);

public:
    // TODO: try to get rid of this
    struct lua_State *m_sandbox_lua;

private:
    struct lua_State *m_lua;
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
    int m_instructions = 0;
};

} // namespace z8::pico8

