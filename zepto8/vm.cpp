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

#include <lol/engine.h>

#include "vm.h"

namespace z8
{

using lol::msg;

const lol::LuaObjectLib* vm::GetLib()
{
    static const lol::LuaObjectLib lib = lol::LuaObjectLib(
        "_z8",

        // Statics
        {
            { "btn",    &vm::btn },
            { "btnp",   &vm::btnp },
            { "cls",    &vm::cls },
            { "cos",    &vm::cos },
            { "cursor", &vm::cursor },
            { "flr",    &vm::flr },
            { "music",  &vm::music },
            { "pget",   &vm::pget },
            { "pset",   &vm::pset },
            { "rnd",    &vm::rnd },
            { "sget",   &vm::sget },
            { "sset",   &vm::sset },
            { "sin",    &vm::sin },
            { "spr",    &vm::spr },
            { "sspr",   &vm::sspr },
            { nullptr, nullptr }
        },

        // Methods
        {
            { nullptr, nullptr },
        },

        // Variables
        {
            { nullptr, nullptr, nullptr },
        });

    return &lib;
}

vm* vm::New(lol::LuaState* l, int argc)
{
    msg::info("requesting new(%d) on vm\n", argc);
}

int vm::btn(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x;
    lol::LuaBool ret;
    s >> x;
    ret = false;
    msg::info("z8:stub:btn(%d)\n", (int)x.GetValue());
    return s << ret;
}

int vm::btnp(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x;
    lol::LuaBool ret;
    s >> x;
    ret = false;
    msg::info("z8:stub:btnp(%d)\n", (int)x.GetValue());
    return s << ret;
}

int vm::cls(lol::LuaState *l)
{
    lol::LuaStack s(l);
    msg::info("z8:stub:cls()\n");
    return 0;
}

int vm::cos(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::cos(x.GetValue() / lol::F_TAU);
    return s << ret;
}

int vm::cursor(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y;
    s >> x >> y;
    msg::info("z8:stub:cursor(%f,%f)\n", x.GetValue(), y.GetValue());
    return 0;
}

int vm::flr(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::floor(x.GetValue());
    return s << ret;
}

int vm::music(lol::LuaState *l)
{
    lol::LuaStack s(l);
    msg::info("z8:stub:music()\n");
    return 0;
}

int vm::pget(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;
    ret = 0;
    msg::info("z8:stub:pget(%d, %d)\n", (int)x.GetValue(), (int)y.GetValue());
    return s << ret;
}

int vm::pset(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y;
    s >> x >> y;
    msg::info("z8:stub:pset(%d, %d...)\n", (int)x.GetValue(), (int)y.GetValue());
    return 0;
}

int vm::rnd(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::rand(x.GetValue());
    return s << ret;
}

int vm::sget(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;
    ret = 0;
    msg::info("z8:stub:sget(%d, %d)\n", (int)x.GetValue(), (int)y.GetValue());
    return s << ret;
}

int vm::sset(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y;
    s >> x >> y;
    msg::info("z8:stub:sset(%d, %d...)\n", (int)x.GetValue(), (int)y.GetValue());
    return 0;
}

int vm::sin(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::sin(x.GetValue() / -lol::F_TAU);
    return s << ret;
}

int vm::spr(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat n, x, y, w, h, flip_x, flip_y;
    s >> n >> x >> y >> w >> h >> flip_x >> flip_y;
    msg::info("z8:stub:spr(%d, %d, %d...)\n", (int)n, (int)x, (int)y);
    return 0;
}

int vm::sspr(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat sx, sy, sw, sh, dx, dy, dw, dh, flip_x, flip_y;
    s >> sx >> sy >> sw >> sh >> dx >> dy;
    msg::info("z8:stub:sspr(%d, %d, %d, %d, %d, %d...)\n", (int)sx, (int)sy, (int)sw, (int)sh, (int)dx, (int)dy);
    return 0;
}

} // namespace z8

