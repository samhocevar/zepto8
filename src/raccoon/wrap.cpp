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

extern "C" {
#include "quickjs/quickjs.h"
}

#include "zepto8.h"
#include "raccoon/vm.h"

namespace z8::raccoon
{

// Convert a standard type to a JSValue
static JSValue js_box(JSContext *ctx, bool x) { return JS_NewBool(ctx, (int)x); }
static JSValue js_box(JSContext *ctx, int x) { return JS_NewInt32(ctx, x); }
static JSValue js_box(JSContext *ctx, double x) { return JS_NewFloat64(ctx, x); }

// Convert a JSValue to a standard type
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

template<typename T, typename R, typename... A, size_t... IS>
static JSValue dispatch(JSContext *ctx, int argc, JSValueConst *argv,
                        R (T::*f)(A...), std::index_sequence<IS...>)
{
    // Retrieve “this” from the JS context.
    T *that = (T *)JS_GetContextOpaque(ctx);

    // Call the API function with the loaded arguments. Some specialization
    // is needed when the wrapped function returns void.
    if constexpr (std::is_same<R, void>::value)
        return (that->*f)((IS < argc ? js_unbox<A>(ctx, argv[IS]) : A())...), JS_UNDEFINED;
    else
        return js_box(ctx, (that->*f)((IS < argc ? js_unbox<A>(ctx, argv[IS]) : A())...));
}

// Helper to create an index sequence from a member function’s signature
template<typename T, typename R, typename... A>
constexpr auto make_seq(R (T::*f)(A...))
{
    return std::index_sequence_for<A...>();
}

// Helper to dispatch C++ functions to JS C bindings
template<auto FN>
static JSValue wrap(JSContext *ctx, JSValueConst,
                    int argc, JSValueConst *argv)
{
    return dispatch(ctx, argc, argv, FN, make_seq(FN));
}

#define JS_DISPATCH_CFUNC_DEF(name, func) \
    { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, 0, \
        { (uint8_t)make_seq(&vm::func).size(), JS_CFUNC_generic, { &wrap<&vm::func> } } \
    }

void vm::js_wrap()
{
    static std::vector<JSCFunctionListEntry> const funcs =
    {
        JS_DISPATCH_CFUNC_DEF("read",     api_read),
        JS_DISPATCH_CFUNC_DEF("write",    api_write),

        JS_DISPATCH_CFUNC_DEF("palset",   api_palset),
        JS_DISPATCH_CFUNC_DEF("fget",     api_fget),
        JS_DISPATCH_CFUNC_DEF("fset",     api_fset),
        JS_DISPATCH_CFUNC_DEF("mget",     api_mget),
        JS_DISPATCH_CFUNC_DEF("mset",     api_mset),

        JS_DISPATCH_CFUNC_DEF("cls",      api_cls),
        JS_DISPATCH_CFUNC_DEF("cam",      api_cam),
        JS_DISPATCH_CFUNC_DEF("map",      api_map),
        JS_DISPATCH_CFUNC_DEF("palm",     api_palm),
        JS_DISPATCH_CFUNC_DEF("palt",     api_palt),
        JS_DISPATCH_CFUNC_DEF("pget",     api_pget),
        JS_DISPATCH_CFUNC_DEF("pset",     api_pset),
        JS_DISPATCH_CFUNC_DEF("spr",      api_spr),
        JS_DISPATCH_CFUNC_DEF("rect",     api_rect),
        JS_DISPATCH_CFUNC_DEF("rectfill", api_rectfill),
        JS_DISPATCH_CFUNC_DEF("print",    api_print),

        JS_DISPATCH_CFUNC_DEF("rnd",      api_rnd),
        JS_DISPATCH_CFUNC_DEF("mid",      api_mid),
        JS_DISPATCH_CFUNC_DEF("mus",      api_mus),
        JS_DISPATCH_CFUNC_DEF("sfx",      api_sfx),
        JS_DISPATCH_CFUNC_DEF("btn",      api_btn),
        JS_DISPATCH_CFUNC_DEF("btnp",     api_btnp),
    };

    // Add functions to global scope
    auto global_obj = JS_GetGlobalObject(m_ctx);
    JS_SetPropertyFunctionList(m_ctx, global_obj, funcs.data(), (int)funcs.size());
    JS_FreeValue(m_ctx, global_obj);
}

} // namespace z8::raccoon

