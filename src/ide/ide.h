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
#include "player.h"
#include "ide/editor.h"
#include "3rdparty/imgui-club/imgui_memory_editor/imgui_memory_editor.h"

namespace z8
{

class ide : public lol::WorldEntity
{
public:
    ide(player *player);
    virtual ~ide();

    virtual void tick_game(float seconds) override;
    virtual void tick_draw(float seconds, lol::Scene &scene) override;

private:
    void render_app();
    void render_menu();
    void render_toolbar();
    void render_windows();

    bool m_commands[5] = { false };

    struct
    {
        bool player = true;
        bool code = true;
        bool maps = false;
        bool music = false;
        bool sprites = false;
        bool ram = true;
        bool rom = true;
        bool palette = true;
    }
    m_show;

    struct
    {
        ImGuiID root;
        ImGuiID main;
        ImGuiID bottom, bottom_left, bottom_right;
    }
    m_dock;

    editor m_editor;
    MemoryEditor m_ram_edit, m_rom_edit;

    player *m_player = nullptr;
    ImFont *m_font = nullptr;
};

} // namespace z8

