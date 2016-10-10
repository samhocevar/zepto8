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

#include <lol/engine.h>

#include "vm.h"

namespace z8
{

using lol::msg;

void vm::getaudio(int channel, void *buffer, int bytes)
{
    UNUSED(channel);
    memset(buffer, 0, bytes);
}

//
// Sound
//

int vm::api::music(lua_State *l)
{
    UNUSED(l);
    msg::info("z8:stub:music\n");
    return 0;
}

int vm::api::sfx(lua_State *l)
{
    UNUSED(l);
    msg::info("z8:stub:sfx\n");
    return 0;
}

} // namespace z8

