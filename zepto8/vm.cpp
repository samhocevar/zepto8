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
    ExecLuaFile("zepto8.lua");

    m_memory.resize(OFFSET_VERSION);
    m_screen.resize(128 * 128);

    for (int n = OFFSET_SCREEN; n < OFFSET_VERSION; ++n)
        m_memory[n] = lol::rand(0, 256);

    m_camera = new lol::Camera();
    m_camera->SetView(lol::mat4(1.f));
    m_camera->SetProjection(lol::mat4::ortho(0.f, 640.f, 0.f, 640.f, -100.f, 100.f));
    lol::Scene& scene = lol::Scene::GetScene();
    scene.PushCamera(m_camera);
    lol::Ticker::Ref(m_camera);

    // FIXME: the image gets deleted by TextureImage class, it
    // does not seem right to me.
    //m_tile = lol::Tiler::Register("fuck", new lol::Image(*m_screen));
    auto img = new lol::Image(lol::ivec2(128, 128));
    img->Unlock(img->Lock<lol::PixelFormat::RGBA_8>());
    m_tile = lol::Tiler::Register("fuck", new lol::Image(*img));
    //m_tile = lol::Tiler::Register("rulez.p8.png");
    m_tile->define_tile(lol::ibox2(0, 0, 128, 128));
}

vm::~vm()
{
    lol::Tiler::Deregister(m_tile);

    lol::Scene& scene = lol::Scene::GetScene();
    lol::Ticker::Unref(m_camera);
    scene.PopCamera(m_camera);
}

void vm::TickGame(float seconds)
{
    lol::WorldEntity::TickGame(seconds);
}

void vm::TickDraw(float seconds, lol::Scene &scene)
{
    lol::WorldEntity::TickDraw(seconds, scene);

    static lol::u8vec4 const palette[] =
    {
        lol::u8vec4(  0,   0,   0, 255), // black
        lol::u8vec4( 32,  51, 123, 255), // dark_blue
        lol::u8vec4(126,  37,  83, 255), // dark_purple
        lol::u8vec4(  0, 144,  61, 255), // dark_green
        lol::u8vec4(171,  82,  54, 255), // brown
        lol::u8vec4( 52,  54,  53, 255), // dark_gray
        lol::u8vec4(194, 195, 199, 255), // light_gray
        lol::u8vec4(255, 241, 232, 255), // white
        lol::u8vec4(255,   0,  77, 255), // red
        lol::u8vec4(255, 155,   0, 255), // orange
        lol::u8vec4(255, 231,  39, 255), // yellow
        lol::u8vec4(  0, 226,  50, 255), // green
        lol::u8vec4( 41, 173, 255, 255), // blue
        lol::u8vec4(132, 112, 169, 255), // indigo
        lol::u8vec4(255, 119, 168, 255), // pink
        lol::u8vec4(255, 214, 197, 255), // peach
    };

    for (int n = 0; n < 128 * 128 / 2; ++n)
    {
        uint8_t data = m_memory[OFFSET_SCREEN + n];
        m_screen[2 * n] = palette[data >> 4];
        m_screen[2 * n + 1] = palette[data & 0xf];
    }

    m_tile->GetTexture()->SetData(m_screen.data());

    int delta = (640 - 512) / 2;
    scene.AddTile(m_tile, 0, lol::vec3(delta, delta, 10.f), 0, lol::vec2(4.f), 0.f);
}

const lol::LuaObjectLib* vm::GetLib()
{
    static const lol::LuaObjectLib lib = lol::LuaObjectLib(
        "_z8",

        // Statics
        {
            { "btn",    &vm::btn },
            { "btnp",   &vm::btnp },
            { "cls",    &vm::cls },
            { "cos",    &vm::cos },
            { "cursor", &vm::cursor },
            { "flr",    &vm::flr },
            { "music",  &vm::music },
            { "pget",   &vm::pget },
            { "pset",   &vm::pset },
            { "rnd",    &vm::rnd },
            { "sget",   &vm::sget },
            { "sset",   &vm::sset },
            { "sin",    &vm::sin },
            { "spr",    &vm::spr },
            { "sspr",   &vm::sspr },
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
    msg::info("requesting new(%d) on vm\n", argc);
}

int vm::btn(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x;
    lol::LuaBool ret;
    s >> x;
    ret = false;
    msg::info("z8:stub:btn(%d)\n", (int)x.GetValue());
    return s << ret;
}

int vm::btnp(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x;
    lol::LuaBool ret;
    s >> x;
    ret = false;
    msg::info("z8:stub:btnp(%d)\n", (int)x.GetValue());
    return s << ret;
}

int vm::cls(lol::LuaState *l)
{
    lol::LuaStack s(l);
    msg::info("z8:stub:cls()\n");
    return 0;
}

int vm::cos(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::cos(x.GetValue() / lol::F_TAU);
    return s << ret;
}

int vm::cursor(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y;
    s >> x >> y;
    msg::info("z8:stub:cursor(%f,%f)\n", x.GetValue(), y.GetValue());
    return 0;
}

int vm::flr(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::floor(x.GetValue());
    return s << ret;
}

int vm::music(lol::LuaState *l)
{
    lol::LuaStack s(l);
    msg::info("z8:stub:music()\n");
    return 0;
}

int vm::pget(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;
    ret = 0;
    msg::info("z8:stub:pget(%d, %d)\n", (int)x.GetValue(), (int)y.GetValue());
    return s << ret;
}

int vm::pset(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y;
    s >> x >> y;
    msg::info("z8:stub:pset(%d, %d...)\n", (int)x.GetValue(), (int)y.GetValue());
    return 0;
}

int vm::rnd(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::rand(x.GetValue());
    return s << ret;
}

int vm::sget(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;
    ret = 0;
    msg::info("z8:stub:sget(%d, %d)\n", (int)x.GetValue(), (int)y.GetValue());
    return s << ret;
}

int vm::sset(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y;
    s >> x >> y;
    msg::info("z8:stub:sset(%d, %d...)\n", (int)x.GetValue(), (int)y.GetValue());
    return 0;
}

int vm::sin(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::sin(x.GetValue() / -lol::F_TAU);
    return s << ret;
}

int vm::spr(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat n, x, y, w, h, flip_x, flip_y;
    s >> n >> x >> y >> w >> h >> flip_x >> flip_y;
    msg::info("z8:stub:spr(%d, %d, %d...)\n", (int)n, (int)x, (int)y);
    return 0;
}

int vm::sspr(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat sx, sy, sw, sh, dx, dy, dw, dh, flip_x, flip_y;
    s >> sx >> sy >> sw >> sh >> dx >> dy;
    msg::info("z8:stub:sspr(%d, %d, %d, %d, %d, %d...)\n", (int)sx, (int)sy, (int)sw, (int)sh, (int)dx, (int)dy);
    return 0;
}

} // namespace z8

