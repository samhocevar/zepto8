//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2018 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#pragma once

#include <lol/engine.h>

#include "cart.h"

// The bios class
// ——————————————
// The actual ZEPTO-8 BIOS: contains the font and the startup code, loaded
// from a regular .p8 cartridge file.

namespace z8
{

class bios
{
public:
    bios();

    std::string const &get_code() const
    {
        return m_cart.get_code();
    }

    uint8_t get_spixel(int16_t x, int16_t y) const
    {
        if (x < 0 || x >= 128 || y < 0 || y >= 128)
            return 0;

        auto const &gfx = m_cart.get_rom().gfx;
        int offset = (128 * y + x) / 2;
        return (x & 1) ? gfx[offset] >> 4 : gfx[offset] & 0xf;
    }

private:
    cart m_cart;
};

} // namespace z8

