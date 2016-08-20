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
    m_memory.resize(OFFSET_VERSION);
    m_screen.resize(128 * 128);
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

    // Update button state
    for (int i = 0; i < 64; ++i)
    {
        if (m_controller->IsKeyPressed(i))
            ++m_buttons[i];
        else
            m_buttons[i] = 0;
    }

    ExecLuaCode("if _update ~= nil then _update() end");
    ExecLuaCode("if _draw ~= nil then _draw() end");
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

    m_tile->GetTexture()->SetData(m_screen.data());

    int delta = (600 - 512) / 2;
    scene.AddTile(m_tile, 0, lol::vec3(delta, delta, 10.f), 0, lol::vec2(4.f), 0.f);
}

void vm::setpixel(int x, int y, int color)
{
    if (x < m_clip.aa.x || x >= m_clip.bb.x
         || y < m_clip.aa.y || y >= m_clip.bb.y)
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
            { "run",      &vm::run },
            { "cartdata", &vm::cartdata },
            { "reload",   &vm::reload },
            { "peek",     &vm::peek },
            { "poke",     &vm::poke },
            { "memcpy",   &vm::memcpy },
            { "memset",   &vm::memset },

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

    // Initialise VM memory and state
    for (int n = OFFSET_SCREEN; n < OFFSET_SCREEN + SIZE_SCREEN; ++n)
        that->m_memory[n] = lol::rand(0, 0);

    ::memset(that->m_buttons, 0, sizeof(that->m_buttons));

    // “The draw state is reset each time a program is run. This is equivalent to calling:
    // clip() camera() pal() color()”
    that->ExecLuaCode("clip() camera() pal() color(1)");

    // Fix code
    code_fixer fixer(that->m_cart.get_code());
    lol::String new_code = fixer.fix();

    //msg::info("Fixed cartridge code:\n%s\n", new_code.C());
    //printf("%s", new_code.C());
    // FIXME: not required yet because we inherit from LuaLoader
    //lol::LuaLoader lua;

    // Execute cartridge code
    that->ExecLuaCode(new_code.C());

    // Run cartridge initialisation routine
    that->ExecLuaCode("if _init ~= nil then _init() end");

    return 0;
}

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

int vm::memcpy(lol::LuaState *l)
{
    int dst = lua_tonumber(l, 1);
    int src = lua_tonumber(l, 2);
    int size = lua_tonumber(l, 3);

    if (dst >= 0 && src >= 0 && size > 0 && dst < 0x8000
         && src < 0x8000 && src + size < 0x8000 && dst + size < 0x8000)
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

    if (dst >= 0 && size > 0 && dst < 0x8000 && dst + size < 0x8000)
    {
        vm *that = (vm *)vm::Find(l);
        ::memset(that->m_memory.data() + dst, val, size);
    }

    return 0;
}

//
// I/O
//

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
// Text
//

int vm::cursor(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y;
    s >> x >> y;
    vm *that = (vm *)vm::Find(l);
    that->m_cursor.x = x;
    that->m_cursor.y = y;
    return 0;
}

int vm::print(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    if (lua_isnoneornil(l, 1))
        return 0;

    char const *str = lua_tostring(l, 1);
    bool use_cursor = lua_isnone(l, 2) || lua_isnone(l, 3);
    int x = use_cursor ? that->m_cursor.x : lua_tonumber(l, 2);
    int y = use_cursor ? that->m_cursor.y : lua_tonumber(l, 3);
    int col = lua_isnone(l, 4) ? that->m_color : lua_tonumber(l, 4);

    int c = that->m_pal[0][col & 0xf];
    int initial_x = x;

    auto pixels = that->m_font.Lock<lol::PixelFormat::RGBA_8>();
    for (int n = 0; str[n]; ++n)
    {
        int ch = str[n];

        if (ch == '\n')
        {
            x = initial_x;
            y += 6;
        }
        else
        {
            int index = ch > 0x20 && ch < 0x9a ? ch - 0x20 : 0;
            int w = index < 0x80 ? 4 : 8;
            int h = 6;

            for (int dy = 0; dy < h - 1; ++dy)
                for (int dx = 0; dx < w - 1; ++dx)
                {
                    if (pixels[(index / 16 * h + dy) * 128 + (index % 16 * w + dx)].r > 0)
                        that->setpixel(x + dx, y + dy, c);
                }

            x += w;
        }
    }

    that->m_font.Unlock(pixels);

    // Add implicit carriage return to the cursor position
    if (use_cursor)
        that->m_cursor = lol::ivec2(initial_x, y + 6);

    return 0;
}

//
// Graphics
//

int vm::camera(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);
    that->m_camera = lol::ivec2(lua_tonumber(l, 1), lua_tonumber(l, 2));
    return 0;
}

int vm::circ(lol::LuaState *l)
{
    // FIXME: use Bresenham or something nice like that
    vm *that = (vm *)vm::Find(l);

    int x = lua_tonumber(l, 1) - that->m_camera.x;
    int y = lua_tonumber(l, 2) - that->m_camera.y;
    int r = lua_tonumber(l, 3);
    int col = lua_isnone(l, 4) ? that->m_color : lua_tonumber(l, 4);

    int c = that->m_pal[0][col & 0xf];

    for (int dy = -r; dy <= r; ++dy)
        for (int dx = -r; dx <= r; ++dx)
        {
            if (dx * dx + dy * dy >= r * r - r
                 && dx * dx + dy * dy <= r * r + r)
                that->setpixel(x + dx, y + dy, c);
        }

    return 0;
}

int vm::circfill(lol::LuaState *l)
{
    // FIXME: use Bresenham or something nice like that
    vm *that = (vm *)vm::Find(l);

    int x = lua_tonumber(l, 1) - that->m_camera.x;
    int y = lua_tonumber(l, 2) - that->m_camera.y;
    int r = lua_tonumber(l, 3);
    int col = lua_isnone(l, 4) ? that->m_color : lua_tonumber(l, 4);

    int c = that->m_pal[0][col & 0xf];

    for (int dy = -r; dy <= r; ++dy)
        for (int dx = -r; dx <= r; ++dx)
        {
            if (dx * dx + dy * dy <= r * r + r)
                that->setpixel(x + dx, y + dy, c);
        }

    return 0;
}

int vm::clip(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    if (lua_isnone(l, 1))
    {
        that->m_clip = lol::ibox2(0, 0, 128, 128);
    }
    else
    {
        int x0 = lua_tonumber(l, 1);
        int y0 = lua_tonumber(l, 2);
        int x1 = x0 + lua_tonumber(l, 3);
        int y1 = y0 + lua_tonumber(l, 4);

        that->m_clip = lol::ibox2(x0, y0, x1, y1);
    }

    return 0;
}

int vm::cls(lol::LuaState *l)
{
    lol::LuaStack s(l);
    vm *that = (vm *)vm::Find(l);
    ::memset(that->m_memory.data() + OFFSET_SCREEN, 0, SIZE_SCREEN);
    return 0;
}

int vm::color(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);
    that->m_color = (int)lua_tonumber(l, 1) & 0xf;
    return 0;
}

int vm::fget(lol::LuaState *l)
{
    if (lua_isnone(l, 1))
        return 0;

    int n = lua_tonumber(l, 1);
    uint8_t bits = 0;

    if (n >= 0 && n < SIZE_GFX_PROPS)
    {
        vm *that = (vm *)vm::Find(l);
        bits = that->m_memory[OFFSET_GFX_PROPS + n];
    }

    if (lua_isnone(l, 2))
        lua_pushnumber(l, bits);
    else
        lua_pushboolean(l, bits & (1 << (int)lua_tonumber(l, 2)));

    return 1;
}

int vm::fset(lol::LuaState *l)
{
    if (lua_isnone(l, 1) || lua_isnone(l, 2))
        return 0;

    int n = lua_tonumber(l, 1);

    if (n >= 0 && n < SIZE_GFX_PROPS)
    {
        vm *that = (vm *)vm::Find(l);
        uint8_t bits = that->m_memory[OFFSET_GFX_PROPS + n];
        int f = lua_tonumber(l, 2);

        if (lua_isnone(l, 3))
            bits = f;
        else if (lua_toboolean(l, 3))
            bits |= 1 << (int)f;
        else
            bits &= ~(1 << (int)f);

        that->m_memory[OFFSET_GFX_PROPS + n] = bits;
    }

    return 0;
}

int vm::line(lol::LuaState *l)
{
    int x0 = lua_tonumber(l, 1);
    int y0 = lua_tonumber(l, 2);
    int x1 = lua_tonumber(l, 3);
    int y1 = lua_tonumber(l, 4);
    int col = lua_tonumber(l, 5);

    vm *that = (vm *)vm::Find(l);
    // FIXME this is shitty temp code
    for (float t = 0.f; t <= 1.0f; t += 1.f / 256)
    {
        that->setpixel((int)lol::mix((float)x0, (float)x1, t) - that->m_camera.x,
                       (int)lol::mix((float)y0, (float)y1, t) - that->m_camera.y, col);
    }

    return 0;
}

int vm::map(lol::LuaState *l)
{
    int cel_x = lua_tonumber(l, 1);
    int cel_y = lua_tonumber(l, 2);
    int sx = lua_tonumber(l, 3);
    int sy = lua_tonumber(l, 4);
    int cel_w = lua_tonumber(l, 5);
    int cel_h = lua_tonumber(l, 6);
    int layer = lua_isnone(l, 7) ? 0xff : lua_tonumber(l, 7);

    vm *that = (vm *)vm::Find(l);

    for (int dy = 0; dy < cel_h * 8; ++dy)
    for (int dx = 0; dx < cel_w * 8; ++dx)
    {
        int cx = cel_x + dx / 8;
        int cy = cel_y + dy / 8;
        if (cx < 0 || cx >= 128 || cy < 0 || cy >= 64)
            continue;

        int line = cy < 32 ? OFFSET_MAP + 128 * cy
                           : OFFSET_MAP2 + 128 * (cy - 32);
        int sprite = that->m_memory[line + cx];

        int c = that->getspixel(sprite % 16 * 8 + dx % 8, sprite / 16 * 8 + dy % 8);
        that->setpixel(sx - that->m_camera.x + dx, sy - that->m_camera.y + dy, c);
    }

    return 0;
}

int vm::mget(lol::LuaState *l)
{
    int x = lua_tonumber(l, 1);
    int y = lua_tonumber(l, 2);
    int n = 0;

    if (x >= 0 && x < 128 && y >= 0 && y < 64)
    {
        vm *that = (vm *)vm::Find(l);
        int line = y < 32 ? OFFSET_MAP + 128 * y
                          : OFFSET_MAP2 + 128 * (y - 32);
        n = that->m_memory[line + x];
    }

    lua_pushnumber(l, n);
    return 1;
}

int vm::mset(lol::LuaState *l)
{
    int x = lua_tonumber(l, 1);
    int y = lua_tonumber(l, 2);
    int n = lua_tonumber(l, 3);

    if (x >= 0 && x < 128 && y >= 0 && y < 64)
    {
        vm *that = (vm *)vm::Find(l);
        int line = y < 32 ? OFFSET_MAP + 128 * y
                          : OFFSET_MAP2 + 128 * (y - 32);
        that->m_memory[line + x] = n;
    }

    return 0;
}

int vm::pal(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    if (lua_isnone(l, 1))
    {
        for (int i = 0; i < 16; ++i)
        {
            that->m_pal[0][i] = that->m_pal[1][i] = i;
            that->m_palt[i] = i ? 0 : 1;
        }
    }
    else
    {
        int c0 = lua_tonumber(l, 1);
        int c1 = lua_tonumber(l, 2);
        int p = lua_tonumber(l, 3);

        that->m_pal[p & 1][c0 & 0xf] = c1 & 0xf;
    }

    return 0;
}

int vm::palt(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    if (lua_isnone(l, 1))
    {
        for (int i = 0; i < 16; ++i)
            that->m_palt[i] = i ? 0 : 1;
    }
    else
    {
        int c = lua_tonumber(l, 1);
        int t = lua_toboolean(l, 2);
        that->m_palt[c & 0xf] = t;
    }

    return 0;
}

int vm::pget(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, ret;
    s >> x >> y;

    vm *that = (vm *)vm::Find(l);
    ret = that->getpixel((int)x - that->m_camera.x, (int)y - that->m_camera.y);

    return s << ret;
}

int vm::pset(lol::LuaState *l)
{
    lol::LuaStack s(l);
    lol::LuaFloat x, y, c(true);
    s >> x >> y >> c;

    vm *that = (vm *)vm::Find(l);
    that->setpixel((int)x - that->m_camera.x, (int)y - that->m_camera.y, (int)c & 0xf);

    return 0;
}

int vm::rect(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    int x0 = lua_tonumber(l, 1) - that->m_camera.x;
    int y0 = lua_tonumber(l, 2) - that->m_camera.y;
    int x1 = lua_tonumber(l, 3) - that->m_camera.x;
    int y1 = lua_tonumber(l, 4) - that->m_camera.y;
    int col = lua_isnone(l, 5) ? that->m_color : lua_tonumber(l, 5);
    int c = that->m_pal[0][col & 0xf];

    for (int y = lol::min(y0, y1); y <= lol::max(y0, y1); ++y)
    {
        that->setpixel(x0, y, c);;
        that->setpixel(x1, y, c);;
    }

    for (int x = lol::min(x0, x1); x <= lol::max(x0, x1); ++x)
    {
        that->setpixel(x, y0, c);;
        that->setpixel(x, y1, c);;
    }

    return 0;
}

int vm::rectfill(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    int x0 = lua_tonumber(l, 1) - that->m_camera.x;
    int y0 = lua_tonumber(l, 2) - that->m_camera.y;
    int x1 = lua_tonumber(l, 3) - that->m_camera.x;
    int y1 = lua_tonumber(l, 4) - that->m_camera.y;
    int col = lua_isnone(l, 5) ? that->m_color : lua_tonumber(l, 5);
    int c = that->m_pal[0][col & 0xf];

    for (int y = lol::min(y0, y1); y <= lol::max(y0, y1); ++y)
        for (int x = lol::min(x0, x1); x <= lol::max(x0, x1); ++x)
            that->setpixel(x, y, c);

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
    that->setspixel((int)x, (int)y, (int)c & 0xf);

    return 0;
}

int vm::spr(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    int n = lua_tonumber(l, 1);
    int x = lua_tonumber(l, 2) - that->m_camera.x;
    int y = lua_tonumber(l, 3) - that->m_camera.y;
    float w = lua_isnoneornil(l, 4) ? 1 : lua_tonumber(l, 4);
    float h = lua_isnoneornil(l, 5) ? 1 : lua_tonumber(l, 5);
    int flip_x = lua_toboolean(l, 6);
    int flip_y = lua_toboolean(l, 7);

    // FIXME: Handle transparency better than that
    // FIXME: implement flip_x and flip_w
    for (int j = 0; j < h * 8; ++j)
        for (int i = 0; i < w * 8; ++i)
        {
            int di = flip_x ? w * 8 - 1 - i : i;
            int dj = flip_y ? h * 8 - 1 - j : j;
            int c = that->getspixel(n % 16 * 8 + di, n / 16 * 8 + dj);
            if (!that->m_palt[c])
                that->setpixel(x - that->m_camera.x + i, y - that->m_camera.y + j, c);
        }

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
    msg::info("z8:stub:music\n");
    return 0;
}

int vm::sfx(lol::LuaState *l)
{
    msg::info("z8:stub:sfx\n");
    return 0;
}

} // namespace z8

