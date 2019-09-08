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

    virtual std::function<void(void *, int)> get_streamer(int channel);

    virtual void button(int index, int state);
    virtual void mouse(lol::ivec2 coords, int buttons);
    virtual void keyboard(char ch);

private:
    std::string get_property_str(JSValue obj, char const *name);

    JSValue api_read(int argc, JSValueConst *argv);
    JSValue api_write(int argc, JSValueConst *argv);
    JSValue api_palset(int argc, JSValueConst *argv);
    JSValue api_fget(int argc, JSValueConst *argv);
    JSValue api_fset(int argc, JSValueConst *argv);
    JSValue api_mget(int argc, JSValueConst *argv);
    JSValue api_mset(int argc, JSValueConst *argv);

    JSValue api_pset(int argc, JSValueConst *argv);
    JSValue api_palm(int argc, JSValueConst *argv);
    JSValue api_palt(int argc, JSValueConst *argv);
    JSValue api_btnp(int argc, JSValueConst *argv);
    JSValue api_cls(int argc, JSValueConst *argv);
    JSValue api_cam(int argc, JSValueConst *argv);
    JSValue api_map(int argc, JSValueConst *argv);
    JSValue api_rect(int argc, JSValueConst *argv);
    JSValue api_rectfill(int argc, JSValueConst *argv);
    JSValue api_spr(int argc, JSValueConst *argv);
    JSValue api_print(int argc, JSValueConst *argv);
    JSValue api_rnd(int argc, JSValueConst *argv);
    JSValue api_mid(int argc, JSValueConst *argv);
    JSValue api_mus(int argc, JSValueConst *argv);

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

