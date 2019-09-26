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

#pragma once

#include "z8lua/lua.h"
#include "z8lua/lauxlib.h"
#include "z8lua/lualib.h"

// The z8lua.h header
// ——————————————————
// This header provides some Lua C API function helpers.

namespace z8::pico8
{

char const *lua_tostringorboolean(lua_State *l, int n);
void lua_pushtostr(lua_State *l, int n, bool do_hex);

} // namespace z8::pico8

