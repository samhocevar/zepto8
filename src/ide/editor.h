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

#if USE_LEGACY_EDITOR
#   include "3rdparty/imgui-color-text-edit/TextEditor.h"
#else
#   include "3rdparty/zep/src/zep.h"
#   include "3rdparty/zep/src/imgui/editor_imgui.h"
#endif

namespace z8
{

class editor
{
public:
    editor();
    ~editor();

    void render();

private:
#if USE_LEGACY_EDITOR
    TextEditor m_widget;
#else
    std::unique_ptr<Zep::ZepEditor_ImGui> m_zep;
#endif
};

} // namespace z8

