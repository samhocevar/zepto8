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

extern "C" {
#include "3rdparty/quickjs/quickjs.h"
}

#include "zepto8.h"

namespace z8::bindings
{

//
// Convert a standard type to a JSValue
//

static JSValue js_box(JSContext *ctx, bool x) { return JS_NewBool(ctx, (int)x); }
static JSValue js_box(JSContext *ctx, int x) { return JS_NewInt32(ctx, x); }
static JSValue js_box(JSContext *ctx, double x) { return JS_NewFloat64(ctx, x); }

//
// Convert a JSValue to a standard type
//

template<typename T> static void js_unbox(JSContext *ctx, T &, JSValueConst jsval);

template<> void js_unbox(JSContext *ctx, int &arg, JSValueConst jsval) { JS_ToInt32(ctx, &arg, jsval); }
template<> void js_unbox(JSContext *ctx, double &arg, JSValueConst jsval) { JS_ToFloat64(ctx, &arg, jsval); }
template<> void js_unbox(JSContext *ctx, std::string &str, JSValueConst jsval)
{
    char const *data = JS_ToCString(ctx, jsval);
    str = std::string(data);
    JS_FreeCString(ctx, data);
}

// Unboxing to std::optional always unboxes
template<typename T> void js_unbox(JSContext *ctx, std::optional<T> &arg, JSValueConst jsval)
{
    js_unbox(ctx, *(arg = T()), jsval);
}

template<typename T> static T js_unbox(JSContext *ctx, JSValueConst jsval)
{
    T ret; js_unbox(ctx, ret, jsval); return ret;
}

//
// JavaScript binding mechanism
//

class js
{
public:
    template<typename T>
    static void init(JSContext *ctx, T *that)
    {
        JS_SetContextOpaque(ctx, that);

        // FIXME: this should not be static, store it in "that" instead!
        static auto lib = typename T::template exported_api<js>().data;

        // Add functions to global scope
        auto global_obj = JS_GetGlobalObject(ctx);
        JS_SetPropertyFunctionList(ctx, global_obj, lib.data(), (int)lib.size());
        JS_FreeValue(ctx, global_obj);
    }

    // Helper to dispatch C++ functions to JavaScript C bindings
    template<auto FN> struct bind
    {
        static JSValue wrap(JSContext *ctx, JSValueConst,
                            int argc, JSValueConst *argv)
        {
            return dispatch(ctx, argc, argv, FN, make_seq(FN));
        }

        size_t size = make_seq(FN).size();

        // Create an index sequence from a member function’s signature
        template<typename T, typename R, typename... A>
        static constexpr auto make_seq(R (T::*)(A...))
        {
            return std::index_sequence_for<A...>();
        }
    };

    struct bind_desc : JSCFunctionListEntry
    {
        template<auto FN>
        bind_desc(char const *str, bind<FN> b)
          : JSCFunctionListEntry({ str,
                JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, 0,
                { { (uint8_t)b.size, JS_CFUNC_generic, { &b.wrap } } } })
        {}
    };

private:
    template<typename T, typename R, typename... A, size_t... IS>
    static JSValue dispatch(JSContext *ctx, int argc, JSValueConst *argv,
                            R (T::*f)(A...), std::index_sequence<IS...>)
    {
        // Retrieve “this” from the JS context.
        T *that = (T *)JS_GetContextOpaque(ctx);

        // Call the API function with the loaded arguments. Some specialization
        // is needed when the wrapped function returns void.
        if constexpr (std::is_same<R, void>::value)
            return (that->*f)(((int)IS < argc ? js_unbox<A>(ctx, argv[IS]) : A())...), JS_UNDEFINED;
        else
            return js_box(ctx, (that->*f)(((int)IS < argc ? js_unbox<A>(ctx, argv[IS]) : A())...));
    }
};

} // namespace z8::bindings

