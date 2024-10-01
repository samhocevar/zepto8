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

#include "zepto8.h"

namespace z8
{

//
// Biquad filter
//

class filter
{
public:
    enum class type
    {
        lpf, hpf, lowshelf, highshelf
    };

    filter(type t, float freq, float q, float gain);

    void init(type t, float freq, float q, float gain);
    float run(float input);

    float c1, c2, c3, c4, c5;
    float linput = 0;
    float llinput = 0;
    float loutput = 0;
    float lloutput = 0;
};

} // namespace z8

