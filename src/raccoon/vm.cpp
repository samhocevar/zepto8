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

/* Helper to dispatch C++ functions to JS C bindings */
typedef JSValue (vm::*api_func)(int, JSValueConst *);

template<api_func f>
static JSValue dispatch(JSContext *ctx, JSValueConst this_val,
                        int argc, JSValueConst *argv)
{
    vm *that = (vm *)JS_GetContextOpaque(ctx);
    return ((*that).*f)(argc, argv);
}

#define JS_DISPATCH_CFUNC_DEF(name, length, func) \
    { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, 0, { length, JS_CFUNC_generic, { &dispatch<&vm::func> } } }

vm::vm()
{
    m_rt = JS_NewRuntime();
    m_ctx = JS_NewContext(m_rt);

    static const JSCFunctionListEntry js_rcn_funcs[] =
    {
        JS_DISPATCH_CFUNC_DEF("read",   1, api_read ),
        JS_DISPATCH_CFUNC_DEF("write",  2, api_write ),

        JS_DISPATCH_CFUNC_DEF("palset", 4, api_palset ),
        JS_DISPATCH_CFUNC_DEF("fget",   2, api_fget ),
        JS_DISPATCH_CFUNC_DEF("fset",   3, api_fset ),

        JS_DISPATCH_CFUNC_DEF("cls",    1, api_cls ),
        JS_DISPATCH_CFUNC_DEF("cam",    2, api_cam ),
        JS_DISPATCH_CFUNC_DEF("map",    6, api_map ),
        JS_DISPATCH_CFUNC_DEF("palm",   2, api_palm ),
        JS_DISPATCH_CFUNC_DEF("palt",   2, api_palt ),
        JS_DISPATCH_CFUNC_DEF("pset",   3, api_pset ),
        JS_DISPATCH_CFUNC_DEF("mget",   2, api_mget ),
        JS_DISPATCH_CFUNC_DEF("spr",    7, api_spr ),
        JS_DISPATCH_CFUNC_DEF("rect",   5, api_rect ),
        JS_DISPATCH_CFUNC_DEF("rectfill", 5, api_rectfill ),
        JS_DISPATCH_CFUNC_DEF("print",  4, api_print ),

        JS_DISPATCH_CFUNC_DEF("rnd",    1, api_rnd ),
        JS_DISPATCH_CFUNC_DEF("mid",    3, api_mid ),
        JS_DISPATCH_CFUNC_DEF("mus",    1, api_mus ),
        JS_DISPATCH_CFUNC_DEF("btnp",   2, api_btnp ),
    };

    // Add functions to global scope
    auto global_obj = JS_GetGlobalObject(m_ctx);
    JS_SetPropertyFunctionList(m_ctx, global_obj, js_rcn_funcs, countof(js_rcn_funcs));
    JS_FreeValue(m_ctx, global_obj);

    JS_SetContextOpaque(m_ctx, this);
}

vm::~vm()
{
}

std::string vm::get_property_str(JSValue obj, char const *name)
{
    JSValue prop = JS_GetPropertyStr(m_ctx, obj, name);
    if (JS_IsUndefined(prop))
        return "";

    char const *tmp = JS_ToCString(m_ctx, prop);
    std::string ret(tmp);
    JS_FreeCString(m_ctx, tmp);
    JS_FreeValue(m_ctx, prop);
    return ret;
}

void vm::load(char const *file)
{
    std::string s;
    lol::File f;
    for (auto const &candidate : lol::sys::get_path_list(file))
    {
        f.Open(candidate, lol::FileAccess::Read);
        if (f.IsValid())
        {
            s = f.ReadString();
            f.Close();

            lol::msg::debug("loaded file %s\n", candidate.c_str());
            break;
        }
    }

    if (s.length() == 0)
        return;

    JSValue bin = JS_ParseJSON(m_ctx, s.c_str(), s.length(), file);
    if (JS_IsException(bin))
    {
        dump_error(m_ctx);
        JS_FreeValue(m_ctx, bin);
        return;
    }

    m_name = get_property_str(bin, "name");
    m_host = get_property_str(bin, "host");
    m_link = get_property_str(bin, "link");

    JSValue version = JS_GetPropertyStr(m_ctx, bin, "version");
    if (!JS_IsUndefined(version))
    {
        if (JS_ToInt32(m_ctx, &m_version, version))
            m_version = -1;
        JS_FreeValue(m_ctx, version);
    }

    JSValue code = JS_GetPropertyStr(m_ctx, bin, "code");
    if (!JS_IsUndefined(code))
    {
        m_code = "";
        for (int i = 0; ; ++i)
        {
            JSValue val = JS_GetPropertyUint32(m_ctx, code, i);
            if (JS_IsUndefined(val))
                break;
            char const *tmp = JS_ToCString(m_ctx, val);
            m_code += tmp;
            JS_FreeCString(m_ctx, tmp);
            m_code += '\n';
        }
        JS_FreeValue(m_ctx, code);
    }

    JSValue rom = JS_GetPropertyStr(m_ctx, bin, "rom");
    if (!JS_IsUndefined(rom))
    {
        std::map<char const *, size_t> sections
        {
            { "spr", offsetof(raccoon::memory, sprites) },
            { "map", offsetof(raccoon::memory, map) },
            { "pal", offsetof(raccoon::memory, palette) },
            { "spf", offsetof(raccoon::memory, flags) },
            { "snd", offsetof(raccoon::memory, sound) },
            { "mus", offsetof(raccoon::memory, music) },
        };

        for (auto kv : sections)
        {
            JSValue data = JS_GetPropertyStr(m_ctx, rom, kv.first);
            if (JS_IsUndefined(data))
                continue;

            size_t offset = kv.second;
            for (int i = 0; ; ++i)
            {
                JSValue val = JS_GetPropertyUint32(m_ctx, data, i);
                if (JS_IsUndefined(val))
                    break;
                char const *tmp = JS_ToCString(m_ctx, val);
                for (int j = 0; tmp[j] && tmp[j + 1] && offset < sizeof(raccoon::memory); j += 2)
                {
                    char str[3] = { tmp[j], tmp[j + 1], '\0' };
                    m_rom[offset++] = std::strtoul(str, nullptr, 16);
                }
                JS_FreeCString(m_ctx, tmp);
                JS_FreeValue(m_ctx, val);
            }
            JS_FreeValue(m_ctx, data);
        }
        JS_FreeValue(m_ctx, rom);
    }
}

void vm::run()
{
    lol::msg::debug("running bin version %d (%s)\n", m_version, m_name.c_str());

    memcpy(&m_ram, &m_rom, sizeof(m_rom));

    eval_buf(m_ctx, m_code, "<cart>", JS_EVAL_TYPE_GLOBAL);

    std::string code =
        "min = Math.min;\n"
        "max = Math.max;\n"
        "sin = Math.sin;\n"
        "cos = Math.cos;\n"
        "atan2 = Math.atan2;\n"
        "abs = Math.abs;\n"
        "flr = Math.floor;\n"
        "sign = Math.sign;\n"
        "sqrt = Math.sqrt;\n"
        "if (typeof init != 'undefined') init();\n";
    eval_buf(m_ctx, code, "<run_code>", JS_EVAL_TYPE_GLOBAL);
}

bool vm::step(float seconds)
{
    UNUSED(seconds);

    std::string code =
        "if (typeof update != 'undefined') update();\n"
        "if (typeof draw != 'undefined') draw();\n";
    eval_buf(m_ctx, code, "<step_code>", JS_EVAL_TYPE_GLOBAL);

    return true;
}

void vm::button(int index, int state)
{
}

void vm::mouse(lol::ivec2 coords, int buttons)
{
}

void vm::keyboard(char ch)
{
}

void vm::render(lol::u8vec4 *screen) const
{
    /* Precompute the current palette for pairs of pixels */
    struct { u8vec4 a, b; } lut[256];
    for (int n = 0; n < 256; ++n)
    {
        lut[n].a = u8vec4(m_ram.palette[n % 16], 0xff);
        lut[n].b = u8vec4(m_ram.palette[n / 16], 0xff);
    }

    /* Render actual screen */
    for (uint8_t p : m_ram.screen)
    {
        *screen++ = lut[p].a;
        *screen++ = lut[p].b;
    }
}

std::function<void(void *, int)> vm::get_streamer(int channel)
{
    return [](void *, int) {};
}

void vm::dump_error(JSContext *ctx)
{
    JSValue exception_val = JS_GetException(ctx);
    int is_error = JS_IsError(ctx, exception_val);
    if (!is_error)
        lol::msg::error("Throw: ");

    char const *str = JS_ToCString(ctx, exception_val);
    if (str)
    {
        lol::msg::error("%s\n", str);
        JS_FreeCString(ctx, str);
    }

    if (is_error)
    {
        JSValue val = JS_GetPropertyStr(ctx, exception_val, "stack");
        if (!JS_IsUndefined(val))
        {
            char const *stack = JS_ToCString(ctx, val);
            lol::msg::error("%s\n", stack);
            JS_FreeCString(ctx, stack);
        }
        JS_FreeValue(ctx, val);
    }
    JS_FreeValue(ctx, exception_val);
}

int vm::eval_buf(JSContext *ctx, std::string const &code,
                 const char *filename, int eval_flags)
{
    int ret = 0;
    JSValue val = JS_Eval(ctx, code.c_str(), code.length(), filename, eval_flags);
    if (JS_IsException(val))
    {
        dump_error(ctx);
        ret = -1;
    }
    JS_FreeValue(ctx, val);
    return ret;
}

}

