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

#include <optional>

#include "zepto8.h"
#include "pico8/vm.h"
#include "pico8/z8lua.h"

namespace z8::pico8
{

// Declare a tuple matching the arguments of a T::* member function
template<typename T, typename R, typename... A>
static auto lua_tuple(R (T::*)(A...)) { return std::tuple<A...>(); }

// Push a standard type to the Lua stack
static void lua_push(lua_State *l, int16_t x) { lua_pushnumber(l, x); }
static void lua_push(lua_State *l, fix32 x) { lua_pushnumber(l, x); }

// Get a standard type from the Lua stack
template<typename T> static void lua_get(lua_State *l, int i, T &);

template<> void lua_get(lua_State *l, int i, fix32 &arg) { arg = lua_tonumber(l, i); }
template<> void lua_get(lua_State *l, int i, uint8_t &arg) { arg = (uint8_t)lua_tonumber(l, i); }
template<> void lua_get(lua_State *l, int i, int16_t &arg) { arg = (int16_t)lua_tonumber(l, i); }
template<> void lua_get(lua_State *l, int i, uint16_t &arg) { arg = (uint16_t)lua_tonumber(l, i); }

template<> void lua_get(lua_State *l, int i, std::string &arg)
{
    lua_pushtostr(l, i, false);
    arg = lua_tostring(l, -1);
    lua_pop(l, 1);
}

// Unboxing to std::optional checks for lua_isnone() first
template<typename T> void lua_get(lua_State *l, int i, std::optional<T> &arg)
{
    if (!lua_isnone(l, i))
        lua_get(l, i, *(arg = T()));
}

// Convert a T::* member function to a lambda taking the same arguments.
// That lambda also retrieves “this” from the Lua state, and pushes the
// return value to the Lua stack. Some specialisation is needed when the
// original function returns void.
template<typename T, typename R, typename... A>
static inline auto lua_wrap(lua_State *l, R (T::*f)(A...))
{
#if HAVE_LUA_GETEXTRASPACE
    T *that = *static_cast<T**>(lua_getextraspace(l));
#else
    lua_getglobal(l, "\x01");
    T *that = (T *)lua_touserdata(l, -1);
    lua_remove(l, -1);
#endif
    return [=](A... args) -> int { lua_push(l, (that->*f)(args...)); return 1; };
}

template<typename T, typename... A>
static inline auto lua_wrap(lua_State *l, void (T::*f)(A...))
{
#if HAVE_LUA_GETEXTRASPACE
    T *that = *static_cast<T**>(lua_getextraspace(l));
#else
    lua_getglobal(l, "\x01");
    T *that = (T *)lua_touserdata(l, -1);
    lua_remove(l, -1);
#endif
    return [=](A... args) -> int { (that->*f)(args...); return 0; };
}

// Helper to dispatch C++ functions to Lua C bindings
template<auto FN>
static int dispatch(lua_State *l)
{
    // Create the argument list tuple
    auto args = lua_tuple(FN);

    // Load arguments from argv into the tuple, with type safety. Uses the
    // technique presented in https://stackoverflow.com/a/54053084/111461
    int i = 0;
    std::apply([&](auto&&... arg) {((lua_get(l, ++i, arg)), ...);}, args);

    // Call the API function with the loaded arguments
    auto f = lua_wrap(l, FN);
    return std::apply(f, args);
}

void vm::install_lua_api()
{
    static const luaL_Reg zepto8lib[] =
    {
        { "run",      &dispatch<&vm::api_run> },
        { "menuitem", &dispatch<&vm::api_menuitem> },
        //{ "reload",   &dispatch<&vm::api_reload> },
        { "peek",     &dispatch<&vm::api_peek> },
        { "peek2",    &dispatch<&vm::api_peek2> },
        { "peek4",    &dispatch<&vm::api_peek4> },
        //{ "poke",     &dispatch<&vm::api_poke> },
        //{ "poke2",    &dispatch<&vm::api_poke2> },
        //{ "poke4",    &dispatch<&vm::api_poke4> },
        //{ "memcpy",   &dispatch<&vm::api_memcpy> },
        //{ "memset",   &dispatch<&vm::api_memset> },
        //{ "stat",     &dispatch<&vm::api_stat> },
        //{ "printh",   &vm::api_printh },
        //{ "extcmd",   &dispatch<&vm::api_extcmd> },

        { "_update_buttons", &dispatch<&vm::api_update_buttons> },
        //{ "btn",  &dispatch<&vm::api_btn> },
        //{ "btnp", &dispatch<&vm::api_btnp> },

        { "cursor", &dispatch<&vm::api_cursor> },
        { "print",  &dispatch<&vm::api_print> },

        { "camera",   &dispatch<&vm::api_camera> },
        { "circ",     &dispatch<&vm::api_circ> },
        { "circfill", &dispatch<&vm::api_circfill> },
        { "clip",     &dispatch<&vm::api_clip> },
        { "cls",      &dispatch<&vm::api_cls> },
        { "color",    &dispatch<&vm::api_color> },
        { "fillp",    &dispatch<&vm::api_fillp> },
        //{ "fget",     &dispatch<&vm::api_fget> },
        //{ "fset",     &dispatch<&vm::api_fset> },
        { "line",     &dispatch<&vm::api_line> },
        { "map",      &dispatch<&vm::api_map> },
        { "mget",     &dispatch<&vm::api_mget> },
        { "mset",     &dispatch<&vm::api_mset> },
        { "pal",      &dispatch<&vm::api_pal> },
        { "palt",     &dispatch<&vm::api_palt> },
        { "pget",     &dispatch<&vm::api_pget> },
        { "pset",     &dispatch<&vm::api_pset> },
        { "rect",     &dispatch<&vm::api_rect> },
        { "rectfill", &dispatch<&vm::api_rectfill> },
        //{ "sget",     &dispatch<&vm::api_sget> },
        //{ "sset",     &dispatch<&vm::api_sset> },
        //{ "spr",      &dispatch<&vm::api_spr> },
        //{ "sspr",     &dispatch<&vm::api_sspr> },

        //{ "music", &dispatch<&vm::api_music> },
        //{ "sfx",   &dispatch<&vm::api_sfx> },

        { "time", &dispatch<&vm::api_time> },

        //{ "__cartdata", &dispatch<&vm::private_cartdata> },
        //{ "__stub",     &dispatch<&vm::private_stub> },

        { nullptr, nullptr },
    };

    lua_pushglobaltable(m_lua);
    luaL_setfuncs(m_lua, zepto8lib, 0);
}

} // namespace z8::pico8

