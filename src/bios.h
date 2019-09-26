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

#pragma once

#include <lol/engine.h>

#include "pico8/cart.h"

// The bios class
// ——————————————
// The actual ZEPTO-8 BIOS: contains the font and the startup code, loaded
// from a regular .p8 cartridge file.

namespace z8::pico8
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

        return m_cart.get_rom().gfx.get(x, y);
    }

private:
    cart m_cart;
};

} // namespace z8

