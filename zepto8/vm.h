//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#pragma once

#include <lol/engine.h>

#include "cart.h"
#include "code-fixer.h"

namespace z8
{

using lol::array;

class vm : public lol::LuaLoader, public lol::LuaObject, public lol::Entity
{
public:
    vm()
    {
        // Register our Lua module
        lol::LuaObjectDef::Register<vm>(GetLuaState());
        ExecLuaFile("zepto8.lua");
    }

    virtual ~vm()
    {
    }

    void load(char const *name)
    {
        // FIXME: we only know PNG for now
        m_cart.load_png(name);
    }

    void run()
    {
        // Fix code
        code_fixer fixer(m_cart.get_code());
        lol::String new_code = fixer.fix();

        //msg::info("Fixed cartridge code:\n%s\n", m_code.C());
        //printf("%s", m_code.C());
        // FIXME: not required yet because we inherit from LuaLoader
        //lol::LuaLoader lua;

        // Execute cartridge code
        ExecLuaCode(new_code.C());

        // Initialize cartridge code
        ExecLuaCode("_init()");
        ExecLuaCode("_draw()");
        ExecLuaCode("_update()");
    }

    static const lol::LuaObjectLib* GetLib();
    static vm* New(lol::LuaState* l, int arg_nb);

    static int btn(lol::LuaState *l);
    static int btnp(lol::LuaState *l);
    static int cls(lol::LuaState *l);
    static int cos(lol::LuaState *l);
    static int cursor(lol::LuaState *l);
    static int flr(lol::LuaState *l);
    static int music(lol::LuaState *l);
    static int pget(lol::LuaState *l);
    static int pset(lol::LuaState *l);
    static int rnd(lol::LuaState *l);
    static int sget(lol::LuaState *l);
    static int sset(lol::LuaState *l);
    static int sin(lol::LuaState *l);
    static int spr(lol::LuaState *l);
    static int sspr(lol::LuaState *l);

private:
    array<uint8_t> m_memory;
    cart m_cart;
};

} // namespace z8

