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

#include "z8lua.h"

#include <cctype>

namespace z8::pico8
{

char const *lua_tostringorboolean(lua_State *l, int n)
{
    if (lua_isnoneornil(l, n))
        return "false";
    if (lua_isstring(l, n))
        return lua_tostring(l, n);
    return lua_toboolean(l, n) ? "true" : "false";
}

void lua_pushtostr(lua_State *l, int n, bool do_hex)
{
    char buffer[20];
    char const *str = buffer;

    if (lua_isnone(l, n))
        str = "[no value]";
    else if (lua_isnil(l, n))
        str = "[nil]";
    else if (lua_type(l, n) == LUA_TSTRING)
        str = lua_tostring(l, n);
    else if (lua_isnumber(l, n))
    {
        fix32 x = lua_tonumber(l, n);
        if (do_hex)
        {
            uint32_t b = (uint32_t)x.bits();
            sprintf(buffer, "0x%04x.%04x", (b >> 16) & 0xffff, b & 0xffff);
        }
        else
        {
            int i = sprintf(buffer, "%.4f", (double)x);
            // Remove trailing zeroes and comma
            while (i > 2 && buffer[i - 1] == '0' && ::isdigit(buffer[i - 2]))
                buffer[--i] = '\0';
            if (i > 2 && buffer[i - 1] == '0' && buffer[i - 2] == '.')
                buffer[i -= 2] = '\0';
        }
    }
    else if (lua_istable(l, n))
        str = "[table]";
    else if (lua_isthread(l, n))
        str = "[thread]";
    else if (lua_isfunction(l, n))
        str = "[function]";
    else
        str = lua_toboolean(l, n) ? "true" : "false";

    lua_pushstring(l, str);
}

} // namespace z8::pico8

