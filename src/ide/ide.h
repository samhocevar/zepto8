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
#include "ide/text-editor.h"
#include "ide/memory-editor.h"

namespace z8
{

class ide : public lol::WorldEntity
{
public:
    ide();
    virtual ~ide();

    virtual bool init_draw() override;
    virtual void tick_game(float seconds) override;
    virtual void tick_draw(float seconds, lol::Scene &scene) override;
    virtual bool release_draw() override;

    void load(std::string const &name);

private:
    void apply_scale();

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

    int m_scale = 2;
    player *m_player; // FIXME: this reference should disappear because player is a lol::entity
    std::unique_ptr<text_editor> m_text_editor;
    std::unique_ptr<memory_editor> m_ram_editor, m_rom_editor;

    std::shared_ptr<lol::Texture> m_screen, m_sprites;

    std::shared_ptr<vm_base> m_vm;
    std::map<int, ImFont *> m_fonts;
};

} // namespace z8

