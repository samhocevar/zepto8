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

#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <lol/engine.h>

#include "zepto8.h"
#include "ide/ide.h"

namespace z8
{

ide::ide(player *player)
{
    lol::LolImGui::Init();

    // Enable docking
    auto &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.Fonts->AddFontFromFileTTF("pico8.ttf", 5 * EDITOR_SCALE);

    auto &style = ImGui::GetStyle();
    style.WindowBorderSize = style.ChildBorderSize = style.PopupBorderSize = style.FrameBorderSize = style.TabBorderSize = EDITOR_SCALE;
    style.FramePadding = lol::vec2(2 * EDITOR_SCALE);
    style.WindowRounding = style.ChildRounding = style.FrameRounding = style.ScrollbarRounding = style.TabRounding = 0.0f;

    m_player = player;
}

ide::~ide()
{
    lol::LolImGui::Shutdown();
}

void ide::tick_game(float seconds)
{
    WorldEntity::tick_game(seconds);

    auto &io = ImGui::GetIO();
    io.Fonts->Fonts[0]->FontSize = 6 * EDITOR_SCALE;
    ImGui::PushFont(io.Fonts->Fonts[0]);

    render_dock();
//    ImGui::ShowDemoWindow();

    m_editor.render();

    ImGui::Begin("pALETTE", nullptr);
    {
        for (int i = 0; i < 16; i++)
        {
            if (i % 4 > 0)
                ImGui::SameLine();
            ImGui::PushID(i);
            ImGui::PushStyleColor(ImGuiCol_Button, (lol::vec4)z8::palette::get(i) / 255.f);
            ImGui::PushStyleColor(ImGuiCol_Text, (lol::vec4)z8::palette::get(i < 6 ? 7 : 0) / 255.f);
            ImGui::Button(lol::format("%2d", i).c_str());
            ImGui::PopStyleColor(2);
            ImGui::PopID();
        }
    }
    ImGui::End();

    ImGui::Begin("cONSOLE", nullptr);
    {
        ImGui::Image(m_player->get_texture(), 3.f * lol::vec2(128.f),
                     lol::vec2(0.f), lol::vec2(1.f));
    }
    ImGui::End();

    ImGui::Begin("mUSIC", nullptr);
    {
        ImGui::TextColored((lol::vec4)z8::palette::get(10) / 255.f, "stuff");
        ImGui::TextColored((lol::vec4)z8::palette::get(5) / 255.f, "more stuff\nlol!!!");
    }
    ImGui::End();

    ImGui::Begin("sPRITES", nullptr);
    ImGui::End();

    ImGui::Begin("mAPS", nullptr);
    ImGui::End();

    ImGui::Begin("ram", nullptr);
    ImGui::End();

    ImGui::Begin("rom", nullptr);
    ImGui::End();

    ImGui::PopFont();
}

void ide::render_dock()
{
    // Create a fullscreen window for the docking space
    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar
                           | ImGuiWindowFlags_NoDocking
                           | ImGuiWindowFlags_NoTitleBar
                           | ImGuiWindowFlags_NoCollapse
                           | ImGuiWindowFlags_NoResize
                           | ImGuiWindowFlags_NoMove
                           | ImGuiWindowFlags_NoBringToFrontOnFocus
                           | ImGuiWindowFlags_NoNavFocus;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, lol::vec2(0));
    ImGui::Begin("ZEPTO-8 IDE", nullptr, flags);
    ImGui::PopStyleVar();

    // Create the actual dock space
    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    ImGui::DockSpace(dockspace_id, lol::vec2(0), dockspace_flags);

    // The main menu bar
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("fILE"))
        {
            ImGui::MenuItem("nEW", nullptr, false, false);
            ImGui::MenuItem("oPEN", nullptr, false, false);
            ImGui::Separator();
            ImGui::MenuItem("eXIT", nullptr, false, false);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("eDIT"))
            ImGui::EndMenu();

        if (ImGui::BeginMenu("vIEW"))
            ImGui::EndMenu();

        if (ImGui::BeginMenu("hELP"))
            ImGui::EndMenu();

        ImGui::EndMenuBar();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void ide::tick_draw(float seconds, lol::Scene &scene)
{
    WorldEntity::tick_draw(seconds, scene);
}

} // namespace z8

