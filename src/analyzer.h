//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2020 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#pragma once

#include <string>

// The analyzer class
// ——————————————————
// This class used to parse and rewrite the PICO-8 code and transcribe it to
// regular Lua code. Now that we use z8lua instead of Lua, this is no longer
// required, and the fix() function just adds some backwards compatibility
// glue code to the source.

namespace z8
{

class analyzer
{
public:
    std::string fix(std::string const &str);

    int m_disable_crlf = 0;
};

}

