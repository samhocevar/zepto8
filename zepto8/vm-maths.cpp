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

int vm::max(lol::LuaState *l)
{
    double x = clamp64(lua_tonumber(l, 1));
    double y = clamp64(lua_tonumber(l, 2));
    lua_pushnumber(l, lol::max(x, y));
    return 1;
}

int vm::min(lol::LuaState *l)
{
    double x = clamp64(lua_tonumber(l, 1));
    double y = clamp64(lua_tonumber(l, 2));
    lua_pushnumber(l, lol::min(x, y));
    return 1;
}

int vm::mid(lol::LuaState *l)
{
    double x = clamp64(lua_tonumber(l, 1));
    double y = clamp64(lua_tonumber(l, 2));
    double z = clamp64(lua_tonumber(l, 3));
    lua_pushnumber(l, x > y ? y > z ? y : lol::min(x, z)
                            : x > z ? x : lol::min(y, z));
    return 1;
}

int vm::flr(lol::LuaState *l)
{
    lua_pushnumber(l, lol::floor(clamp64(lua_tonumber(l, 1))));
    return 1;
}

int vm::cos(lol::LuaState *l)
{
    double x = clamp64(lua_tonumber(l, 1));
    lua_pushnumber(l, clamp64(lol::cos(-lol::D_TAU * clamp64(x))));
    return 1;
}

int vm::sin(lol::LuaState *l)
{
    double x = clamp64(lua_tonumber(l, 1));
    lua_pushnumber(l, clamp64(lol::sin(-lol::D_TAU * clamp64(x))));
    return 1;
}

int vm::atan2(lol::LuaState *l)
{
    double x = clamp64(lua_tonumber(l, 1));
    double y = clamp64(lua_tonumber(l, 2));
    // This could simply be atan2(-y,x) but since PICO-8 decided that
    // atan2(0,0) = 0.75 we need to do the same in our version.
    double a = 0.75 + clamp64(lol::atan2(x, y) / lol::D_TAU);
    lua_pushnumber(l, a >= 1 ? a - 1 : a);
    return 1;
}

int vm::sqrt(lol::LuaState *l)
{
    double x = clamp64(lua_tonumber(l, 1));
    /* FIXME PICO-8 actually returns stuff for negative values */
    lua_pushnumber(l, x >= 0 ? lol::sqrt(x) : 0);
    return 1;
}

int vm::abs(lol::LuaState *l)
{
    lua_pushnumber(l, lol::abs(clamp64(lua_tonumber(l, 1))));
    return 1;
}

int vm::sgn(lol::LuaState *l)
{
    lua_pushnumber(l, lua_tonumber(l, 1) >= 0.0 ? 1.0 : -1.0);
    return 1;
}

int vm::rnd(lol::LuaState *l)
{
    /* FIXME: behaves incorrectly when b is negative */
    double a = 0.f;
    double b = lua_isnone(l, 1) ? 1.0
             : clamp64(lol::min(lua_tonumber(l, 1), 32767.99));
    lua_pushnumber(l, clamp64(lol::rand(a, b)));
    return 1;
}

int vm::srand(lol::LuaState *l)
{
    /* FIXME: we do nothing; is this right? */
    lua_tonumber(l, 1);
    return 0;
}

int vm::band(lol::LuaState *l)
{
    double x = lua_tonumber(l, 1);
    double y = lua_tonumber(l, 2);
    lua_pushnumber(l, (int16_t)x & (int16_t)y);
    return 1;
}

int vm::bor(lol::LuaState *l)
{
    double x = lua_tonumber(l, 1);
    double y = lua_tonumber(l, 2);
    lua_pushnumber(l, (int16_t)x | (int16_t)y);
    return 1;
}

int vm::bxor(lol::LuaState *l)
{
    double x = lua_tonumber(l, 1);
    double y = lua_tonumber(l, 2);
    lua_pushnumber(l, (int16_t)x ^ (int16_t)y);
    return 1;
}

int vm::bnot(lol::LuaState *l)
{
    lua_pushnumber(l, ~uint16_t(lua_tonumber(l, 1)));
    return 1;
}

int vm::shl(lol::LuaState *l)
{
    double x = lua_tonumber(l, 1);
    double y = lua_tonumber(l, 2);
    lua_pushnumber(l, (int16_t)x << (int16_t)y);
    return 1;
}

int vm::shr(lol::LuaState *l)
{
    double x = lua_tonumber(l, 1);
    double y = lua_tonumber(l, 2);
    lua_pushnumber(l, (int16_t)x >> (int16_t)y);
    return 1;
}

} // namespace z8

