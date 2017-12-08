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

uint8_t vm::getpixel(fix32 x, fix32 y)
{
    if (x < m_clip.aa.x || x >= m_clip.bb.x
         || y < m_clip.aa.y || y >= m_clip.bb.y)
        return 0;

    int offset = OFFSET_SCREEN + (128 * (int)y + (int)x) / 2;
    return ((int)x & 1) ? m_memory[offset] >> 4 : m_memory[offset] & 0xf;
}

void vm::setpixel(fix32 x, fix32 y, fix32 c)
{
    if (x < m_clip.aa.x || x >= m_clip.bb.x
         || y < m_clip.aa.y || y >= m_clip.bb.y)
        return;

    int color = m_pal[0][(int)c & 0xf];

    if ((m_fillp.bits() >> (((int)x & 3) + 4 * ((int)y & 3))) & 0x10000)
    {
        if (m_fillp.bits() & 0x8000) // Special transparency bit
            return;
        color = m_pal[0][((int)c >> 4) & 0xf];
    }

    int offset = OFFSET_SCREEN + (128 * (int)y + (int)x) / 2;
    int mask = ((int)x & 1) ? 0x0f : 0xf0;
    int p = ((int)x & 1) ? color << 4 : color;
    m_memory[offset] = (m_memory[offset] & mask) | p;
}

void vm::setspixel(int x, int y, int color)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 128)
        return;

    int offset = OFFSET_GFX + (128 * y + x) / 2;
    int mask = (x & 1) ? 0x0f : 0xf0;
    int p = (x & 1) ? color << 4 : color;
    m_memory[offset] = (m_memory[offset] & mask) | p;
}

int vm::getspixel(int x, int y)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 128)
        return 0;

    int offset = OFFSET_GFX + (128 * y + x) / 2;
    return (x & 1) ? m_memory[offset] >> 4 : m_memory[offset] & 0xf;
}

void vm::hline(fix32 x1, fix32 x2, fix32 y, fix32 c)
{
    if (x1 > x2)
        std::swap(x1, x2);

    // FIXME: very inefficient when fillp is active
    if (m_fillp.bits())
    {
        for (fix32 x = x1; x <= x2; x += fix32(1.0))
            setpixel(x, y, c);
        return;
    }

    if (y < m_clip.aa.y || y >= m_clip.bb.y)
        return;

    x1 = fix32::max(x1, m_clip.aa.x);
    x2 = fix32::min(x2, m_clip.bb.x - fix32(1.0));

    if (x1 > x2)
        return;

    int color = m_pal[0][(int)c & 0xf];
    int offset = OFFSET_SCREEN + (128 * (int)y) / 2;

    if ((int)x1 & 1)
    {
        m_memory[offset + (int)x1 / 2]
            = (m_memory[offset + (int)x1 / 2] & 0x0f) | (color << 4);
        x1 += fix32(1.0);
    }

    if (((int)x2 & 1) == 0)
    {
        m_memory[offset + (int)x2 / 2]
            = (m_memory[offset + (int)x2 / 2] & 0xf0) | color;
        x2 -= fix32(1.0);
    }

    ::memset(get_mem(offset + (int)x1 / 2), color * 0x11, ((int)x2 - (int)x1 + 1) / 2);
}

void vm::vline(fix32 x, fix32 y1, fix32 y2, fix32 c)
{
    if (y1 > y2)
        std::swap(y1, y2);

    // FIXME: very inefficient when fillp is active
    if (m_fillp.bits())
    {
        for (fix32 y = y1; y <= y2; y += fix32(1.0))
            setpixel(x, y, c);
        return;
    }

    if (x < m_clip.aa.x || x >= m_clip.bb.x)
        return;

    y1 = fix32::max(y1, m_clip.aa.y);
    y2 = fix32::min(y2, m_clip.bb.y - fix32(1.0));

    if (y1 > y2)
        return;

    int mask = ((int)x & 1) ? 0x0f : 0xf0;
    int color = m_pal[0][(int)c & 0xf];
    int p = ((int)x & 1) ? color << 4 : color;

    for (int y = (int)y1; y <= (int)y2; ++y)
    {
        int offset = OFFSET_SCREEN + (128 * y + (int)x) / 2;
        m_memory[offset] = (m_memory[offset] & mask) | p;
    }
}

//
// Text
//

int vm::api::cursor(lua_State *l)
{
    vm *that = get_this(l);
    that->m_cursor.x = lua_tofix32(l, 1);
    that->m_cursor.y = lua_tofix32(l, 2);
    return 0;
}

static void lua_pushtostr(lua_State *l, bool do_hex)
{
    char buffer[20];
    char const *str = buffer;

    if (lua_isnone(l, 1))
        str = "[no value]";
    else if (lua_isnil(l, 1))
        str = "[nil]";
    else if (lua_type(l, 1) == LUA_TSTRING)
        str = lua_tostring(l, 1);
    else if (lua_isnumber(l, 1))
    {
        fix32 x = lua_tofix32(l, 1);
        if (do_hex)
        {
            uint32_t b = (uint32_t)x.bits();
            sprintf(buffer, "0x%04x.%04x", (b >> 16) & 0xffff, b & 0xffff);
        }
        else
        {
            int n = sprintf(buffer, "%.4f", (double)x);
            // Remove trailing zeroes and comma
            while (n > 2 && buffer[n - 1] == '0' && ::isdigit(buffer[n - 2]))
                buffer[--n] = '\0';
            if (n > 2 && buffer[n - 1] == '0' && buffer[n - 2] == '.')
                buffer[n -= 2] = '\0';
        }
    }
    else if (lua_istable(l, 1))
        str = "[table]";
    else if (lua_isthread(l, 1))
        str = "[thread]";
    else if (lua_isfunction(l, 1))
        str = "[function]";
    else
        str = lua_toboolean(l, 1) ? "true" : "false";

    lua_pushstring(l, str);
}

int vm::api::print(lua_State *l)
{
    vm *that = get_this(l);

    if (lua_isnone(l, 1))
        return 0;

    // Leverage lua_pushtostr() to make sure we have a string
    lua_pushtostr(l, false);
    char const *str = lua_tostring(l, -1);
    lua_pop(l, 1);

    bool use_cursor = lua_isnone(l, 2) || lua_isnone(l, 3);
    fix32 x = use_cursor ? that->m_cursor.x : lua_tofix32(l, 2);
    fix32 y = use_cursor ? that->m_cursor.y : lua_tofix32(l, 3);
    if (!lua_isnone(l, 4))
        that->m_colors = lua_tofix32(l, 4);
    fix32 initial_x = x;

    auto pixels = that->m_font.lock<lol::PixelFormat::RGBA_8>();
    for (int n = 0; str[n]; ++n)
    {
        int ch = (int)(uint8_t)str[n];

        if (ch == '\n')
        {
            x = initial_x;
            y += fix32(6.0);
        }
        else
        {
            // PICO-8 characters end at 0x99, but we use characters
            // 0x9a…0x9f for the ZEPTO-8 logo. Lol.
            int index = ch > 0x20 && ch < 0xa0 ? ch - 0x20 : 0;
            int16_t w = index < 0x60 ? 4 : 8;
            int h = 6;

            for (int16_t dy = 0; dy < h; ++dy)
                for (int16_t dx = 0; dx < w; ++dx)
                {
                    fix32 screen_x = x - that->m_camera.x + fix32(dx);
                    fix32 screen_y = y - that->m_camera.y + fix32(dy);

                    if (pixels[(index / 16 * h + dy) * 128 + (index % 16 * w + dx)].r > 0)
                        that->setpixel(screen_x, screen_y, that->m_colors);
                }

            x += fix32(w);
        }
    }

    that->m_font.unlock(pixels);

    // In PICO-8 scrolling only happens _after_ the whole string was printed,
    // even if it contained carriage returns or if the cursor was already
    // below the threshold value.
    if (use_cursor)
    {
        int16_t const lines = 6;

        // FIXME: is this affected by the camera?
        if (y > fix32(116.0))
        {
            uint8_t *screen = that->get_mem(OFFSET_SCREEN);
            memmove(screen, screen + lines * 64, SIZE_SCREEN - lines * 64);
            ::memset(screen + SIZE_SCREEN - lines * 64, 0, lines * 64);
            y -= fix32(lines);
        }

        that->m_cursor.x = initial_x;
        that->m_cursor.y = y + fix32(lines);
    }

    return 0;
}

int vm::api::tostr(lua_State *l)
{
    bool do_hex = lua_toboolean(l, 2);
    lua_pushtostr(l, do_hex);
    return 1;
}

int vm::api::tonum(lua_State *l)
{
    UNUSED(l);
    msg::info("z8:stub:tonum\n");
    lua_pushfix32(l, fix32(0.0));
    return 1;
}

//
// Graphics
//

int vm::api::camera(lua_State *l)
{
    vm *that = get_this(l);
    that->m_camera.x = lua_tofix32(l, 1);
    that->m_camera.y = lua_tofix32(l, 2);
    return 0;
}

int vm::api::circ(lua_State *l)
{
    vm *that = get_this(l);

    fix32 x = lua_tofix32(l, 1) - that->m_camera.x;
    fix32 y = lua_tofix32(l, 2) - that->m_camera.y;
    int16_t r = (int16_t)lua_tofix32(l, 3);
    if (!lua_isnone(l, 4))
        that->m_colors = lua_tofix32(l, 4);

    for (int16_t dx = r, dy = 0, err = 0; dx >= dy; )
    {
        fix32 fdx(dx), fdy(dy);

        that->setpixel(x + fdx, y + fdy, that->m_colors);
        that->setpixel(x + fdy, y + fdx, that->m_colors);
        that->setpixel(x - fdy, y + fdx, that->m_colors);
        that->setpixel(x - fdx, y + fdy, that->m_colors);
        that->setpixel(x - fdx, y - fdy, that->m_colors);
        that->setpixel(x - fdy, y - fdx, that->m_colors);
        that->setpixel(x + fdy, y - fdx, that->m_colors);
        that->setpixel(x + fdx, y - fdy, that->m_colors);

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

int vm::api::circfill(lua_State *l)
{
    vm *that = get_this(l);

    fix32 x = lua_tofix32(l, 1) - that->m_camera.x;
    fix32 y = lua_tofix32(l, 2) - that->m_camera.y;
    int16_t r = (int16_t)lua_tofix32(l, 3);
    if (!lua_isnone(l, 4))
        that->m_colors = lua_tofix32(l, 4);

    for (int16_t dx = r, dy = 0, err = 0; dx >= dy; )
    {
        fix32 fdx(dx), fdy(dy);

        /* Some minor overdraw here, but nothing serious */
        that->hline(x - fdx, x + fdx, y - fdy, that->m_colors);
        that->hline(x - fdx, x + fdx, y + fdy, that->m_colors);
        that->vline(x - fdy, y - fdx, y + fdx, that->m_colors);
        that->vline(x + fdy, y - fdx, y + fdx, that->m_colors);

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

int vm::api::clip(lua_State *l)
{
    vm *that = get_this(l);

    /* XXX: there were rendering issues with Hyperspace by J.Fry when we were
     * only checking for lua_isnone(l,1) (instead of 4) because first argument
     * was actually "" instead of nil. */
    if (lua_isnone(l, 4))
    {
        that->m_clip.aa.x = that->m_clip.aa.y = 0.0;
        that->m_clip.bb.x = that->m_clip.bb.y = 128.0;
    }
    else
    {
        fix32 x0 = lua_tofix32(l, 1);
        fix32 y0 = lua_tofix32(l, 2);
        fix32 x1 = x0 + lua_tofix32(l, 3);
        fix32 y1 = y0 + lua_tofix32(l, 4);

        /* FIXME: check the clamp order… before or after above addition? */
        that->m_clip.aa.x = fix32::max(x0, 0.0);
        that->m_clip.aa.y = fix32::max(y0, 0.0);
        that->m_clip.bb.x = fix32::min(x1, 128.0);
        that->m_clip.bb.y = fix32::min(y1, 128.0);
    }

    return 0;
}

int vm::api::cls(lua_State *l)
{
    int c = (int)lua_tofix32(l, 1) & 0xf;
    vm *that = get_this(l);
    ::memset(&that->m_memory[OFFSET_SCREEN], c * 0x11, SIZE_SCREEN);
    that->m_cursor.x = that->m_cursor.y = 0.0;
    return 0;
}

int vm::api::color(lua_State *l)
{
    vm *that = get_this(l);
    that->m_colors = lua_tofix32(l, 1);
    return 0;
}

int vm::api::fillp(lua_State *l)
{
    vm *that = get_this(l);
    that->m_fillp = lua_tofix32(l, 1);
    return 0;
}

int vm::api::fget(lua_State *l)
{
    if (lua_isnone(l, 1))
        return 0;

    int n = (int)lua_tofix32(l, 1);
    uint8_t bits = 0;

    if (n >= 0 && n < SIZE_GFX_PROPS)
    {
        vm *that = get_this(l);
        bits = that->m_memory[OFFSET_GFX_PROPS + n];
    }

    if (lua_isnone(l, 2))
        lua_pushnumber(l, bits);
    else
        lua_pushboolean(l, bits & (1 << (int)lua_tofix32(l, 2)));

    return 1;
}

int vm::api::fset(lua_State *l)
{
    if (lua_isnone(l, 1) || lua_isnone(l, 2))
        return 0;

    int n = (int)lua_tofix32(l, 1);

    if (n >= 0 && n < SIZE_GFX_PROPS)
    {
        vm *that = get_this(l);
        uint8_t bits = that->m_memory[OFFSET_GFX_PROPS + n];
        int f = (int)lua_tofix32(l, 2);

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

int vm::api::line(lua_State *l)
{
    vm *that = get_this(l);

    fix32 x0 = fix32::floor(lua_tofix32(l, 1) - that->m_camera.x);
    fix32 y0 = fix32::floor(lua_tofix32(l, 2) - that->m_camera.y);
    fix32 x1 = fix32::floor(lua_tofix32(l, 3) - that->m_camera.x);
    fix32 y1 = fix32::floor(lua_tofix32(l, 4) - that->m_camera.y);
    if (!lua_isnone(l, 5))
        that->m_colors = lua_tofix32(l, 5);

    if (x0 == x1 && y0 == y1)
    {
        that->setpixel(x0, y0, that->m_colors);
    }
    else if (fix32::abs(x1 - x0) > fix32::abs(y1 - y0))
    {
        for (int16_t x = (int16_t)fix32::min(x0, x1); x <= (int16_t)fix32::max(x0, x1); ++x)
        {
            int16_t y = lol::round(lol::mix((double)y0, (double)y1, (x - (double)x0) / (double)(x1 - x0)));
            that->setpixel(fix32(x), fix32(y), that->m_colors);
        }
    }
    else
    {
        for (int16_t y = (int16_t)fix32::min(y0, y1); y <= (int16_t)fix32::max(y0, y1); ++y)
        {
            int16_t x = lol::round(lol::mix((double)x0, (double)x1, (y -(double)y0) / (double)(y1 - y0)));
            that->setpixel(fix32(x), fix32(y), that->m_colors);
        }
    }

    return 0;
}

int vm::api::map(lua_State *l)
{
    vm *that = get_this(l);

    int cel_x = (int)lua_tofix32(l, 1);
    int cel_y = (int)lua_tofix32(l, 2);
    fix32 sx = lua_tofix32(l, 3) - that->m_camera.x;
    fix32 sy = lua_tofix32(l, 4) - that->m_camera.y;
    // PICO-8 documentation: “If cel_w and cel_h are not specified,
    // defaults to 128,32”.
    bool no_size = lua_isnone(l, 5) && lua_isnone(l, 6);
    int cel_w = no_size ? 128 : (int)lua_tofix32(l, 5);
    int cel_h = no_size ? 32 : (int)lua_tofix32(l, 6);
    int layer = (int)lua_tofix32(l, 7);

    for (int16_t dy = 0; dy < cel_h * 8; ++dy)
    for (int16_t dx = 0; dx < cel_w * 8; ++dx)
    {
        int16_t cx = cel_x + dx / 8;
        int16_t cy = cel_y + dy / 8;
        if (cx < 0 || cx >= 128 || cy < 0 || cy >= 64)
            continue;

        int16_t line = cy < 32 ? OFFSET_MAP + 128 * cy
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
                fix32 c = fix32(that->m_pal[0][col & 0xf]);
                that->setpixel(sx + fix32(dx), sy + fix32(dy), c);
            }
        }
    }

    return 0;
}

int vm::api::mget(lua_State *l)
{
    int x = (int)lua_tofix32(l, 1);
    int y = (int)lua_tofix32(l, 2);
    int16_t n = 0;

    if (x >= 0 && x < 128 && y >= 0 && y < 64)
    {
        vm *that = get_this(l);
        int line = y < 32 ? OFFSET_MAP + 128 * y
                          : OFFSET_MAP2 + 128 * (y - 32);
        n = that->m_memory[line + x];
    }

    lua_pushfix32(l, fix32(n));
    return 1;
}

int vm::api::mset(lua_State *l)
{
    int x = (int)lua_tofix32(l, 1);
    int y = (int)lua_tofix32(l, 2);
    int n = (int)lua_tofix32(l, 3);

    if (x >= 0 && x < 128 && y >= 0 && y < 64)
    {
        vm *that = get_this(l);
        int line = y < 32 ? OFFSET_MAP + 128 * y
                          : OFFSET_MAP2 + 128 * (y - 32);
        that->m_memory[line + x] = n;
    }

    return 0;
}

int vm::api::pal(lua_State *l)
{
    vm *that = get_this(l);

    if (lua_isnone(l, 1) || lua_isnone(l, 2))
    {
        // PICO-8 documentation: “pal() to reset to system defaults (including
        // transparency values and fill pattern)”
        for (int i = 0; i < 16; ++i)
        {
            that->m_pal[0][i] = that->m_pal[1][i] = i;
            that->m_palt[i] = i ? 0 : 1;
        }
        that->m_fillp = 0.0;
    }
    else
    {
        int c0 = (int)lua_tofix32(l, 1);
        int c1 = (int)lua_tofix32(l, 2);
        int p = (int)lua_tofix32(l, 3);

        that->m_pal[p & 1][c0 & 0xf] = c1 & 0xf;
    }

    return 0;
}

int vm::api::palt(lua_State *l)
{
    vm *that = get_this(l);

    if (lua_isnone(l, 1) || lua_isnone(l, 2))
    {
        for (int i = 0; i < 16; ++i)
            that->m_palt[i] = i ? 0 : 1;
    }
    else
    {
        int c = (int)lua_tofix32(l, 1);
        int t = lua_toboolean(l, 2);
        that->m_palt[c & 0xf] = t;
    }

    return 0;
}

int vm::api::pget(lua_State *l)
{
    vm *that = get_this(l);

    /* pget() is affected by camera() and by clip() */
    fix32 x = lua_tofix32(l, 1) - that->m_camera.x;
    fix32 y = lua_tofix32(l, 2) - that->m_camera.y;

    lua_pushfix32(l, fix32(that->getpixel(x, y)));

    return 1;
}

int vm::api::pset(lua_State *l)
{
    vm *that = get_this(l);

    fix32 x = lua_tofix32(l, 1) - that->m_camera.x;
    fix32 y = lua_tofix32(l, 2) - that->m_camera.y;
    if (!lua_isnone(l, 3))
        that->m_colors = lua_tofix32(l, 3);

    that->setpixel(x, y, that->m_colors);

    return 0;
}

int vm::api::rect(lua_State *l)
{
    vm *that = get_this(l);

    fix32 x0 = lua_tofix32(l, 1) - that->m_camera.x;
    fix32 y0 = lua_tofix32(l, 2) - that->m_camera.y;
    fix32 x1 = lua_tofix32(l, 3) - that->m_camera.x;
    fix32 y1 = lua_tofix32(l, 4) - that->m_camera.y;
    if (!lua_isnone(l, 5))
        that->m_colors = lua_tofix32(l, 5);

    if (x0 > x1)
        std::swap(x0, x1);

    if (y0 > y1)
        std::swap(y0, y1);

    that->hline(x0, x1, y0, that->m_colors);
    that->hline(x0, x1, y1, that->m_colors);

    if (y0 + fix32(1.0) < y1)
    {
        that->vline(x0, y0 + fix32(1.0), y1 - fix32(1.0), that->m_colors);
        that->vline(x1, y0 + fix32(1.0), y1 - fix32(1.0), that->m_colors);
    }

    return 0;
}

int vm::api::rectfill(lua_State *l)
{
    vm *that = get_this(l);

    fix32 x0 = lua_tofix32(l, 1) - that->m_camera.x;
    fix32 y0 = lua_tofix32(l, 2) - that->m_camera.y;
    fix32 x1 = lua_tofix32(l, 3) - that->m_camera.x;
    fix32 y1 = lua_tofix32(l, 4) - that->m_camera.y;
    if (!lua_isnone(l, 5))
        that->m_colors = lua_tofix32(l, 5);

    if (y0 > y1)
        std::swap(y0, y1);

    // FIXME: broken when y0 = 0.5, y1 = 1.4... 2nd line is not printed
    for (fix32 y = y0; y <= y1; y += fix32(1.0))
        that->hline(x0, x1, y, that->m_colors);

    return 0;
}

int vm::api::sget(lua_State *l)
{
    vm *that = get_this(l);

    fix32 x = lua_tofix32(l, 1);
    fix32 y = lua_tofix32(l, 2);

    lua_pushnumber(l, that->getspixel((int)x, (int)y));

    return 1;
}

int vm::api::sset(lua_State *l)
{
    vm *that = get_this(l);

    fix32 x = lua_tofix32(l, 1);
    fix32 y = lua_tofix32(l, 2);
    fix32 col = lua_isnone(l, 3) ? that->m_colors : lua_tofix32(l, 3);
    int c = that->m_pal[0][(int)col & 0xf];

    that->setspixel((int)x, (int)y, c);

    return 0;
}

int vm::api::spr(lua_State *l)
{
    vm *that = get_this(l);

    // FIXME: should we abort if n == 0?
    int n = (int)lua_tofix32(l, 1);
    fix32 x = lua_tofix32(l, 2) - that->m_camera.x;
    fix32 y = lua_tofix32(l, 3) - that->m_camera.y;
    int w8 = lua_isnoneornil(l, 4) ? 8 : (int)(lua_tofix32(l, 4) * fix32(8.0));
    int h8 = lua_isnoneornil(l, 5) ? 8 : (int)(lua_tofix32(l, 5) * fix32(8.0));
    int flip_x = lua_toboolean(l, 6);
    int flip_y = lua_toboolean(l, 7);

    for (int16_t j = 0; j < h8; ++j)
        for (int16_t i = 0; i < w8; ++i)
        {
            int di = flip_x ? w8 - 1 - i : i;
            int dj = flip_y ? h8 - 1 - j : j;
            int col = that->getspixel(n % 16 * 8 + di, n / 16 * 8 + dj);
            if (!that->m_palt[col])
            {
                fix32 c = fix32(that->m_pal[0][col]);
                that->setpixel(x + fix32(i), y + fix32(j), c);
            }
        }

    return 0;
}

int vm::api::sspr(lua_State *l)
{
    vm *that = get_this(l);

    int sx = (int)lua_tofix32(l, 1);
    int sy = (int)lua_tofix32(l, 2);
    int sw = (int)lua_tofix32(l, 3);
    int sh = (int)lua_tofix32(l, 4);
    fix32 dx = lua_tofix32(l, 5) - that->m_camera.x;
    fix32 dy = lua_tofix32(l, 6) - that->m_camera.y;
    int dw = lua_isnone(l, 7) ? sw : (int)lua_tofix32(l, 7);
    int dh = lua_isnone(l, 8) ? sh : (int)lua_tofix32(l, 8);
    int flip_x = lua_toboolean(l, 9);
    int flip_y = lua_toboolean(l, 10);

    // Iterate over destination pixels
    for (int16_t j = 0; j < dh; ++j)
    for (int16_t i = 0; i < dw; ++i)
    {
        int di = flip_x ? dw - 1 - i : i;
        int dj = flip_y ? dh - 1 - j : j;

        // Find source
        int x = sx + sw * di / dw;
        int y = sy + sh * dj / dh;

        int col = that->getspixel(x, y);
        if (!that->m_palt[col])
        {
            fix32 c = fix32(that->m_pal[0][col]);
            that->setpixel(dx + fix32(i), dy + fix32(j), c);
        }
    }

    return 0;
}

} // namespace z8

