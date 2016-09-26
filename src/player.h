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

#include "zepto8.h"
#include "cart.h"
#include "vm.h"

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

    void load(char const *name) { m_vm.load(name); }
    void run() { m_vm.run(); }

private:
    vm m_vm;
    array<u8vec4> m_screen;

    lol::Camera *m_scenecam;
    lol::TileSet *m_tile;
    lol::Controller *m_controller;
    lol::InputProfile m_input;
};

} // namespace z8

