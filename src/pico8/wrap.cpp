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
#include <variant>

#include "zepto8.h"
#include "pico8/vm.h"
#include "pico8/z8lua.h"

namespace z8::pico8
{

// Push a standard type to the Lua stack
template<typename T> static int lua_push(lua_State *l, T const &);

template<> int lua_push(lua_State *l, bool const &x) { lua_pushboolean(l, x); return 1; }
template<> int lua_push(lua_State *l, int16_t const &x) { lua_pushnumber(l, x); return 1; }
template<> int lua_push(lua_State *l, fix32 const &x) { lua_pushnumber(l, x); return 1; }
template<> int lua_push(lua_State *l, std::string const &s) { lua_pushlstring(l, s.c_str(), (int)s.size()); return 1; }
template<> int lua_push(lua_State *l, std::nullptr_t const &) { lua_pushnil(l); return 1; }

// Boxing an std::variant pushes the active alternative
template<typename... T> int lua_push(lua_State *l, std::variant<T...> const &x)
{
    return std::visit([l](auto&& arg) -> int { return lua_push(l, arg); }, x);
}

// Boxing an std::optional returns 0 or 1 depending on whether there is an object
template<typename T> int lua_push(lua_State *l, std::optional<T> const &x)
{
    return x ? lua_push(l, *x) : 0;
}

// Get a standard type from the Lua stack
template<typename T> static T lua_get(lua_State *l, int i);
template<typename T> static void lua_get(lua_State *l, int i, T &);

template<> void lua_get(lua_State *l, int i, fix32 &arg) { arg = lua_tonumber(l, i); }
template<> void lua_get(lua_State *l, int i, bool &arg) { arg = (bool)lua_toboolean(l, i); }
template<> void lua_get(lua_State *l, int i, uint8_t &arg) { arg = (uint8_t)lua_tonumber(l, i); }
template<> void lua_get(lua_State *l, int i, int16_t &arg) { arg = (int16_t)lua_tonumber(l, i); }

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
        arg = lua_get<T>(l, i);
}

template<typename T> static T lua_get(lua_State *l, int i)
{
    T ret; lua_get(l, i, ret); return ret;
}

// Call a T::* member function with arguments pulled from the Lua stack,
// and push the result to the Lua stack.
template<typename T, typename R, typename... A, size_t... IS>
static inline int dispatch(lua_State *l, R (T::*f)(A...),
                           std::index_sequence<IS...>)
{
    // Retrieve “this” from the Lua state.
#if HAVE_LUA_GETEXTRASPACE
    T *that = *static_cast<T**>(lua_getextraspace(l));
#else
    lua_getglobal(l, "\x01");
    T *that = (T *)lua_touserdata(l, -1);
    lua_remove(l, -1);
#endif

    // Store this for API functions that we don’t know yet how to wrap
    that->m_sandbox_lua = l;

    // Call the API function with the loaded arguments. Some specialization
    // is needed when the wrapped function returns void.
    if constexpr (std::is_same<R, void>::value)
        return (that->*f)(lua_get<A>(l, IS + 1)...), 0;
    else
        return lua_push(l, (that->*f)(lua_get<A>(l, IS + 1)...));
}

// Helper to create an index sequence from a member function’s signature
template<typename T, typename R, typename... A>
constexpr auto make_seq(R (T::*)(A...))
{
    return std::index_sequence_for<A...>();
}

// Helper to dispatch C++ functions to Lua C bindings
template<auto FN>
static int wrap(lua_State *l)
{
    return dispatch(l, FN, make_seq(FN));
}

void vm::install_lua_api()
{
    // Store a pointer to us in global state
#if HAVE_LUA_GETEXTRASPACE
    *static_cast<vm**>(lua_getextraspace(m_lua)) = this;
#else
    lua_pushlightuserdata(m_lua, this);
    lua_setglobal(m_lua, "\x01");
#endif

    static const luaL_Reg zepto8lib[] =
    {
        { "run",      &wrap<&vm::api_run> },
        { "menuitem", &wrap<&vm::api_menuitem> },
        { "reload",   &wrap<&vm::api_reload> },
        { "peek",     &wrap<&vm::api_peek> },
        { "peek2",    &wrap<&vm::api_peek2> },
        { "peek4",    &wrap<&vm::api_peek4> },
        { "poke",     &wrap<&vm::api_poke> },
        { "poke2",    &wrap<&vm::api_poke2> },
        { "poke4",    &wrap<&vm::api_poke4> },
        { "memcpy",   &wrap<&vm::api_memcpy> },
        { "memset",   &wrap<&vm::api_memset> },
        { "stat",     &wrap<&vm::api_stat> },
        { "printh",   &wrap<&vm::api_printh> },
        { "extcmd",   &wrap<&vm::api_extcmd> },

        { "_update_buttons", &wrap<&vm::api_update_buttons> },
        { "btn",  &wrap<&vm::api_btn> },
        { "btnp", &wrap<&vm::api_btnp> },

        { "cursor", &wrap<&vm::api_cursor> },
        { "print",  &wrap<&vm::api_print> },

        { "camera",   &wrap<&vm::api_camera> },
        { "circ",     &wrap<&vm::api_circ> },
        { "circfill", &wrap<&vm::api_circfill> },
        { "clip",     &wrap<&vm::api_clip> },
        { "cls",      &wrap<&vm::api_cls> },
        { "color",    &wrap<&vm::api_color> },
        { "fillp",    &wrap<&vm::api_fillp> },
        { "fget",     &wrap<&vm::api_fget> },
        { "fset",     &wrap<&vm::api_fset> },
        { "line",     &wrap<&vm::api_line> },
        { "map",      &wrap<&vm::api_map> },
        { "mget",     &wrap<&vm::api_mget> },
        { "mset",     &wrap<&vm::api_mset> },
        { "pal",      &wrap<&vm::api_pal> },
        { "palt",     &wrap<&vm::api_palt> },
        { "pget",     &wrap<&vm::api_pget> },
        { "pset",     &wrap<&vm::api_pset> },
        { "rect",     &wrap<&vm::api_rect> },
        { "rectfill", &wrap<&vm::api_rectfill> },
        { "sget",     &wrap<&vm::api_sget> },
        { "sset",     &wrap<&vm::api_sset> },
        { "spr",      &wrap<&vm::api_spr> },
        { "sspr",     &wrap<&vm::api_sspr> },

        { "music", &wrap<&vm::api_music> },
        { "sfx",   &wrap<&vm::api_sfx> },

        { "time", &wrap<&vm::api_time> },

        { "__cartdata", &wrap<&vm::private_cartdata> },
        { "__stub",     &wrap<&vm::private_stub> },

        { nullptr, nullptr },
    };

    lua_pushglobaltable(m_lua);
    luaL_setfuncs(m_lua, zepto8lib, 0);
}

} // namespace z8::pico8

