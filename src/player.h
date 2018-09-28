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

#pragma once

#include <lol/engine.h>

#include "zepto8.h"
#include "cart.h"
#include "vm/vm.h"

// The player class
// ————————————————
// This is a high-level Lol Engine entity that runs the ZEPTO-8 VM.

namespace z8
{

using lol::array;
using lol::u8vec4;

class player : public lol::WorldEntity
{
public:
    player();
    virtual ~player();

    virtual void TickGame(float seconds);
    virtual void TickDraw(float seconds, lol::Scene &scene);

    void load(char const *name);
    void run();

private:
    vm m_vm;
    array<u8vec4> m_screen;
    int m_streams[4];

    lol::Camera *m_scenecam;
    lol::TileSet *m_tile;
    lol::Controller *m_controller;
    lol::InputProfile m_input;
    lol::InputDevice *m_mouse;
};

} // namespace z8

