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

ide::ide()
{
    lol::LolImGui::Init();

    m_editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
    m_editor.SetText("-- test\nprint(\"hello world!\")\n\n");
}

ide::~ide()
{
    lol::LolImGui::Shutdown();
}

void ide::TickGame(float seconds)
{
    WorldEntity::TickGame(seconds);

    //ImGui::ShowDemoWindow();
    m_editor.Render("TextEditor");

    ImGui::Begin("Palette");
    {
        for (int i = 0; i < 16; i++)
        {
            if (i % 4 > 0)
                ImGui::SameLine();
            ImGui::PushID(i);
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)((lol::vec4)z8::palette::get(i) / 255.f));
            ImGui::Button(lol::format("%2d", i).c_str());
            ImGui::PopStyleColor(1);
            ImGui::PopID();
        }
    }
    ImGui::End();
}

void ide::TickDraw(float seconds, lol::Scene &scene)
{
    WorldEntity::TickDraw(seconds, scene);
}

} // namespace z8

