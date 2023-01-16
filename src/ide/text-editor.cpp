//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2020 Sam Hocevar <sam@hocevar.net>
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

#include <string>     // std::string
#include <map>        // std::map
#include <memory>     // std::shared_ptr
#include <functional> // std::function

#include "zepto8.h"
#include "ide/text-editor.h"
#include "pico8/pico8.h"

#include <lol/engine.h> // for the ImGui headers and much more stuff

#include "zep.h"
#include "zep/mode_vim.h"
#include "zep/mode_standard.h"
#include "zep/filesystem.h"

#include "zep/imgui/editor_imgui.h"

#define BUFFER_NAME "code.p8"

namespace z8
{

#define TEST_TEXT \
    "--                 " \
       "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\n" \
    "--  !\"#$%&'()*+,-./0123456789:;<=>?\n" \
    "-- @ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\n" \
    "-- `abcdefghijklmnopqrstuvwxyz{|}~\x7f\n" \
    "-- \x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f\n" \
    "-- \x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f\n" \
    "-- \xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf\n" \
    "-- \xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\n" \
    "-- \xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\n" \
    "-- \xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf\n" \
    "-- \xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\n" \
    "-- \xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff\n"

class zep_filesystem : public Zep::IZepFileSystem
{
public:
    virtual std::string Read(const Zep::ZepPath& filePath) override { return ""; }
    virtual bool Write(const Zep::ZepPath& filePath, const void* pData, size_t size) override { return true; }
    virtual Zep::ZepPath GetSearchRoot(const Zep::ZepPath& start, bool& foundGit) const override { return start; }
    virtual Zep::ZepPath GetConfigPath() const override { return ""; }
    virtual const Zep::ZepPath& GetWorkingDirectory() const override { return m_cwd; }
    virtual void SetWorkingDirectory(const Zep::ZepPath& path) override { }
    virtual void SetFlags(uint32_t flags) override { }
    virtual bool IsDirectory(const Zep::ZepPath& path) const override { return false; }
    virtual bool IsReadOnly(const Zep::ZepPath& path) const override { return false; }
    virtual bool Exists(const Zep::ZepPath& path) const override { return path == BUFFER_NAME; }
    virtual bool MakeDirectories(const Zep::ZepPath& path) override { return false; }

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
        : Zep::ZepEditor_ImGui(Zep::ZepPath(), Zep::NVec2f(1.f), Zep::ZepEditorFlags::DisableThreads, new zep_filesystem())
    {}

    virtual ~editor_impl()
    {}

    Zep::ZepBuffer *m_buffer;
};

class zep_theme : public Zep::ZepTheme
{
public:
    zep_theme()
    {
        add_color(Zep::ThemeColor::Text,         pico8::palette::light_gray);
        add_color(Zep::ThemeColor::Text,         pico8::palette::light_gray);
        add_color(Zep::ThemeColor::Normal,       pico8::palette::light_gray);
        add_color(Zep::ThemeColor::Parenthesis,  pico8::palette::light_gray);
        add_color(Zep::ThemeColor::Background,   pico8::palette::dark_blue);
        add_color(Zep::ThemeColor::LineNumber,   pico8::palette::orange);
        add_color(Zep::ThemeColor::CursorNormal, pico8::palette::red);
        add_color(Zep::ThemeColor::Comment,      pico8::palette::indigo);
        add_color(Zep::ThemeColor::Keyword,      pico8::palette::pink);
        add_color(Zep::ThemeColor::Identifier,   pico8::palette::green);
        add_color(Zep::ThemeColor::Number,       pico8::palette::blue);
        add_color(Zep::ThemeColor::String,       pico8::palette::blue);
    }

    virtual Zep::NVec4f const &GetColor(Zep::ThemeColor themeColor) const
    {
        auto it = m_palette.find(themeColor);
        if (it != m_palette.end())
            return it->second;
        return Zep::ZepTheme::GetColor(themeColor);
    }

    //virtual Zep::NVec4f GetComplement(const Zep::NVec4f& col) const
    //virtual Zep::NVec4f GetUniqueColor(uint32_t id) const

private:
    void add_color(Zep::ThemeColor tid, int cid)
    {
        auto col = pico8::palette::get(cid);
        m_palette[tid] = Zep::NVec4f(col.r, col.g, col.b, col.a);
    }

    std::map<Zep::ThemeColor, Zep::NVec4f> m_palette;
};

text_editor::text_editor()
  : m_impl(std::make_unique<editor_impl>())
{
    //m_impl->SetMode(Zep::ZepMode_Standard::StaticName());
    m_impl->RegisterSyntaxFactory({".p8"}, Zep::SyntaxProvider { "p8", Zep::tSyntaxFactory([](Zep::ZepBuffer* pBuffer)
    {
        return std::make_shared<Zep::ZepSyntax>(*pBuffer, pico8::api::keywords, pico8::api::functions);
    })});

    m_impl->m_buffer = m_impl->InitWithText(BUFFER_NAME, "");
    m_impl->m_buffer->SetTheme(std::static_pointer_cast<Zep::ZepTheme, zep_theme>(std::make_shared<zep_theme>()));
}

text_editor::~text_editor()
{
}

void text_editor::attach(std::shared_ptr<z8::vm_base> vm)
{
    m_vm = vm;
    m_impl->m_buffer->SetText(m_vm->get_code());
}

void text_editor::render()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, pico8::palette::get(5));

    // Needs to be done in render() because it uses the current ImGui font size
    if (m_fontsize != ImGui::GetFontSize())
    {
        m_fontsize = ImGui::GetFontSize();
        m_impl->GetDisplay().GetFont(Zep::ZepTextType::Text).InvalidateCharCache();
    }

    if (ImGui::IsWindowFocused())
    {
        ImGuiIO& io = ImGui::GetIO();
        io.WantCaptureKeyboard = true;
        io.WantTextInput = true;
        m_impl->HandleInput();
    }

    m_impl->UpdateWindowState();
    m_impl->SetDisplayRegion(Zep::toNVec2f(ImGui::GetCursorScreenPos()),
                             Zep::toNVec2f(ImGui::GetContentRegionAvail()) + Zep::toNVec2f(ImGui::GetCursorScreenPos()));
    m_impl->Display();

    ImGui::PopStyleColor(1);
}

} // namespace z8

