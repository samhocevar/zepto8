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
#include "pico8/z8lua.h"

namespace z8::pico8
{

using lol::msg;

opt<bool> vm::private_cartdata()
{
    // No argument given: we return whether there is data
    if (lua_isnone(m_lua, 1))
        return m_cartdata.size() > 0;

    if (!lua_isstring(m_lua, 1))
    {
        // Nil or invalid argument given: get rid of cart data
        m_cartdata = "";
        return std::nullopt;
    }

    m_cartdata = lua_tostring(m_lua, 1);
    msg::info("z8:stub:cartdata \"%s\"\n", m_cartdata.c_str());
    return false;
}

} // namespace z8

