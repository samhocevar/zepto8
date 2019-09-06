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

#include <lol/engine.h>

extern "C" {
#include "quickjs/quickjs.h"
}

#include "zepto8.h"
#include "player.h"
#include "raccoon/memory.h"

namespace z8::raccoon
{

class vm : z8::vm_base
{
    friend class z8::player;

public:
    vm();
    virtual ~vm();

    virtual void load(char const *name);
    virtual void run();
    virtual bool step(float seconds);

    virtual void render(lol::u8vec4 *screen) const;

    virtual void button(int index, int state);
    virtual void mouse(lol::ivec2 coords, int buttons);
    virtual void keyboard(char ch);

private:
    std::string get_property_str(JSValue obj, char const *name);

    JSValue api_read(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv);
    JSValue api_write(JSContext *ctx, JSValueConst this_val,
                      int argc, JSValueConst *argv);
    JSValue api_palset(JSContext *ctx, JSValueConst this_val,
                       int argc, JSValueConst *argv);
    JSValue api_pset(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv);
    JSValue api_palm(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv);
    JSValue api_palt(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv);
    JSValue api_btnp(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv);
    JSValue api_fget(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv);
    JSValue api_cls(JSContext *ctx, JSValueConst this_val,
                    int argc, JSValueConst *argv);
    JSValue api_cam(JSContext *ctx, JSValueConst this_val,
                    int argc, JSValueConst *argv);
    JSValue api_map(JSContext *ctx, JSValueConst this_val,
                    int argc, JSValueConst *argv);
    JSValue api_rect(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv);
    JSValue api_rectfill(JSContext *ctx, JSValueConst this_val,
                         int argc, JSValueConst *argv);
    JSValue api_spr(JSContext *ctx, JSValueConst this_val,
                    int argc, JSValueConst *argv);
    JSValue api_print(JSContext *ctx, JSValueConst this_val,
                      int argc, JSValueConst *argv);
    JSValue api_rnd(JSContext *ctx, JSValueConst this_val,
                    int argc, JSValueConst *argv);
    JSValue api_mid(JSContext *ctx, JSValueConst this_val,
                    int argc, JSValueConst *argv);
    JSValue api_mget(JSContext *ctx, JSValueConst this_val,
                     int argc, JSValueConst *argv);

    static void dump_error(JSContext *ctx);

    static int eval_buf(JSContext *ctx, std::string const &code,
                        const char *filename, int eval_flags);

private:
    JSRuntime *m_rt;
    JSContext *m_ctx;

    std::string m_code;
    std::string m_name, m_link, m_host;
    int m_version = -1;

    memory m_rom;
    memory m_ram;
};

}

