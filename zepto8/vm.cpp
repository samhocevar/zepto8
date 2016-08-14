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

    // Initialise VM memory and state
    m_memory.resize(OFFSET_VERSION);
    m_screen.resize(128 * 128);

    for (int n = OFFSET_SCREEN; n < OFFSET_VERSION; ++n)
        m_memory[n] = lol::rand(0, 2);

    m_color = 15;
    m_camera = lol::ivec2(0, 0);
    m_cursor = lol::ivec2(0, 0);

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
            { "cartdata", &vm::cartdata },
            { "reload",   &vm::reload },
            { "peek",     &vm::peek },
            { "poke",     &vm::poke },

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
            { "line",     &vm::line },
            { "map",      &vm::map },
            { "mget",     &vm::mget },
            { "mset",     &vm::mset },
            { "pal",      &vm::pal },
            { "pget",     &vm::pget },
            { "pset",     &vm::pset },
            { "rect",     &vm::rect },
            { "rectfill", &vm::rectfill },
            { "sget",     &vm::sget },
            { "sset",     &vm::sset },
            { "spr",      &vm::spr },
            { "sspr",     &vm::sspr },

            { "music", &vm::music },

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

int vm::cartdata(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x;
    s >> x;
    msg::info("z8:stub:cartdata %d\n", (int)x);
    return 0;
}

int vm::reload(lol::LuaState *l)
{
    lol::LuaStack s(l);
    msg::info("z8:stub:reload\n");
    return 0;
}

int vm::peek(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat in_addr, ret;
    s >> in_addr;
    int addr = (int)(float)in_addr;
    if (addr < 0 || addr > 0x7fff)
        return 0;

    vm *that = (vm *)vm::Find(l);
    ret = that->m_memory[addr];
    return s << ret;
}

int vm::poke(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat in_addr, in_val;
    s >> in_addr >> in_val;
    int addr = (int)(float)in_addr;
    if (addr < 0 || addr > 0x7fff)
        return 0;

    vm *that = (vm *)vm::Find(l);
    that->m_memory[addr] = (uint16_t)(int)(float)in_val;
    return 0;
}

//
// I/O
//

int vm::btn(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x;
    lol::LuaBool ret;
    s >> x;
    ret = false;
    msg::info("z8:stub:btn %d\n", (int)x);
    return s << ret;
}

int vm::btnp(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x;
    lol::LuaBool ret;
    s >> x;
    ret = false;
    msg::info("z8:stub:btnp %d\n", (int)x);
    return s << ret;
}

//
// Text
//

int vm::cursor(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y;
    s >> x >> y;
    msg::info("z8:stub:cursor %f %f\n", (float)x, (float)y);
    return 0;
}

int vm::print(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaString str;
    lol::LuaFloat x(true), y(true), col(true);
    s >> str >> x >> y >> col;
    msg::info("z8:stub:print \"%s\" [%d %d [%d]]\n", str.GetValue().C(), (int)x, (int)y, (int)col);
    return 0;
}

//
// Graphics
//

int vm::camera(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y;
    s >> x >> y;
    msg::info("z8:stub:camera %d %d\n", (int)x, (int)y);
    return 0;
}

int vm::circ(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, r, col(true);
    s >> x >> y >> r >> col;
    msg::info("z8:stub:circ %d %d %d [%d]\n", (int)x, (int)y, (int)r, (int)col);
    return 0;
}

int vm::circfill(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat in_x, in_y, in_r, col(true);
    s >> in_x >> in_y >> in_r >> col;

    vm *that = (vm *)vm::Find(l);

    int x = (int)in_x, y = (int)in_y, r = (int)in_r;
    for (int dy = -r; dy <= r; ++dy)
        for (int dx = -r; dx <= r; ++dx)
        {
            if (dx * dx + dy * dy <= r * r)
                that->setpixel(x + dx, y + dy, (int)col & 0xf);
        }

    //msg::info("z8:stub:circfill %d %d %d [%d]\n", (int)x, (int)y, (int)r, (int)col);
    return 0;
}

int vm::clip(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x(true), y(true), w(true), h(true);
    s >> x >> y >> w >> h;
    msg::info("z8:stub:clip [%d %d %d %d]\n", (int)x, (int)y, (int)w, (int)h);
    return 0;
}

int vm::cls(lol::LuaState *l)
{
    lol::LuaStack s(l);
    vm *that = (vm *)vm::Find(l);
    memset(that->m_memory.data() + OFFSET_SCREEN, 0, SIZE_SCREEN);
    return 0;
}

int vm::color(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat col;
    s >> col;
    vm *that = (vm *)vm::Find(l);
    that->m_color = (int)col & 0xf;
    return 0;
}

int vm::line(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x0, y0, x1, y1, col(true);
    s >> x0 >> y0 >> x1 >> y1 >> col;
    msg::info("z8:stub:line %d %d %d %d [%d]\n", (int)x0, (int)y0, (int)x1, (int)y1, (int)col);

    vm *that = (vm *)vm::Find(l);
    // FIXME this is shitty temp code
    for (float t = 0.f; t <= 1.0f; t += 1.f / 256)
    {
        that->setpixel((int)lol::mix((float)x0, (float)x1, t),
                       (int)lol::mix((float)y0, (float)y1, t), col);
    }

    return 0;
}

int vm::map(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat cel_x, cel_y, sx, sy, cel_w, cel_h, layer(true);
    s >> cel_x >> cel_y >> sx >> sy >> cel_w >> cel_h >> layer;
    msg::info("z8:stub:map %d %d %d %d %d %d [%d]\n", (int)cel_x, (int)cel_y, (int)sx, (int)sy, (int)cel_w, (int)cel_h, (int)layer);
    return 0;
}

int vm::mget(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;
    msg::info("z8:stub:mget %d %d\n", (int)x, (int)y);
    ret = 0;
    return s << ret;
}

int vm::mset(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, v;
    s >> x >> y >> v;
    msg::info("z8:stub:mset %d %d %d\n", (int)x, (int)y, (int)v);
    return 0;
}

int vm::pal(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat c0(true), c1(true), p(true);
    s >> c0 >> c1 >> p;

    msg::info("z8:stub:pal [%d %d [%d]]\n", (int)c0, (int)c1, (int)p);
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

int vm::rect(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat in_x0, in_y0, in_x1, in_y1, col(true);
    s >> in_x0 >> in_y0 >> in_x1 >> in_y1 >> col;

    int x0 = lol::max(0, lol::min(int(in_x0), int(in_x1)));
    int y0 = lol::max(0, lol::min(int(in_y0), int(in_y1)));
    int x1 = lol::min(127, lol::max(int(in_x0), int(in_x1)));
    int y1 = lol::min(127, lol::max(int(in_y0), int(in_y1)));

    vm *that = (vm *)vm::Find(l);
    for (int y = y0; y <= y1; ++y)
    {
        that->setpixel(x0, y, (int)col & 0xf);
        that->setpixel(x1, y, (int)col & 0xf);
    }

    for (int x = x0; x <= x1; ++x)
    {
        that->setpixel(x, y0, (int)col & 0xf);
        that->setpixel(x, y1, (int)col & 0xf);
    }

    return 0;
}

int vm::rectfill(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat in_x0, in_y0, in_x1, in_y1, col(true);
    s >> in_x0 >> in_y0 >> in_x1 >> in_y1 >> col;

    int x0 = lol::max(0, lol::min(int(in_x0), int(in_x1)));
    int y0 = lol::max(0, lol::min(int(in_y0), int(in_y1)));
    int x1 = lol::min(127, lol::max(int(in_x0), int(in_x1)));
    int y1 = lol::min(127, lol::max(int(in_y0), int(in_y1)));

    vm *that = (vm *)vm::Find(l);
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
            that->setpixel(x, y, (int)col & 0xf);

    return 0;
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

int vm::sset(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, c(true);
    s >> x >> y >> c;

    vm *that = (vm *)vm::Find(l);
    that->setpixel((int)x, (int)y, (int)c & 0xf);

    return 0;
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

    // FIXME: Handle transparency better than that
    vm *that = (vm *)vm::Find(l);
    for (int j = 0; j < (int)((float)h * 8); ++j)
        for (int i = 0; i < (int)((float)w * 8); ++i)
        {
            int c = that->getspixel((int)n % 16 * 8 + i, (int)n / 16 * 8 + j);
            if (c)
                that->setpixel((int)x + i, (int)y + j, c);
        }

    //msg::info("z8:stub:spr(%d, %d, %d, %f, %f, %d, %d)\n", (int)n, (int)x, (int)y, (float)w, (float)h, (int)flip_x, (int)flip_y);

    return 0;
}

int vm::sspr(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat sx, sy, sw, sh, dx, dy, dw(true), dh(true), flip_x(true), flip_y(true);
    s >> sx >> sy >> sw >> sh >> dx >> dy >> dw >> dh >> flip_x >> flip_y;
    msg::info("z8:stub:sspr %d %d %d %d %d %d [%d %d] [%d] [%d]\n", (int)sx, (int)sy, (int)sw, (int)sh, (int)dx, (int)dy, (int)dw, (int)dh, (int)flip_x, (int)flip_y);
    return 0;
}

//
// Sound
//

int vm::music(lol::LuaState *l)
{
    lol::LuaStack s(l);
    msg::info("z8:stub:music\n");
    return 0;
}

} // namespace z8

