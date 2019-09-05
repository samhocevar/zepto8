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

#include <fstream>
#include <sstream>

extern "C" {
#include "quickjs/quickjs.h"
}

#include "zepto8.h"
#include "raccoon.h"
#include "player.h"

#define countof(x) (sizeof(x) / sizeof((x)[0]))

#define CPP_JS_CFUNC_DEF(name, length, func1) { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, 0, { length, JS_CFUNC_generic, { func1 } } }

namespace z8::raccoon
{

class vm;

/* Helper to dispatch C++ functions to JS C bindings */
typedef JSValue (vm::*api_func)(JSContext *, JSValueConst,
                                int, JSValueConst *);

template<api_func f>
static JSValue dispatch(JSContext *ctx, JSValueConst this_val,
                        int argc, JSValueConst *argv)
{
    vm *that = (vm *)JS_GetContextOpaque(ctx);
    return ((*that).*f)(ctx, this_val, argc, argv);
}

class vm
{
public:
    vm()
    {
        m_rt = JS_NewRuntime();
        m_ctx = JS_NewContext(m_rt);

        static const JSCFunctionListEntry js_rcn_funcs[] =
        {
            CPP_JS_CFUNC_DEF("read",   1, &dispatch<&vm::api_read> ),
            CPP_JS_CFUNC_DEF("write",  2, &dispatch<&vm::api_write> ),

            CPP_JS_CFUNC_DEF("palset", 4, &dispatch<&vm::api_palset> ),
            CPP_JS_CFUNC_DEF("palt",   2, &dispatch<&vm::api_palt> ),
            CPP_JS_CFUNC_DEF("pset",   3, &dispatch<&vm::api_pset> ),
            CPP_JS_CFUNC_DEF("fget",   1, &dispatch<&vm::api_fget> ),
            CPP_JS_CFUNC_DEF("flr",    1, &dispatch<&vm::api_flr> ),
            CPP_JS_CFUNC_DEF("mget",   2, &dispatch<&vm::api_mget> ),
            CPP_JS_CFUNC_DEF("rnd",    1, &dispatch<&vm::api_rnd> ),
        };

        // Add functions to global scope
        auto global_obj = JS_GetGlobalObject(m_ctx);
        JS_SetPropertyFunctionList(m_ctx, global_obj, js_rcn_funcs, countof(js_rcn_funcs));
        JS_FreeValue(m_ctx, global_obj);

        JS_SetContextOpaque(m_ctx, this);
    }

    void run(char const *file)
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

        JSValue name = JS_GetPropertyStr(m_ctx, bin, "name");
        if (!JS_IsUndefined(name))
        {
            char const *tmp = JS_ToCString(m_ctx, name);
            m_name = tmp;
            JS_FreeCString(m_ctx, tmp);
            JS_FreeValue(m_ctx, name);
        }

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
            /* TODO */
            JS_FreeValue(m_ctx, rom);
        }

        lol::msg::debug("running bin version %d (%s)\n", m_version, m_name.c_str());

        eval_buf(m_ctx, m_code, "<cart>", JS_EVAL_TYPE_GLOBAL);

        std::string glue_code =
            "min = Math.min;\n"
            "max = Math.max;\n"
            "sin = Math.sin;\n"
            "cos = Math.cos;\n"
            "sqrt = Math.sqrt;\n"
            "if (typeof init != 'undefined') init();\n";
        eval_buf(m_ctx, glue_code, "<init_code>", JS_EVAL_TYPE_GLOBAL);

        glue_code =
            "if (typeof update != 'undefined') update();\n"
            "if (typeof draw != 'undefined') draw();\n";
        for (;;)
            eval_buf(m_ctx, glue_code, "<loop_code>", JS_EVAL_TYPE_GLOBAL);
    }

private:
    JSValue api_read(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv)
    {
        int p;
        if (JS_ToInt32(ctx, &p, argv[0]))
            return JS_EXCEPTION;
        return JS_NewInt32(ctx, m_ram[p & 0xffff]);
    }

    JSValue api_write(JSContext *ctx, JSValueConst this_val,
                      int argc, JSValueConst *argv)
    {
        int p, x;
        if (JS_ToInt32(ctx, &p, argv[0]))
            return JS_EXCEPTION;
        if (JS_ToInt32(ctx, &x, argv[1]))
            return JS_EXCEPTION;
        m_ram[p & 0xffff] = x;
        return JS_UNDEFINED;
    }

    JSValue api_palset(JSContext *ctx, JSValueConst this_val,
                       int argc, JSValueConst *argv)
    {
        int x, y, z, t;
        if (JS_ToInt32(ctx, &x, argv[0]))
            return JS_EXCEPTION;
        if (JS_ToInt32(ctx, &y, argv[1]))
            return JS_EXCEPTION;
        if (JS_ToInt32(ctx, &z, argv[2]))
            return JS_EXCEPTION;
        if (JS_ToInt32(ctx, &t, argv[3]))
            return JS_EXCEPTION;
        lol::msg::info("stub: palset(%d, %d, %d, %d)\n", x, y, z, t);
        return JS_NewInt32(ctx, x);
    }

    JSValue api_pset(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv)
    {
        int x, y, z;
        if (JS_ToInt32(ctx, &x, argv[0]))
            return JS_EXCEPTION;
        if (JS_ToInt32(ctx, &y, argv[1]))
            return JS_EXCEPTION;
        if (JS_ToInt32(ctx, &z, argv[2]))
            return JS_EXCEPTION;
        lol::msg::info("stub: pset(%d, %d, %d)\n", x, y, z);
        return JS_NewInt32(ctx, x);
    }

    JSValue api_palt(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv)
    {
        int x, y;
        if (JS_ToInt32(ctx, &x, argv[0]))
            return JS_EXCEPTION;
        if (JS_ToInt32(ctx, &y, argv[1]))
            return JS_EXCEPTION;
        lol::msg::info("stub: palt(%d, %d)\n", x, y);
        return JS_NewInt32(ctx, x);
    }

    JSValue api_fget(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv)
    {
        int x, y;
        if (JS_ToInt32(ctx, &x, argv[0]))
            return JS_EXCEPTION;
        lol::msg::info("stub: fget(%d)\n", x);
        return JS_NewInt32(ctx, x);
    }

    JSValue api_flr(JSContext *ctx, JSValueConst this_val,
                    int argc, JSValueConst *argv)
    {
        int x, y;
        if (JS_ToInt32(ctx, &x, argv[0]))
            return JS_EXCEPTION;
        lol::msg::info("stub: flr(%d)\n", x);
        return JS_NewInt32(ctx, x);
    }

    JSValue api_rnd(JSContext *ctx, JSValueConst this_val,
                    int argc, JSValueConst *argv)
    {
        double x;
        if (argc == 0 || JS_ToFloat64(ctx, &x, argv[0]))
            x = 1.0;
        return JS_NewFloat64(ctx, lol::rand(x));
    }

    JSValue api_mget(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv)
    {
        int x, y;
        if (JS_ToInt32(ctx, &x, argv[0]))
            return JS_EXCEPTION;
        if (JS_ToInt32(ctx, &y, argv[1]))
            return JS_EXCEPTION;
        lol::msg::info("stub: mget(%d, %d)\n", x, y);
        return JS_NewInt32(ctx, x);
    }

    static void dump_error(JSContext *ctx)
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

    static int eval_buf(JSContext *ctx, std::string const &code,
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

    JSRuntime *m_rt;
    JSContext *m_ctx;

    std::string m_name;
    std::string m_code;
    int m_version = -1;

    z8::raccoon::memory m_rom;
    z8::raccoon::memory m_ram;
};

}

int main(int argc, char **argv)
{
    lol::sys::init(argc, argv);

    lol::getopt opt(argc, argv);

    for (;;)
    {
        int c = opt.parse();
        if (c == -1)
            break;

        switch (c)
        {
        default:
            return EXIT_FAILURE;
        }
    }

    lol::ivec2 win_size(z8::WINDOW_WIDTH, z8::WINDOW_HEIGHT);
    lol::Application app("zepto-8", win_size, 60.0f);

    if (argc >= 2 && lol::ends_with(argv[1], ".rcn.json"))
    {
        z8::raccoon::vm vm;
        vm.run(argv[1]);
        return EXIT_SUCCESS;
    }

    z8::player *player = new z8::player(win_size);

    if (argc >= 2)
    {
        player->load(argv[1]);
        player->run();
    }

    app.Run();

    return EXIT_SUCCESS;
}

