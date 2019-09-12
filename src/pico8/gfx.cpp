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

#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <lol/engine.h>

#include "pico8/vm.h"
#include "bios.h"

namespace z8::pico8
{

using lol::msg;

/* Return color bits for use with set_pixel().
 *  - bits 0x0000ffff: fillp pattern
 *  - bits 0x000f0000: default color (palette applied)
 *  - bits 0x00f00000: color for patterns (palette applied)
 *  - bit  0x01000000: transparency for patterns */
uint32_t vm::lua_to_color_bits(lua_State *l, int n)
{
    auto &ds = m_ram.draw_state;

    if (!lua_isnone(l, n))
    {
        /* From the PICO-8 documentation:
         *  -- bit  0x1000.0000 means the non-colour bits should be observed
         *  -- bit  0x0100.0000 transparency bit
         *  -- bits 0x00FF.0000 are the usual colour bits
         *  -- bits 0x0000.FFFF are interpreted as the fill pattern */
        fix32 col = lua_tonumber(l, n);
        ds.pen = (col.bits() >> 16) & 0xff;

        if (col.bits() & 0x10000000 && ds.fillp_flag) // 0x5f34
        {
            ds.fillp[0] = col.bits();
            ds.fillp[1] = col.bits() >> 8;
            ds.fillp_trans = (col.bits() >> 24) & 0x1;
        }
    }

    int32_t bits = 0;

    bits |= ds.fillp[0];
    bits |= ds.fillp[1] << 8;
    bits |= ds.fillp_trans << 24;

    uint8_t c1 = ds.pen & 0xf;
    uint8_t c2 = (ds.pen >> 4) & 0xf;

    bits |= (ds.pal[0][c1] & 0xf) << 16;
    bits |= (ds.pal[0][c2] & 0xf) << 20;

    return bits;
}

uint8_t vm::get_pixel(int16_t x, int16_t y) const
{
    auto &ds = m_ram.draw_state;

    if (x < ds.clip.x1 || x >= ds.clip.x2 || y < ds.clip.y1 || y >= ds.clip.y2)
        return 0;

    return m_ram.screen.get(x, y);
}

void vm::set_pixel(int16_t x, int16_t y, uint32_t color_bits)
{
    auto &ds = m_ram.draw_state;

    if (x < ds.clip.x1 || x >= ds.clip.x2 || y < ds.clip.y1 || y >= ds.clip.y2)
        return;

    uint8_t color = (color_bits >> 16) & 0xf;
    if ((color_bits >> ((x & 3) + 4 * (y & 3))) & 0x1)
    {
        if (color_bits & 0x1000000) // Special transparency bit
            return;
        color = (color_bits >> 20) & 0xf;
    }

    m_ram.screen.set(x, y, color);
}

void vm::setspixel(int16_t x, int16_t y, uint8_t color)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 128)
        return;

    m_ram.gfx.set(x, y, color);
}

uint8_t vm::getspixel(int16_t x, int16_t y)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 128)
        return 0;

    return m_ram.gfx.get(x, y);
}

void vm::hline(int16_t x1, int16_t x2, int16_t y, uint32_t color_bits)
{
    auto &ds = m_ram.draw_state;

    if (y < ds.clip.y1 || y >= ds.clip.y2)
        return;

    if (x1 > x2)
        std::swap(x1, x2);

    x1 = std::max(x1, (int16_t)ds.clip.x1);
    x2 = std::min(x2, (int16_t)(ds.clip.x2 - 1));

    if (x1 > x2)
        return;

    // FIXME: very inefficient when fillp is active
    if (color_bits & 0xffff)
    {
        for (int16_t x = x1; x <= x2; ++x)
            set_pixel(x, y, color_bits);
    }
    else
    {
        uint8_t *p = m_ram.screen.data[y];
        uint8_t color = (color_bits >> 16) & 0xf;

        if (x1 & 1)
        {
            p[x1 / 2] = (p[x1 / 2] & 0x0f) | (color << 4);
            ++x1;
        }

        if ((x2 & 1) == 0)
        {
            p[x2 / 2] = (p[x2 / 2] & 0xf0) | color;
            --x2;
        }

        ::memset(p + x1 / 2, color * 0x11, (x2 - x1 + 1) / 2);
    }
}

void vm::vline(int16_t x, int16_t y1, int16_t y2, uint32_t color_bits)
{
    auto &ds = m_ram.draw_state;

    if (x < ds.clip.x1 || x >= ds.clip.x2)
        return;

    if (y1 > y2)
        std::swap(y1, y2);

    y1 = std::max(y1, (int16_t)ds.clip.y1);
    y2 = std::min(y2, (int16_t)(ds.clip.y2 - 1));

    if (y1 > y2)
        return;

    // FIXME: very inefficient when fillp is active
    if (color_bits & 0xffff)
    {
        for (int16_t y = y1; y <= y2; ++y)
            set_pixel(x, y, color_bits);
    }
    else
    {
        uint8_t mask = (x & 1) ? 0x0f : 0xf0;
        uint8_t color = (color_bits >> 16) & 0xf;
        uint8_t p = (x & 1) ? color << 4 : color;

        for (int16_t y = y1; y <= y2; ++y)
            m_ram.screen.data[y][x] = (m_ram.screen.data[y][x] & mask) | p;
    }
}


//
// Text
//

int vm::api_cursor(lua_State *l)
{
    m_ram.draw_state.cursor.x = (uint8_t)lua_tonumber(l, 1);
    m_ram.draw_state.cursor.y = (uint8_t)lua_tonumber(l, 2);
    if (!lua_isnone(l, 3))
    {
        fix32 colors = lua_tonumber(l, 3);
        m_ram.draw_state.pen = (uint8_t)colors;
    }
    return 0;
}

int vm::api_print(lua_State *l)
{
    auto &ds = m_ram.draw_state;

    if (lua_isnone(l, 1))
        return 0;

    // Leverage lua_pushtostr() to make sure we have a string
    lua_pushtostr(l, false);
    char const *str = lua_tostring(l, -1);
    lua_pop(l, 1);

    // FIXME: make x and y int16_t instead?
    bool use_cursor = lua_isnone(l, 2) || lua_isnone(l, 3);
    fix32 x = use_cursor ? fix32(ds.cursor.x) : lua_tonumber(l, 2);
    fix32 y = use_cursor ? fix32(ds.cursor.y) : lua_tonumber(l, 3);
    // FIXME: we ignore fillp here, but should we set it in lua_to_color_bits()?
    uint32_t color_bits = lua_to_color_bits(l, 4) & 0xf0000;
    fix32 initial_x = x;

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
            // PICO-8 characters end at 0x99.
            int index = ch > 0x20 && ch < 0x9a ? ch - 0x20 : 0;
            int16_t w = index < 0x60 ? 4 : 8;
            int font_x = index % (128 / w) * w;
            int font_y = index / (128 / w) * 6 - (w / 8 * 18);

            for (int16_t dy = 0; dy < 5; ++dy)
                for (int16_t dx = 0; dx < w; ++dx)
                {
                    int16_t screen_x = (int16_t)x - ds.camera.x() + dx;
                    int16_t screen_y = (int16_t)y - ds.camera.y() + dy;

                    if (m_bios->get_spixel(font_x + dx, font_y + dy))
                        set_pixel(screen_x, screen_y, color_bits);
                }

            x += fix32(w);
        }
    }

    // In PICO-8 scrolling only happens _after_ the whole string was printed,
    // even if it contained carriage returns or if the cursor was already
    // below the threshold value.
    if (use_cursor)
    {
        int16_t const lines = 6;

        // FIXME: is this affected by the camera?
        if (y > fix32(116.0))
        {
            uint8_t *s = m_ram.screen.data[0];
            memmove(s, s + lines * 64, sizeof(m_ram.screen) - lines * 64);
            ::memset(s + sizeof(m_ram.screen) - lines * 64, 0, lines * 64);
            y -= fix32(lines);
        }

        ds.cursor.x = (uint8_t)initial_x;
        ds.cursor.y = (uint8_t)(y + fix32(lines));
    }
    else
    {
        // FIXME: should a multiline print update y?
        ds.cursor.x = (uint8_t)initial_x;
        ds.cursor.y = (uint8_t)y;
    }

    return 0;
}

//
// Graphics
//

void vm::api_camera(int16_t x, int16_t y)
{
    auto &ds = m_ram.draw_state;

    ds.camera.set_x(x);
    ds.camera.set_y(y);
}

int vm::api_circ(lua_State *l)
{
    auto &ds = m_ram.draw_state;

    int16_t x = (int16_t)lua_tonumber(l, 1) - ds.camera.x();
    int16_t y = (int16_t)lua_tonumber(l, 2) - ds.camera.y();
    int16_t r = (int16_t)lua_tonumber(l, 3);
    uint32_t color_bits = lua_to_color_bits(l, 4);

    for (int16_t dx = r, dy = 0, err = 0; dx >= dy; )
    {
        set_pixel(x + dx, y + dy, color_bits);
        set_pixel(x + dy, y + dx, color_bits);
        set_pixel(x - dy, y + dx, color_bits);
        set_pixel(x - dx, y + dy, color_bits);
        set_pixel(x - dx, y - dy, color_bits);
        set_pixel(x - dy, y - dx, color_bits);
        set_pixel(x + dy, y - dx, color_bits);
        set_pixel(x + dx, y - dy, color_bits);

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

int vm::api_circfill(lua_State *l)
{
    auto &ds = m_ram.draw_state;

    int16_t x = (int16_t)lua_tonumber(l, 1) - ds.camera.x();
    int16_t y = (int16_t)lua_tonumber(l, 2) - ds.camera.y();
    int16_t r = (int16_t)lua_tonumber(l, 3);
    uint32_t color_bits = lua_to_color_bits(l, 4);

    for (int16_t dx = r, dy = 0, err = 0; dx >= dy; )
    {
        /* Some minor overdraw here, but nothing serious */
        hline(x - dx, x + dx, y - dy, color_bits);
        hline(x - dx, x + dx, y + dy, color_bits);
        vline(x - dy, y - dx, y + dx, color_bits);
        vline(x + dy, y - dx, y + dx, color_bits);

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

int vm::api_clip(lua_State *l)
{
    int x1 = 0, y1 = 0, x2 = 128, y2 = 128;

    /* XXX: there were rendering issues with Hyperspace by J.Fry when we were
     * only checking for lua_isnone(l,1) (instead of 4) because first argument
     * was actually "" instead of nil. */
    if (!lua_isnone(l, 4))
    {
        x1 = std::max(x1, (int)lua_tonumber(l, 1));
        y1 = std::max(y1, (int)lua_tonumber(l, 2));
        x2 = std::min(x2, x1 + (int)lua_tonumber(l, 3));
        y2 = std::min(y2, y1 + (int)lua_tonumber(l, 4));
    }

    auto &ds = m_ram.draw_state;
    ds.clip.x1 = x1;
    ds.clip.y1 = y1;
    ds.clip.x2 = x2;
    ds.clip.y2 = y2;
    return 0;
}

int vm::api_cls(lua_State *l)
{
    int c = (int)lua_tonumber(l, 1) & 0xf;
    ::memset(&m_ram.screen, c * 0x11, sizeof(m_ram.screen));

    // Documentation: “Clear the screen and reset the clipping rectangle”.
    auto &ds = m_ram.draw_state;
    ds.cursor.x = ds.cursor.y = 0;
    ds.clip.x1 = ds.clip.y1 = 0;
    ds.clip.x2 = ds.clip.y2 = 128;
    // Undocumented: set cursor to 0,0
    m_ram.draw_state.cursor.x = 0;
    m_ram.draw_state.cursor.y = 0;
    return 0;
}

int vm::api_color(lua_State *l)
{
    fix32 colors = lua_tonumber(l, 1);
    m_ram.draw_state.pen = (uint8_t)colors;
    return 0;
}

int vm::api_fillp(lua_State *l)
{
    fix32 fillp = lua_tonumber(l, 1);
    m_ram.draw_state.fillp[0] = fillp.bits() >> 16;
    m_ram.draw_state.fillp[1] = fillp.bits() >> 24;
    m_ram.draw_state.fillp_trans = (fillp.bits() >> 15) & 1;
    return 0;
}

int vm::api_fget(lua_State *l)
{
    if (lua_isnone(l, 1))
        return 0;

    int n = (int)lua_tonumber(l, 1);
    uint8_t bits = 0;

    if (n >= 0 && n < (int)sizeof(m_ram.gfx_props))
    {
        bits = m_ram.gfx_props[n];
    }

    if (lua_isnone(l, 2))
        lua_pushnumber(l, bits);
    else
        lua_pushboolean(l, bits & (1 << (int)lua_tonumber(l, 2)));

    return 1;
}

int vm::api_fset(lua_State *l)
{
    if (lua_isnone(l, 1) || lua_isnone(l, 2))
        return 0;

    int n = (int)lua_tonumber(l, 1);

    if (n >= 0 && n < (int)sizeof(m_ram.gfx_props))
    {
        uint8_t bits = m_ram.gfx_props[n];
        int f = (int)lua_tonumber(l, 2);

        if (lua_isnone(l, 3))
            bits = f;
        else if (lua_toboolean(l, 3))
            bits |= 1 << (int)f;
        else
            bits &= ~(1 << (int)f);

        m_ram.gfx_props[n] = bits;
    }

    return 0;
}

int vm::api_line(lua_State *l)
{
    auto &ds = m_ram.draw_state;

    int16_t x0, y0, x1, y1;

    // Polyline mode if and only if there are 2 arguments
    if (lua_isnone(l, 3) && !lua_isnone(l, 2))
    {
        x0 = ds.polyline.x();
        y0 = ds.polyline.y();
        x1 = (int16_t)lua_tonumber(l, 1);
        y1 = (int16_t)lua_tonumber(l, 2);
    }
    else
    {
        x0 = (int16_t)lua_tonumber(l, 1);
        y0 = (int16_t)lua_tonumber(l, 2);
        x1 = (int16_t)lua_tonumber(l, 3);
        y1 = (int16_t)lua_tonumber(l, 4);
    }

    // Store polyline state
    ds.polyline.set_x(x1);
    ds.polyline.set_y(y1);

    x0 -= ds.camera.x(); y0 -= ds.camera.y();
    x1 -= ds.camera.x(); y1 -= ds.camera.y();

    uint32_t color_bits = lua_to_color_bits(l, 5);

    if (x0 == x1 && y0 == y1)
    {
        set_pixel(x0, y0, color_bits);
    }
    else if (lol::abs(x1 - x0) > lol::abs(y1 - y0))
    {
        for (int16_t x = std::min(x0, x1); x <= std::max(x0, x1); ++x)
        {
            int16_t y = (int16_t)lol::round(lol::mix((double)y0, (double)y1, (double)(x - x0) / (x1 - x0)));
            set_pixel(x, y, color_bits);
        }
    }
    else
    {
        for (int16_t y = std::min(y0, y1); y <= std::max(y0, y1); ++y)
        {
            int16_t x = (int16_t)lol::round(lol::mix((double)x0, (double)x1, (double)(y - y0) / (y1 - y0)));
            set_pixel(x, y, color_bits);
        }
    }

    return 0;
}

int vm::api_map(lua_State *l)
{
    auto &ds = m_ram.draw_state;

    int16_t cel_x = (int16_t)lua_tonumber(l, 1);
    int16_t cel_y = (int16_t)lua_tonumber(l, 2);
    int16_t sx = (int16_t)lua_tonumber(l, 3) - ds.camera.x();
    int16_t sy = (int16_t)lua_tonumber(l, 4) - ds.camera.y();
    // PICO-8 documentation: “If cel_w and cel_h are not specified,
    // defaults to 128,32”.
    bool no_size = lua_isnone(l, 5) && lua_isnone(l, 6);
    int16_t cel_w = no_size ? 128 : (int16_t)lua_tonumber(l, 5);
    int16_t cel_h = no_size ? 32 : (int16_t)lua_tonumber(l, 6);
    int16_t layer = (int16_t)lua_tonumber(l, 7);

    for (int16_t dy = 0; dy < cel_h * 8; ++dy)
    for (int16_t dx = 0; dx < cel_w * 8; ++dx)
    {
        int16_t cx = cel_x + dx / 8;
        int16_t cy = cel_y + dy / 8;
        if (cx < 0 || cx >= 128 || cy < 0 || cy >= 64)
            continue;

        uint8_t sprite = m_ram.map[128 * cy + cx];
        uint8_t bits = m_ram.gfx_props[sprite];
        if (layer && !(bits & layer))
            continue;

        if (sprite)
        {
            int col = getspixel(sprite % 16 * 8 + dx % 8, sprite / 16 * 8 + dy % 8);
            if ((ds.pal[0][col] & 0x10) == 0)
            {
                uint32_t color_bits = (ds.pal[0][col] & 0xf) << 16;
                set_pixel(sx + dx, sy + dy, color_bits);
            }
        }
    }

    return 0;
}

int vm::api_mget(lua_State *l)
{
    int x = (int)lua_tonumber(l, 1);
    int y = (int)lua_tonumber(l, 2);
    uint8_t n = 0;

    if (x >= 0 && x < 128 && y >= 0 && y < 64)
        n = m_ram.map[128 * y + x];

    lua_pushnumber(l, fix32(n));
    return 1;
}

int vm::api_mset(lua_State *l)
{
    int x = (int)lua_tonumber(l, 1);
    int y = (int)lua_tonumber(l, 2);
    int n = (int)lua_tonumber(l, 3);

    if (x >= 0 && x < 128 && y >= 0 && y < 64)
        m_ram.map[128 * y + x] = n;

    return 0;
}

int vm::api_pal(lua_State *l)
{
    auto &ds = m_ram.draw_state;

    if (lua_isnone(l, 1) || lua_isnone(l, 2))
    {
        // PICO-8 documentation: “pal() to reset to system defaults (including
        // transparency values and fill pattern)”
        for (int i = 0; i < 16; ++i)
        {
            ds.pal[0][i] = i | (i ? 0x00 : 0x10);
            ds.pal[1][i] = i;
        }
        ds.fillp[0] = 0;
        ds.fillp[1] = 0;
        ds.fillp_trans = 0;
    }
    else
    {
        int c0 = (int)lua_tonumber(l, 1) & 0xf;
        int c1 = (int)lua_tonumber(l, 2);
        int p = (int)lua_tonumber(l, 3);

        if (p & 1)
        {
            ds.pal[1][c0] = c1 & 0xff;
        }
        else
        {
            // Transparency bit is preserved
            ds.pal[p & 1][c0] &= 0x10;
            ds.pal[p & 1][c0] |= c1 & 0xf;
        }
    }

    return 0;
}

int vm::api_palt(lua_State *l)
{
    auto &ds = m_ram.draw_state;

    if (lua_isnone(l, 1) || lua_isnone(l, 2))
    {
        for (int i = 0; i < 16; ++i)
        {
            ds.pal[0][i] &= 0xf;
            ds.pal[0][i] |= i ? 0x00 : 0x10;
        }
    }
    else
    {
        int c = (int)lua_tonumber(l, 1) & 0xf;
        int t = lua_toboolean(l, 2);
        ds.pal[0][c] &= 0xf;
        ds.pal[0][c] |= t ? 0x10 : 0x00;
    }

    return 0;
}

int vm::api_pget(lua_State *l)
{
    auto &ds = m_ram.draw_state;

    /* pget() is affected by camera() and by clip() */
    // FIXME: "and by clip()"? wut?
    int16_t x = (int16_t)lua_tonumber(l, 1) - ds.camera.x();
    int16_t y = (int16_t)lua_tonumber(l, 2) - ds.camera.y();
    lua_pushnumber(l, fix32(get_pixel(x, y)));

    return 1;
}

int vm::api_pset(lua_State *l)
{
    auto &ds = m_ram.draw_state;

    int16_t x = (int16_t)lua_tonumber(l, 1) - ds.camera.x();
    int16_t y = (int16_t)lua_tonumber(l, 2) - ds.camera.y();
    uint32_t color_bits = lua_to_color_bits(l, 3);
    set_pixel(x, y, color_bits);

    return 0;
}

int vm::api_rect(lua_State *l)
{
    auto &ds = m_ram.draw_state;

    int16_t x0 = (int16_t)lua_tonumber(l, 1) - ds.camera.x();
    int16_t y0 = (int16_t)lua_tonumber(l, 2) - ds.camera.y();
    int16_t x1 = (int16_t)lua_tonumber(l, 3) - ds.camera.x();
    int16_t y1 = (int16_t)lua_tonumber(l, 4) - ds.camera.y();
    uint32_t color_bits = lua_to_color_bits(l, 5);

    if (x0 > x1)
        std::swap(x0, x1);

    if (y0 > y1)
        std::swap(y0, y1);

    hline(x0, x1, y0, color_bits);
    hline(x0, x1, y1, color_bits);

    if (y0 + 1 < y1)
    {
        vline(x0, y0 + 1, y1 - 1, color_bits);
        vline(x1, y0 + 1, y1 - 1, color_bits);
    }

    return 0;
}

int vm::api_rectfill(lua_State *l)
{
    auto &ds = m_ram.draw_state;

    int16_t x0 = (int16_t)lua_tonumber(l, 1) - ds.camera.x();
    int16_t y0 = (int16_t)lua_tonumber(l, 2) - ds.camera.y();
    int16_t x1 = (int16_t)lua_tonumber(l, 3) - ds.camera.x();
    int16_t y1 = (int16_t)lua_tonumber(l, 4) - ds.camera.y();
    uint32_t color_bits = lua_to_color_bits(l, 5);

    if (y0 > y1)
        std::swap(y0, y1);

    for (int16_t y = y0; y <= y1; ++y)
        hline(x0, x1, y, color_bits);

    return 0;
}

int vm::api_sget(lua_State *l)
{
    int16_t x = (int16_t)lua_tonumber(l, 1);
    int16_t y = (int16_t)lua_tonumber(l, 2);

    lua_pushnumber(l, getspixel(x, y));

    return 1;
}

int vm::api_sset(lua_State *l)
{
    auto &ds = m_ram.draw_state;

    int16_t x = lua_tonumber(l, 1);
    int16_t y = lua_tonumber(l, 2);
    uint8_t col = lua_isnone(l, 3) ? ds.pen : (uint8_t)lua_tonumber(l, 3);
    int c = ds.pal[0][col & 0xf];

    setspixel(x, y, c);

    return 0;
}

int vm::api_spr(lua_State *l)
{
    auto &ds = m_ram.draw_state;

    // FIXME: should we abort if n == 0?
    int16_t n = (int16_t)lua_tonumber(l, 1);
    int16_t x = (int16_t)lua_tonumber(l, 2) - ds.camera.x();
    int16_t y = (int16_t)lua_tonumber(l, 3) - ds.camera.y();
    int16_t w8 = lua_isnoneornil(l, 4) ? 8 : (int16_t)(lua_tonumber(l, 4) * fix32(8.0));
    int16_t h8 = lua_isnoneornil(l, 5) ? 8 : (int16_t)(lua_tonumber(l, 5) * fix32(8.0));
    int flip_x = lua_toboolean(l, 6);
    int flip_y = lua_toboolean(l, 7);

    for (int16_t j = 0; j < h8; ++j)
        for (int16_t i = 0; i < w8; ++i)
        {
            int16_t di = flip_x ? w8 - 1 - i : i;
            int16_t dj = flip_y ? h8 - 1 - j : j;
            uint8_t col = getspixel(n % 16 * 8 + di, n / 16 * 8 + dj);
            if ((ds.pal[0][col] & 0x10) == 0)
            {
                uint32_t color_bits = (ds.pal[0][col] & 0xf) << 16;
                set_pixel(x + i, y + j, color_bits);
            }
        }

    return 0;
}

int vm::api_sspr(lua_State *l)
{
    auto &ds = m_ram.draw_state;

    int16_t sx = (int16_t)lua_tonumber(l, 1);
    int16_t sy = (int16_t)lua_tonumber(l, 2);
    int16_t sw = (int16_t)lua_tonumber(l, 3);
    int16_t sh = (int16_t)lua_tonumber(l, 4);
    int16_t dx = (int16_t)lua_tonumber(l, 5) - ds.camera.x();
    int16_t dy = (int16_t)lua_tonumber(l, 6) - ds.camera.y();
    int16_t dw = lua_isnone(l, 7) ? sw : (int16_t)lua_tonumber(l, 7);
    int16_t dh = lua_isnone(l, 8) ? sh : (int16_t)lua_tonumber(l, 8);
    int flip_x = lua_toboolean(l, 9);
    int flip_y = lua_toboolean(l, 10);

    // Iterate over destination pixels
    // FIXME: maybe clamp if target area is too big?
    for (int16_t j = 0; j < dh; ++j)
    for (int16_t i = 0; i < dw; ++i)
    {
        int16_t di = flip_x ? dw - 1 - i : i;
        int16_t dj = flip_y ? dh - 1 - j : j;

        // Find source
        int16_t x = sx + sw * di / dw;
        int16_t y = sy + sh * dj / dh;

        uint8_t col = getspixel(x, y);
        if ((ds.pal[0][col] & 0x10) == 0)
        {
            uint32_t color_bits = (ds.pal[0][col] & 0xf) << 16;
            set_pixel(dx + i, dy + j, color_bits);
        }
    }

    return 0;
}

} // namespace z8

