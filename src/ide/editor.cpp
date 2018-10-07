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

#include "editor.h"

namespace z8
{

static TextEditor::LanguageDefinition const& get_lang_def();
static TextEditor::Palette const &get_palette();

editor::editor()
{
    m_widget.SetLanguageDefinition(get_lang_def());
    m_widget.SetPalette(get_palette());

    // Debug text
    m_widget.SetText("-- pico-8 syntax test\n-- by sam\n\n"
        "function _init()\n cls()\n step = 1\n tmp = rnd(17)\n lst = {\"lol\"}\nend\n\n"
        "function _update()\n if (btnp(\xc2\x97) or btnp(\xc2\x8e)) step = 0\n\n"
        " if step < #lst then\n  step += 1\n end\nend\n\n"
        "function _draw()\n local x = 28\n local y = 120\n\n map(0, 0, 0, 0, 16, 16)\nend\n\n"
        "--  !\"#$%&'()*+,-./0123456789:;<=>?\n"
        "-- @ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\n"
        "-- `abcdefghijklmnopqrstuvwxyz{|}~\x7f\n"
        // ImGui does not like invalid UTF-8, so we use \xc2\x80 instead of \x80
        "-- \xc2\x80\xc2\x81\xc2\x82\xc2\x83\xc2\x84\xc2\x85\xc2\x86\xc2\x87"
           "\xc2\x88\xc2\x89\xc2\x8a\xc2\x8b\xc2\x8c\xc2\x8d\xc2\x8e\xc2\x8f\n"
        "-- \xc2\x90\xc2\x91\xc2\x92\xc2\x93\xc2\x94\xc2\x95\xc2\x96\xc2\x97"
           "\xc2\x98\xc2\x99\n");
}

editor::~editor()
{
}

void editor::render()
{
    ImGui::Begin("cODE", nullptr);
    m_widget.Render("Text Editor");
    ImGui::End();
}

static TextEditor::LanguageDefinition const& get_lang_def()
{
    static bool inited = false;
    static TextEditor::LanguageDefinition ret;

    if (!inited)
    {
        // This is the list from Lua’s luaX_tokens
        static char const* const keywords[] =
        {
            "and", "break", "do", "else", "elseif",
            "end", "false", "for", "function", "goto", "if",
            "in", "local", "nil", "not", "or", "repeat",
            "return", "then", "true", "until", "while",
        };

        for (auto& k : keywords)
            ret.mKeywords.insert(k);

        static const char* const identifiers[] =
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

        for (auto& k : identifiers)
        {
            TextEditor::Identifier id;
            id.mDeclaration = "Built-in function";
            ret.mIdentifiers.insert(std::make_pair(std::string(k), id));
        }

        #define ADD_COLOR(regex, index) \
            ret.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>(regex, TextEditor::PaletteIndex::index))
        ADD_COLOR("(\\-\\-|//).*", Comment);
        ADD_COLOR("L?\\\"(\\\\.|[^\\\"])*\\\"", String);
        ADD_COLOR("\\\'[^\\\']*\\\'", String);
        ADD_COLOR("[+-]?0[xX]([0-9a-fA-F]+([.][0-9a-fA-F]*)?|[.][0-9a-fA-F]+)", Number);
        ADD_COLOR("[+-]?0[bB]([01]+([.][01]*)?|[.][0-1]+)", Number);
        ADD_COLOR("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)", Number);
        ADD_COLOR("[a-zA-Z_][a-zA-Z0-9_]*", Identifier);
        ADD_COLOR("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", Punctuation);
        #undef ADD_COLOR

        ret.mCommentStart = "\\-\\-\\[\\[";
        ret.mCommentEnd = "\\]\\]";

        ret.mCaseSensitive = true;
        ret.mAutoIndentation = false;

        ret.mName = "PICO-8";

        inited = true;
    }

    return ret;
}

static TextEditor::Palette const &get_palette()
{
    static TextEditor::Palette p =
    { {
        0xffffffff, // None
        0xffa877ff, // Keyword (PICO-8 pink)
        0xffffad29, // Number (PICO-8 blue)
        0xffffad29, // String (PICO-8 blue)
        0xff70a0e0, // Char literal
        0xffe8f1ff, // Punctuation (PICO-8 white)
        0xff409090, // Preprocessor
        0xffc7c3c2, // Identifier (PICO-8 light gray)
        0xff36e400, // Known identifier (PICO-8 green)
        0xffc040a0, // Preproc identifier
        0xff9c7683, // Comment (single line) (PICO-8 indigo)
        0xff9c7683, // Comment (multi line) (PICO-8 indigo)
        //0xff000000, // Background (PICO-8 black)
        0xff4f575f, // Background (PICO-8 dark gray)
        0xff4d00ff, // Cursor (PICO-8 red)
        0x80a06020, // Selection
        0x800020ff, // ErrorMarker
        0x40f08000, // Breakpoint
        0xff00a3ff, // Line number (PICO-8 orange)
        0x40000000, // Current line fill
        0x40808080, // Current line fill (inactive)
        0x40a0a0a0, // Current line edge
    } };
    return p;
}

} // namespace z8

