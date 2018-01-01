//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2017 Sam Hocevar <sam@hocevar.net>
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

int vm::api_rnd(lua_State *l)
{
    uint32_t x = ((uint64_t)m_seed.bits() * 279470273ul) % 4294967291ul;
    m_seed = fix32::frombits(x);

    /* FIXME: behaves incorrectly when b is negative */
    double a = 0.f;
    double b = lua_isnone(l, 1) ? 1.0
             : (double)fix32::min(lua_tofix32(l, 1), fix32::frombits(0x7ffffff));
    double c = lol::mix(a, b, x / (1.0 + (uint32_t)-1));
    lua_pushfix32(l, fix32(c));
    return 1;
}

int vm::api_srand(lua_State *l)
{
    m_seed = lua_tofix32(l, 1);
    if (m_seed.bits() == 0)
        m_seed = fix32::frombits(0xdeadbeef);
    return 0;
}

} // namespace z8

