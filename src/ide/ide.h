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
#include "player.h"
#include "ide/editor.h"

namespace z8
{

class ide : public lol::WorldEntity
{
public:
    ide(player *player);
    virtual ~ide();

    virtual void TickGame(float seconds);
    virtual void TickDraw(float seconds, lol::Scene &scene);

private:
    void render_dock();
    void render_editor();

    editor m_editor;
    player *m_player;
};

} // namespace z8

