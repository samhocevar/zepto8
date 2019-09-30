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

#include <memory>

namespace z8
{

class text_editor
{
public:
    text_editor();
    ~text_editor();

    void render();

private:
    std::unique_ptr<class editor_impl> m_impl;
    float m_fontsize = 0.f;
};

} // namespace z8

