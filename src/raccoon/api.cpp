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

JSValue vm::api_read(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv)
{
    int p;
    if (JS_ToInt32(ctx, &p, argv[0]))
        return JS_EXCEPTION;
    return JS_NewInt32(ctx, m_ram[p & 0xffff]);
}

JSValue vm::api_write(JSContext *ctx, JSValueConst this_val,
                      int argc, JSValueConst *argv)
{
    int p, x;
    if (JS_ToInt32(ctx, &p, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &x, argv[1]))
        return JS_EXCEPTION;
    m_ram[p & 0xffff] = x;
    return JS_UNDEFINED;
}

JSValue vm::api_palset(JSContext *ctx, JSValueConst this_val,
                       int argc, JSValueConst *argv)
{
    int n, r, g, b;
    if (JS_ToInt32(ctx, &n, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &r, argv[1]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &g, argv[2]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &b, argv[3]))
        return JS_EXCEPTION;
    m_ram.palette[n & 0xf] = lol::u8vec3(r, g, b);
    return JS_UNDEFINED;
}

JSValue vm::api_pset(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv)
{
    int x, y, z;
    if (JS_ToInt32(ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &y, argv[1]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &z, argv[2]))
        return JS_EXCEPTION;
    lol::msg::info("stub: pset(%d, %d, %d)\n", x, y, z);
    return JS_NewInt32(ctx, x);
}

JSValue vm::api_palm(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv)
{
    int x, y;
    if (JS_ToInt32(ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &y, argv[1]))
        return JS_EXCEPTION;
    lol::msg::info("stub: palm(%d, %d)\n", x, y);
    return JS_UNDEFINED;
}

JSValue vm::api_palt(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv)
{
    int x, y;
    if (JS_ToInt32(ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &y, argv[1]))
        return JS_EXCEPTION;
    lol::msg::info("stub: palt(%d, %d)\n", x, y);
    return JS_NewInt32(ctx, x);
}

JSValue vm::api_btnp(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv)
{
    int x, y;
    if (JS_ToInt32(ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &y, argv[1]))
        return JS_EXCEPTION;
    lol::msg::info("stub: btnp(%d, %d)\n", x, y);
    return JS_NewInt32(ctx, x);
}

JSValue vm::api_fget(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv)
{
    int x;
    if (JS_ToInt32(ctx, &x, argv[0]))
        return JS_EXCEPTION;
    lol::msg::info("stub: fget(%d)\n", x);
    return JS_NewInt32(ctx, x);
}

JSValue vm::api_cls(JSContext *ctx, JSValueConst this_val,
                    int argc, JSValueConst *argv)
{
    int x;
    if (argc == 0 || JS_ToInt32(ctx, &x, argv[0]))
        x = 0;
    lol::msg::info("stub: cls(%d)\n", x);
    return JS_NewInt32(ctx, x);
}

JSValue vm::api_cam(JSContext *ctx, JSValueConst this_val,
                    int argc, JSValueConst *argv)
{
    int x, y;
    if (JS_ToInt32(ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &y, argv[1]))
        return JS_EXCEPTION;
    lol::msg::info("stub: cam(%d, %d)\n", x, y);
    return JS_NewInt32(ctx, x);
}

JSValue vm::api_map(JSContext *ctx, JSValueConst this_val,
                    int argc, JSValueConst *argv)
{
    int x, y, z, t, u, v;
    if (JS_ToInt32(ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &y, argv[1]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &z, argv[2]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &t, argv[3]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &u, argv[4]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &v, argv[5]))
        return JS_EXCEPTION;
    lol::msg::info("stub: map(%d, %d, %d, %d, %d, %d)\n", x, y, z, t, u, v);
    return JS_NewInt32(ctx, x);
}

JSValue vm::api_rect(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv)
{
    int x, y, z, t, u;
    if (JS_ToInt32(ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &y, argv[1]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &z, argv[2]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &t, argv[3]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &u, argv[4]))
        return JS_EXCEPTION;
    lol::msg::info("stub: rect(%d, %d, %d, %d, %d)\n", x, y, z, t, u);
    return JS_NewInt32(ctx, x);
}

JSValue vm::api_rectfill(JSContext *ctx, JSValueConst this_val,
                         int argc, JSValueConst *argv)
{
    int x, y, z, t, u;
    if (JS_ToInt32(ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &y, argv[1]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &z, argv[2]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &t, argv[3]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &u, argv[4]))
        return JS_EXCEPTION;
    lol::msg::info("stub: rectfill(%d, %d, %d, %d, %d)\n", x, y, z, t, u);
    return JS_NewInt32(ctx, x);
}

JSValue vm::api_spr(JSContext *ctx, JSValueConst this_val,
                    int argc, JSValueConst *argv)
{
    int x, y, z, t, u, v, w;
    if (JS_ToInt32(ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &y, argv[1]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &z, argv[2]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &t, argv[3]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &u, argv[4]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &v, argv[5]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &w, argv[5]))
        return JS_EXCEPTION;
    lol::msg::info("stub: spr(%d, %d, %d, %d, %d, %d, %d)\n", x, y, z, t, u, v, w);
    return JS_UNDEFINED;
}

JSValue vm::api_print(JSContext *ctx, JSValueConst this_val,
                      int argc, JSValueConst *argv)
{
    int x, y, z;
    if (JS_ToInt32(ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &y, argv[1]))
        return JS_EXCEPTION;
    char const *str = JS_ToCString(ctx, argv[2]);
    if (JS_ToInt32(ctx, &z, argv[3]))
        return JS_EXCEPTION;
    lol::msg::info("stub: print(%d, %d, %s, %d)\n", x, y, str, z);
    JS_FreeCString(ctx, str);
    return JS_UNDEFINED;
}

JSValue vm::api_rnd(JSContext *ctx, JSValueConst this_val,
                    int argc, JSValueConst *argv)
{
    double x;
    if (argc == 0 || JS_ToFloat64(ctx, &x, argv[0]))
        x = 1.0;
    return JS_NewFloat64(ctx, lol::rand(x));
}

JSValue vm::api_mid(JSContext *ctx, JSValueConst this_val,
                    int argc, JSValueConst *argv)
{
    double x, y, z, ret;
    if (JS_ToFloat64(ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &y, argv[1]))
        return JS_EXCEPTION;
    if (JS_ToFloat64(ctx, &z, argv[2]))
        return JS_EXCEPTION;
    ret = x > y ? y > z ? y : std::min(x, z)
                : x > z ? x : std::min(y, z);
    return JS_NewFloat64(ctx, ret);
}

JSValue vm::api_mget(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv)
{
    int x, y;
    if (JS_ToInt32(ctx, &x, argv[0]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &y, argv[1]))
        return JS_EXCEPTION;
    lol::msg::info("stub: mget(%d, %d)\n", x, y);
    return JS_NewInt32(ctx, x);
}

}

