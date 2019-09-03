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
#include "quickjs/quickjs-libc.h"
}

#include "zepto8.h"
#include "player.h"

#define countof(x) (sizeof(x) / sizeof((x)[0]))

#define CPP_JS_CFUNC_DEF(name, length, func1) { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, 0, { length, JS_CFUNC_generic, { func1 } } }

static JSValue js_palt(JSContext *ctx, JSValueConst this_val,
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

static JSValue js_fget(JSContext *ctx, JSValueConst this_val,
                       int argc, JSValueConst *argv)
{
    int x, y;
    if (JS_ToInt32(ctx, &x, argv[0]))
        return JS_EXCEPTION;
    lol::msg::info("stub: fget(%d)\n", x);
    return JS_NewInt32(ctx, x);
}

static JSValue js_flr(JSContext *ctx, JSValueConst this_val,
                      int argc, JSValueConst *argv)
{
    int x, y;
    if (JS_ToInt32(ctx, &x, argv[0]))
        return JS_EXCEPTION;
    lol::msg::info("stub: flr(%d)\n", x);
    return JS_NewInt32(ctx, x);
}

static JSValue js_rnd(JSContext *ctx, JSValueConst this_val,
                      int argc, JSValueConst *argv)
{
    lol::msg::info("stub: rnd()\n");
    return JS_NewInt32(ctx, 0);
}

static JSValue js_mget(JSContext *ctx, JSValueConst this_val,
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

static const JSCFunctionListEntry js_rcn_funcs[] =
{
    CPP_JS_CFUNC_DEF("palt", 2, js_palt ),
    CPP_JS_CFUNC_DEF("fget", 1, js_fget ),
    CPP_JS_CFUNC_DEF("flr", 1, js_flr ),
    CPP_JS_CFUNC_DEF("mget", 2, js_mget ),
    CPP_JS_CFUNC_DEF("rnd", 0, js_rnd ),
};

static int eval_buf(JSContext *ctx, const char *buf, int buf_len,
                    const char *filename, int eval_flags)
{
    JSValue val;
    int ret;

    val = JS_Eval(ctx, buf, buf_len, filename, eval_flags);
    if (JS_IsException(val)) {
        js_std_dump_error(ctx);
        ret = -1;
    } else {
        ret = 0;
    }
    JS_FreeValue(ctx, val);
    return ret;
}

static int eval_file(JSContext *ctx, const char *filename, int module)
{
    uint8_t *buf;
    int ret, eval_flags;
    size_t buf_len;

    buf = js_load_file(ctx, &buf_len, filename);
    if (!buf) {
        perror(filename);
        exit(1);
    }

    if (module < 0) {
        module = JS_DetectModule((const char *)buf, buf_len);
    }
    if (module)
        eval_flags = JS_EVAL_TYPE_MODULE;
    else
        eval_flags = JS_EVAL_TYPE_GLOBAL;
    ret = eval_buf(ctx, (const char *)buf, buf_len, filename, eval_flags);
    js_free(ctx, buf);
    return ret;
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

    if (argc >= 2 && lol::ends_with(argv[1], ".rcn.js"))
    {
        JSRuntime *rt = JS_NewRuntime();
        if (!rt)
            ; /* TODO */

        JSContext *ctx = JS_NewContext(rt);
        if (!ctx)
            ; /* TODO */

#if 0
        // This is required for console.log()
        js_std_add_helpers(ctx, 0, nullptr);
#endif

        // Add functions to global scope
        auto global_obj = JS_GetGlobalObject(ctx);
        JS_SetPropertyFunctionList(ctx, global_obj, js_rcn_funcs, countof(js_rcn_funcs));
        JS_FreeValue(ctx, global_obj);

#if 0
        size_t buf_len;
        uint8_t *buf = js_load_file(ctx, &buf_len, argv[1]);
        if (!buf)
            ; /* TODO */

        JSValue val = JS_ParseJSON(ctx, (char const *)buf, buf_len, argv[1]);
#endif

        eval_file(ctx, argv[1], -1);

        char const *code = "init(); update(); draw();";
        eval_buf(ctx, code, strlen(code), "<code>", JS_EVAL_TYPE_GLOBAL);

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

