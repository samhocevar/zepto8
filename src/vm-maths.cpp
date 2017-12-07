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
    lua_pushfix32(l, fix32::max(lua_tofix32(l, 1), lua_tofix32(l, 2)));
    return 1;
}

int vm::api::min(lua_State *l)
{
    lua_pushfix32(l, fix32::min(lua_tofix32(l, 1), lua_tofix32(l, 2)));
    return 1;
}

int vm::api::mid(lua_State *l)
{
    fix32 x = lua_tofix32(l, 1);
    fix32 y = lua_tofix32(l, 2);
    fix32 z = lua_tofix32(l, 3);
    lua_pushfix32(l, x > y ? y > z ? y : fix32::min(x, z)
                           : x > z ? x : fix32::min(y, z));
    return 1;
}

int vm::api::ceil(lua_State *l)
{
    lua_pushfix32(l, fix32::ceil(lua_tofix32(l, 1)));
    return 1;
}

int vm::api::flr(lua_State *l)
{
    lua_pushfix32(l, fix32::floor(lua_tofix32(l, 1)));
    return 1;
}

int vm::api::cos(lua_State *l)
{
    fix32 x = lua_tofix32(l, 1);
    lua_pushfix32(l, fix32(lol::cos(-lol::D_TAU * (double)x)));
    return 1;
}

int vm::api::sin(lua_State *l)
{
    fix32 x = lua_tofix32(l, 1);
    lua_pushfix32(l, fix32(lol::sin(-lol::D_TAU * (double)x)));
    return 1;
}

int vm::api::atan2(lua_State *l)
{
    fix32 x = lua_tofix32(l, 1);
    fix32 y = lua_tofix32(l, 2);

    // This could simply be atan2(-y,x) but since PICO-8 decided that
    // atan2(0,0) = 0.75 we need to do the same in our version.
    double a = 0.75 + lol::atan2((double)x, (double)y) / lol::D_TAU;
    lua_pushfix32(l, fix32(a >= 1 ? a - 1 : a));
    return 1;
}

int vm::api::sqrt(lua_State *l)
{
    fix32 x = lua_tofix32(l, 1);
    /* FIXME PICO-8 actually returns stuff for negative values */
    lua_pushfix32(l, fix32(x >= fix32(0.0) ? lol::sqrt((double)x) : 0));
    return 1;
}

int vm::api::abs(lua_State *l)
{
    lua_pushfix32(l, fix32::abs(lua_tofix32(l, 1)));
    return 1;
}

int vm::api::sgn(lua_State *l)
{
    fix32 x = lua_tofix32(l, 1);
    lua_pushfix32(l, fix32(x >= fix32(0.0) ? 1.0 : -1.0));
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
             : (double)fix32::min(lua_tofix32(l, 1), fix32::frombits(0x7ffffff));
    double c = lol::mix(a, b, x / (1.0 + (uint32_t)-1));
    lua_pushfix32(l, fix32(c));
    return 1;
}

int vm::api::srand(lua_State *l)
{
    vm *that = get_this(l);
    that->m_seed = lua_tofix32(l, 1).bits();
    if (that->m_seed == 0)
        that->m_seed = 0xdeadbeef;
    return 0;
}

int vm::api::band(lua_State *l)
{
    lua_pushfix32(l, lua_tofix32(l, 1) & lua_tofix32(l, 2));
    return 1;
}

int vm::api::bor(lua_State *l)
{
    lua_pushfix32(l, lua_tofix32(l, 1) | lua_tofix32(l, 2));
    return 1;
}

int vm::api::bxor(lua_State *l)
{
    lua_pushfix32(l, lua_tofix32(l, 1) ^ lua_tofix32(l, 2));
    return 1;
}

int vm::api::bnot(lua_State *l)
{
    lua_pushfix32(l, ~lua_tofix32(l, 1));
    return 1;
}

int vm::api::shl(lua_State *l)
{
    // If y is >= 32, result is always zero.
    int32_t xbits = lua_tofix32(l, 1).bits();
    int y = (int)lua_tofix32(l, 2);
    lua_pushfix32(l, fix32::frombits(xbits << y));
    return 1;
}

int vm::api::shr(lua_State *l)
{
    // If y is >= 32, only the sign is preserved, so it's
    // the same as for y == 31.
    int32_t xbits = lua_tofix32(l, 1).bits();
    int y = (int)lua_tofix32(l, 2);
    lua_pushfix32(l, fix32::frombits(xbits >> std::min(y, 31)));
    return 1;
}

int vm::api::lshr(lua_State *l)
{
    int32_t xbits = lua_tofix32(l, 1).bits();
    int y = (int)lua_tofix32(l, 2);
    lua_pushfix32(l, fix32::frombits((uint32_t)xbits >> y));
    return 1;
}

int vm::api::rotl(lua_State *l)
{
    int32_t xbits = lua_tofix32(l, 1).bits();
    int y = 0x1f & (int)lua_tofix32(l, 2);
    lua_pushfix32(l, fix32::frombits((xbits << y) | (xbits >> (32 - y))));
    return 1;
}

int vm::api::rotr(lua_State *l)
{
    int32_t xbits = lua_tofix32(l, 1).bits();
    int y = 0x1f & (int)lua_tofix32(l, 2);
    lua_pushfix32(l, fix32::frombits((xbits >> y) | (xbits << (32 - y))));
    return 1;
}

} // namespace z8

