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
    class code_fixer
    {
    public:
        code_fixer(lol::String const &code);
        lol::String fix();

        lol::array<lol::ivec2> m_notequals;
        lol::array<lol::ivec3> m_reassignments;

    private:
        lol::String m_code;
    };
}

