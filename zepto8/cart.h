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

#pragma once

#include <lol/engine.h>

namespace z8
{

class cart
{
public:
    cart()
      : m_version(0)
    {}

    lol::array<uint8_t> const &get_data() const
    {
        return m_data;
    }

    lol::String const &get_code() const
    {
        return m_code;
    }

    int get_version() const
    {
        return m_version;
    }

    void load_png(char const *filename);
    void load_p8(char const *filename);

private:
    lol::array<uint8_t> m_data;
    lol::String m_code;
    int m_version;
};

} // namespace z8

