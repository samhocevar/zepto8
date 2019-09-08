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

#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <lol/engine.h>

extern "C" {
#include "quickjs/quickjs.h"
}

#include "zepto8.h"
#include "raccoon/vm.h"

namespace z8::raccoon
{

template<typename T> static void setpixel(T &data, int x, int y, uint8_t c)
{
    uint8_t &p = data[y][x / 2];
    p = p & (x & 1 ? 0x0f : 0xf0) | (x & 1 ? c << 4 : c);
}

template<typename T> static uint8_t getpixel(T const &data, int x, int y)
{
    uint8_t const &p = data[y][x / 2];
    return (x & 1 ? p >> 4 : p) & 0xf;
}

JSValue vm::api_read(int argc, JSValueConst *argv)
{
    int p;
    if (JS_ToInt32(m_ctx, &p, argv[0]))
        return JS_EXCEPTION;
    return JS_NewInt32(m_ctx, m_ram[p & 0xffff]);
}

JSValue vm::api_write(int argc, JSValueConst *argv)
{
    int p, x;
    if (JS_ToInt32(m_ctx, &p, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &x, argv[1]))
        return JS_EXCEPTION;
    m_ram[p & 0xffff] = x;
    return JS_UNDEFINED;
}

JSValue vm::api_palset(int argc, JSValueConst *argv)
{
    int n, r, g, b;
    if (JS_ToInt32(m_ctx, &n, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &r, argv[1]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &g, argv[2]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &b, argv[3]))
        return JS_EXCEPTION;
    m_ram.palette[n & 0xf] = lol::u8vec3(r, g, b);
    return JS_UNDEFINED;
}

JSValue vm::api_pset(int argc, JSValueConst *argv)
{
    int x, y, c;
    if (JS_ToInt32(m_ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &y, argv[1]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &c, argv[2]))
        return JS_EXCEPTION;
    if (x < 0 || x >= 128 || y < 0 || y >= 64)
        return JS_UNDEFINED;
    setpixel(m_ram.screen, x, y, c & 15);
    return JS_UNDEFINED;
}

JSValue vm::api_palm(int argc, JSValueConst *argv)
{
    int c0, c1;
    if (JS_ToInt32(m_ctx, &c0, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &c1, argv[1]))
        return JS_EXCEPTION;
	m_ram.palmod[c0 & 0xf] = c1 & 0xf;
    return JS_UNDEFINED;
}

JSValue vm::api_palt(int argc, JSValueConst *argv)
{
    int x, y;
    if (JS_ToInt32(m_ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &y, argv[1]))
        return JS_EXCEPTION;
    lol::msg::info("stub: palt(%d, %d)\n", x, y);
    return JS_NewInt32(m_ctx, x);
}

JSValue vm::api_btnp(int argc, JSValueConst *argv)
{
    int x, y;
    if (JS_ToInt32(m_ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &y, argv[1]))
        return JS_EXCEPTION;
    lol::msg::info("stub: btnp(%d, %d)\n", x, y);
    return JS_NewInt32(m_ctx, x);
}

JSValue vm::api_fget(int argc, JSValueConst *argv)
{
    int n, f;
    if (JS_ToInt32(m_ctx, &n, argv[0]))
        return JS_EXCEPTION;
    if (n < 0 || n >= 192)
        return JS_UNDEFINED;
    uint8_t field = m_ram.flags[n];
    if (argc == 1)
        return JS_NewInt32(m_ctx, field);
    if (JS_ToInt32(m_ctx, &f, argv[1]))
        return JS_EXCEPTION;
    return JS_NewInt32(m_ctx, (field >> f) & 0x1);
}

JSValue vm::api_fset(int argc, JSValueConst *argv)
{
    int n, f, v;
    if (JS_ToInt32(m_ctx, &n, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &f, argv[1]))
        return JS_EXCEPTION;
    if (n < 0 || n >= 192)
        return JS_UNDEFINED;
    if (argc == 3)
    {
        if (JS_ToInt32(m_ctx, &v, argv[2]))
            return JS_EXCEPTION;
        uint8_t mask = 1 << f;
        f = (m_ram.flags[n] & ~mask) | (v ? mask : 0);
    }
    m_ram.flags[n] = f;
    return JS_UNDEFINED;
}

JSValue vm::api_cls(int argc, JSValueConst *argv)
{
    int c;
    if (argc == 0 || JS_ToInt32(m_ctx, &c, argv[0]))
        c = 0;
    memset(m_ram.screen, (c & 15) * 0x11, sizeof(m_ram.screen));
    return JS_UNDEFINED;
}

JSValue vm::api_cam(int argc, JSValueConst *argv)
{
    int x, y;
    if (JS_ToInt32(m_ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &y, argv[1]))
        return JS_EXCEPTION;
    lol::msg::info("stub: cam(%d, %d)\n", x, y);
    return JS_NewInt32(m_ctx, x);
}

JSValue vm::api_map(int argc, JSValueConst *argv)
{
    int x, y, z, t, u, v;
    if (JS_ToInt32(m_ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &y, argv[1]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &z, argv[2]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &t, argv[3]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &u, argv[4]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &v, argv[5]))
        return JS_EXCEPTION;
    lol::msg::info("stub: map(%d, %d, %d, %d, %d, %d)\n", x, y, z, t, u, v);
    return JS_NewInt32(m_ctx, x);
}

JSValue vm::api_rect(int argc, JSValueConst *argv)
{
    int x, y, z, t, u;
    if (JS_ToInt32(m_ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &y, argv[1]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &z, argv[2]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &t, argv[3]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &u, argv[4]))
        return JS_EXCEPTION;
    lol::msg::info("stub: rect(%d, %d, %d, %d, %d)\n", x, y, z, t, u);
    return JS_NewInt32(m_ctx, x);
}

JSValue vm::api_rectfill(int argc, JSValueConst *argv)
{
    int x, y, w, h, c;
    if (JS_ToInt32(m_ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &y, argv[1]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &w, argv[2]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &h, argv[3]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &c, argv[4]))
        return JS_EXCEPTION;
    int x0 = std::max(x, 0);
    int x1 = std::min(x + w, 127);
    int y0 = std::max(y, 0);
    int y1 = std::min(y + h, 127);
    for (y = y0; y <= y1; ++y)
        for (x = x0; x <= x1; ++x)
            setpixel(m_ram.screen, x, y, c & 15);
    return JS_NewInt32(m_ctx, x);
}

JSValue vm::api_spr(int argc, JSValueConst *argv)
{
    int n, x, y, fx, fy;
    double w, h;
    if (JS_ToInt32(m_ctx, &n, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &x, argv[1]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &y, argv[2]))
        return JS_EXCEPTION;
    if (JS_ToFloat64(m_ctx, &w, argv[3]))
        return JS_EXCEPTION;
    if (JS_ToFloat64(m_ctx, &h, argv[4]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &fx, argv[5]))
        fx = 0;
    if (JS_ToInt32(m_ctx, &fy, argv[6]))
        fy = 0;
    int sx = n % 16 * 8, sy = n / 16 * 8;
    int sw = (int)(w * 8), sh = (int)(h * 8);
    for (int dy = 0; dy < sh; ++dy)
        for (int dx = 0; dx < sw; ++dx)
        {
            auto c = getpixel(m_ram.sprites,
                              fx ? sx + sw - 1 - dx : sx + dx,
                              fy ? sy + sh - 1 - dy : sy + dy);
            c = m_ram.palmod[c] & 0xf;
            setpixel(m_ram.screen, x + dx, y + dy, c);
        }
    return JS_UNDEFINED;
}

JSValue vm::api_print(int argc, JSValueConst *argv)
{
    int x, y, z;
    if (JS_ToInt32(m_ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &y, argv[1]))
        return JS_EXCEPTION;
    char const *str = JS_ToCString(m_ctx, argv[2]);
    if (JS_ToInt32(m_ctx, &z, argv[3]))
        return JS_EXCEPTION;
    lol::msg::info("stub: print(%d, %d, %s, %d)\n", x, y, str, z);
    JS_FreeCString(m_ctx, str);
    return JS_UNDEFINED;
}

JSValue vm::api_rnd(int argc, JSValueConst *argv)
{
    double x;
    if (argc == 0 || JS_ToFloat64(m_ctx, &x, argv[0]))
        x = 1.0;
    return JS_NewFloat64(m_ctx, lol::rand(x));
}

JSValue vm::api_mid(int argc, JSValueConst *argv)
{
    double x, y, z, ret;
    if (JS_ToFloat64(m_ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToFloat64(m_ctx, &y, argv[1]))
        return JS_EXCEPTION;
    if (JS_ToFloat64(m_ctx, &z, argv[2]))
        return JS_EXCEPTION;
    ret = x > y ? y > z ? y : std::min(x, z)
                : x > z ? x : std::min(y, z);
    return JS_NewFloat64(m_ctx, ret);
}

JSValue vm::api_mget(int argc, JSValueConst *argv)
{
    int x, y;
    if (JS_ToInt32(m_ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &y, argv[1]))
        return JS_EXCEPTION;
    if (x < 0 || x >= 128 || y < 0 || y >= 64)
        return JS_UNDEFINED;
    return JS_NewInt32(m_ctx, m_ram.map[y][x]);
}

JSValue vm::api_mset(int argc, JSValueConst *argv)
{
    int x, y, n;
    if (JS_ToInt32(m_ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &y, argv[1]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &n, argv[2]))
        return JS_EXCEPTION;
    if (x < 0 || x >= 128 || y < 0 || y >= 64)
        return JS_UNDEFINED;
    m_ram.map[y][x] = n;
    return JS_UNDEFINED;
}

JSValue vm::api_mus(int argc, JSValueConst *argv)
{
    int n;
    if (JS_ToInt32(m_ctx, &n, argv[0]))
        return JS_EXCEPTION;
    lol::msg::info("stub: mus(%d)\n", n);
    return JS_UNDEFINED;
}

}

