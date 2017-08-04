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
    class analyzer
    {
    public:
        analyzer(lol::String const &code);
        lol::String fix();

        lol::array<int> m_notequals;
        lol::array<int> m_cpp_comments;

        lol::array<int> m_reassign_ops;
        lol::array<lol::ivec3> m_reassigns;

        int m_disable_crlf = 0;
        lol::array<int> m_if_dos;
        lol::array<lol::ivec2> m_short_ifs;
        lol::array<lol::ivec2> m_short_prints;

    private:
        void bump(int offset, int delta);

        lol::String m_code;
    };
}

