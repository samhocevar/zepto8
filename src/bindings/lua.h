//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2020 Sam Hocevar <sam@hocevar.net>
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

#include <optional>
#include <variant>

#include "3rdparty/z8lua/lua.h"
#include "3rdparty/z8lua/lauxlib.h"
#include "3rdparty/z8lua/lualib.h"

namespace z8::bindings
{

//
// Push a standard type to the Lua stack
//

template<typename T> static int lua_push(lua_State *l, T const &);

template<> int lua_push(lua_State *l, bool const &x) { lua_pushboolean(l, x); return 1; }
template<> int lua_push(lua_State *l, uint8_t const &x) { lua_pushnumber(l, x); return 1; }
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

// Boxing an std::tuple pushes each value
template<typename... T> int lua_push(lua_State *l, std::tuple<T...> const &t)
{
    std::apply([l](auto&&... x){ ((lua_push(l, x)), ...); }, t);
    return (int)sizeof...(T);
}

// Boxing an std::vector pushes each value
template<typename... T> int lua_push(lua_State *l, std::vector<T...> const &v)
{
    for (auto &x : v)
        lua_push(l, x);
    return (int)v.size();
}

//
// Get a standard type from the Lua stack
//

template<typename T> static T lua_get(lua_State *l, int i);
template<typename T> static void lua_get(lua_State *l, int i, T &);

template<> void lua_get(lua_State *l, int i, fix32 &arg) { arg = lua_tonumber(l, i); }
template<> void lua_get(lua_State *l, int i, bool &arg) { arg = (bool)lua_toboolean(l, i); }
template<> void lua_get(lua_State *l, int i, uint8_t &arg) { arg = (uint8_t)lua_tonumber(l, i); }
template<> void lua_get(lua_State *l, int i, int16_t &arg) { arg = (int16_t)lua_tonumber(l, i); }
template<> void lua_get(lua_State *l, int n, std::string &arg) { if (lua_isstring(l, n)) arg = lua_tostring(l, n); }

// Unboxing to std::optional checks for lua_isnone() first
template<typename T> void lua_get(lua_State *l, int i, std::optional<T> &arg)
{
    if (!lua_isnone(l, i))
        arg = lua_get<T>(l, i);
}

// Unboxing to std::vector unboxes the whole stack
template<typename T> void lua_get(lua_State *l, int i, std::vector<T> &arg)
{
    while (!lua_isnone(l, i))
        arg.push_back(lua_get<T>(l, i++));
}

template<typename T> static T lua_get(lua_State *l, int i)
{
    T ret; lua_get(l, i, ret); return ret;
}

//
// Lua binding mechanism
//

class lua
{
public:
    template<typename T>
    static void init(lua_State *l, T *that)
    {
        // Store a pointer to the caller in the global state
#if HAVE_LUA_GETEXTRASPACE
        *static_cast<vm**>(lua_getextraspace(l)) = that;
#else
        lua_pushlightuserdata(l, that);
        lua_setglobal(l, "\x01");
#endif

        auto lib = typename T::template exported_api<lua>().data;
        lib.push_back({});

        lua_pushglobaltable(l);
        luaL_setfuncs(l, lib.data(), 0);
    }

    // Helper to dispatch C++ functions to Lua C bindings
    template<auto FN> struct bind
    {
        static int wrap(lua_State *l)
        {
            return dispatch(l, FN, make_seq(FN));
        }

        // Create an index sequence from a member function’s signature
        template<typename T, typename R, typename... A>
        static constexpr auto make_seq(R (T::*)(A...))
        {
            return std::index_sequence_for<A...>();
        }
    };

    struct bind_desc : luaL_Reg
    {
        bind_desc()
          : luaL_Reg({ nullptr, nullptr })
        {}

        template<auto FN>
        bind_desc(char const *str, bind<FN> b)
          : luaL_Reg({ str, &b.wrap })
        {}
    };

private:
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
};

} // namespace z8::bindings

