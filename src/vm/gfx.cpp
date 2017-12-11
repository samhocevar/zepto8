//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2017 Sam Hocevar <sam@hocevar.net>
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

/* Return color bits for use with set_pixel().
 *  - bits 0x0000ffff: fillp pattern
 *  - bits 0x000f0000: default color (palette applied)
 *  - bits 0x00f00000: color for patterns (palette applied)
 *  - bit  0x01000000: transparency for patterns */
uint32_t vm::get_color_bits() const
{
    int32_t bits = 0;
    uint8_t c1 = (m_colors.bits() >> 16) & 0xf;
    uint8_t c2 = (m_colors.bits() >> 20) & 0xf;

    /* From the PICO-8 documentation:
     *  -- bit  0x1000.0000 means the non-colour bits should be observed
     *  -- bit  0x0100.0000 transparency bit
     *  -- bits 0x00FF.0000 are the usual colour bits
     *  -- bits 0x0000.FFFF are interpreted as the fill pattern */
    if (m_colors.bits() & 0x1000000)
    {
        bits |= (m_colors.bits() & 0x100ffff);
    }
    else
    {
        bits |= (m_fillp.bits() >> 16) & 0xffff;
        bits |= (m_fillp.bits() << 9) & 0x1000000;
    }

    bits |= m_pal[0][c1] << 16;
    bits |= m_pal[0][c2] << 20;

    return bits;
}

uint8_t vm::get_pixel(int16_t x, int16_t y) const
{
    if (x < get_clip_aa_x() || x >= get_clip_bb_x()
         || y < get_clip_aa_y() || y >= get_clip_bb_y())
        return 0;

    int offset = OFFSET_SCREEN + (128 * y + x) / 2;
    return (x & 1) ? m_memory[offset] >> 4 : m_memory[offset] & 0xf;
}

void vm::set_pixel(int16_t x, int16_t y, uint32_t color_bits)
{
    if (x < get_clip_aa_x() || x >= get_clip_bb_x()
         || y < get_clip_aa_y() || y >= get_clip_bb_y())
        return;

    uint8_t color = (color_bits >> 16) & 0xf;
    if ((color_bits >> ((x & 3) + 4 * (y & 3))) & 0x1)
    {
        if (color_bits & 0x1000000) // Special transparency bit
            return;
        color = (color_bits >> 20) & 0xf;
    }

    int offset = OFFSET_SCREEN + (128 * y + x) / 2;
    uint8_t mask = (x & 1) ? 0x0f : 0xf0;
    uint8_t p = (x & 1) ? color << 4 : color;
    m_memory[offset] = (m_memory[offset] & mask) | p;
}

void vm::setspixel(int16_t x, int16_t y, uint8_t color)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 128)
        return;

    int offset = OFFSET_GFX + (128 * y + x) / 2;
    uint8_t mask = (x & 1) ? 0x0f : 0xf0;
    uint8_t p = (x & 1) ? color << 4 : color;
    m_memory[offset] = (m_memory[offset] & mask) | p;
}

uint8_t vm::getspixel(int16_t x, int16_t y)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 128)
        return 0;

    int offset = OFFSET_GFX + (128 * y + x) / 2;
    return (x & 1) ? m_memory[offset] >> 4 : m_memory[offset] & 0xf;
}

void vm::hline(int16_t x1, int16_t x2, int16_t y, uint32_t color_bits)
{
    if (y < get_clip_aa_y() || y >= get_clip_bb_y())
        return;

    if (x1 > x2)
        std::swap(x1, x2);

    x1 = std::max(x1, (int16_t)get_clip_aa_x());
    x2 = std::min(x2, (int16_t)(get_clip_bb_x() - 1));

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
        uint8_t color = (color_bits >> 16) & 0xf;
        int offset = OFFSET_SCREEN + (128 * y) / 2;

        if (x1 & 1)
        {
            m_memory[offset + x1 / 2]
                = (m_memory[offset + x1 / 2] & 0x0f) | (color << 4);
            ++x1;
        }

        if ((x2 & 1) == 0)
        {
            m_memory[offset + x2 / 2]
                = (m_memory[offset + x2 / 2] & 0xf0) | color;
            --x2;
        }

        ::memset(get_mem(offset + x1 / 2), color * 0x11, (x2 - x1 + 1) / 2);
    }
}

void vm::vline(int16_t x, int16_t y1, int16_t y2, uint32_t color_bits)
{
    if (x < get_clip_aa_x() || x >= get_clip_bb_x())
        return;

    if (y1 > y2)
        std::swap(y1, y2);

    y1 = std::max(y1, (int16_t)get_clip_aa_y());
    y2 = std::min(y2, (int16_t)(get_clip_bb_y() - 1));

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
        {
            int offset = OFFSET_SCREEN + (128 * y + x) / 2;
            m_memory[offset] = (m_memory[offset] & mask) | p;
        }
    }
}

int16_t vm::get_camera_x() const
{
    uint8_t const *p = get_mem(OFFSET_CAMERA);
    return (int16_t)(p[0] | (p[1] << 8));
}

int16_t vm::get_camera_y() const
{
    uint8_t const *p = get_mem(OFFSET_CAMERA);
    return (int16_t)(p[2] | (p[3] << 8));
}


//
// Text
//

int vm::api_cursor(lua_State *l)
{
    *get_mem(OFFSET_CURSOR_X) = (uint8_t)lua_tofix32(l, 1);
    *get_mem(OFFSET_CURSOR_Y) = (uint8_t)lua_tofix32(l, 2);
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

int vm::api_print(lua_State *l)
{
    if (lua_isnone(l, 1))
        return 0;

    // Leverage lua_pushtostr() to make sure we have a string
    lua_pushtostr(l, false);
    char const *str = lua_tostring(l, -1);
    lua_pop(l, 1);

    // FIXME: make x and y int16_t instead?
    bool use_cursor = lua_isnone(l, 2) || lua_isnone(l, 3);
    fix32 x = use_cursor ? fix32(*get_mem(OFFSET_CURSOR_X)) : lua_tofix32(l, 2);
    fix32 y = use_cursor ? fix32(*get_mem(OFFSET_CURSOR_Y)) : lua_tofix32(l, 3);
    if (!lua_isnone(l, 4))
        m_colors = lua_tofix32(l, 4);
    fix32 initial_x = x;

    uint32_t color_bits = get_color_bits();

    auto pixels = m_font.lock<lol::PixelFormat::RGBA_8>();
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
                    int16_t screen_x = (int16_t)x - get_camera_x() + dx;
                    int16_t screen_y = (int16_t)y - get_camera_y() + dy;

                    if (pixels[(index / 16 * h + dy) * 128 + (index % 16 * w + dx)].r > 0)
                        set_pixel(screen_x, screen_y, color_bits);
                }

            x += fix32(w);
        }
    }

    m_font.unlock(pixels);

    // In PICO-8 scrolling only happens _after_ the whole string was printed,
    // even if it contained carriage returns or if the cursor was already
    // below the threshold value.
    if (use_cursor)
    {
        int16_t const lines = 6;

        // FIXME: is this affected by the camera?
        if (y > fix32(116.0))
        {
            uint8_t *screen = get_mem(OFFSET_SCREEN);
            memmove(screen, screen + lines * 64, SIZE_SCREEN - lines * 64);
            ::memset(screen + SIZE_SCREEN - lines * 64, 0, lines * 64);
            y -= fix32(lines);
        }

        *get_mem(OFFSET_CURSOR_X) = (uint8_t)initial_x;
        *get_mem(OFFSET_CURSOR_Y) = (uint8_t)(y + fix32(lines));
    }

    return 0;
}

int vm::api_tostr(lua_State *l)
{
    bool do_hex = lua_toboolean(l, 2);
    lua_pushtostr(l, do_hex);
    return 1;
}

int vm::api_tonum(lua_State *l)
{
    char const *str = lua_tostring(l, 1);

    // If parsing failed, PICO-8 returns nothing
    if (!analyzer::is_numeral(str))
        return 0;

    fix32 x = str[1] == 'b' ? fix32::parse_binary(str) : lua_tofix32(l, 1);
    lua_pushfix32(l, x);
    return 1;
}

//
// Graphics
//

int vm::api_camera(lua_State *l)
{
    int16_t x = (int16_t)lua_tofix32(l, 1);
    int16_t y = (int16_t)lua_tofix32(l, 2);
    uint8_t *p = get_mem(OFFSET_CAMERA);
    p[0] = (uint8_t)x; p[1] = (uint8_t)(x >> 8);
    p[2] = (uint8_t)y; p[3] = (uint8_t)(y >> 8);
    return 0;
}

int vm::api_circ(lua_State *l)
{
    int16_t x = (int16_t)lua_tofix32(l, 1) - get_camera_x();
    int16_t y = (int16_t)lua_tofix32(l, 2) - get_camera_y();
    int16_t r = (int16_t)lua_tofix32(l, 3);
    if (!lua_isnone(l, 4))
        m_colors = lua_tofix32(l, 4);

    uint32_t color_bits = get_color_bits();

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
    int16_t x = (int16_t)lua_tofix32(l, 1) - get_camera_x();
    int16_t y = (int16_t)lua_tofix32(l, 2) - get_camera_y();
    int16_t r = (int16_t)lua_tofix32(l, 3);
    if (!lua_isnone(l, 4))
        m_colors = lua_tofix32(l, 4);

    uint32_t color_bits = get_color_bits();

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
        x1 = std::max(x1, (int)lua_tofix32(l, 1));
        y1 = std::max(y1, (int)lua_tofix32(l, 2));
        x2 = std::min(x2, x1 + (int)lua_tofix32(l, 3));
        y2 = std::min(y2, y1 + (int)lua_tofix32(l, 4));
    }

    uint8_t *p = get_mem(OFFSET_CLIP);
    p[0] = (uint8_t)x1; p[1] = (uint8_t)y1;
    p[2] = (uint8_t)x2; p[3] = (uint8_t)y2;
    return 0;
}

int vm::api_cls(lua_State *l)
{
    int c = (int)lua_tofix32(l, 1) & 0xf;
    ::memset(&m_memory[OFFSET_SCREEN], c * 0x11, SIZE_SCREEN);
    *get_mem(OFFSET_CURSOR_X) = *get_mem(OFFSET_CURSOR_Y) = 0;
    return 0;
}

int vm::api_color(lua_State *l)
{
    m_colors = lua_tofix32(l, 1);
    return 0;
}

int vm::api_fillp(lua_State *l)
{
    m_fillp = lua_tofix32(l, 1);
    return 0;
}

int vm::api_fget(lua_State *l)
{
    if (lua_isnone(l, 1))
        return 0;

    int n = (int)lua_tofix32(l, 1);
    uint8_t bits = 0;

    if (n >= 0 && n < SIZE_GFX_PROPS)
    {
        bits = m_memory[OFFSET_GFX_PROPS + n];
    }

    if (lua_isnone(l, 2))
        lua_pushnumber(l, bits);
    else
        lua_pushboolean(l, bits & (1 << (int)lua_tofix32(l, 2)));

    return 1;
}

int vm::api_fset(lua_State *l)
{
    if (lua_isnone(l, 1) || lua_isnone(l, 2))
        return 0;

    int n = (int)lua_tofix32(l, 1);

    if (n >= 0 && n < SIZE_GFX_PROPS)
    {
        uint8_t bits = m_memory[OFFSET_GFX_PROPS + n];
        int f = (int)lua_tofix32(l, 2);

        if (lua_isnone(l, 3))
            bits = f;
        else if (lua_toboolean(l, 3))
            bits |= 1 << (int)f;
        else
            bits &= ~(1 << (int)f);

        m_memory[OFFSET_GFX_PROPS + n] = bits;
    }

    return 0;
}

int vm::api_line(lua_State *l)
{
    int16_t x0 = (int16_t)lua_tofix32(l, 1) - get_camera_x();
    int16_t y0 = (int16_t)lua_tofix32(l, 2) - get_camera_y();
    int16_t x1 = (int16_t)lua_tofix32(l, 3) - get_camera_x();
    int16_t y1 = (int16_t)lua_tofix32(l, 4) - get_camera_y();
    if (!lua_isnone(l, 5))
        m_colors = lua_tofix32(l, 5);

    uint32_t color_bits = get_color_bits();

    if (x0 == x1 && y0 == y1)
    {
        set_pixel(x0, y0, color_bits);
    }
    else if (lol::abs(x1 - x0) > lol::abs(y1 - y0))
    {
        for (int16_t x = std::min(x0, x1); x <= std::max(x0, x1); ++x)
        {
            int16_t y = lol::round(lol::mix((double)y0, (double)y1, (double)(x - x0) / (x1 - x0)));
            set_pixel(x, y, color_bits);
        }
    }
    else
    {
        for (int16_t y = std::min(y0, y1); y <= std::max(y0, y1); ++y)
        {
            int16_t x = lol::round(lol::mix((double)x0, (double)x1, (double)(y - y0) / (y1 - y0)));
            set_pixel(x, y, color_bits);
        }
    }

    return 0;
}

int vm::api_map(lua_State *l)
{
    int16_t cel_x = (int16_t)lua_tofix32(l, 1);
    int16_t cel_y = (int16_t)lua_tofix32(l, 2);
    int16_t sx = (int16_t)lua_tofix32(l, 3) - get_camera_x();
    int16_t sy = (int16_t)lua_tofix32(l, 4) - get_camera_y();
    // PICO-8 documentation: “If cel_w and cel_h are not specified,
    // defaults to 128,32”.
    bool no_size = lua_isnone(l, 5) && lua_isnone(l, 6);
    int16_t cel_w = no_size ? 128 : (int16_t)lua_tofix32(l, 5);
    int16_t cel_h = no_size ? 32 : (int16_t)lua_tofix32(l, 6);
    int16_t layer = (int16_t)lua_tofix32(l, 7);

    for (int16_t dy = 0; dy < cel_h * 8; ++dy)
    for (int16_t dx = 0; dx < cel_w * 8; ++dx)
    {
        int16_t cx = cel_x + dx / 8;
        int16_t cy = cel_y + dy / 8;
        if (cx < 0 || cx >= 128 || cy < 0 || cy >= 64)
            continue;

        int16_t line = cy < 32 ? OFFSET_MAP + 128 * cy
                               : OFFSET_MAP2 + 128 * (cy - 32);
        int sprite = m_memory[line + cx];

        uint8_t bits = m_memory[OFFSET_GFX_PROPS + sprite];
        if (layer && !(bits & layer))
            continue;

        if (sprite)
        {
            int col = getspixel(sprite % 16 * 8 + dx % 8, sprite / 16 * 8 + dy % 8);
            if (!m_palt[col])
            {
                uint32_t color_bits = m_pal[0][col & 0xf] << 16;
                set_pixel(sx + dx, sy + dy, color_bits);
            }
        }
    }

    return 0;
}

int vm::api_mget(lua_State *l)
{
    int x = (int)lua_tofix32(l, 1);
    int y = (int)lua_tofix32(l, 2);
    int16_t n = 0;

    if (x >= 0 && x < 128 && y >= 0 && y < 64)
    {
        int line = y < 32 ? OFFSET_MAP + 128 * y
                          : OFFSET_MAP2 + 128 * (y - 32);
        n = m_memory[line + x];
    }

    lua_pushfix32(l, fix32(n));
    return 1;
}

int vm::api_mset(lua_State *l)
{
    int x = (int)lua_tofix32(l, 1);
    int y = (int)lua_tofix32(l, 2);
    int n = (int)lua_tofix32(l, 3);

    if (x >= 0 && x < 128 && y >= 0 && y < 64)
    {
        int line = y < 32 ? OFFSET_MAP + 128 * y
                          : OFFSET_MAP2 + 128 * (y - 32);
        m_memory[line + x] = n;
    }

    return 0;
}

int vm::api_pal(lua_State *l)
{
    if (lua_isnone(l, 1) || lua_isnone(l, 2))
    {
        // PICO-8 documentation: “pal() to reset to system defaults (including
        // transparency values and fill pattern)”
        for (int i = 0; i < 16; ++i)
        {
            m_pal[0][i] = m_pal[1][i] = i;
            m_palt[i] = i ? 0 : 1;
        }
        m_fillp = 0.0;
    }
    else
    {
        int c0 = (int)lua_tofix32(l, 1);
        int c1 = (int)lua_tofix32(l, 2);
        int p = (int)lua_tofix32(l, 3);

        m_pal[p & 1][c0 & 0xf] = c1 & 0xf;
    }

    return 0;
}

int vm::api_palt(lua_State *l)
{
    if (lua_isnone(l, 1) || lua_isnone(l, 2))
    {
        for (int i = 0; i < 16; ++i)
            m_palt[i] = i ? 0 : 1;
    }
    else
    {
        int c = (int)lua_tofix32(l, 1);
        int t = lua_toboolean(l, 2);
        m_palt[c & 0xf] = t;
    }

    return 0;
}

int vm::api_pget(lua_State *l)
{
    /* pget() is affected by camera() and by clip() */
    int16_t x = (int16_t)lua_tofix32(l, 1) - get_camera_x();
    int16_t y = (int16_t)lua_tofix32(l, 2) - get_camera_y();

    lua_pushfix32(l, fix32(get_pixel(x, y)));

    return 1;
}

int vm::api_pset(lua_State *l)
{
    int16_t x = (int16_t)lua_tofix32(l, 1) - get_camera_x();
    int16_t y = (int16_t)lua_tofix32(l, 2) - get_camera_y();
    if (!lua_isnone(l, 3))
        m_colors = lua_tofix32(l, 3);

    uint32_t color_bits = get_color_bits();
    set_pixel(x, y, color_bits);

    return 0;
}

int vm::api_rect(lua_State *l)
{
    int16_t x0 = (int16_t)lua_tofix32(l, 1) - get_camera_x();
    int16_t y0 = (int16_t)lua_tofix32(l, 2) - get_camera_y();
    int16_t x1 = (int16_t)lua_tofix32(l, 3) - get_camera_x();
    int16_t y1 = (int16_t)lua_tofix32(l, 4) - get_camera_y();
    if (!lua_isnone(l, 5))
        m_colors = lua_tofix32(l, 5);

    uint32_t color_bits = get_color_bits();

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
    int16_t x0 = (int16_t)lua_tofix32(l, 1) - get_camera_x();
    int16_t y0 = (int16_t)lua_tofix32(l, 2) - get_camera_y();
    int16_t x1 = (int16_t)lua_tofix32(l, 3) - get_camera_x();
    int16_t y1 = (int16_t)lua_tofix32(l, 4) - get_camera_y();
    if (!lua_isnone(l, 5))
        m_colors = lua_tofix32(l, 5);

    uint32_t color_bits = get_color_bits();

    if (y0 > y1)
        std::swap(y0, y1);

    for (int16_t y = y0; y <= y1; ++y)
        hline(x0, x1, y, color_bits);

    return 0;
}

int vm::api_sget(lua_State *l)
{
    int16_t x = (int16_t)lua_tofix32(l, 1);
    int16_t y = (int16_t)lua_tofix32(l, 2);

    lua_pushnumber(l, getspixel(x, y));

    return 1;
}

int vm::api_sset(lua_State *l)
{
    int16_t x = lua_tofix32(l, 1);
    int16_t y = lua_tofix32(l, 2);
    uint8_t col = lua_isnone(l, 3) ? (uint8_t)m_colors : (uint8_t)lua_tofix32(l, 3);
    int c = m_pal[0][col & 0xf];

    setspixel(x, y, c);

    return 0;
}

int vm::api_spr(lua_State *l)
{
    // FIXME: should we abort if n == 0?
    int16_t n = (int16_t)lua_tofix32(l, 1);
    int16_t x = (int16_t)lua_tofix32(l, 2) - get_camera_x();
    int16_t y = (int16_t)lua_tofix32(l, 3) - get_camera_y();
    int16_t w8 = lua_isnoneornil(l, 4) ? 8 : (int16_t)(lua_tofix32(l, 4) * fix32(8.0));
    int16_t h8 = lua_isnoneornil(l, 5) ? 8 : (int16_t)(lua_tofix32(l, 5) * fix32(8.0));
    int flip_x = lua_toboolean(l, 6);
    int flip_y = lua_toboolean(l, 7);

    for (int16_t j = 0; j < h8; ++j)
        for (int16_t i = 0; i < w8; ++i)
        {
            int16_t di = flip_x ? w8 - 1 - i : i;
            int16_t dj = flip_y ? h8 - 1 - j : j;
            uint8_t col = getspixel(n % 16 * 8 + di, n / 16 * 8 + dj);
            if (!m_palt[col])
            {
                uint32_t color_bits = m_pal[0][col] << 16;
                set_pixel(x + i, y + j, color_bits);
            }
        }

    return 0;
}

int vm::api_sspr(lua_State *l)
{
    int16_t sx = (int16_t)lua_tofix32(l, 1);
    int16_t sy = (int16_t)lua_tofix32(l, 2);
    int16_t sw = (int16_t)lua_tofix32(l, 3);
    int16_t sh = (int16_t)lua_tofix32(l, 4);
    int16_t dx = (int16_t)lua_tofix32(l, 5) - get_camera_x();
    int16_t dy = (int16_t)lua_tofix32(l, 6) - get_camera_y();
    int16_t dw = lua_isnone(l, 7) ? sw : (int16_t)lua_tofix32(l, 7);
    int16_t dh = lua_isnone(l, 8) ? sh : (int16_t)lua_tofix32(l, 8);
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
        if (!m_palt[col])
        {
            uint32_t color_bits = m_pal[0][col] << 16;
            set_pixel(dx + i, dy + j, color_bits);
        }
    }

    return 0;
}

} // namespace z8

