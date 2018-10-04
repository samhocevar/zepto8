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

    io.Fonts->AddFontFromFileTTF("pico8.ttf", 15);
    auto &style = ImGui::GetStyle();
    style.WindowBorderSize = 3.0f;
    style.ChildBorderSize = 3.0f;
    style.FrameBorderSize = 3.0f;
    style.TabBorderSize = 3.0f;
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;

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
    ImGui::PushFont(io.Fonts->Fonts[0]);

    render_dock();
//    ImGui::ShowDemoWindow();

    m_editor.render();

    ImGui::Begin("Palette");
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

    ImGui::Begin("Console");
    {
        ImGui::Image(m_player->get_texture(), 3.f * lol::vec2(128.f),
                     lol::vec2(0.f), lol::vec2(1.f));
    }
    ImGui::End();

    ImGui::Begin("Music");
    {
        ImGui::TextColored((lol::vec4)z8::palette::get(10) / 255.f, "stuff");
        ImGui::TextColored((lol::vec4)z8::palette::get(5) / 255.f, "more stuff\nlol!!!");
    }
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
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("New", nullptr, false, false);
            ImGui::MenuItem("Open", nullptr, false, false);
            ImGui::Separator();
            ImGui::MenuItem("Exit", nullptr, false, false);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
            ImGui::EndMenu();

        if (ImGui::BeginMenu("View"))
            ImGui::EndMenu();

        if (ImGui::BeginMenu("Help"))
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

