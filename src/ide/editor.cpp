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

#include <set>
#include <string>

#include "zepto8.h"
#include "editor.h"

#include "3rdparty/zep/src/zep.h"
#include "3rdparty/zep/src/mode_vim.h"
#include "3rdparty/zep/src/mode_standard.h"
#include "3rdparty/zep/src/filesystem.h"
#include "3rdparty/zep/src/imgui/editor_imgui.h"

#define FAKE_FILE_NAME "code.p8"

namespace z8
{

// This is the list from Lua’s luaX_tokens
static std::set<std::string> pico8_keywords =
{
    "and", "break", "do", "else", "elseif",
    "end", "false", "for", "function", "goto", "if",
    "in", "local", "nil", "not", "or", "repeat",
    "return", "then", "true", "until", "while",
};

static std::set<std::string> pico8_identifiers =
{
    // Implemented in pico8lib (from z8lua)
    "max", "min", "mid", "ceil", "flr", "cos", "sin", "atan2", "sqrt",
    "abs", "sgn", "band", "bor", "bxor", "bnot", "shl", "shr", "lshr",
    "rotl", "rotr", "tostr", "tonum", "srand", "rnd",
    // Implemented in the ZEPTO-8 VM
    "run", "menuitem", "reload", "peek", "peek4", "poke", "poke4",
    "memcpy", "memset", "stat", "printh", "extcmd", "_update_buttons",
    "btn", "btnp", "cursor", "print", "camera", "circ", "circfill",
    "clip", "cls", "color", "fillp", "fget", "fset", "line", "map",
    "mget", "mset", "pal", "palt", "pget", "pset", "rect", "rectfill",
    "sget", "sset", "spr", "sspr", "music", "sfx", "time",
    // Implemented in the ZEPTO-8 BIOS
    "cocreate", "coresume", "costatus", "yield", "trace", "stop",
    "count", "add", "sub", "foreach", "all", "del", "t", "dget",
    "dset", "cartdata", "load", "save", "info", "abort", "folder",
    "resume", "reboot", "dir", "ls", "flip", "mapdraw",
    // Not implemented but we should!
    "assert", "getmetatable", "setmetatable",
};

#define TEST_TEXT \
    "-- pico-8 syntax test\n-- by sam\n\n" \
    "function _init()\n cls()\n step = 1\n tmp = rnd(17)\n lst = {\"lol\"}\nend\n\n" \
    "function _update()\n if (btnp(\x97) or btnp(\x8e)) step = 0\n\n" \
    " if step < #lst then\n  step += 1\n end\nend\n\n" \
    "function _draw()\n local x = 28\n local y = 120\n\n map(0, 0, 0, 0, 16, 16)\nend\n\n" \
    "--  !\"#$%&'()*+,-./0123456789:;<=>?\n" \
    "-- @ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\n" \
    "-- `abcdefghijklmnopqrstuvwxyz{|}~\x7f\n" \
    "-- \x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f\n" \
    "-- \x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\n"

class zep_filesystem : public Zep::IZepFileSystem
{
public:
    virtual std::string Read(const Zep::ZepPath& filePath) override
    {
        if (filePath == FAKE_FILE_NAME)
            return TEST_TEXT;
        return "";
    }

    virtual bool Write(const Zep::ZepPath& filePath, const void* pData, size_t size) override
    {
        return true;
    }

    virtual const Zep::ZepPath& GetWorkingDirectory() const override
    {
        return m_cwd;
    }

    virtual void SetWorkingDirectory(const Zep::ZepPath& path) override
    {
    }

    virtual bool IsDirectory(const Zep::ZepPath& path) const override { return false; }
    virtual bool IsReadOnly(const Zep::ZepPath& path) const override { return false; }
    virtual bool Exists(const Zep::ZepPath& path) const override { return path == FAKE_FILE_NAME; }

    // A callback API for scaning
    virtual void ScanDirectory(const Zep::ZepPath& path, std::function<bool(const Zep::ZepPath& path, bool& dont_recurse)> fnScan) const override
    {
    }

    // Equivalent means 'the same file'
    virtual bool Equivalent(const Zep::ZepPath& path1, const Zep::ZepPath& path2) const override
    {
        return path1 == path2;
    }

    virtual Zep::ZepPath Canonical(const Zep::ZepPath& path) const override { return path; }

private:
    Zep::ZepPath m_cwd = "";
};

class editor_impl : public Zep::ZepEditor_ImGui
{
public:
    editor_impl()
        : Zep::ZepEditor_ImGui("", Zep::ZepEditorFlags::DisableThreads, new zep_filesystem())
    {}

    virtual ~editor_impl()
    {}
};

class zep_theme : public Zep::ZepTheme
{
public:
    zep_theme()
    {
        m_palette[Zep::ThemeColor::Text] = z8::palette::light_gray;
        m_palette[Zep::ThemeColor::Normal] = z8::palette::light_gray;
        m_palette[Zep::ThemeColor::Parenthesis] = z8::palette::light_gray;
        m_palette[Zep::ThemeColor::Background] = z8::palette::dark_blue;
        m_palette[Zep::ThemeColor::LineNumber] = z8::palette::orange;
        m_palette[Zep::ThemeColor::CursorNormal] = z8::palette::red;
        m_palette[Zep::ThemeColor::Comment] = z8::palette::indigo;
        m_palette[Zep::ThemeColor::Keyword] = z8::palette::pink;
        m_palette[Zep::ThemeColor::Identifier] = z8::palette::green;
        m_palette[Zep::ThemeColor::Number] = z8::palette::blue;
        m_palette[Zep::ThemeColor::String] = z8::palette::blue;
    }

    virtual Zep::NVec4f GetColor(Zep::ThemeColor themeColor) const
    {
        auto it = m_palette.find(themeColor);
        if (it != m_palette.end())
        {
            auto col = z8::palette::get(it->second);
            return Zep::NVec4f(col.r, col.g, col.b, col.a);
        }
        return Zep::ZepTheme::GetColor(themeColor);
    }

    //virtual Zep::NVec4f GetComplement(const Zep::NVec4f& col) const
    //virtual Zep::NVec4f GetUniqueColor(uint32_t id) const

private:
    std::map<Zep::ThemeColor, int> m_palette;
};

editor::editor()
  : m_impl(std::make_unique<editor_impl>())
{
    //m_impl->SetMode(Zep::ZepMode_Standard::StaticName());
    m_impl->RegisterSyntaxFactory({".p8"}, Zep::tSyntaxFactory([](Zep::ZepBuffer* pBuffer)
    {
        return std::make_shared<Zep::ZepSyntax>(*pBuffer, pico8_keywords, pico8_identifiers);
    }));

    m_impl->InitWithFileOrDir(FAKE_FILE_NAME);

    auto* buffer = m_impl->GetFileBuffer(FAKE_FILE_NAME);
    buffer->SetTheme(std::static_pointer_cast<Zep::ZepTheme, zep_theme>(std::make_shared<zep_theme>()));
}

editor::~editor()
{
}

void editor::render()
{
    m_impl->UpdateWindowState();
    m_impl->SetDisplayRegion(Zep::toNVec2f(ImGui::GetCursorScreenPos()),
                             Zep::toNVec2f(ImGui::GetContentRegionAvail()) + Zep::toNVec2f(ImGui::GetCursorScreenPos()));
    m_impl->Display();
    m_impl->HandleInput();
}

} // namespace z8

