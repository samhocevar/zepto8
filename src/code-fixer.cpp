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

#include <lol/engine.h>

#include <pegtl.hh>
#include <pegtl/trace.hh>

#include "code-fixer.h"
#define WITH_PICO8 1
#include "lua53-parse.h"

using lol::String;
using lol::ivec2;
using lol::ivec3;
using lol::msg;

namespace z8
{

//
// Actions executed during analysis
//

template<typename R>
struct analyze_action : pegtl::nothing<R> {};

template<>
struct analyze_action<lua53::short_if_statement>
{
    static void apply(pegtl::action_input const &in, code_fixer &f)
    {
        UNUSED(f);
        msg::info("unsupported short_if_statement at line %ld:%ld: %s\n", in.line(), in.byte_in_line(), in.string().c_str());
    }
};

template<>
struct analyze_action<lua53::reassign_op>
{
    static void apply(pegtl::action_input const &in, code_fixer &f)
    {
#if 0
        msg::info("reassignment operator %ld:%ld(%ld): %s\n", in.line(), in.byte_in_line(), in.byte(), in.string().c_str());
#endif
        f.m_reassign_ops.push(in.byte());
    }
};

template<>
struct analyze_action<lua53::reassignment>
{
    static void apply(pegtl::action_input const &in, code_fixer &f)
    {
        lol::ivec3 pos(in.byte(), f.m_reassign_ops.pop(), in.byte() + in.size());
        f.m_reassigns.push(pos);
    }
};

template<>
struct analyze_action<lua53::operator_notequal>
{
    static void apply(pegtl::action_input const &in, code_fixer &f)
    {
        /* XXX: Try to remove elements that are now invalid because
         * of backtracking. See https://github.com/ColinH/PEGTL/issues/32 */
        while (f.m_notequals.count() &&
               f.m_notequals.last() >= (int)in.byte())
            f.m_notequals.pop();
        f.m_notequals.push(in.byte());
    }
};

//
// Actions executed during translation
//

code_fixer::code_fixer(String const &code)
  : m_code(code)
{
    // Some versions of PICO-8 add this to the end of the cartridge; we have
    // to fix it to use the old syntax, but we also have to add a carriage
    // return before it, otherwise we may end up with invalid tokens such as
    // “endif” if there is no carriage return after the last “end”.
    static char const *invalid = "if(_update60)_update=function()";
    static char const *valid = "\nif(_update60)then _update=function()";

    int tofix = m_code.index_of(invalid);
    if (tofix != lol::INDEX_NONE)
    {
        m_code = m_code.sub(0, tofix) + valid + m_code.sub(tofix + strlen(invalid)) + " end";
    }
}

void code_fixer::bump(int offset, int delta)
{
    for (int &pos : m_notequals)
        if (pos >= offset)
            pos += delta;

    for (ivec3 &pos : m_reassigns)
    {
        if (pos[0] >= offset)
            pos[0] += delta;
        if (pos[1] >= offset)
            pos[1] += delta;
        if (pos[2] >= offset)
            pos[2] += delta;
    }
}

String code_fixer::fix()
{
    m_notequals.empty();
    m_reassigns.empty();

    msg::info("Checking grammar\n");
    pegtl::analyze< lua53::grammar >();
    msg::info("Checking code\n");
    pegtl::parse_string<lua53::grammar, analyze_action>(m_code.C(), "code", *this);
    msg::info("Code seems valid\n");

    String code = m_code;

    /* Fix a+=b → a=a+(b) etc. */
    for (ivec3 const &pos : m_reassigns)
    {
        ASSERT(code[pos[1]] == '+' ||
               code[pos[1]] == '-' ||
               code[pos[1]] == '*' ||
               code[pos[1]] == '/' ||
               code[pos[1]] == '%');
        ASSERT(code[pos[1] + 1] == '=');

#if 0
        String var = code.sub(pos[0], pos[1] - pos[0]);
        String op = code.sub(pos[1], 2);
        String arg = code.sub(pos[1] + 2, pos[2] - pos[1] - 2);
        msg::info("Reassignment %d/%d/%d: “%s”  “%s”  “%s”\n", pos[0], pos[1], pos[2], var.C(), op.C(), arg.C());
#endif

        /* 1. build the string ‘=a+(b)’ */
        String dst = "=" // FIXME: Lol Engine is missing +(char,String)
                   + code.sub(pos[0], pos[1] - pos[0])
                   + code[pos[1]]
                   + '(' + code.sub(pos[1] + 2, pos[2] - pos[1] - 2) + ')';

        /* 2. insert that string where ‘+=b’ is currently */
        code = code.sub(0, pos[1])
                 + dst
                 + code.sub(pos[2]);
        bump(pos[1] + 2, dst.count() - (pos[2] - pos[1]));
    }

    /* Fix != → ~= */
    for (int const &offset : m_notequals)
    {
        ASSERT(code[offset] == '!');
        code[offset] = '~';
    }

    return code;
}

} // namespace z8

