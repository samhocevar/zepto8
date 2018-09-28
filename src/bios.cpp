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

#include <lol/engine.h>

#include "bios.h"

namespace z8
{

bios::bios()
{
    // Initialize BIOS
    for (auto const &file : lol::sys::get_path_list("bios.p8"))
    {
        lol::File f;
        f.Open(file, lol::FileAccess::Read);
        bool exists = f.IsValid();
        f.Close();

        if (exists && m_cart.load(file.c_str()))
            break;
    }
}

} // namespace z8

