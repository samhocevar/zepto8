//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016–2024 Sam Hocevar <sam@hocevar.net>
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

#include <lol/math>  // lol::round, lol::mix
#include <algorithm> // std::swap
#include <cmath>     // std::min, std::max

#include "pico8/vm.h"
#include "bios.h"

namespace z8::pico8
{

/* Return color bits for use with set_pixel().
 *  - bits 0x0000ffff: fillp pattern
 *  - bits 0x000f0000: default color (palette applied)
 *  - bits 0x00f00000: color for patterns (palette applied)
 *  - bit  0x01000000: transparency for patterns */
uint32_t vm::to_color_bits(opt<fix32> c)
{
    auto &ds = m_ram.draw_state;

    if (c)
    {
        /* From the PICO-8 documentation:
         *  -- bit  0x1000.0000 means the non-colour bits should be observed
         *  -- bit  0x0100.0000 transparency bit
         *  -- bits 0x00FF.0000 are the usual colour bits
         *  -- bits 0x0000.FFFF are interpreted as the fill pattern */
        fix32 col = *c;
        ds.pen = (col.bits() >> 16) & 0xff;

        if (col.bits() & 0x1000'0000 && ds.fillp_flag) // 0x5f34
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

    bits |= (ds.draw_palette[c1] & 0xf) << 16;
    bits |= (ds.draw_palette[c2] & 0xf) << 20;

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
    auto &hw = m_ram.hw_state;

    if (x < ds.clip.x1 || x >= ds.clip.x2 || y < ds.clip.y1 || y >= ds.clip.y2)
        return;

    uint8_t color = (color_bits >> 16) & 0xf;

    // This is where the fillp pattern is actually handled
    if ((color_bits >> (15 - (x & 3) - 4 * (y & 3))) & 0x1)
    {
        if (color_bits & 0x100'0000) // Special transparency bit
            return;
        color = (color_bits >> 20) & 0xf;
    }

    if (hw.bit_mask)
    {
        uint8_t old = m_ram.screen.get(x, y);
        color = (old & ~(hw.bit_mask & 7))
              | (color & (hw.bit_mask & 7) & (hw.bit_mask >> 4));
    }

    m_ram.screen.set(x, y, color);
}

void vm::hline(int16_t x1, int16_t x2, int16_t y, uint32_t color_bits)
{
    using std::min, std::max;

    auto &ds = m_ram.draw_state;
    auto &hw = m_ram.hw_state;

    if (y < ds.clip.y1 || y >= ds.clip.y2)
        return;

    if (x1 > x2)
        std::swap(x1, x2);

    x1 = max(x1, (int16_t)ds.clip.x1);
    x2 = min(x2, (int16_t)(ds.clip.x2 - 1));

    if (x1 > x2)
        return;

    // Cannot use shortcut code when fillp or bitplanes are active
    if ((color_bits & 0xffff) || hw.bit_mask)
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
    using std::min, std::max;

    auto &ds = m_ram.draw_state;
    auto &hw = m_ram.hw_state;

    if (x < ds.clip.x1 || x >= ds.clip.x2)
        return;

    if (y1 > y2)
        std::swap(y1, y2);

    y1 = max(y1, (int16_t)ds.clip.y1);
    y2 = min(y2, (int16_t)(ds.clip.y2 - 1));

    if (y1 > y2)
        return;

    // Cannot use shortcut code when fillp or bitplanes are active
    if ((color_bits & 0xffff) || hw.bit_mask)
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
            auto &data = m_ram.screen.data[y][x / 2];
            data = (data & mask) | p;
        }
    }
}


//
// Text
//

tup<uint8_t, uint8_t, uint8_t> vm::api_cursor(uint8_t x, uint8_t y, opt<uint8_t> in_c)
{
    auto &ds = m_ram.draw_state;
    uint8_t c = in_c ? *in_c : ds.pen;

    std::swap(ds.cursor.x, x);
    std::swap(ds.cursor.y, y);
    std::swap(ds.pen, c);

    return std::make_tuple(x, y, c);
}

void vm::api_print(opt<rich_string> str, opt<fix32> opt_x, opt<fix32> opt_y,
                   opt<fix32> c)
{
    auto &ds = m_ram.draw_state;
    auto &font = m_ram.custom_font;

    if (!str)
        return;

    // The presence of y indicates whether mode is print(s,c) or print(s,x,y,c)
    bool has_coords = !!opt_y;
    // FIXME: make x and y int16_t instead?
    fix32 x = has_coords ? *opt_x : fix32(ds.cursor.x);
    fix32 y = has_coords ? *opt_y : fix32(ds.cursor.y);
    // FIXME: we ignore fillp here, but should we set it in to_color_bits()?
    uint32_t color_bits = to_color_bits(has_coords ? c : opt_x) & 0xf'0000;
    fix32 initial_x = x;

    auto print_state = m_ram.hw_state.print_state;

    int16_t width = 4;
    int16_t height = 6;

    for (uint8_t ch : *str)
    {
        if (ch == '\0')
            break;

        switch (ch)
        {
        case '\n':
            x = initial_x;
            y += fix32(height);
            height = 6;
            break;
        case '\r':
            x = initial_x;
            break;
        case 14:
            print_state.custom = 1;
            break;
        case 15:
            print_state.custom = 0;
            break;
        default:
            {
                if (print_state.custom)
                {
                    int16_t w = std::min(int(ch < 0x80 ? font.width : font.extended_width), 8);
                    int16_t h = std::min(int(font.height), 8);
                    auto &g = font.glyphs[ch - 1];

                    for (int16_t dy = 0; dy < h; ++dy)
                    {
                        uint8_t gl = g[dy];

                        for (int16_t dx = 0; dx < w; ++dx, gl >>= 1)
                        {
                            int16_t screen_x = (int16_t)x - ds.camera.x + dx + font.offset.x;
                            int16_t screen_y = (int16_t)y - ds.camera.y + dy + font.offset.y;

                            if (gl & 0x1)
                                set_pixel(screen_x, screen_y, color_bits);
                        }
                    }

                    x += fix32(w);
                    height = std::max(height, int16_t(font.height));
                }
                else
                {
                    int16_t w = ch < 0x80 ? 4 : 8;
                    int offset = ch < 0x80 ? ch : 2 * ch - 0x80;
                    int font_x = offset % 32 * 4;
                    int font_y = offset / 32 * 6;

                    for (int16_t dy = 0; dy < 5; ++dy)
                        for (int16_t dx = 0; dx < w; ++dx)
                        {
                            int16_t screen_x = (int16_t)x - ds.camera.x + dx;
                            int16_t screen_y = (int16_t)y - ds.camera.y + dy;

                            if (m_bios->get_spixel(font_x + dx, font_y + dy))
                                set_pixel(screen_x, screen_y, color_bits);
                        }

                    x += fix32(w);
                }
            }
            break;
        }
    }

    // In PICO-8 scrolling only happens _after_ the whole string was printed,
    // even if it contained carriage returns or if the cursor was already
    // below the threshold value.
    if (has_coords)
    {
        // FIXME: should a multiline print update y?
        ds.cursor.x = (uint8_t)initial_x;
        ds.cursor.y = (uint8_t)y;
    }
    else
    {
        // FIXME: is this affected by the camera?
        if (y > fix32(128.0 - 2 * height))
        {
            uint8_t *s = m_ram.screen.data[0];
            memmove(s, s + height * 64, sizeof(m_ram.screen) - height * 64);
            ::memset(s + sizeof(m_ram.screen) - height * 64, 0, height * 64);
            y -= fix32(height);
        }

        ds.cursor.x = (uint8_t)initial_x;
        ds.cursor.y = (uint8_t)(y + fix32(height));
    }
}

//
// Graphics
//

tup<int16_t, int16_t> vm::api_camera(int16_t x, int16_t y)
{
    auto &ds = m_ram.draw_state;

    auto prev = ds.camera;
    ds.camera.x = x;
    ds.camera.y = y;
    return std::make_tuple(prev.x, prev.y);
}

void vm::api_circ(int16_t x, int16_t y, int16_t r, opt<fix32> c)
{
    auto &ds = m_ram.draw_state;

    x -= ds.camera.x;
    y -= ds.camera.y;
    uint32_t color_bits = to_color_bits(c);
    // seems to come from https://rosettacode.org/wiki/Bitmap/Midpoint_circle_algorithm#BASIC256
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
        if (err < r - 1)
        {
            err += 1 + 2 * dy;
        }
        else
        {
            dx -= 1;
            err += 1 + 2 * (dy-dx);
        }
    }
}

void vm::api_circfill(int16_t x, int16_t y, int16_t r, opt<fix32> c)
{
    auto &ds = m_ram.draw_state;

    x -= ds.camera.x;
    y -= ds.camera.y;
    uint32_t color_bits = to_color_bits(c);
    // seems to come from https://rosettacode.org/wiki/Bitmap/Midpoint_circle_algorithm#BASIC256
    for (int16_t dx = r, dy = 0, err = 0; dx >= dy; )
    {
        // Some minor overdraw here when dx == 0 or dx == dy, but nothing serious
        hline(x - dx, x + dx, y - dy, color_bits);
        hline(x - dx, x + dx, y + dy, color_bits);
        hline(x - dy, x + dy, y - dx, color_bits);
        hline(x - dy, x + dy, y + dx, color_bits);

        dy += 1;
        if (err < r - 1)
        {
            err += 1 + 2 * dy;
        }
        else
        {
            dx -= 1;
            err += 1 + 2 * (dy - dx);
        }
    }
}

tup<uint8_t, uint8_t, uint8_t, uint8_t> vm::api_clip(int16_t x, int16_t y,
                                                     int16_t w, opt<int16_t> h, bool intersect)
{
    auto &ds = m_ram.draw_state;
    uint8_t x1 = intersect ? ds.clip.x1 : 0;
    uint8_t y1 = intersect ? ds.clip.y1 : 0;
    uint8_t x2 = intersect ? ds.clip.x2 : 128;
    uint8_t y2 = intersect ? ds.clip.y2 : 128;

    // All three arguments are required for the non-default behaviour
    if (h)
    {
        x2 = uint8_t(std::min(int16_t(x2), int16_t(x + w)));
        y2 = uint8_t(std::min(int16_t(y2), int16_t(y + *h)));
        x1 = uint8_t(std::max(int16_t(x1), x));
        y1 = uint8_t(std::max(int16_t(y1), y));
    }

    std::swap(ds.clip.x1, x1);
    std::swap(ds.clip.y1, y1);
    std::swap(ds.clip.x2, x2);
    std::swap(ds.clip.y2, y2);

    return std::make_tuple(x1, y1, x2, y2);
}

void vm::api_cls(uint8_t c)
{
    ::memset(&m_ram.screen, c % 0x10 * 0x11, sizeof(m_ram.screen));

    // Documentation: “Clear the screen and reset the clipping rectangle”.
    auto &ds = m_ram.draw_state;
    ds.cursor.x = ds.cursor.y = 0;
    ds.clip.x1 = ds.clip.y1 = 0;
    ds.clip.x2 = ds.clip.y2 = 128;
    // Undocumented: set cursor to 0,0
    m_ram.draw_state.cursor.x = 0;
    m_ram.draw_state.cursor.y = 0;
}

uint8_t vm::api_color(opt<uint8_t> c)
{
    uint8_t col = c ? *c : 6;
    std::swap(col, m_ram.draw_state.pen);
    return col;
}

fix32 vm::api_fillp(fix32 fillp)
{
    auto &ds = m_ram.draw_state;

    int32_t prev = (ds.fillp[0] << 16)
                 | (ds.fillp[1] << 24)
                 | ((ds.fillp_trans & 1) << 15);
    ds.fillp[0] = fillp.bits() >> 16;
    ds.fillp[1] = fillp.bits() >> 24;
    ds.fillp_trans = (fillp.bits() >> 15) & 1;
    return fix32::frombits(prev);
}

opt<var<int16_t, bool>> vm::api_fget(opt<int16_t> n, opt<int16_t> f)
{
    if (!n)
        return std::nullopt; // return 0 arguments

    int16_t bits = 0;
    if (*n >= 0 && *n < (int)sizeof(m_ram.gfx_flags))
        bits = m_ram.gfx_flags[*n];

    if (f)
        return (bool)(bits & (1 << *f)); // return a boolean

    return bits; // return a number
}

void vm::api_fset(opt<int16_t> n, opt<int16_t> f, opt<bool> b)
{
    if (!n || !f || *n < 0 || *n >= (int16_t)sizeof(m_ram.gfx_flags))
        return;

    uint8_t &data = m_ram.gfx_flags[*n];

    if (!b)
        data = (uint8_t)*f;
    else if (*b)
        data |= 1 << *f;
    else
        data &= ~(1 << *f);
}

void vm::api_line(opt<fix32> arg0, opt<fix32> arg1, opt<fix32> arg2,
                  opt<fix32> arg3, opt<fix32> arg4)
{
    using std::abs, std::min, std::max;

    auto &ds = m_ram.draw_state;

    uint32_t color_bits;

    if (!arg0 || !arg1)
    {
        ds.polyline_flag |= 0x1;
        color_bits = to_color_bits(arg0);
        return;
    }

    int16_t x0, y0, x1, y1;

    if (!arg2 || !arg3)
    {
        // Polyline mode (with 2 or 3 arguments)
        x0 = ds.polyline.x;
        y0 = ds.polyline.y;
        x1 = int16_t(*arg0);
        y1 = int16_t(*arg1);
        color_bits = to_color_bits(arg2);
    }
    else
    {
        // Standard mode
        x0 = int16_t(*arg0);
        y0 = int16_t(*arg1);
        x1 = int16_t(*arg2);
        y1 = int16_t(*arg3);
        color_bits = to_color_bits(arg4);
    }

    // Store polyline state
    ds.polyline.x = x1;
    ds.polyline.y = y1;

    // If this is the polyline bootstrap, do nothing
    if (ds.polyline_flag & 0x1)
    {
        ds.polyline_flag &= ~0x1;
        return;
    }

    x0 -= ds.camera.x; y0 -= ds.camera.y;
    x1 -= ds.camera.x; y1 -= ds.camera.y;

    bool horiz = abs(x1 - x0) >= abs(y1 - y0);
    int16_t dx = x0 <= x1 ? 1 : -1;
    int16_t dy = y0 <= y1 ? 1 : -1;

    // Clamping helps with performance even if pixels are clipped later
    int16_t x = lol::clamp(int(x0), -1, 128);
    int16_t y = lol::clamp(int(y0), -1, 128);
    int16_t xend = lol::clamp(int(x1), -1, 128);
    int16_t yend = lol::clamp(int(y1), -1, 128);

    for (;;)
    {
        set_pixel(x, y, color_bits);

        if (horiz)
        {
            if (x == xend)
                break;
            x += dx;
            y = (int16_t)lol::round(lol::mix((double)y0, (double)y1, (double)(x - x0) / (x1 - x0)));
        }
        else
        {
            if (y == yend)
                break;
            y += dy;
            x = (int16_t)lol::round(lol::mix((double)x0, (double)x1, (double)(y - y0) / (y1 - y0)));
        }
    }
}

void vm::api_tline(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                   fix32 mx, fix32 my, opt<fix32> in_mdx, opt<fix32> in_mdy, int16_t layer)
{
    using std::abs, std::min;

    auto &ds = m_ram.draw_state;

    // mdx, mdy default to 1/8, 0
    fix32 mdx = in_mdx ? *in_mdx : fix32::frombits(0x2000);
    fix32 mdy = in_mdy ? *in_mdy : fix32(0);

    // Retrieve masks for wrap-around and subtract 0x0.0001
    fix32 xmask = fix32(ds.tline.mask.x) - fix32::frombits(1);
    fix32 ymask = fix32(ds.tline.mask.y) - fix32::frombits(1);

    x0 -= ds.camera.x; y0 -= ds.camera.y;
    x1 -= ds.camera.x; y1 -= ds.camera.y;

    bool horiz = abs(x1 - x0) >= abs(y1 - y0);
    int16_t dx = horiz ? x0 <= x1 ? 1 : -1 : 0;
    int16_t dy = horiz ? 0 : y0 <= y1 ? 1 : -1;

    // Clamping helps with performance even if pixels are clipped later
    int16_t x = lol::clamp(int(x0), -1, 128);
    int16_t y = lol::clamp(int(y0), -1, 128);
    int16_t xend = lol::clamp(int(x1), -1, 128);
    int16_t yend = lol::clamp(int(y1), -1, 128);

    // Advance texture coordinates; do it in steps to avoid overflows
    int16_t delta = abs(horiz ? x - x0 : y - y0);
    while (delta)
    {
        int16_t step = min(int16_t(8192), delta);
        mx = (mx & ~xmask) | ((mx + mdx * fix32(step)) & xmask);
        my = (my & ~ymask) | ((my + mdy * fix32(step)) & ymask);
        delta -= step;
    }

    for (;;)
    {
        // Find sprite in map memory
        int sx = (ds.tline.offset.x + int(mx)) & 0x7f;
        int sy = (ds.tline.offset.y + int(my)) & 0x3f;
        uint8_t sprite = m_ram.map[128 * sy + sx];
        uint8_t bits = m_ram.gfx_flags[sprite];

        // If found, draw pixel
        if ((sprite || ds.sprite_zero == 0x8) && (!layer || (bits & layer)))
        {
            int col = m_ram.gfx.get(sprite % 16 * 8 + (int(mx << 3) & 0x7),
                                    sprite / 16 * 8 + (int(my << 3) & 0x7));
            if ((ds.draw_palette[col] & 0xf0) == 0)
            {
                uint32_t color_bits = (ds.draw_palette[col] & 0xf) << 16;
                set_pixel(x, y, color_bits);
            }
        }

        // Advance source coordinates
        mx = (mx & ~xmask) | ((mx + mdx) & xmask);
        my = (my & ~ymask) | ((my + mdy) & ymask);

        // Advance destination coordinates
        if (horiz)
        {
            if (x == xend)
                break;
            x += dx;
            y = (int16_t)lol::round(lol::mix((double)y0, (double)y1, (double)(x - x0) / (x1 - x0)));
        }
        else
        {
            if (y == yend)
                break;
            y += dy;
            x = (int16_t)lol::round(lol::mix((double)x0, (double)x1, (double)(y - y0) / (y1 - y0)));
        }
    }
}

// Tested on PICO-8 1.1.12c: fractional part of all arguments is ignored.
void vm::api_map(int16_t cel_x, int16_t cel_y, int16_t sx, int16_t sy,
                 opt<int16_t> in_cel_w, opt<int16_t> in_cel_h, int16_t layer)
{
    auto &ds = m_ram.draw_state;

    sx -= ds.camera.x;
    sy -= ds.camera.y;

    // PICO-8 documentation: “If cel_w and cel_h are not specified,
    // defaults to 128,32”.
    bool no_size = !in_cel_w && !in_cel_h;
    int16_t src_w = (no_size ? 128 : *in_cel_w) * 8;
    int16_t src_h = (no_size ? 32 : *in_cel_h) * 8;
    int16_t src_x = cel_x * 8;
    int16_t src_y = cel_y * 8;

    // Clamp to screen
    int16_t mx = std::max(-sx, 0);
    int16_t my = std::max(-sy, 0);
    src_x += mx;
    src_y += my;
    src_w -= mx + std::max(src_w + sx - 128, 0);
    src_h -= my + std::max(src_h + sy - 128, 0);
    sx += mx;
    sy += my;

    for (int16_t dy = 0; dy < src_h; ++dy)
    for (int16_t dx = 0; dx < src_w; ++dx)
    {
        int16_t cx = (src_x + dx) / 8;
        int16_t cy = (src_y + dy) / 8;
        if (cx < 0 || cx >= 128 || cy < 0 || cy >= 64)
            continue;

        uint8_t sprite = m_ram.map[128 * cy + cx];
        uint8_t bits = m_ram.gfx_flags[sprite];
        if (layer && !(bits & layer))
            continue;

        if (sprite || ds.sprite_zero == 0x8)
        {
            int col = m_ram.gfx.get(sprite % 16 * 8 + (src_x + dx) % 8,
                                    sprite / 16 * 8 + (src_y + dy) % 8);
            if ((ds.draw_palette[col] & 0xf0) == 0)
            {
                uint32_t color_bits = (ds.draw_palette[col] & 0xf) << 16;
                set_pixel(sx + dx, sy + dy, color_bits);
            }
        }
    }
}

fix32 vm::api_mget(int16_t x, int16_t y)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 64)
        return 0;

    return m_ram.map[128 * y + x];
}

void vm::api_mset(int16_t x, int16_t y, uint8_t n)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 64)
        return;

    m_ram.map[128 * y + x] = n;
}

void vm::api_oval(int16_t x0, int16_t y0, int16_t x1, int16_t y1, opt<fix32> c)
{
    auto &ds = m_ram.draw_state;

    x0 -= ds.camera.x;
    y0 -= ds.camera.y;
    x1 -= ds.camera.x;
    y1 -= ds.camera.y;
    uint32_t color_bits = to_color_bits(c);

    if (x0 > x1)
        std::swap(x0, x1);

    if (y0 > y1)
        std::swap(y0, y1);

    // FIXME: not elegant at all
    float xc = float(x0 + x1) / 2;
    float yc = float(y0 + y1) / 2;

    auto plot = [&](int16_t x, int16_t y)
    {
        set_pixel(x, y, color_bits);
        set_pixel(int16_t(2 * xc) - x, y, color_bits);
        set_pixel(x, int16_t(2 * yc) - y, color_bits);
        set_pixel(int16_t(2 * xc) - x, int16_t(2 * yc) - y, color_bits);
    };

    // Cutoff for slope = 0.5 happens at x = a²/sqrt(a²+b²)
    float a = float(x1 - x0) / 2;
    float b = float(y1 - y0) / 2;
    float cutoff = a / sqrt(1 + b * b / (a * a));

    for (float dx = 0; dx <= cutoff; ++dx)
    {
        int16_t x = int16_t(ceil(xc + dx));
        int16_t y = int16_t(round(yc - b / a * sqrt(a * a - dx * dx)));
        plot(x, y);
    }

    cutoff = b / sqrt(1 + a * a / (b * b));
    for (float dy = 0; dy < cutoff; ++dy)
    {
        int16_t y = int16_t(ceil(yc + dy));
        int16_t x = int16_t(round(xc - a / b * sqrt(b * b - dy * dy)));
        plot(x, y);
    }
}

void vm::api_ovalfill(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                      opt<fix32> c)
{
    auto &ds = m_ram.draw_state;

    x0 -= ds.camera.x;
    y0 -= ds.camera.y;
    x1 -= ds.camera.x;
    y1 -= ds.camera.y;
    uint32_t color_bits = to_color_bits(c);

    if (y0 > y1)
        std::swap(y0, y1);

    if (y0 > y1)
        std::swap(y0, y1);

    // FIXME: not elegant at all
    float xc = float(x0 + x1) / 2;
    float yc = float(y0 + y1) / 2;

    float a = float(x1 - x0) / 2;
    float b = float(y1 - y0) / 2;
    for (int16_t y = int16_t(ceil(yc)); y <= y1; ++y)
    {
        float dy = y - yc;
        int16_t x = int16_t(round(xc - a / b * sqrt(b * b - dy * dy)));
        hline(int16_t(2 * xc) - x, x, y, color_bits);
        hline(int16_t(2 * xc) - x, x, int16_t(2 * yc) - y, color_bits);
    }
}

opt<uint8_t> vm::api_pal(opt<uint8_t> c0, opt<uint8_t> c1, uint8_t p)
{
    auto &ds = m_ram.draw_state;
    auto &hw = m_ram.hw_state;

    if (!c0 || !c1)
    {
        // PICO-8 documentation: “pal() to reset to system defaults (including
        // transparency values and fill pattern)”
        for (int i = 0; i < 16; ++i)
        {
            ds.draw_palette[i] = i | (i ? 0x00 : 0x10);
            ds.screen_palette[i] = i;
            hw.raster.palette[i] = 0;
        }
        ds.fillp[0] = 0;
        ds.fillp[1] = 0;
        ds.fillp_trans = 0;
        return std::nullopt;
    }
    else
    {
        uint8_t &data = (p == 2) ? hw.raster.palette[*c0 & 0xf]
                      : (p == 1) ? ds.screen_palette[*c0 & 0xf]
                                 : ds.draw_palette[*c0 & 0xf];
        uint8_t prev = data;

        if (p == 1 || p == 2)
            data = *c1 & 0xff;
        else
            data = (data & 0x10) | (*c1 & 0xf); // Transparency bit is preserved

        return prev;
    }
}

var<int16_t, bool> vm::api_palt(opt<int16_t> c, opt<bool> t)
{
    auto &ds = m_ram.draw_state;

    if (!t)
    {
        int16_t prev = 0;
        for (int i = 0; i < 16; ++i)
        {
            prev |= ((ds.draw_palette[i] & 0x10) >> 4) << (15 - i);
            ds.draw_palette[i] &= 0xf;
            // If c is set, set transparency according to the (15 - i)th bit.
            // Otherwise, only color 0 is transparent.
            ds.draw_palette[i] |= (c ? *c & (1 << (15 - i)) : i == 0) ? 0x10 : 0x00;
        }
        return prev;
    }
    else
    {
        uint8_t &data = ds.draw_palette[*c & 0xf];
        bool prev = data & 0x10;
        data = (data & 0xf) | (*t ? 0x10 : 0x00);
        return prev;
    }
}

fix32 vm::api_pget(int16_t x, int16_t y)
{
    auto &ds = m_ram.draw_state;

    /* pget() is affected by camera() and by clip() */
    // FIXME: "and by clip()"? wut?
    x -= ds.camera.x;
    y -= ds.camera.y;
    return get_pixel(x, y);
}

void vm::api_pset(int16_t x, int16_t y, opt<fix32> c)
{
    auto &ds = m_ram.draw_state;

    x -= ds.camera.x;
    y -= ds.camera.y;
    uint32_t color_bits = to_color_bits(c);
    set_pixel(x, y, color_bits);
}

void vm::api_rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, opt<fix32> c)
{
    auto &ds = m_ram.draw_state;

    x0 -= ds.camera.x;
    y0 -= ds.camera.y;
    x1 -= ds.camera.x;
    y1 -= ds.camera.y;
    uint32_t color_bits = to_color_bits(c);

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
}

void vm::api_rectfill(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                      opt<fix32> c)
{
    auto &ds = m_ram.draw_state;

    x0 -= ds.camera.x;
    y0 -= ds.camera.y;
    x1 -= ds.camera.x;
    y1 -= ds.camera.y;
    uint32_t color_bits = to_color_bits(c);

    if (y0 > y1)
        std::swap(y0, y1);

    for (int16_t y = y0; y <= y1; ++y)
        hline(x0, x1, y, color_bits);
}

int16_t vm::api_sget(int16_t x, int16_t y)
{
    return m_ram.gfx.safe_get(x, y);
}

void vm::api_sset(int16_t x, int16_t y, opt<int16_t> c)
{
    auto &ds = m_ram.draw_state;

    uint8_t col = c ? (uint8_t)*c : ds.pen;
    m_ram.gfx.safe_set(x, y, ds.draw_palette[col & 0xf]);
}

void vm::api_spr(int16_t n, int16_t x, int16_t y, opt<fix32> w,
                 opt<fix32> h, bool flip_x, bool flip_y)
{
    auto &ds = m_ram.draw_state;

    x -= ds.camera.x;
    y -= ds.camera.y;

    int16_t w8 = w ? (int16_t)(*w * fix32(8.0)) : 8;
    int16_t h8 = h ? (int16_t)(*h * fix32(8.0)) : 8;

    for (int16_t j = 0; j < h8; ++j)
        for (int16_t i = 0; i < w8; ++i)
        {
            int16_t di = flip_x ? w8 - 1 - i : i;
            int16_t dj = flip_y ? h8 - 1 - j : j;
            uint8_t col = m_ram.gfx.safe_get(n % 16 * 8 + di, n / 16 * 8 + dj);
            if ((ds.draw_palette[col] & 0xf0) == 0)
            {
                uint32_t color_bits = (ds.draw_palette[col] & 0xf) << 16;
                set_pixel(x + i, y + j, color_bits);
            }
        }
}

void vm::api_sspr(int16_t sx, int16_t sy, int16_t sw, int16_t sh,
                  int16_t dx, int16_t dy, opt<int16_t> in_dw,
                  opt<int16_t> in_dh, bool flip_x, bool flip_y)
{
    auto &ds = m_ram.draw_state;

    dx -= ds.camera.x;
    dy -= ds.camera.y;
    int16_t dw = in_dw ? *in_dw : sw;
    int16_t dh = in_dh ? *in_dh : sh;

    // Support negative dw and dh by flipping the target rectangle
    if (dw < 0) { dw = -dw; dx -= dw - 1; flip_x = !flip_x; }
    if (dh < 0) { dh = -dh; dy -= dh - 1; flip_y = !flip_y; }

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

        uint8_t col = m_ram.gfx.safe_get(x, y);
        if ((ds.draw_palette[col] & 0xf0) == 0)
        {
            uint32_t color_bits = (ds.draw_palette[col] & 0xf) << 16;
            set_pixel(dx + i, dy + j, color_bits);
        }
    }
}

} // namespace z8::pico8

