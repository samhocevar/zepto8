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

#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <lol/engine.h>

#include "imgui_internal.h" // for the docking API

#include "zepto8.h"
#include "ide/ide.h"
#include "pico8/pico8.h"
#include "3rdparty/portable-file-dialogs/portable-file-dialogs.h"

#define CUSTOM_FONT 0

namespace z8
{

ide::ide()
{
    lol::gui::init();

    // Enable docking
    auto &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.Fonts->AddFontDefault();

    m_text_editor = std::make_unique<text_editor>();
    m_ram_editor = std::make_unique<memory_editor>();
    m_rom_editor = std::make_unique<memory_editor>();

    apply_scale();
}

ide::~ide()
{
    lol::gui::shutdown();
}

void ide::load(std::string const &name)
{
    m_player = new z8::player(true, lol::ends_with(name, ".rcn.json"));
    m_player->get_texture(); // HACK: disable player rendering
    m_player->load(name);
    m_player->run();

    m_vm = m_player->get_vm();

    m_text_editor->attach(m_vm);
    m_ram_editor->attach(m_vm->ram());
    m_rom_editor->attach(m_vm->rom());
}

void ide::apply_scale()
{
    auto &style = ImGui::GetStyle();
    style.WindowBorderSize = style.ChildBorderSize = style.PopupBorderSize = style.FrameBorderSize = style.TabBorderSize = (float)m_scale;
    style.FramePadding = lol::vec2(2.f * m_scale);
    style.WindowRounding = style.ChildRounding = style.FrameRounding = style.ScrollbarRounding = style.TabRounding = 0.0f;

    // Useless
    style.Colors[ImGuiCol_ChildBg] = pico8::palette::get(5);

    style.Colors[ImGuiCol_Tab]          = pico8::palette::get(0);
    style.Colors[ImGuiCol_TabHovered]   = pico8::palette::get(8);
    style.Colors[ImGuiCol_TabActive]    = pico8::palette::get(8);
    style.Colors[ImGuiCol_TabUnfocused] = pico8::palette::get(0);
    style.Colors[ImGuiCol_TabUnfocusedActive] = pico8::palette::get(0);

    style.Colors[ImGuiCol_TitleBg]       = pico8::palette::get(5);
    style.Colors[ImGuiCol_TitleBgActive] = pico8::palette::get(8);
}

bool ide::init_draw()
{
    m_screen = std::make_shared<lol::Texture>(ivec2(128, 128), lol::PixelFormat::RGBA_8);
    m_sprites = std::make_shared<lol::Texture>(ivec2(128, 128), lol::PixelFormat::RGBA_8);

    return true;
}

bool ide::release_draw()
{
    m_screen.reset();
    m_sprites.reset();

    return true;
}

void ide::tick_game(float seconds)
{
    WorldEntity::tick_game(seconds);

    if (!m_fonts[m_scale])
    {
        ImGui::EndFrame();

        char const *filename = "data/zepto8.ttf";

        // Mark all 256-char ranges covered by the PICO-8 charset
        std::unordered_set<uint32_t> pico8_ranges;
        for (int i = 0; i < 256; ++i)
            for (char32_t ch : pico8::charset::to_utf32[i])
                pico8_ranges.insert(ch >> 8);

        // Create an array of char ranges for AddFontFromFileTTF()
        std::vector<ImWchar> char_ranges;
        for (uint16_t c : pico8_ranges)
        {
            char_ranges.push_back(c ? c * 256 : 1);
            char_ranges.push_back(c * 256 + 256);
        }
        char_ranges.push_back(0);

        // Initialize BIOS
        for (auto const &file : lol::sys::get_path_list(filename))
        {
            lol::File f;
            f.Open(file, lol::FileAccess::Read);
            bool exists = f.IsValid();
            f.Close();

            if (exists)
            {
                auto &io = ImGui::GetIO();
                m_fonts[m_scale] = io.Fonts->AddFontFromFileTTF(file.c_str(), 6.0f * m_scale, nullptr, char_ranges.data());
                lol::gui::refresh_fonts();
                break;
            }
        }

#if CUSTOM_FONT
        auto atlas = IM_NEW(ImFontAtlas)();
        atlas->TexWidth = 128;
        atlas->TexHeight = 32;
        atlas->TexUvScale = lol::vec2(1 / 128.f, 1 / 32.f);
        atlas->TexUvWhitePixel = lol::vec2(5 / 128.f, 0 / 32.f);

        ImFontConfig config;
        config.FontData = ImGui::MemAlloc(1);
        config.FontDataSize = 1;
        config.SizePixels = 6 * m_scale; // Not really needed it seems
        config.GlyphRanges = char_ranges;

        m_font = atlas->AddFont(&config);
        // Initialisation from ImFontAtlasBuildSetupFont()
        m_font->FontSize = config.SizePixels;
        m_font->ConfigData = &atlas->ConfigData[0];
        m_font->ContainerAtlas = atlas;
        m_font->Ascent = 0;
        m_font->Descent = 6 * m_scale;
        m_font->ConfigDataCount++;

        int const delta = m_scale / 2;

        // Printable ASCII chars
        for (int ch = 0x10; ch < 0x80; ++ch)
        {
            int x = ch % 0x20 * 4, y = ch / 0x20 * 6 - 6;
            m_font->AddGlyph(ch, delta, delta, 3 * m_scale + delta, 5 * m_scale + delta,
                             x / 128.f, y / 32.f, (x + 3) / 128.f, (y + 5) / 32.f, 4.f * m_scale);
        }

        // Double-width chars
        for (int ch = 0x80; ch < 0x100; ++ch)
        {
            int x = ch % 0x10 * 8, y = ch / 0x10 * 6 + 2;
            m_font->AddGlyph(ch, delta, delta, 7 * m_scale + delta, 5 * m_scale + delta,
                             x / 128.f, y / 32.f, (x + 7) / 128.f, (y + 5) / 32.f, 8.f * m_scale);
        }

        m_font->BuildLookupTable();
#endif
        ImGui::NewFrame();
    }

#if CUSTOM_FONT
    if (m_font->ContainerAtlas->TexID == nullptr)
        m_font->ContainerAtlas->TexID = m_player->get_font_texture();
#endif

    ImGui::PushFont(m_fonts[m_scale]);

    render_app();

    ImGui::PopFont();
}

void ide::render_app()
{
    // Create a fullscreen window for the docking space
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking
                           | ImGuiWindowFlags_NoTitleBar
                           | ImGuiWindowFlags_NoCollapse
                           | ImGuiWindowFlags_NoResize
                           | ImGuiWindowFlags_NoMove
                           | ImGuiWindowFlags_NoBringToFrontOnFocus
                           | ImGuiWindowFlags_NoNavFocus;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, lol::vec2(3.f * m_scale));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, pico8::palette::get(5));

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(lol::vec2(viewport->Size.x, 0.f));
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::Begin("menu window", nullptr, flags | ImGuiWindowFlags_MenuBar);
        render_menu();
        render_toolbar();
        // Store window size so that we can place the rest of the app
        lol::vec2 menu_size = ImGui::GetWindowSize();
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, lol::vec2(0));

    ImGui::SetNextWindowPos(lol::vec2(viewport->Pos.x, viewport->Pos.y + menu_size.y));
    ImGui::SetNextWindowSize(lol::vec2(viewport->Size.x, viewport->Size.y - menu_size.y));
    ImGui::Begin("dock", nullptr, flags);
        // Create the actual dock space
        m_dock.root = ImGui::GetID("MyDockspace");
        bool first_frame = ImGui::DockBuilderGetNode(m_dock.root) == nullptr;
        ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        ImGui::DockSpace(m_dock.root, lol::vec2(0), dockspace_flags);
        // Temporary hack because I can’t get the layout stuff to work
        m_dock.bottom_left = m_dock.bottom_right = m_dock.main = m_dock.root;
    ImGui::End();

    ImGui::PopStyleVar(3);

    render_windows();
}

void ide::render_menu()
{
    // The main menu bar
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("New", nullptr, &m_commands[0], false);
            ImGui::MenuItem("Open", nullptr, &m_commands[1], true);
            ImGui::Separator();
            ImGui::MenuItem("Exit", nullptr, &m_commands[2], false);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::MenuItem("Cut", nullptr, nullptr, false);
            ImGui::MenuItem("Copy", nullptr, nullptr, false);
            ImGui::MenuItem("Paste", nullptr, nullptr, false);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Player", nullptr, &m_show.player, true);
            ImGui::MenuItem("Code", nullptr, &m_show.code, true);
            ImGui::Separator();

            if (ImGui::BeginMenu("Windows"))
            {
#if 0
                ImGui::MenuItem("Palette", nullptr, &m_show.palette, true);
#endif
                ImGui::MenuItem("Maps", nullptr, &m_show.maps, true);
                ImGui::MenuItem("Sprites", nullptr, &m_show.sprites, true);
                ImGui::MenuItem("Music", nullptr, &m_show.music, true);
                ImGui::Separator();
                ImGui::MenuItem("ROM", nullptr, &m_show.rom, true);
                ImGui::MenuItem("RAM", nullptr, &m_show.ram, true);
                ImGui::EndMenu();
            }
            ImGui::Separator();

            if (ImGui::BeginMenu("Zoom"))
            {
                bool z1 = m_scale == 1, z2 = m_scale == 2, z3 = m_scale == 3;
                ImGui::MenuItem("x1", nullptr, &z1, true);
                ImGui::MenuItem("x2", nullptr, &z2, true);
                ImGui::MenuItem("x3", nullptr, &z3, true);
                int scale = (z1 && m_scale != 1) ? 1
                          : (z2 && m_scale != 2) ? 2
                          : (z3 && m_scale != 3) ? 3
                          : m_scale;
                if (m_scale != scale)
                {
                    m_scale = scale;
                    apply_scale();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            ImGui::MenuItem("No help yet", nullptr, nullptr, false);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void ide::render_toolbar()
{
    // Play / Pause / Restart
    ImGui::PushStyleColor(ImGuiCol_Button, pico8::palette::get(pico8::palette::red));
    ImGui::Button("|>");
    ImGui::SameLine();
    ImGui::Button("||");
    ImGui::SameLine();
    ImGui::Button("()");
    ImGui::SameLine();
    ImGui::PopStyleColor();

    // Some spacing
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5 * m_scale);

    // Palette buttons
    for (int i = 0; i < 16; i++)
    {
        ImGui::PushID(i);
        ImGui::PushStyleColor(ImGuiCol_Button, pico8::palette::get(i));
        ImGui::PushStyleColor(ImGuiCol_Text, pico8::palette::get(i < 6 ? 7 : 0));
        ImGui::Button(lol::format("%2d", i).c_str());
        ImGui::SameLine();
        ImGui::PopStyleColor(2);
        ImGui::PopID();
    }
}

void ide::render_windows()
{
    if (m_show.music)
    {
        ImGui::SetNextWindowDockID(m_dock.main, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(lol::ivec2(64, 32), ImGuiCond_FirstUseEver);
        ImGui::Begin("Music", &m_show.music);
        {
            ImGui::TextColored(pico8::palette::get(10), "stuff");
            ImGui::TextColored(pico8::palette::get(5), "more stuff\nlol!!!");
        }
        ImGui::End();
    }

    if (m_show.sprites)
    {
        ImGui::SetNextWindowDockID(m_dock.main, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(lol::ivec2(64, 32), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Sprites", &m_show.sprites))
        {
        }
        ImGui::End();
    }

    if (m_show.maps)
    {
        ImGui::SetNextWindowDockID(m_dock.main, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(lol::ivec2(64, 32), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Maps", &m_show.maps))
        {
        }
        ImGui::End();
    }

    if (m_show.code)
    {
        ImGui::SetNextWindowDockID(m_dock.main, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(lol::ivec2(480, 480), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Code", &m_show.code))
            m_text_editor->render();
        ImGui::End();
    }

    if (m_show.rom)
    {
        ImGui::SetNextWindowDockID(m_dock.bottom_left, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(lol::ivec2(400, 450), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(lol::ivec2(512, 256), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("ROM", &m_show.rom))
            m_rom_editor->render();
        ImGui::End();
    }

    if (m_show.ram)
    {
        ImGui::SetNextWindowDockID(m_dock.bottom, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(lol::ivec2(60, 480), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(lol::ivec2(512, 246), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("RAM", &m_show.ram))
            m_ram_editor->render();
        ImGui::End();
    }

    if (m_show.player)
    {
        ImGui::SetNextWindowPos(lol::ivec2(800, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(lol::ivec2(420, 450), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Player", &m_show.player))
        {
            lol::vec2 avail_size = ImGui::GetContentRegionAvail();
            float ratio = std::floor(std::max(1.f, std::min(avail_size.x / 128.f, avail_size.y / 128.f)));
            ImGui::Image(m_screen.get(), ratio * lol::vec2(128.f),
                         lol::vec2(0.f), lol::vec2(1.f));
            ImGui::PushStyleColor(ImGuiCol_Button, pico8::palette::get(pico8::palette::black));
            for (auto ch : { "⬅️", "➡️", "⬆️", "⬇️", "🅾️", "❎" })
            {
                ImGui::Button(ch);
                ImGui::SameLine();
            }
            ImGui::PopStyleColor();
        }
        ImGui::End();
    }
}

void ide::tick_draw(float seconds, lol::Scene &scene)
{
    WorldEntity::tick_draw(seconds, scene);

    if (m_commands[1])
    {
        pfd::open_file("Open File", ".", { "PICO-8 cartridges", "*.p8 *.p8.png", "All Files", "*" });
        m_commands[1] = false;
    }

    std::vector<lol::u8vec4> buf(128 * 128);
    m_vm->render(buf.data());
    m_screen->Bind();
    m_screen->SetData(buf.data());
}

} // namespace z8

