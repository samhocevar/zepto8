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
    int x, y, z;
    if (JS_ToInt32(m_ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &y, argv[1]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &z, argv[2]))
        return JS_EXCEPTION;
    lol::msg::info("stub: pset(%d, %d, %d)\n", x, y, z);
    return JS_NewInt32(m_ctx, x);
}

JSValue vm::api_palm(int argc, JSValueConst *argv)
{
    int x, y;
    if (JS_ToInt32(m_ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(m_ctx, &y, argv[1]))
        return JS_EXCEPTION;
    lol::msg::info("stub: palm(%d, %d)\n", x, y);
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
    int x;
    if (JS_ToInt32(m_ctx, &x, argv[0]))
        return JS_EXCEPTION;
    lol::msg::info("stub: fget(%d)\n", x);
    return JS_NewInt32(m_ctx, x);
}

JSValue vm::api_cls(int argc, JSValueConst *argv)
{
    int x;
    if (argc == 0 || JS_ToInt32(m_ctx, &x, argv[0]))
        x = 0;
    lol::msg::info("stub: cls(%d)\n", x);
    return JS_NewInt32(m_ctx, x);
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
    lol::msg::info("stub: rectfill(%d, %d, %d, %d, %d)\n", x, y, z, t, u);
    return JS_NewInt32(m_ctx, x);
}

JSValue vm::api_spr(int argc, JSValueConst *argv)
{
    int x, y, z, t, u, v, w;
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
    if (JS_ToInt32(m_ctx, &w, argv[5]))
        return JS_EXCEPTION;
    lol::msg::info("stub: spr(%d, %d, %d, %d, %d, %d, %d)\n", x, y, z, t, u, v, w);
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
    lol::msg::info("stub: mget(%d, %d)\n", x, y);
    return JS_NewInt32(m_ctx, x);
}

}

