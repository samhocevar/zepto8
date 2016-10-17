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

#define DEBUG_EXPORT_WAV 0

namespace z8
{

enum
{
    WINDOW_WIDTH = 576,
    WINDOW_HEIGHT = 576,
};

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
    OFFSET_END        = 0x8000,

    SIZE_GFX        = OFFSET_GFX2       - OFFSET_GFX,
    SIZE_GFX2       = OFFSET_MAP        - OFFSET_GFX2,
    SIZE_MAP2       = SIZE_GFX2,
    SIZE_MAP        = OFFSET_GFX_PROPS  - OFFSET_MAP,
    SIZE_GFX_PROPS  = OFFSET_SONG       - OFFSET_GFX_PROPS,
    SIZE_SONG       = OFFSET_SFX        - OFFSET_SONG,
    SIZE_SFX        = OFFSET_USER_DATA  - OFFSET_SFX,
    SIZE_USER_DATA  = OFFSET_PERSISTENT - OFFSET_USER_DATA,
    SIZE_PERSISTENT = OFFSET_DRAW_STATE - OFFSET_PERSISTENT,
    SIZE_DRAW_STATE = OFFSET_HW_STATE   - OFFSET_DRAW_STATE,
    SIZE_HW_STATE   = OFFSET_GPIO_PINS  - OFFSET_HW_STATE,
    SIZE_GPIO_PINS  = OFFSET_SCREEN     - OFFSET_GPIO_PINS,
    SIZE_SCREEN     = OFFSET_END        - OFFSET_SCREEN,

    SIZE_MEMORY     = OFFSET_END,
};

}

