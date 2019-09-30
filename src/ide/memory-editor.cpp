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

#include "zepto8.h"
#include "memory-editor.h"
#include "pico8/pico8.h"

namespace z8
{

memory_editor::memory_editor(std::tuple<uint8_t *, size_t> area)
  : m_area(area)
{
    m_editor.OptShowAscii = false;
    m_editor.OptUpperCaseHex = false;
    m_editor.OptShowOptions = false;
}

memory_editor::~memory_editor()
{
}

void memory_editor::render()
{
    m_editor.DrawContents(std::get<0>(m_area), std::get<1>(m_area));
}

} // namespace z8

