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

    // Emulate the official PICO-8 behaviour (as of 0.1.9)
    if (lol::abs(x) == 1.0 && y == -32768.0)
    {
        msg::debug("\x1b[93;41mcrashing due to atan2(%g,%g) call (LOL)\x1b[0m\n", x, y);
        lol::Ticker::Shutdown();
        lua_pushstring(l, "PICO-8 atan2() bug");
        lua_error(l);
        return 0;
    }

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
    vm *that = (vm *)vm::Find(l);
    uint32_t x = ((uint64_t)that->m_seed * 279470273ul) % 4294967291ul;
    that->m_seed = x;

    /* FIXME: behaves incorrectly when b is negative */
    double a = 0.f;
    double b = lua_isnone(l, 1) ? 1.0
             : clamp64(lol::min(lua_tonumber(l, 1), 32767.99));
    double c = lol::mix(a, b, x / (1.0 + (uint32_t)-1));
    lua_pushnumber(l, clamp64(c));
    return 1;
}

int vm::srand(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);
    that->m_seed = double2fixed(lua_tonumber(l, 1));
    if (that->m_seed == 0)
        that->m_seed = 0xdeadbeef;
    return 0;
}

int vm::band(lol::LuaState *l)
{
    int32_t x = double2fixed(lua_tonumber(l, 1));
    int32_t y = double2fixed(lua_tonumber(l, 2));
    lua_pushnumber(l, fixed2double(x & y));
    return 1;
}

int vm::bor(lol::LuaState *l)
{
    int32_t x = double2fixed(lua_tonumber(l, 1));
    int32_t y = double2fixed(lua_tonumber(l, 2));
    lua_pushnumber(l, fixed2double(x | y));
    return 1;
}

int vm::bxor(lol::LuaState *l)
{
    int32_t x = double2fixed(lua_tonumber(l, 1));
    int32_t y = double2fixed(lua_tonumber(l, 2));
    lua_pushnumber(l, fixed2double(x ^ y));
    return 1;
}

int vm::bnot(lol::LuaState *l)
{
    int32_t x = double2fixed(lua_tonumber(l, 1));
    lua_pushnumber(l, fixed2double(~x));
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

