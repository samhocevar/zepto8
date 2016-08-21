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

void vm::setpixel(int x, int y, int color)
{
    x -= m_camera.x;
    y -= m_camera.y;

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
    if (x < m_clip.aa.x || x >= m_clip.bb.x
         || y < m_clip.aa.y || y >= m_clip.bb.y)
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

//
// Text
//

int vm::cursor(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);
    that->m_cursor = lol::ivec2(lua_tonumber(l, 1), lua_tonumber(l, 2));
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
        int ch = (int)(uint8_t)str[n];

        if (ch == '\n')
        {
            x = initial_x;
            y += 6;
        }
        else
        {
            int index = ch > 0x20 && ch < 0x9a ? ch - 0x20 : 0;
            int w = index < 0x60 ? 4 : 8;
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
    vm *that = (vm *)vm::Find(l);

    int x = lua_tonumber(l, 1);
    int y = lua_tonumber(l, 2);
    int r = lua_tonumber(l, 3);
    int col = lua_isnone(l, 4) ? that->m_color : lua_tonumber(l, 4);

    int c = that->m_pal[0][col & 0xf];

    for (int dx = r, dy = 0, err = 0; dx >= dy; )
    {
        that->setpixel(x + dx, y + dy, c);
        that->setpixel(x + dy, y + dx, c);
        that->setpixel(x - dy, y + dx, c);
        that->setpixel(x - dx, y + dy, c);
        that->setpixel(x - dx, y - dy, c);
        that->setpixel(x - dy, y - dx, c);
        that->setpixel(x + dy, y - dx, c);
        that->setpixel(x + dx, y - dy, c);

        dy += 1;
        err += 1 + 2 * dy;
        // XXX: original Bresenham has a different test, but
        // this one seems to match PICO-8 better.
        if (2 * (err - dx) > r + 1)
        {
            dx -= 1;
            err += 1 - 2 * dx;
        }
    }

    return 0;
}

int vm::circfill(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    int x = lua_tonumber(l, 1);
    int y = lua_tonumber(l, 2);
    int r = lua_tonumber(l, 3);
    int col = lua_isnone(l, 4) ? that->m_color : lua_tonumber(l, 4);

    int c = that->m_pal[0][col & 0xf];

    for (int dx = r, dy = 0, err = 0; dx >= dy; )
    {
        // FIXME: there is some overdraw here, but nothing
        // too grave; maybe fix this one day
        for (int e = -dx; e <= dx; ++e)
        {
            that->setpixel(x + dy, y + e, c);
            that->setpixel(x - dy, y + e, c);
            that->setpixel(x + e, y + dy, c);
            that->setpixel(x + e, y - dy, c);
        }

        dy += 1;
        err += 1 + 2 * dy;
        // XXX: original Bresenham has a different test, but
        // this one seems to match PICO-8 better.
        if (2 * (err - dx) > r + 1)
        {
            dx -= 1;
            err += 1 - 2 * dx;
        }
    }

    return 0;
}

int vm::clip(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    /* XXX: there were rendering issues with Hyperspace by J.Fry when we were
     * only checking for lua_isnone(l,1) (instead of 4) because first argument
     * was actually "" instead of nil. */
    if (lua_isnone(l, 4))
    {
        that->m_clip = lol::ibox2(0, 0, 128, 128);
    }
    else
    {
        int x0 = lua_tonumber(l, 1);
        int y0 = lua_tonumber(l, 2);
        int x1 = x0 + lua_tonumber(l, 3);
        int y1 = y0 + lua_tonumber(l, 4);

        that->m_clip = lol::ibox2(lol::max(x0, 0), lol::max(y0, 0),
                                  lol::min(x1, 128), lol::min(y1, 128));
    }

    return 0;
}

int vm::cls(lol::LuaState *l)
{
    int c = lua_tonumber(l, 1);
    vm *that = (vm *)vm::Find(l);
    ::memset(that->m_memory.data() + OFFSET_SCREEN, (c & 0xf) * 0x11, SIZE_SCREEN);
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
    vm *that = (vm *)vm::Find(l);

    int x0 = lua_tonumber(l, 1);
    int y0 = lua_tonumber(l, 2);
    int x1 = lua_tonumber(l, 3);
    int y1 = lua_tonumber(l, 4);
    int col = lua_isnone(l, 5) ? that->m_color : lua_tonumber(l, 5);
    int c = that->m_pal[0][col & 0xf];

    if (x0 == x1 && y0 == y1)
    {
        that->setpixel(x0, y0, c);
    }
    else if (lol::abs(x1 - x0) > lol::abs(y1 - y0))
    {
        for (int x = lol::min(x0, x1); x <= lol::max(x0, x1); ++x)
        {
            int y = lol::round(lol::mix((float)y0, (float)y1, (float)(x - x0) / (x1 - x0)));
            that->setpixel(x, y, c);
        }
    }
    else
    {
        for (int y = lol::min(y0, y1); y <= lol::max(y0, y1); ++y)
        {
            int x = lol::round(lol::mix((float)x0, (float)x1, (float)(y - y0) / (y1 - y0)));
            that->setpixel(x, y, c);
        }
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
    int layer = lua_tonumber(l, 7);

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

        uint8_t bits = that->m_memory[OFFSET_GFX_PROPS + sprite];
        if (layer && !(bits & layer))
            continue;

        if (sprite)
        {
            int col = that->getspixel(sprite % 16 * 8 + dx % 8, sprite / 16 * 8 + dy % 8);
            if (!that->m_palt[col])
            {
                int c = that->m_pal[0][col & 0xf];
                that->setpixel(sx + dx, sy + dy, c);
            }
        }
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
    vm *that = (vm *)vm::Find(l);

    int x = lua_tonumber(l, 1);
    int y = lua_tonumber(l, 2);
    int col = lua_isnone(l, 3) ? that->m_color : lua_tonumber(l, 3);
    int c = that->m_pal[0][col & 0xf];

    that->setpixel(x, y, c);

    return 0;
}

int vm::rect(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    int x0 = lua_tonumber(l, 1);
    int y0 = lua_tonumber(l, 2);
    int x1 = lua_tonumber(l, 3);
    int y1 = lua_tonumber(l, 4);
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

    int x0 = lua_tonumber(l, 1);
    int y0 = lua_tonumber(l, 2);
    int x1 = lua_tonumber(l, 3);
    int y1 = lua_tonumber(l, 4);
    int col = lua_isnone(l, 5) ? that->m_color : lua_tonumber(l, 5);
    int c = that->m_pal[0][col & 0xf];

    for (int y = lol::min(y0, y1); y <= lol::max(y0, y1); ++y)
        for (int x = lol::min(x0, x1); x <= lol::max(x0, x1); ++x)
            that->setpixel(x, y, c);

    return 0;
}

int vm::sget(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    int x = lua_tonumber(l, 1);
    int y = lua_tonumber(l, 2);

    lua_pushnumber(l, that->getspixel(x, y));

    return 1;
}

int vm::sset(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    int x = lua_tonumber(l, 1);
    int y = lua_tonumber(l, 2);
    int col = lua_isnone(l, 5) ? that->m_color : lua_tonumber(l, 5);
    int c = that->m_pal[0][col & 0xf];

    that->setspixel(x, y, c);

    return 0;
}

int vm::spr(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    // FIXME: should we abort if n == 0?
    int n = lua_tonumber(l, 1);
    int x = lua_tonumber(l, 2);
    int y = lua_tonumber(l, 3);
    float w = lua_isnoneornil(l, 4) ? 1 : lua_tonumber(l, 4);
    float h = lua_isnoneornil(l, 5) ? 1 : lua_tonumber(l, 5);
    int flip_x = lua_toboolean(l, 6);
    int flip_y = lua_toboolean(l, 7);

    for (int j = 0; j < h * 8; ++j)
        for (int i = 0; i < w * 8; ++i)
        {
            int di = flip_x ? w * 8 - 1 - i : i;
            int dj = flip_y ? h * 8 - 1 - j : j;
            int col = that->getspixel(n % 16 * 8 + di, n / 16 * 8 + dj);
            if (!that->m_palt[col])
            {
                int c = that->m_pal[0][col];
                that->setpixel(x + i, y + j, c);
            }
        }

    return 0;
}

int vm::sspr(lol::LuaState *l)
{
    vm *that = (vm *)vm::Find(l);

    int sx = lua_tonumber(l, 1);
    int sy = lua_tonumber(l, 2);
    int sw = lua_tonumber(l, 3);
    int sh = lua_tonumber(l, 4);
    int dx = lua_tonumber(l, 5);
    int dy = lua_tonumber(l, 6);
    int dw = lua_isnone(l, 7) ? sw : lua_tonumber(l, 7);
    int dh = lua_isnone(l, 8) ? sh : lua_tonumber(l, 8);
    int flip_x = lua_toboolean(l, 9);
    int flip_y = lua_toboolean(l, 10);

    // Iterate over destination pixels
    for (int j = 0; j < dh; ++j)
    for (int i = 0; i < dw; ++i)
    {
        int di = flip_x ? dw - 1 - i : i;
        int dj = flip_y ? dh - 1 - j : j;

        // Find source
        int x = sx + sw * di / dw;
        int y = sy + sh * dj / dh;

        int col = that->getspixel(x, y);
        if (!that->m_palt[col])
        {
            int c = that->m_pal[0][col];
            that->setpixel(dx + i, dy + j, c);
        }
    }

    return 0;
}

} // namespace z8

