//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#include <lol/engine.h>

#include "vm.h"

namespace z8
{

using lol::msg;

vm::vm()
  : m_instructions(0)
{
    lua_State *l = GetLuaState();

    // Store a pointer to us in global state
    set_this(l);

    // Automatically yield every 1000 instructions
    lua_sethook(l, &vm::hook, LUA_MASKCOUNT, 1000);

    // Register our Lua module
    lol::LuaObjectDef::Register<vm>(l);

    ExecLuaFile("data/zepto8.lua");

    // Load font
    m_font.Load("data/font.png");

    // Clear memory
    ::memset(get_mem(), 0, SIZE_MEMORY);
}

vm::~vm()
{
}

void vm::set_this(lua_State *l)
{
    lua_pushlightuserdata(l, this);
    lua_setglobal(l, "\x01");
}

vm* vm::get_this(lua_State *l)
{
    lua_getglobal(l, "\x01");
    vm *ret = (vm *)lua_touserdata(l, -1);
    lua_remove(l, -1);
    return ret;
}

void vm::hook(lua_State *l, lua_Debug *)
{
    vm *that = get_this(l);

    // The value 135000 was found using trial and error
    that->m_instructions += 1000;
    if (that->m_instructions >= 135000)
        lua_yield(l, 0);
}

void vm::load(char const *name)
{
    m_cart.load(name);
}

void vm::run()
{
    // Start the cartridge!
    ExecLuaCode("run()");
}

void vm::step(float seconds)
{
    UNUSED(seconds);

    lua_State *l = GetLuaState();
    lua_getglobal(l, "_z8");
    lua_getfield(l, -1, "tick");
    lua_pcall(l, 0, 0, 0);

    m_instructions = 0;
}

const lol::LuaObjectLib* vm::GetLib()
{
    static const lol::LuaObjectLib lib = lol::LuaObjectLib(
        "_z8",

        // Statics
        {
            { "run",      &vm::api::run },
            { "menuitem", &vm::api::menuitem },
            { "cartdata", &vm::api::cartdata },
            { "reload",   &vm::api::reload },
            { "peek",     &vm::api::peek },
            { "poke",     &vm::api::poke },
            { "memcpy",   &vm::api::memcpy },
            { "memset",   &vm::api::memset },
            { "dget",     &vm::api::dget },
            { "dset",     &vm::api::dset },
            { "stat",     &vm::api::stat },
            { "printh",   &vm::api::printh },

            { "_update_buttons", &vm::api::update_buttons },
            { "btn",  &vm::api::btn },
            { "btnp", &vm::api::btnp },

            { "cursor", &vm::api::cursor },
            { "print",  &vm::api::print },

            { "max",   &vm::api::max },
            { "min",   &vm::api::min },
            { "mid",   &vm::api::mid },
            { "flr",   &vm::api::flr },
            { "cos",   &vm::api::cos },
            { "sin",   &vm::api::sin },
            { "atan2", &vm::api::atan2 },
            { "sqrt",  &vm::api::sqrt },
            { "abs",   &vm::api::abs },
            { "sgn",   &vm::api::sgn },
            { "rnd",   &vm::api::rnd },
            { "srand", &vm::api::srand },
            { "band",  &vm::api::band },
            { "bor",   &vm::api::bor },
            { "bxor",  &vm::api::bxor },
            { "bnot",  &vm::api::bnot },
            { "shl",   &vm::api::shl },
            { "shr",   &vm::api::shr },

            { "camera",   &vm::api::camera },
            { "circ",     &vm::api::circ },
            { "circfill", &vm::api::circfill },
            { "clip",     &vm::api::clip },
            { "cls",      &vm::api::cls },
            { "color",    &vm::api::color },
            { "fget",     &vm::api::fget },
            { "fset",     &vm::api::fset },
            { "line",     &vm::api::line },
            { "map",      &vm::api::map },
            { "mget",     &vm::api::mget },
            { "mset",     &vm::api::mset },
            { "pal",      &vm::api::pal },
            { "palt",     &vm::api::palt },
            { "pget",     &vm::api::pget },
            { "pset",     &vm::api::pset },
            { "rect",     &vm::api::rect },
            { "rectfill", &vm::api::rectfill },
            { "sget",     &vm::api::sget },
            { "sset",     &vm::api::sset },
            { "spr",      &vm::api::spr },
            { "sspr",     &vm::api::sspr },

            { "music", &vm::api::music },
            { "sfx",   &vm::api::sfx },

            { "time", &vm::api::time },

            { nullptr, nullptr }
        },

        // Methods
        {
            { nullptr, nullptr },
        },

        // Variables
        {
            { nullptr, nullptr, nullptr },
        });

    return &lib;
}

vm* vm::New(lua_State* l, int argc)
{
    // FIXME: I have no idea what this function is for
    UNUSED(l);
    msg::info("requesting new(%d) on vm\n", argc);
    return nullptr;
}

//
// System
//

int vm::api::run(lua_State *l)
{
    vm *that = get_this(l);

    // Initialise VM state (TODO: check what else to init)
    ::memset(that->m_buttons, 0, sizeof(that->m_buttons));

    // Load cartridge code and call _z8.run() on it
    lua_getglobal(l, "_z8");
    lua_getfield(l, -1, "run");
    luaL_loadstring(l, that->m_cart.get_lua().C());
    lua_pcall(l, 1, 0, 0);

    return 0;
}

int vm::api::menuitem(lua_State *l)
{
    UNUSED(l);
    msg::info("z8:stub:menuitem\n");
    return 0;
}

int vm::api::cartdata(lua_State *l)
{
    int x = (int)lua_tonumber(l, 1);
    msg::info("z8:stub:cartdata %d\n", x);
    return 0;
}

int vm::api::reload(lua_State *l)
{
    int dst = 0, src = 0, size = OFFSET_CODE;

    // PICO-8 fully reloads the cartridge if not all arguments are passed
    if (!lua_isnone(l, 3))
    {
        dst = (int)lua_toclamp64(l, 1) & 0xffff;
        src = (int)lua_toclamp64(l, 2) & 0xffff;
        size = (int)lua_toclamp64(l, 3);
    }

    if (size <= 0)
        return 0;

    size = size & 0xffff;

    // Attempting to write outside the memory area raises an error. Everything
    // else seems legal, especially reading from anywhere.
    if (dst + size > SIZE_MEMORY)
        return luaL_error(l, "bad memory access");

    vm *that = get_this(l);

    // If reading from after the cart, fill with zeroes
    if (src > OFFSET_CODE)
    {
        int amount = lol::min(size, 0x10000 - src);
        ::memset(that->get_mem(dst), 0, amount);
        dst += amount;
        src = (src + amount) & 0xffff;
        size -= amount;
    }

    // Now copy possibly legal data
    int amount = lol::min(size, OFFSET_CODE - src);
    ::memcpy(that->get_mem(dst), that->m_cart.get_rom().data() + src, amount);
    dst += amount;
    size -= amount;

    // If there is anything left to copy, it’s zeroes again
    ::memset(that->get_mem(dst), 0, size);

    return 0;
}

int vm::api::peek(lua_State *l)
{
    // Note: peek() is the same as peek(0)
    int addr = (int)lua_toclamp64(l, 1) & 0xffff;
    if (addr < 0 || addr >= SIZE_MEMORY)
        return 0;

    vm *that = get_this(l);
    lua_pushnumber(l, that->m_memory[addr]);
    return 1;
}

int vm::api::poke(lua_State *l)
{
    // Note: poke() is the same as poke(0, 0)
    int addr = (int)lua_toclamp64(l, 1) & 0xffff;
    int val = (int)lua_toclamp64(l, 2);
    if (addr < 0 || addr >= SIZE_MEMORY)
        return luaL_error(l, "bad memory access");

    vm *that = get_this(l);
    that->m_memory[addr] = (uint8_t)val;
    return 0;
}

int vm::api::memcpy(lua_State *l)
{
    int dst = (int)lua_toclamp64(l, 1) & 0xffff;
    int src = (int)lua_toclamp64(l, 2) & 0xffff;
    int size = (int)lua_toclamp64(l, 3);

    if (size <= 0)
        return 0;

    size = size & 0xffff;

    // Attempting to write outside the memory area raises an error. Everything
    // else seems legal, especially reading from anywhere.
    if (dst + size > SIZE_MEMORY)
        return luaL_error(l, "bad memory access");

    vm *that = get_this(l);

    // If source is outside main memory, this will be memset(0). But we
    // delay the operation in case the source and the destinations overlap.
    int saved_dst = dst, saved_size = 0;
    if (src > SIZE_MEMORY)
    {
        saved_size = lol::min(size, 0x10000 - src);
        dst += saved_size;
        src = (src + saved_size) & 0xffff;
        size -= saved_size;
    }

    // Now copy possibly legal data
    int amount = lol::min(size, SIZE_MEMORY - src);
    memmove(that->get_mem(dst), that->get_mem(src), amount);
    dst += amount;
    size -= amount;

    // Fill possible zeroes we saved before, and if there is still something
    // to copy, it’s zeroes again
    ::memset(that->get_mem(saved_dst), 0, saved_size);
    ::memset(that->get_mem(dst), 0, size);

    return 0;
}

int vm::api::memset(lua_State *l)
{
    int dst = (int)lua_toclamp64(l, 1) & 0xffff;
    int val = (int)lua_toclamp64(l, 2) & 0xffff;
    int size = (int)lua_toclamp64(l, 3);

    if (size <= 0)
        return 0;

    size = size & 0xffff;

    if (dst < 0 || dst + size >= SIZE_MEMORY)
        return luaL_error(l, "bad memory access");

    vm *that = get_this(l);
    ::memset(that->get_mem(dst), val, size);

    return 0;
}

int vm::api::dget(lua_State *l)
{
    int n = (int)lua_toclamp64(l, 1);
    uint32_t ret = 0;
    if (n >= 0 && n < SIZE_PERSISTENT / 4)
    {
        uint8_t const *p = get_this(l)->get_mem(OFFSET_PERSISTENT + n * 4);
        ret = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
    }
    lua_pushnumber(l, fixed2double(int32_t(ret)));
    return 1;
}

int vm::api::dset(lua_State *l)
{
    int n = (int)lua_toclamp64(l, 1);
    if (n >= 0 && n < SIZE_PERSISTENT / 4)
    {
        uint32_t x = (uint32_t)double2fixed(lua_tonumber(l, 2));
        uint8_t *p = get_this(l)->get_mem(OFFSET_PERSISTENT + n * 4);
        p[0] = x;
        p[1] = x >> 8;
        p[2] = x >> 16;
        p[3] = x >> 24;
    }
    return 0;
}

int vm::api::stat(lua_State *l)
{
    vm *that = get_this(l);
    int ret = 0, id = (int)lua_toclamp64(l, 1);

    if (id == 0)
    {
        // From the PICO-8 documentation:
        // x:0 returns current Lua memory useage (0..1024MB)
        ret = ((int)lua_gc(l, LUA_GCCOUNT, 0) << 4)
            + ((int)lua_gc(l, LUA_GCCOUNTB, 0) >> 6);
    }
    else if (id == 1)
    {
        // From the PICO-8 documentation:
        // x:1 returns cpu useage for last frame (1.0 means 100% at 30fps)
        // TODO
    }
    else if (id >= 16 && id <= 19)
    {
        // undocumented parameters for stat(n):
        // 16..19: the sfx currently playing on each channel or -1 for none
        ret = that->m_channels[id - 16].m_sfx;
    }
    else if (id >= 20 && id <= 23)
    {
        // undocumented parameters for stat(n):
        // 20..23: the currently playing row number (0..31) or -1 for none
        // TODO
    }

    lua_pushnumber(l, ret);
    return 1;
}

int vm::api::printh(lua_State *l)
{
    char const *str;
    if (lua_isnoneornil(l, 1))
        str = "false";
    else if (lua_isstring(l, 1))
        str = lua_tostring(l, 1);
    else
        str = lua_toboolean(l, 1) ? "true" : "false";

    fprintf(stdout, "%s\n", str);
    fflush(stdout);

    return 0;
}

//
// I/O
//

int vm::api::update_buttons(lua_State *l)
{
    vm *that = get_this(l);

    // Update button state
    for (int i = 0; i < 64; ++i)
    {
        if (that->m_buttons[1][i])
            ++that->m_buttons[0][i];
        else
            that->m_buttons[0][i] = 0;
    }

    return 0;
}

int vm::api::btn(lua_State *l)
{
    vm *that = get_this(l);

    if (lua_isnone(l, 1))
    {
        int bits = 0;
        for (int i = 0; i < 16; ++i)
            bits |= that->m_buttons[0][i] ? 1 << i : 0;
        lua_pushnumber(l, bits);
    }
    else
    {
        int index = (int)lua_tonumber(l, 1) + 8 * (int)lua_tonumber(l, 2);
        lua_pushboolean(l, that->m_buttons[0][index]);
    }

    return 1;
}

int vm::api::btnp(lua_State *l)
{
    auto was_pressed = [](int i)
    {
        // “Same as btn() but only true when the button was not pressed the last frame”
        if (i == 1)
            return true;
        // “btnp() also returns true every 4 frames after the button is held for 15 frames.”
        if (i > 15 && i % 4 == 0)
            return true;
        return false;
    };

    vm *that = get_this(l);

    if (lua_isnone(l, 1))
    {
        int bits = 0;
        for (int i = 0; i < 16; ++i)
            bits |= was_pressed(that->m_buttons[0][i]) ? 1 << i : 0;
        lua_pushnumber(l, bits);
    }
    else
    {
        int index = (int)lua_tonumber(l, 1) + 8 * (int)lua_tonumber(l, 2);
        lua_pushboolean(l, was_pressed(that->m_buttons[0][index]));
    }

    return 1;
}

//
// Deprecated
//

int vm::api::time(lua_State *l)
{
    vm *that = get_this(l);
    float time = lol::fmod(that->m_timer.Poll(), 65536.0f);
    lua_pushnumber(l, time < 32768.f ? time : time - 65536.0f);
    return 1;
}

} // namespace z8

