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
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;
    ret = lol::max((float)x, (float)y);
    return s << ret;
}

int vm::min(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;
    ret = lol::min((float)x, (float)y);
    return s << ret;
}

int vm::mid(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, z, ret;
    s >> x >> y >> z;
    ret = (float)x > (float)y ? (float)y > (float)z ? (float)y
                                                    : lol::min((float)x, (float)z)
                              : (float)x > (float)z ? (float)x
                                                    : lol::min((float)y, (float)z);
    return s << ret;
}

int vm::flr(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::floor((float)x);
    return s << ret;
}

int vm::cos(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::cos((float)x * -lol::F_TAU);
    return s << ret;
}

int vm::sin(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::sin((float)x * -lol::F_TAU);
    return s << ret;
}

int vm::atan2(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;
    ret = lol::F_TAU * lol::atan2(-(float)y, (float)x);
    return s << ret;
}

int vm::sqrt(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    /* FIXME PICO-8 actually returns stuff for negative values */
    ret = (float)x >= 0.f ? lol::sqrt((float)x) : 0.f;
    return s << ret;
}

int vm::abs(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::abs((float)x);
    return s << ret;
}

int vm::sgn(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = (float)x >= 0.f ? 1.f : 0.f;
    return s << ret;
}

int vm::rnd(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::rand((float)x);
    return s << ret;
}

int vm::srand(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x;
    s >> x;
    /* FIXME: we do nothing; is this right? */
    return 0;
}

int vm::band(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;
    ret = (float)((int16_t)(float)x & (int16_t)(float)y);
    return s << ret;
}

int vm::bor(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;
    ret = (float)((int16_t)(float)x | (int16_t)(float)y);
    return s << ret;
}

int vm::bxor(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;
    ret = (float)((int16_t)(float)x ^ (int16_t)(float)y);
    return s << ret;
}

int vm::bnot(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = (float)~(int16_t)(float)x;
    return s << ret;
}

int vm::shl(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;
    ret = (float)((uint16_t)(float)x << (int16_t)(float)y);
    return s << ret;
}

int vm::shr(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;
    ret = (float)((uint16_t)(float)x >> (int16_t)(float)y);
    return s << ret;
}

} // namespace z8

