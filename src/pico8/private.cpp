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

#include "pico8/vm.h"

namespace z8::pico8
{

using lol::msg;

int vm::private_cartdata(lua_State *l)
{
    if (lua_isnone(l, 1))
    {
        // No argument given: we return whether there is data
        lua_pushboolean(l, m_cartdata.size() > 0);
        return 1;
    }
    else if (!lua_isstring(l, 1))
    {
        // Nil or invalid argument given: get rid of cart data
        m_cartdata = "";
        return 0;
    }

    m_cartdata = lua_tostring(l, 1);
    msg::info("z8:stub:cartdata \"%s\"\n", m_cartdata.c_str());
    lua_pushboolean(l, false);
    return 1;
}

} // namespace z8

