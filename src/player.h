//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2019 Sam Hocevar <sam@hocevar.net>
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

// The player class
// ————————————————
// This is a high-level Lol Engine entity that runs the ZEPTO-8 VM.

namespace z8
{

using lol::array;
using lol::ivec2;
using lol::u8vec4;

class player : public lol::WorldEntity
{
public:
    player(ivec2 window_size, bool is_raccoon = false);
    virtual ~player();

    virtual void tick_game(float seconds) override;
    virtual void tick_draw(float seconds, lol::Scene &scene) override;

    void load(char const *name);
    void run();

    // HACK: if get_texture() is called, rendering is disabled
    lol::Texture *get_texture();
    lol::Texture *get_font_texture();

#if 0 // FIXME: PICO-8 specific
    uint8_t *get_ram() { return (uint8_t *)&m_vm.get_ram(); }
    uint8_t *get_rom() { return (uint8_t *)&m_vm.m_cart.get_rom(); }
#endif

private:
    std::shared_ptr<vm_base> m_vm;

    std::map<lol::input::key, int> m_input_map;
    array<u8vec4> m_screen;
    bool m_render = true;
    int m_streams[4];

    lol::Camera *m_scenecam;
    lol::TileSet *m_tile, *m_font_tile;
};

} // namespace z8

