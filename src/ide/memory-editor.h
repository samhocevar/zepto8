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

#include "3rdparty/imgui-club/imgui_memory_editor/imgui_memory_editor.h"

namespace z8
{

class memory_editor
{
public:
    memory_editor(std::tuple<uint8_t *, size_t> area);
    ~memory_editor();

    void render();

private:
    std::tuple<uint8_t *, size_t> m_area;
    MemoryEditor m_editor;
};

} // namespace z8

