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
    lua_pushnumber(l, lol::max(lua_tonumber(l, 1), lua_tonumber(l, 2)));
    return 1;
}

int vm::min(lol::LuaState *l)
{
    lua_pushnumber(l, lol::min(lua_tonumber(l, 1), lua_tonumber(l, 2)));
    return 1;
}

int vm::mid(lol::LuaState *l)
{
    auto x = lua_tonumber(l, 1);
    auto y = lua_tonumber(l, 2);
    auto z = lua_tonumber(l, 3);
    lua_pushnumber(l, x > y ? y > z ? y : lol::min(x, z)
                            : x > z ? x : lol::min(y, z));
    return 1;
}

int vm::flr(lol::LuaState *l)
{
    lua_pushnumber(l, lol::floor(float(lua_tonumber(l, 1))));
    return 1;
}

int vm::cos(lol::LuaState *l)
{
    lua_pushnumber(l, lol::cos(-lol::F_TAU * float(lua_tonumber(l, 1))));
    return 1;
}

int vm::sin(lol::LuaState *l)
{
    lua_pushnumber(l, lol::sin(-lol::F_TAU * float(lua_tonumber(l, 1))));
    return 1;
}

int vm::atan2(lol::LuaState *l)
{
    float x = lua_tonumber(l, 1);
    float y = lua_tonumber(l, 2);
    lua_pushnumber(l, lol::F_TAU * lol::atan2(-y, x));
    return 1;
}

int vm::sqrt(lol::LuaState *l)
{
    float x = lua_tonumber(l, 1);
    /* FIXME PICO-8 actually returns stuff for negative values */
    lua_pushnumber(l, x >= 0.f ? lol::sqrt(x) : 0.f);
    return 1;
}

int vm::abs(lol::LuaState *l)
{
    lua_pushnumber(l, lol::abs(float(lua_tonumber(l, 1))));
    return 1;
}

int vm::sgn(lol::LuaState *l)
{
    lua_pushnumber(l, lua_tonumber(l, 1) > 0 ? 1.f : -1.f);
    return 1;
}

int vm::rnd(lol::LuaState *l)
{
    lua_pushnumber(l, lol::rand(float(lua_tonumber(l, 1))));
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
    float x = lua_tonumber(l, 1);
    float y = lua_tonumber(l, 2);
    lua_pushnumber(l, (int16_t)x & (int16_t)y);
    return 1;
}

int vm::bor(lol::LuaState *l)
{
    float x = lua_tonumber(l, 1);
    float y = lua_tonumber(l, 2);
    lua_pushnumber(l, (int16_t)x | (int16_t)y);
    return 1;
}

int vm::bxor(lol::LuaState *l)
{
    float x = lua_tonumber(l, 1);
    float y = lua_tonumber(l, 2);
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
    float x = lua_tonumber(l, 1);
    float y = lua_tonumber(l, 2);
    lua_pushnumber(l, (int16_t)x << (int16_t)y);
    return 1;
}

int vm::shr(lol::LuaState *l)
{
    float x = lua_tonumber(l, 1);
    float y = lua_tonumber(l, 2);
    lua_pushnumber(l, (int16_t)x >> (int16_t)y);
    return 1;
}

} // namespace z8

