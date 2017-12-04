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

int vm::api::max(lua_State *l)
{
    double x = lua_toclamp64(l, 1);
    double y = lua_toclamp64(l, 2);
    lua_pushnumber(l, lol::max(x, y));
    return 1;
}

int vm::api::min(lua_State *l)
{
    double x = lua_toclamp64(l, 1);
    double y = lua_toclamp64(l, 2);
    lua_pushnumber(l, lol::min(x, y));
    return 1;
}

int vm::api::mid(lua_State *l)
{
    double x = lua_toclamp64(l, 1);
    double y = lua_toclamp64(l, 2);
    double z = lua_toclamp64(l, 3);
    lua_pushnumber(l, x > y ? y > z ? y : lol::min(x, z)
                            : x > z ? x : lol::min(y, z));
    return 1;
}

int vm::api::ceil(lua_State *l)
{
    lua_pushnumber(l, lol::ceil(lua_toclamp64(l, 1)));
    return 1;
}

int vm::api::flr(lua_State *l)
{
    lua_pushnumber(l, lol::floor(lua_toclamp64(l, 1)));
    return 1;
}

int vm::api::cos(lua_State *l)
{
    double x = lua_toclamp64(l, 1);
    lua_pushnumber(l, clamp64(lol::cos(-lol::D_TAU * x)));
    return 1;
}

int vm::api::sin(lua_State *l)
{
    double x = lua_toclamp64(l, 1);
    lua_pushnumber(l, clamp64(lol::sin(-lol::D_TAU * x)));
    return 1;
}

int vm::api::atan2(lua_State *l)
{
    double x = lua_toclamp64(l, 1);
    double y = lua_toclamp64(l, 2);

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

int vm::api::sqrt(lua_State *l)
{
    double x = lua_toclamp64(l, 1);
    /* FIXME PICO-8 actually returns stuff for negative values */
    lua_pushnumber(l, x >= 0 ? clamp64(lol::sqrt(x)) : 0);
    return 1;
}

int vm::api::abs(lua_State *l)
{
    lua_pushnumber(l, clamp64(lol::abs(lua_toclamp64(l, 1))));
    return 1;
}

int vm::api::sgn(lua_State *l)
{
    lua_pushnumber(l, lua_toclamp64(l, 1) >= 0.0 ? 1.0 : -1.0);
    return 1;
}

int vm::api::rnd(lua_State *l)
{
    vm *that = get_this(l);
    uint32_t x = ((uint64_t)that->m_seed * 279470273ul) % 4294967291ul;
    that->m_seed = x;

    /* FIXME: behaves incorrectly when b is negative */
    double a = 0.f;
    double b = lua_isnone(l, 1) ? 1.0
             : clamp64(lol::min(lua_toclamp64(l, 1), 32767.99));
    double c = lol::mix(a, b, x / (1.0 + (uint32_t)-1));
    lua_pushnumber(l, clamp64(c));
    return 1;
}

int vm::api::srand(lua_State *l)
{
    vm *that = get_this(l);
    that->m_seed = double2fixed(lua_tonumber(l, 1));
    if (that->m_seed == 0)
        that->m_seed = 0xdeadbeef;
    return 0;
}

int vm::api::band(lua_State *l)
{
    int32_t x = double2fixed(lua_tonumber(l, 1));
    int32_t y = double2fixed(lua_tonumber(l, 2));
    lua_pushnumber(l, fixed2double(x & y));
    return 1;
}

int vm::api::bor(lua_State *l)
{
    int32_t x = double2fixed(lua_tonumber(l, 1));
    int32_t y = double2fixed(lua_tonumber(l, 2));
    lua_pushnumber(l, fixed2double(x | y));
    return 1;
}

int vm::api::bxor(lua_State *l)
{
    int32_t x = double2fixed(lua_tonumber(l, 1));
    int32_t y = double2fixed(lua_tonumber(l, 2));
    lua_pushnumber(l, fixed2double(x ^ y));
    return 1;
}

int vm::api::bnot(lua_State *l)
{
    int32_t x = double2fixed(lua_tonumber(l, 1));
    lua_pushnumber(l, fixed2double(~x));
    return 1;
}

int vm::api::shl(lua_State *l)
{
    // PICO-8 seems to use y modulo 32
    int32_t x = double2fixed(lua_tonumber(l, 1));
    int32_t y = int(lua_toclamp64(l, 2)) & 0x1f;
    lua_pushnumber(l, fixed2double(x << y));
    return 1;
}

int vm::api::shr(lua_State *l)
{
    // PICO-8 seems to use y modulo 32
    int32_t x = double2fixed(lua_tonumber(l, 1));
    int32_t y = int(lua_toclamp64(l, 2)) & 0x1f;
    lua_pushnumber(l, fixed2double(x >> y));
    return 1;
}

} // namespace z8

