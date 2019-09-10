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

#include "zepto8.h"
#include "raccoon/vm.h"
#include "raccoon/font.h"

namespace z8::raccoon
{

template<typename T> static void setpixel(T &data, int x, int y, uint8_t c)
{
    uint8_t &p = data[y][x / 2];
    p = (p & (x & 1 ? 0x0f : 0xf0)) | (x & 1 ? c << 4 : c);
}

template<typename T> static uint8_t getpixel(T const &data, int x, int y)
{
    uint8_t const &p = data[y][x / 2];
    return (x & 1 ? p >> 4 : p) & 0xf;
}

int vm::api_read(int p)
{
    return m_ram[p & 0xffff];
}

void vm::api_write(int p, int x)
{
    m_ram[p & 0xffff] = x;
}

void vm::api_palset(int n, int r, int g, int b)
{
    m_ram.palette[n & 0xf] = lol::u8vec3(r, g, b);
}

void vm::api_pset(int x, int y, int c)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 64)
        return;
    setpixel(m_ram.screen, x, y, c & 15);
}

void vm::api_palm(int c0, int c1)
{
    uint8_t &data = m_ram.palmod[c0 & 0xf];
    data = (data & 0xf0) | (c1 & 0xf);
}

void vm::api_palt(int c, int v)
{
    uint8_t &data = m_ram.palmod[c & 0xf];
    data = (data & 0x7f) | (v ? 0x80 : 0x00);
}

bool vm::api_btn(int i, std::optional<int> p)
{
    int bit = 1 << (i + 8 * (p.has_value() ? p.value() : 0));
    return m_ram.gamepad[0] & bit;
}

bool vm::api_btnp(int i, std::optional<int> p)
{
    int bit = 1 << (i + 8 * (p.has_value() ? p.value() : 0));
    return (m_ram.gamepad[0] & bit) && !(m_ram.gamepad[1] & bit);
}

int vm::api_fget(int n, std::optional<int> f)
{
    if (n < 0 || n >= 192)
        return 0;
    uint8_t field = m_ram.flags[n];
    if (!f.has_value())
        return field;
    return (field >> f.value()) & 0x1;
}

void vm::api_fset(int n, int f, std::optional<int> v)
{
    if (n < 0 || n >= 192)
        return;
    if (v.has_value())
    {
        uint8_t mask = 1 << f;
        f = (m_ram.flags[n] & ~mask) | (v.value() ? mask : 0);
    }
    m_ram.flags[n] = f;
}

void vm::api_cls(std::optional<int> c)
{
    memset(m_ram.screen, (c.value() & 15) * 0x11, sizeof(m_ram.screen));
}

void vm::api_cam(int x, int y)
{
    m_ram.camera.x = (int16_t)x;
    m_ram.camera.y = (int16_t)y;
}

void vm::api_map(int celx, int cely, int sx, int sy, int celw, int celh)
{
    sx -= m_ram.camera.x;
    sy -= m_ram.camera.y;
    for (int y = 0; y < celh; ++y)
    for (int x = 0; x < celw; ++x)
    {
        if (celx + x < 0 || celx + x >= 128 || cely + y < 0 || cely + y >= 64)
            continue;
        int n = m_ram.map[cely + y][celx + x];
        int startx = sx + x * 8, starty = sy + y * 8;
        int sprx = n % 16 * 8, spry = n / 16 * 8;

        for (int dy = 0; dy < 8; ++dy)
        for (int dx = 0; dx < 8; ++dx)
        {
            if (startx + dx < 0 || startx + dx >= 128 ||
                starty + dy < 0 || starty + dy >= 128)
                continue;
            if (sprx + dx < 0 || sprx + dx >= 128 ||
                spry + dy < 0 || spry + dy >= 96)
                continue;
            auto c = getpixel(m_ram.sprites, sprx + dx, spry + dy);
            if (m_ram.palmod[c] & 0x80)
                continue;
            c = m_ram.palmod[c] & 0xf;
            setpixel(m_ram.screen, startx + dx, starty + dy, c);
        }
    }
}

void vm::api_rect(int x, int y, int w, int h, int c)
{
    lol::msg::info("stub: rect(%d, %d, %d, %d, %d)\n", x, y, w, h, c);
}

void vm::api_rectfill(int x, int y, int w, int h, int c)
{
    x -= m_ram.camera.x;
    y -= m_ram.camera.y;
    int x0 = std::max(x, 0);
    int x1 = std::min(x + w, 127);
    int y0 = std::max(y, 0);
    int y1 = std::min(y + h, 127);
    for (y = y0; y <= y1; ++y)
        for (x = x0; x <= x1; ++x)
            setpixel(m_ram.screen, x, y, c & 15);
}

void vm::api_spr(int n, int x, int y,
                 std::optional<double> w, std::optional<double> h,
                 std::optional<int> fx, std::optional<int> fy)
{
    x -= m_ram.camera.x;
    y -= m_ram.camera.y;
    int sx = n % 16 * 8, sy = n / 16 * 8;
    int sw = w.has_value() ? (int)(w.value() * 8) : 8;
    int sh = h.has_value() ? (int)(h.value() * 8) : 8;
    int flip_x = fx.has_value() && fx.value();
    int flip_y = fy.has_value() && fy.value();

    for (int dy = 0; dy < sh; ++dy)
        for (int dx = 0; dx < sw; ++dx)
        {
            if (x + dx < 0 || x + dx >= 128 || y + dy < 0 || y + dy >= 128)
                continue;
            auto c = getpixel(m_ram.sprites,
                              flip_x ? sx + sw - 1 - dx : sx + dx,
                              flip_y ? sy + sh - 1 - dy : sy + dy);
            if (m_ram.palmod[c] & 0x80)
                continue;
            c = m_ram.palmod[c] & 0xf;
            setpixel(m_ram.screen, x + dx, y + dy, c);
        }
}

void vm::api_print(int x, int y, std::string str, int c)
{
    x -= m_ram.camera.x;
    y -= m_ram.camera.y;
    c &= 15;
    for (uint8_t ch : str)
    {
        if (ch < 0x20 || ch >= 0x80)
            continue;
        uint32_t data = font_data[ch - 0x20];

        if (ch == ',')
            x -= 1;

        for (int dx = 0; dx < 3; ++dx)
        for (int dy = 0; dy < 7; ++dy)
            if (data & (1 << (dx * 8 + dy)))
                setpixel(m_ram.screen, x + dx, y + dy, c);
        if (data & 0xff0000)
            x += 4;
        else if (data & 0xff00)
            x += 3;
        else if (data & 0xff)
            x += 2;
        else if (ch == ' ')
            x += 4;
    }
}

double vm::api_rnd(std::optional<double> x)
{
    return x.has_value() ? std::floor(lol::rand(x.value()))
                         : lol::rand(1.0);
}

double vm::api_mid(double x, double y, double z)
{
    return x > y ? y > z ? y : std::min(x, z)
                 : x > z ? x : std::min(y, z);
}

int vm::api_mget(int x, int y)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 64)
        return 0;
    return m_ram.map[y][x];
}

void vm::api_mset(int x, int y, int n)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 64)
        return;
    m_ram.map[y][x] = n;
}

void vm::api_mus(int n)
{
    lol::msg::info("stub: mus(%d)\n", n);
}

void vm::api_sfx(int a, int b, int c, int d)
{
    lol::msg::info("stub: sfx(%d, %d, %d, %d)\n", a, b, c, d);
}

}

