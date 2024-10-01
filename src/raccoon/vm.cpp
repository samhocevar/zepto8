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

#include <lol/engine.h> // lol::sys
#include <lol/file>   // lol::file::read
#include <lol/vector> // lol::u8vec4
#include <lol/msg>    // lol::msg

extern "C" {
#include "3rdparty/quickjs/quickjs.h"
}

#include "zepto8.h"
#include "raccoon/vm.h"
#include "bios.h" // TODO: remove references to PICO-8 stuff
#include "bindings/js.h"

namespace z8::raccoon
{

static std::string get_property_str(JSContext *ctx, JSValue obj,
                                    char const *name);
static void dump_error(JSContext *ctx);
static int eval_buf(JSContext *ctx, std::string const &code,
                    const char *filename, int eval_flags);

vm::vm()
{
    m_rt = JS_NewRuntime();
    m_ctx = JS_NewContext(m_rt);

    bindings::js::init(m_ctx, this);
}

vm::~vm()
{
}

void vm::load(std::string const &file)
{
    std::string s;
    if (!lol::file::read(lol::sys::get_data_path(file), s))
        return;

    lol::msg::debug("loaded file %s\n", file.c_str());

    JSValue bin = JS_ParseJSON(m_ctx, s.c_str(), s.length(), file.c_str());
    if (JS_IsException(bin))
    {
        dump_error(m_ctx);
        JS_FreeValue(m_ctx, bin);
        return;
    }

    m_name = get_property_str(m_ctx, bin, "name");
    m_host = get_property_str(m_ctx, bin, "host");
    m_link = get_property_str(m_ctx, bin, "link");

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
                    m_rom[offset++] = (int)std::strtoul(str, nullptr, 16);
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

    memset(&m_ram, 0, sizeof(m_ram));
    memcpy(&m_ram, &m_rom, offsetof(decltype(m_ram), end_of_rom));

    // Initialise gamepad state
    m_ram.gamepad.layouts[0] = 0;
    m_ram.gamepad.layouts[1] = 1;
    m_ram.gamepad.layouts[2] = 1;
    m_ram.gamepad.layouts[3] = 1;

    // Load the mini JS api
    std::string js_api =
        "min = Math.min;\n"
        "max = Math.max;\n"
        "sin = Math.sin;\n"
        "cos = Math.cos;\n"
        "atan2 = Math.atan2;\n"
        "abs = Math.abs;\n"
        "flr = Math.floor;\n"
        "ceil = Math.ceil;\n"
        "sign = Math.sign;\n"
        "sqrt = Math.sqrt;\n"
        "r = rnd; l = line; p = pset; c = cls; b = btn;\n";
    eval_buf(m_ctx, js_api, "<js_api>", JS_EVAL_TYPE_GLOBAL);

    eval_buf(m_ctx, m_code, "<load_cart>", JS_EVAL_TYPE_GLOBAL);

    std::string code =
        "if (typeof init != 'undefined') init();\n";
    eval_buf(m_ctx, code, "<run_code>", JS_EVAL_TYPE_GLOBAL);
}

bool vm::step(float /* seconds */)
{
    std::string code =
        "if (typeof update != 'undefined') update();\n"
        "if (typeof draw != 'undefined') draw();\n";
    eval_buf(m_ctx, code, "<step_code>", JS_EVAL_TYPE_GLOBAL);

    m_ram.gamepad.prev_buttons = m_ram.gamepad.buttons;
    m_ram.gamepad.buttons.fill(0);

    return true;
}

void vm::button(int index, int state)
{
    if (state)
        m_ram.gamepad.buttons[0] |= 1 << index;
}

void vm::mouse(lol::ivec2 coords, int buttons)
{
}

void vm::text(char ch)
{
}

void vm::render(lol::u8vec4 *screen) const
{
    /* Precompute the current palette for pairs of pixels */
    struct { lol::u8vec4 a, b; } lut[256];
    for (int n = 0; n < 256; ++n)
    {
        lut[n].a = lol::u8vec4(m_ram.palette[n % 16].color, 0xff);
        lut[n].b = lol::u8vec4(m_ram.palette[n / 16].color, 0xff);
    }

    /* Render actual screen */
    for (auto &line : m_ram.screen.data)
    for (uint8_t p : line)
    {
        *screen++ = lut[p].a;
        *screen++ = lut[p].b;
    }
}

int vm::get_ansi_color(uint8_t c) const
{
    // FIXME: this is the PICO-8 palette for now
    static int const ansi_palette[] =
    {
         16, // 000000 → 000000
         17, // 1d2b53 → 00005f
         89, // 7e2553 → 87005f
         29, // 008751 → 00875f
        131, // ab5236 → ab5236
        240, // 5f574f → 5f5f5f
        251, // c2c3c7 → c6c6c6
        230, // fff1e8 → ffffdf
        197, // ff004d → ff005f
        214, // ffa300 → ffaf00
        220, // ffec27 → ffdf00
         47, // 00e436 → 00ff5f
         39, // 29adff → 00afff
        103, // 83769c → 8787af
        211, // ff77a8 → f787af
        223, // ffccaa → ffdfaf
    };

    return ansi_palette[c & 15];
}

std::string const &vm::get_code() const
{
    return m_code;
}

u4mat2<128, 128> const &vm::get_front_screen() const
{
    return m_ram.screen;
}

void vm::get_audio(void* buffer, size_t frames)
{
    memset(buffer, 0, frames);
}

std::tuple<uint8_t *, size_t> vm::ram()
{
    return std::make_tuple(&m_ram[0], sizeof(m_ram));
}

std::tuple<uint8_t *, size_t> vm::rom()
{
    return std::make_tuple(&m_rom[0], offsetof(decltype(m_rom), end_of_rom));
}

static std::string get_property_str(JSContext *ctx, JSValue obj, char const *name)
{
    JSValue prop = JS_GetPropertyStr(ctx, obj, name);
    if (JS_IsUndefined(prop))
        return "";

    char const *tmp = JS_ToCString(ctx, prop);
    std::string ret(tmp);
    JS_FreeCString(ctx, tmp);
    JS_FreeValue(ctx, prop);
    return ret;
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

} // namespace z8::raccoon

