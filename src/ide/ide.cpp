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
#include "3rdparty/portable-file-dialogs/portable-file-dialogs.h"

#define CUSTOM_FONT 0

namespace z8
{

ide::ide(player *player)
{
    lol::LolImGui::Init();

    // Enable docking
    auto &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    auto &style = ImGui::GetStyle();
    style.WindowBorderSize = style.ChildBorderSize = style.PopupBorderSize = style.FrameBorderSize = style.TabBorderSize = EDITOR_SCALE;
    style.FramePadding = lol::vec2(2 * EDITOR_SCALE);
    style.WindowRounding = style.ChildRounding = style.FrameRounding = style.ScrollbarRounding = style.TabRounding = 0.0f;

    // Useless
    style.Colors[ImGuiCol_ChildBg] = z8::palette::get(5);

    style.Colors[ImGuiCol_Tab]          = z8::palette::get(0);
    style.Colors[ImGuiCol_TabHovered]   = z8::palette::get(8);
    style.Colors[ImGuiCol_TabActive]    = z8::palette::get(8);
    style.Colors[ImGuiCol_TabUnfocused] = z8::palette::get(0);
    style.Colors[ImGuiCol_TabUnfocusedActive] = z8::palette::get(0);

    style.Colors[ImGuiCol_TitleBg]       = z8::palette::get(5);
    style.Colors[ImGuiCol_TitleBgActive] = z8::palette::get(5);

    m_player = player;

    m_ram_edit.OptShowAscii = m_rom_edit.OptShowAscii = false;
    m_ram_edit.OptUpperCaseHex = m_rom_edit.OptUpperCaseHex = false;
    m_ram_edit.OptShowOptions = m_rom_edit.OptShowOptions = false;

    io.Fonts->AddFontDefault();
}

ide::~ide()
{
    lol::LolImGui::Shutdown();
}

void ide::tick_game(float seconds)
{
    WorldEntity::tick_game(seconds);

#if CUSTOM_FONT
    if (!m_font)
    {
        //ImGui::EndFrame();
        auto atlas = IM_NEW(ImFontAtlas)();
        atlas->TexWidth = 128;
        atlas->TexHeight = 32;
        atlas->TexUvScale = lol::vec2(1 / 128.f, 1 / 32.f);
        atlas->TexUvWhitePixel = lol::vec2(5 / 128.f, 0 / 32.f);

        static ImWchar const char_ranges[] = { 0x20, 0x9a, 0 };
        ImFontConfig config;
        config.FontData = ImGui::MemAlloc(1);
        config.FontDataSize = 1;
        config.SizePixels = 6 * EDITOR_SCALE; // Not really needed it seems
        config.GlyphRanges = char_ranges;

        m_font = atlas->AddFont(&config);
        // Initialisation from ImFontAtlasBuildSetupFont()
        m_font->FontSize = config.SizePixels;
        m_font->ConfigData = &atlas->ConfigData[0];
        m_font->ContainerAtlas = atlas;
        m_font->Ascent = 0;
        m_font->Descent = 6 * EDITOR_SCALE;
        m_font->ConfigDataCount++;

        int const delta = EDITOR_SCALE / 2;

        // Printable ASCII chars
        for (int ch = 0x20; ch < 0x80; ++ch)
        {
            int x = ch % 0x20 * 4, y = ch / 0x20 * 6 - 6;
            m_font->AddGlyph(ch, delta, delta, 3 * EDITOR_SCALE + delta, 5 * EDITOR_SCALE + delta,
                             x / 128.f, y / 32.f, (x + 3) / 128.f, (y + 5) / 32.f, 4.f * EDITOR_SCALE);
        }

        // Double-width chars
        for (int ch = 0x80; ch < 0x9a; ++ch)
        {
            int x = ch % 0x10 * 8, y = ch / 0x10 * 6 + 2;
            m_font->AddGlyph(ch, delta, delta, 7 * EDITOR_SCALE + delta, 5 * EDITOR_SCALE + delta,
                             x / 128.f, y / 32.f, (x + 7) / 128.f, (y + 5) / 32.f, 8.f * EDITOR_SCALE);
        }

        m_font->BuildLookupTable();
        //ImGui::NewFrame();
    }

    if (m_font->ContainerAtlas->TexID == nullptr)
        m_font->ContainerAtlas->TexID = m_player->get_font_texture();

    ImGui::PushFont(m_font);
#endif

    render_dock();
//    ImGui::ShowDemoWindow();

    ImGui::SetNextWindowPos(lol::ivec2(1050, 550), ImGuiCond_FirstUseEver);
    ImGui::Begin("pALETTE", nullptr);
    {
        for (int i = 0; i < 16; i++)
        {
            if (i % 4 > 0)
                ImGui::SameLine();
            ImGui::PushID(i);
            ImGui::PushStyleColor(ImGuiCol_Button, z8::palette::get(i));
            ImGui::PushStyleColor(ImGuiCol_Text, z8::palette::get(i < 6 ? 7 : 0));
            ImGui::Button(lol::format("%2d", i).c_str());
            ImGui::PopStyleColor(2);
            ImGui::PopID();
        }
    }
    ImGui::End();

    ImGui::SetNextWindowDockID(m_dockspace_id, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(lol::ivec2(64, 32), ImGuiCond_FirstUseEver);
    ImGui::Begin("mUSIC", nullptr);
    {
        ImGui::TextColored(z8::palette::get(10), "stuff");
        ImGui::TextColored(z8::palette::get(5), "more stuff\nlol!!!");
    }
    ImGui::End();

    ImGui::SetNextWindowDockID(m_dockspace_id, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(lol::ivec2(64, 32), ImGuiCond_FirstUseEver);
    ImGui::Begin("sPRITES", nullptr);
    ImGui::End();

    ImGui::SetNextWindowDockID(m_dockspace_id, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(lol::ivec2(64, 32), ImGuiCond_FirstUseEver);
    ImGui::Begin("mAPS", nullptr);
    ImGui::End();

    ImGui::SetNextWindowDockID(m_dockspace_id, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(lol::ivec2(480, 480), ImGuiCond_FirstUseEver);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, z8::palette::get(5));
    m_editor.render();
    ImGui::PopStyleColor(1);

    ImGui::SetNextWindowPos(lol::ivec2(60, 480), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(lol::ivec2(512, 246), ImGuiCond_FirstUseEver);
    ImGui::Begin("ram", nullptr);
        m_ram_edit.DrawContents(m_player->get_ram(), 0x8000);
    ImGui::End();

    ImGui::SetNextWindowPos(lol::ivec2(400, 450), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(lol::ivec2(512, 256), ImGuiCond_FirstUseEver);
    ImGui::Begin("rom", nullptr);
        m_rom_edit.DrawContents(m_player->get_rom(), 0x5e00);
    ImGui::End();

    ImGui::SetNextWindowPos(lol::ivec2(800, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(lol::ivec2(400, 420), ImGuiCond_FirstUseEver);
    ImGui::Begin("cONSOLE", nullptr);
    {
        ImGui::Image(m_player->get_texture(), 3.f * lol::vec2(128.f),
                     lol::vec2(0.f), lol::vec2(1.f));
    }
    ImGui::End();

#if CUSTOM_FONT
    ImGui::PopFont();
#endif
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
    m_dockspace_id = ImGui::GetID("MyDockspace");
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    ImGui::DockSpace(m_dockspace_id, lol::vec2(0), dockspace_flags);

    // The main menu bar
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("fILE"))
        {
            ImGui::MenuItem("nEW", nullptr, &m_commands[0], false);
            ImGui::MenuItem("oPEN", nullptr, &m_commands[1], true);
            ImGui::Separator();
            ImGui::MenuItem("eXIT", nullptr, &m_commands[2], false);
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

    if (m_commands[1])
    {
        pfd::open_file("Open File", ".", { "PICO-8 cartridges", "*.p8 *.p8.png", "All Files", "*" });
        m_commands[1] = false;
    }
}

} // namespace z8

