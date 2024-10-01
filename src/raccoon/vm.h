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

#include <optional>   // std::optional
#include <lol/vector> // lol::ivec2

#include "zepto8.h"
#include "player.h"
#include "raccoon/memory.h"

namespace z8::raccoon
{

class vm : z8::vm_base
{
    friend class z8::player;

public:
    vm();
    virtual ~vm();

    virtual void load(std::string const &file);
    virtual void run();
    virtual bool step(float seconds);

    virtual void render(lol::u8vec4 *screen) const;

    virtual std::string const &get_code() const;
    virtual u4mat2<128, 128> const & get_front_screen() const;
    virtual int get_ansi_color(uint8_t c) const;

    virtual std::function<void(void *, int)> get_streamer(int channel);

    virtual void button(int index, int state);
    virtual void mouse(lol::ivec2 coords, int buttons);
    virtual void text(char ch);

    virtual std::tuple<uint8_t *, size_t> ram();
    virtual std::tuple<uint8_t *, size_t> rom();

private:
    void js_wrap();

private:
    void api_debug(std::string s);

    int api_read(int p);
    void api_write(int p, int x);
    void api_palset(int n, int r, int g, int b);
    int api_fget(int n, std::optional<int> f);
    void api_fset(int n, int f, std::optional<int> v);
    int api_mget(int x, int y);
    void api_mset(int x, int y, int n);
    int api_pget(int x, int y);
    void api_pset(int x, int y, int c);

    void api_palm(int c0, int c1);
    void api_palt(int c, int v);
    bool api_btn(int i, std::optional<int> p);
    bool api_btnp(int i, std::optional<int> p);
    std::string api_btns(int i, std::optional<int> p);
    void api_cls(std::optional<int> c);
    void api_cam(int x, int y);
    void api_map(int celx, int cely, int sx, int sy, int celw, int celh);
    void api_line(int x0, int y0, int x1, int y1, int c);
    void api_circ(int x, int y, int r, int c);
    void api_circfill(int x, int y, int r, int c);
    void api_rect(int x, int y, int w, int h, int c);
    void api_rectfill(int x, int y, int w, int h, int c);
    void api_spr(int n, int x, int y,
                 std::optional<double> w, std::optional<double> h,
                 std::optional<int> fx, std::optional<int> fy);
    void api_print(int x, int y, std::string str, int c);
    double api_rnd(std::optional<double> x);
    double api_mid(double x, double y, double z);
    void api_mus(int n);
    void api_sfx(int a, int b, int c, int d);

public:
    // The API we export to extension languages
    template<typename T> struct exported_api
    {
        template<auto U> using bind = typename T::template bind<U>;

        std::vector<typename T::bind_desc> data =
        {
            { "debug",    bind<&vm::api_debug>() },

            { "read",     bind<&vm::api_read>() },
            { "write",    bind<&vm::api_write>() },

            { "palset",   bind<&vm::api_palset>() },
            { "fget",     bind<&vm::api_fget>() },
            { "fset",     bind<&vm::api_fset>() },
            { "mget",     bind<&vm::api_mget>() },
            { "mset",     bind<&vm::api_mset>() },

            { "cls",      bind<&vm::api_cls>() },
            { "cam",      bind<&vm::api_cam>() },
            { "map",      bind<&vm::api_map>() },
            { "palm",     bind<&vm::api_palm>() },
            { "palt",     bind<&vm::api_palt>() },
            { "pget",     bind<&vm::api_pget>() },
            { "pset",     bind<&vm::api_pset>() },
            { "spr",      bind<&vm::api_spr>() },
            { "line",     bind<&vm::api_line>() },
            { "circ",     bind<&vm::api_circ>() },
            { "circfill", bind<&vm::api_circfill>() },
            { "rect",     bind<&vm::api_rect>() },
            { "rectfill", bind<&vm::api_rectfill>() },
            { "print",    bind<&vm::api_print>() },

            { "rnd",      bind<&vm::api_rnd>() },
            { "mid",      bind<&vm::api_mid>() },
            { "mus",      bind<&vm::api_mus>() },
            { "sfx",      bind<&vm::api_sfx>() },
            { "btn",      bind<&vm::api_btn>() },
            { "btnp",     bind<&vm::api_btnp>() },
            { "btns",     bind<&vm::api_btns>() },
        };
    };

private:
    struct JSRuntime *m_rt;
    struct JSContext *m_ctx;

    std::string m_code;
    std::string m_name, m_link, m_host;
    int32_t m_version = -1;

    memory m_rom;
    memory m_ram;
};

}

