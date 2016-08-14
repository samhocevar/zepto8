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

enum
{
    OFFSET_GFX        = 0x0000,
    OFFSET_GFX2       = 0x1000,
    OFFSET_MAP2       = 0x1000,
    OFFSET_MAP        = 0x2000,
    OFFSET_GFX_PROPS  = 0x3000,
    OFFSET_SONG       = 0x3100,
    OFFSET_SFX        = 0x3200,
    OFFSET_USER_DATA  = 0x4300,
    OFFSET_CODE       = 0x4300,
    OFFSET_PERSISTENT = 0x5e00,
    OFFSET_DRAW_STATE = 0x5f00,
    OFFSET_HW_STATE   = 0x5f40,
    OFFSET_GPIO_PINS  = 0x5f80,
    OFFSET_SCREEN     = 0x6000,
    OFFSET_VERSION    = 0x8000,
};

}

