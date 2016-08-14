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

    // Initialise VM memory
    m_memory.resize(OFFSET_VERSION);
    m_screen.resize(128 * 128);

    for (int n = OFFSET_SCREEN; n < OFFSET_VERSION; ++n)
        m_memory[n] = lol::rand(0, 2);

    // Create an ortho camera
    m_camera = new lol::Camera();
    m_camera->SetView(lol::mat4(1.f));
    m_camera->SetProjection(lol::mat4::ortho(0.f, 600.f, 0.f, 600.f, -100.f, 100.f));
    lol::Scene& scene = lol::Scene::GetScene();
    scene.PushCamera(m_camera);
    lol::Ticker::Ref(m_camera);

    // FIXME: the image gets deleted by TextureImage class, it
    // does not seem right to me.
    auto img = new lol::Image(lol::ivec2(128, 128));
    img->Unlock(img->Lock<lol::PixelFormat::RGBA_8>()); // ensure RGBA_8 is present
    m_tile = lol::Tiler::Register("fuck", new lol::Image(*img), lol::ivec2(128, 128), lol::ivec2(1, 1));
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

    ExecLuaCode("_update()");
    ExecLuaCode("_draw()");
}

void vm::TickDraw(float seconds, lol::Scene &scene)
{
    lol::WorldEntity::TickDraw(seconds, scene);

    lol::Renderer::Get()->SetClearColor(lol::Color::black);

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
        m_screen[2 * n] = palette[data & 0xf];
        m_screen[2 * n + 1] = palette[data >> 4];
    }

    m_tile->GetTexture()->SetData(m_screen.data());

    int delta = (600 - 512) / 2;
    scene.AddTile(m_tile, 0, lol::vec3(delta, delta, 10.f), 0, lol::vec2(4.f), 0.f);
}

void vm::setpixel(int x, int y, int color)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 128)
        return;

    int offset = OFFSET_SCREEN + (128 * y + x) / 2;
    int m1 = (x & 1) ? 0x0f : 0xf0;
    int m2 = (x & 1) ? color << 4 : color;
    m_memory[offset] = (m_memory[offset] & m1) | m2;
}

int vm::getpixel(int x, int y)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 128)
        return 0;

    int offset = OFFSET_SCREEN + (128 * y + x) / 2;
    return (x & 1) ? m_memory[offset] >> 4 : m_memory[offset] & 0xf;
}

void vm::setspixel(int x, int y, int color)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 128)
        return;

    int offset = OFFSET_GFX + (128 * y + x) / 2;
    int m1 = (x & 1) ? 0x0f : 0xf0;
    int m2 = (x & 1) ? color << 4 : color;
    m_memory[offset] = (m_memory[offset] & m1) | m2;
}

int vm::getspixel(int x, int y)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 128)
        return 0;

    int offset = OFFSET_GFX + (128 * y + x) / 2;
    return (x & 1) ? m_memory[offset] >> 4 : m_memory[offset] & 0xf;
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
            { "max",    &vm::max },
            { "mid",    &vm::mid },
            { "min",    &vm::min },
            { "music",  &vm::music },
            { "pget",   &vm::pget },
            { "pset",   &vm::pset },
            { "rnd",    &vm::rnd },
            { "sget",   &vm::sget },
            { "srand",  &vm::srand },
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
    msg::info("z8:stub:btn(%d)\n", (int)x);
    return s << ret;
}

int vm::btnp(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x;
    lol::LuaBool ret;
    s >> x;
    ret = false;
    msg::info("z8:stub:btnp(%d)\n", (int)x);
    return s << ret;
}

int vm::cls(lol::LuaState *l)
{
    lol::LuaStack s(l);
    vm *that = (vm *)vm::Find(l);
    memset(that->m_memory.data() + OFFSET_SCREEN, 0, SIZE_SCREEN);
    return 0;
}

int vm::cos(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::cos((float)x * -lol::F_TAU);
    return s << ret;
}

int vm::cursor(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y;
    s >> x >> y;
    msg::info("z8:stub:cursor(%f,%f)\n", (float)x, (float)y);
    return 0;
}

int vm::flr(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::floor((float)x);
    return s << ret;
}

int vm::max(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;
    ret = lol::max((float)x, (float)y);
    return s << ret;
}

int vm::mid(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, z, ret;
    s >> x >> y >> z;
    ret = (float)x > (float)y ? (float)y > (float)z ? (float)y
                                                    : lol::min((float)x, (float)z)
                              : (float)x > (float)z ? (float)x
                                                    : lol::min((float)y, (float)z);
    return s << ret;
}

int vm::min(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;
    ret = lol::min((float)x, (float)y);
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

    vm *that = (vm *)vm::Find(l);
    ret = that->getpixel((int)x, (int)y);

    return s << ret;
}

int vm::pset(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, c(true);
    s >> x >> y >> c;

    vm *that = (vm *)vm::Find(l);
    that->setpixel((int)x, (int)y, (int)c & 0xf);

    return 0;
}

int vm::rnd(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::rand((float)x);
    return s << ret;
}

int vm::sget(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;

    vm *that = (vm *)vm::Find(l);
    ret = that->getspixel((int)x, (int)y);

    return s << ret;
}

int vm::srand(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x;
    s >> x;
    /* FIXME: we do nothing; is this right? */
    return 0;
}

int vm::sset(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, c(true);
    s >> x >> y >> c;

    vm *that = (vm *)vm::Find(l);
    that->setpixel((int)x, (int)y, (int)c & 0xf);

    return 0;
}

int vm::sin(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, ret;
    s >> x;
    ret = lol::sin((float)x * -lol::F_TAU);
    return s << ret;
}

int vm::spr(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat n, x, y, w(true), h(true), flip_x(true), flip_y(true);
    s >> n >> x >> y >> w >> h >> flip_x >> flip_y;
    if ((float)w == 0)
        w = 1;
    if ((float)h == 0)
        h = 1;

    vm *that = (vm *)vm::Find(l);
    for (int j = 0; j < (int)((float)h * 8); ++j)
        for (int i = 0; i < (int)((float)w * 8); ++i)
        {
            int c = that->getspixel((int)n % 16 * 8 + i, (int)n / 16 * 8 + j);
            that->setpixel((int)x + i, (int)y + j, c);
        }

    //msg::info("z8:stub:spr(%d, %d, %d, %f, %f, %d, %d)\n", (int)n, (int)x, (int)y, (float)w, (float)h, (int)flip_x, (int)flip_y);

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

