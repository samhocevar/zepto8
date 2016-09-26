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
{
    // Register our Lua module
    lol::LuaObjectDef::Register<vm>(GetLuaState());
    ExecLuaFile("data/zepto8.lua");

    // Initialise the PRNG
    ExecLuaCode("srand(0)");

    // Load font
    m_font.Load("data/font.png");

    // Bind controls
    m_controller = new lol::Controller("default controller");

    m_input << lol::InputProfile::Keyboard(0, "Left");
    m_input << lol::InputProfile::Keyboard(1, "Right");
    m_input << lol::InputProfile::Keyboard(2, "Up");
    m_input << lol::InputProfile::Keyboard(3, "Down");
    m_input << lol::InputProfile::Keyboard(4, "Z");
    m_input << lol::InputProfile::Keyboard(4, "C");
    m_input << lol::InputProfile::Keyboard(4, "N");
    m_input << lol::InputProfile::Keyboard(4, "Insert");
    m_input << lol::InputProfile::Keyboard(5, "X");
    m_input << lol::InputProfile::Keyboard(5, "V");
    m_input << lol::InputProfile::Keyboard(5, "M");
    m_input << lol::InputProfile::Keyboard(5, "Delete");

    m_input << lol::InputProfile::Keyboard(6, "Return");

    m_input << lol::InputProfile::Keyboard(8, "S");
    m_input << lol::InputProfile::Keyboard(9, "F");
    m_input << lol::InputProfile::Keyboard(10, "E");
    m_input << lol::InputProfile::Keyboard(11, "D");
    m_input << lol::InputProfile::Keyboard(12, "LShift");
    m_input << lol::InputProfile::Keyboard(12, "A");
    m_input << lol::InputProfile::Keyboard(13, "Tab");
    m_input << lol::InputProfile::Keyboard(13, "Q");

    m_controller->Init(m_input);
    m_controller->SetInputCount(64 /* keys */, 0 /* axes */);

    // Create an ortho camera
    m_scenecam = new lol::Camera();
    m_scenecam->SetView(lol::mat4(1.f));
    m_scenecam->SetProjection(lol::mat4::ortho(0.f, 600.f, 0.f, 600.f, -100.f, 100.f));
    lol::Scene& scene = lol::Scene::GetScene();
    scene.PushCamera(m_scenecam);
    lol::Ticker::Ref(m_scenecam);

    // FIXME: the image gets deleted by TextureImage class, it
    // does not seem right to me.
    auto img = new lol::Image(lol::ivec2(128, 128));
    img->Unlock(img->Lock<lol::PixelFormat::RGBA_8>()); // ensure RGBA_8 is present
    m_tile = lol::Tiler::Register("fuck", new lol::Image(*img), lol::ivec2(128, 128), lol::ivec2(1, 1));

    // Allocate memory
    m_memory.resize(SIZE_MEMORY);
    m_screen.resize(128 * 128);

    // Clear screen
    ::memset(m_memory.data() + OFFSET_SCREEN, 0, SIZE_SCREEN);
}

vm::~vm()
{
    lol::Tiler::Deregister(m_tile);

    lol::Scene& scene = lol::Scene::GetScene();
    lol::Ticker::Unref(m_scenecam);
    scene.PopCamera(m_scenecam);
}

void vm::TickGame(float seconds)
{
    lol::WorldEntity::TickGame(seconds);

    luaL_dostring(GetLuaState(), "_z8.tick()");
}

void vm::TickDraw(float seconds, lol::Scene &scene)
{
    lol::WorldEntity::TickDraw(seconds, scene);

    lol::Renderer::Get()->SetClearColor(lol::Color::black);

    static lol::u8vec4 const palette[] =
    {
        lol::u8vec4(0x00, 0x00, 0x00, 0xff), // black
        lol::u8vec4(0x1d, 0x2b, 0x53, 0xff), // dark_blue
        lol::u8vec4(0x7e, 0x25, 0x53, 0xff), // dark_purple
        lol::u8vec4(0x00, 0x87, 0x51, 0xff), // dark_green
        lol::u8vec4(0xab, 0x52, 0x36, 0xff), // brown
        lol::u8vec4(0x5f, 0x57, 0x4f, 0xff), // dark_gray
        lol::u8vec4(0xc2, 0xc3, 0xc7, 0xff), // light_gray
        lol::u8vec4(0xff, 0xf1, 0xe8, 0xff), // white
        lol::u8vec4(0xff, 0x00, 0x4d, 0xff), // red
        lol::u8vec4(0xff, 0xa3, 0x00, 0xff), // orange
        lol::u8vec4(0xff, 0xec, 0x27, 0xff), // yellow
        lol::u8vec4(0x00, 0xe4, 0x36, 0xff), // green
        lol::u8vec4(0x29, 0xad, 0xff, 0xff), // blue
        lol::u8vec4(0x83, 0x76, 0x9c, 0xff), // indigo
        lol::u8vec4(0xff, 0x77, 0xa8, 0xff), // pink
        lol::u8vec4(0xff, 0xcc, 0xaa, 0xff), // peach
    };

    for (int n = 0; n < 128 * 128 / 2; ++n)
    {
        uint8_t data = m_memory[OFFSET_SCREEN + n];
        m_screen[2 * n] = palette[m_pal[1][data & 0xf]];
        m_screen[2 * n + 1] = palette[m_pal[1][data >> 4]];
    }

    m_tile->GetTexture()->Bind();
    m_tile->GetTexture()->SetData(m_screen.data());

    int delta = (600 - 512) / 2;
    scene.AddTile(m_tile, 0, lol::vec3(delta, delta, 10.f), 0, lol::vec2(4.f), 0.f);
}

const lol::LuaObjectLib* vm::GetLib()
{
    static const lol::LuaObjectLib lib = lol::LuaObjectLib(
        "_z8",

        // Statics
        {
            { "run",      &vm::run },
            { "flip",     &vm::flip },
            { "menuitem", &vm::menuitem },
            { "cartdata", &vm::cartdata },
            { "reload",   &vm::reload },
            { "peek",     &vm::peek },
            { "poke",     &vm::poke },
            { "memcpy",   &vm::memcpy },
            { "memset",   &vm::memset },
            { "dget",     &vm::dget },
            { "dset",     &vm::dset },
            { "stat",     &vm::stat },
            { "printh",   &vm::printh },

            { "_update_buttons", &vm::update_buttons },
            { "btn",  &vm::btn },
            { "btnp", &vm::btnp },

            { "cursor", &vm::cursor },
            { "print",  &vm::print },

            { "max",   &vm::max },
            { "min",   &vm::min },
            { "mid",   &vm::mid },
            { "flr",   &vm::flr },
            { "cos",   &vm::cos },
            { "sin",   &vm::sin },
            { "atan2", &vm::atan2 },
            { "sqrt",  &vm::sqrt },
            { "abs",   &vm::abs },
            { "sgn",   &vm::sgn },
            { "rnd",   &vm::rnd },
            { "srand", &vm::srand },
            { "band",  &vm::band },
            { "bor",   &vm::bor },
            { "bxor",  &vm::bxor },
            { "bnot",  &vm::bnot },
            { "shl",   &vm::shl },
            { "shr",   &vm::shr },

            { "camera",   &vm::camera },
            { "circ",     &vm::circ },
            { "circfill", &vm::circfill },
            { "clip",     &vm::clip },
            { "cls",      &vm::cls },
            { "color",    &vm::color },
            { "fget",     &vm::fget },
            { "fset",     &vm::fset },
            { "line",     &vm::line },
            { "map",      &vm::map },
            { "mget",     &vm::mget },
            { "mset",     &vm::mset },
            { "pal",      &vm::pal },
            { "palt",     &vm::palt },
            { "pget",     &vm::pget },
            { "pset",     &vm::pset },
            { "rect",     &vm::rect },
            { "rectfill", &vm::rectfill },
            { "sget",     &vm::sget },
            { "sset",     &vm::sset },
            { "spr",      &vm::spr },
            { "sspr",     &vm::sspr },

            { "music", &vm::music },
            { "sfx",   &vm::sfx },

            { "time", &vm::time },

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

vm* vm::New(lol::LuaState* l, int argc)
{
    // FIXME: I have no idea what this function is for
    UNUSED(l);
    msg::info("requesting new(%d) on vm\n", argc);
    return nullptr;
}

//
// System
//

int vm::run(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    // Initialise VM state (TODO: check what else to init)
    ::memset(that->m_buttons, 0, sizeof(that->m_buttons));

    // From the PICO-8 documentation:
    // “The draw state is reset each time a program is run. This is equivalent to calling:
    // clip() camera() pal() color()”
    //
    // Note from Sam: this should probably be color(6) instead.
    if (luaL_loadstring(l, "clip() camera() pal() color(6)") == 0)
        lua_pcall(l, 0, LUA_MULTRET, 0);

    // Load cartridge code into a global identifier
    if (luaL_loadstring(l, that->m_cart.get_lua().C()) == 0)
    {
        lua_pushvalue(l, -1);
        lua_setglobal(l, ".code");
    }

    // Execute cartridge code
    lua_getglobal(l, ".code");
    lua_pcall(l, 0, LUA_MULTRET, 0);

    // Run cartridge initialisation routine
    if (luaL_loadstring(l, "if _init ~= nil then _init() end") == 0)
        lua_pcall(l, 0, LUA_MULTRET, 0);

    return 0;
}

int vm::flip(lol::LuaState *l)
{
    UNUSED(l);
    // Only print stub message the first time, or we’ll flood the console
    static bool show_stub = true;
    if (show_stub)
        msg::info("z8:stub:flip\n");
    show_stub = false;
    return 0;
}

int vm::menuitem(lol::LuaState *l)
{
    UNUSED(l);
    msg::info("z8:stub:menuitem\n");
    return 0;
}

int vm::cartdata(lol::LuaState *l)
{
    int x = (int)lua_tonumber(l, 1);
    msg::info("z8:stub:cartdata %d\n", x);
    return 0;
}

int vm::reload(lol::LuaState *l)
{
    UNUSED(l);
    msg::info("z8:stub:reload\n");
    return 0;
}

int vm::peek(lol::LuaState *l)
{
    int addr = (int)lua_tonumber(l, 1);
    if (addr < 0 || addr >= SIZE_MEMORY)
        return 0;

    vm *that = (vm *)vm::Find(l);
    lua_pushnumber(l, that->m_memory[addr]);
    return 1;
}

int vm::poke(lol::LuaState *l)
{
    int addr = (int)lua_tonumber(l, 1);
    int val = (int)lua_tonumber(l, 2);
    if (addr < 0 || addr >= SIZE_MEMORY)
        return 0;

    vm *that = (vm *)vm::Find(l);
    that->m_memory[addr] = (uint16_t)val;
    return 0;
}

int vm::memcpy(lol::LuaState *l)
{
    int dst = lua_tonumber(l, 1);
    int src = lua_tonumber(l, 2);
    int size = lua_tonumber(l, 3);

    /* FIXME: should the memory wrap around maybe? */
    if (dst >= 0 && src >= 0 && size > 0
         && dst < SIZE_MEMORY && src < SIZE_MEMORY
         && src + size <= SIZE_MEMORY && dst + size <= SIZE_MEMORY)
    {
        vm *that = (vm *)vm::Find(l);
        memmove(that->m_memory.data() + dst, that->m_memory.data() + src, size);
    }

    return 0;
}

int vm::memset(lol::LuaState *l)
{
    int dst = lua_tonumber(l, 1);
    int val = lua_tonumber(l, 2);
    int size = lua_tonumber(l, 3);

    if (dst >= 0 && size > 0
         && dst < SIZE_MEMORY && dst + size <= SIZE_MEMORY)
    {
        vm *that = (vm *)vm::Find(l);
        ::memset(that->m_memory.data() + dst, val, size);
    }

    return 0;
}

int vm::dget(lol::LuaState *l)
{
    msg::info("z8:stub:dget\n");
    lua_pushnumber(l, 0);
    return 1;
}

int vm::dset(lol::LuaState *l)
{
    UNUSED(l);
    msg::info("z8:stub:dset\n");
    return 0;
}

int vm::stat(lol::LuaState *l)
{
    msg::info("z8:stub:stat\n");
    lua_pushnumber(l, 0);
    return 1;
}

int vm::printh(lol::LuaState *l)
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

int vm::update_buttons(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    // Update button state
    for (int i = 0; i < 64; ++i)
    {
        if (that->m_controller->IsKeyPressed(i))
            ++that->m_buttons[i];
        else
            that->m_buttons[i] = 0;
    }

    return 0;
}

int vm::btn(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    if (lua_isnone(l, 1))
    {
        int bits = 0;
        for (int i = 0; i < 16; ++i)
            bits |= that->m_buttons[i] ? 1 << i : 0;
        lua_pushnumber(l, bits);
    }
    else
    {
        int index = (int)lua_tonumber(l, 1) + 8 * (int)lua_tonumber(l, 2);
        lua_pushboolean(l, that->m_buttons[index]);
    }

    return 1;
}

int vm::btnp(lol::LuaState *l)
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

    vm *that = (vm *)vm::Find(l);

    if (lua_isnone(l, 1))
    {
        int bits = 0;
        for (int i = 0; i < 16; ++i)
            bits |= was_pressed(that->m_buttons[i]) ? 1 << i : 0;
        lua_pushnumber(l, bits);
    }
    else
    {
        int index = (int)lua_tonumber(l, 1) + 8 * (int)lua_tonumber(l, 2);
        lua_pushboolean(l, was_pressed(that->m_buttons[index]));
    }

    return 1;
}

//
// Sound
//

int vm::music(lol::LuaState *l)
{
    UNUSED(l);
    msg::info("z8:stub:music\n");
    return 0;
}

int vm::sfx(lol::LuaState *l)
{
    UNUSED(l);
    msg::info("z8:stub:sfx\n");
    return 0;
}

//
// Deprecated
//

int vm::time(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);
    float time = lol::fmod(that->m_timer.Poll(), 65536.0f);
    lua_pushnumber(l, time < 32768.f ? time : time - 65536.0f);
    return 1;
}

} // namespace z8

