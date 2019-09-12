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

extern "C" {
#include "quickjs/quickjs.h"
}

#include "zepto8.h"
#include "raccoon/vm.h"

#define countof(x) (sizeof(x) / sizeof((x)[0]))

namespace z8::raccoon
{

// Count arguments in a T::* member function prototype
template<typename T, typename R, typename... A>
constexpr int count_args(R (T::*)(A...)) { return (int)sizeof...(A); }

// Declare a tuple matching the arguments of a T::* member function
template<typename T, typename R, typename... A>
auto js_tuple(R (T::*)(A...)) { return std::tuple<A...>(); }

// Convert a standard type to a JSValue
JSValue js_box(JSContext *ctx, bool x) { return JS_NewBool(ctx, (int)x); }
JSValue js_box(JSContext *ctx, int x) { return JS_NewInt32(ctx, x); }
JSValue js_box(JSContext *ctx, double x) { return JS_NewFloat64(ctx, x); }

// Convert a JSValue to a standard type
static void js_unbox(JSContext *ctx, int &arg, JSValueConst jsval) { JS_ToInt32(ctx, &arg, jsval); }
static void js_unbox(JSContext *ctx, std::optional<int> &arg, JSValueConst jsval) { int tmp = 0; JS_ToInt32(ctx, &tmp, jsval); arg = tmp; }
static void js_unbox(JSContext *ctx, double &arg, JSValueConst jsval) { JS_ToFloat64(ctx, &arg, jsval); }
static void js_unbox(JSContext *ctx, std::optional<double> &arg, JSValueConst jsval) { double tmp = 0.0; JS_ToFloat64(ctx, &tmp, jsval); arg = tmp; }
static void js_unbox(JSContext *ctx, std::string &str, JSValueConst jsval)
{
    char const *data = JS_ToCString(ctx, jsval);
    str = std::string(data);
    JS_FreeCString(ctx, data);
}

// Convert a T::* member function to a lambda taking the same arguments.
// That lambda also retrieves “this” from the JS context, and converts
// the return value to a JSValue. Some specialisation is needed when the
// original function returns void.
template<typename T, typename R, typename... A>
static inline auto js_wrap(JSContext *ctx, R (T::*f)(A...))
{
    T *that = (T *)JS_GetContextOpaque(ctx);
    return [ctx, that, f](A... args) -> JSValue { return js_box(ctx, (that->*f)(args...)); };
}

template<typename T, typename... A>
static inline auto js_wrap(JSContext *ctx, void (T::*f)(A...))
{
    T *that = (T *)JS_GetContextOpaque(ctx);
    return [that, f](A... args) -> JSValue { (that->*f)(args...); return JS_UNDEFINED; };
}

// Helper to dispatch C++ functions to JS C bindings
template<auto FN>
static JSValue dispatch(JSContext *ctx, JSValueConst this_val,
                        int argc, JSValueConst *argv)
{
    // Create the argument list tuple
    auto args = js_tuple(FN);

    // Load arguments from argv into the tuple, with type safety. Uses the
    // technique presented in https://stackoverflow.com/a/54053084/111461
    int i = 0;
    std::apply([&](auto&&... arg) {((i < argc ? js_unbox(ctx, arg, argv[i++]) : i++), ...);}, args);

    // Call the API function with the loaded arguments
    auto f = js_wrap(ctx, FN);
    return std::apply(f, args);
}

#define JS_DISPATCH_CFUNC_DEF(name, func) \
    { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, 0, { count_args(&vm::func), JS_CFUNC_generic, { &dispatch<&vm::func> } } }

void vm::js_wrap()
{
    static const JSCFunctionListEntry js_rcn_funcs[] =
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
    JS_SetPropertyFunctionList(m_ctx, global_obj, js_rcn_funcs, countof(js_rcn_funcs));
    JS_FreeValue(m_ctx, global_obj);
}

}

