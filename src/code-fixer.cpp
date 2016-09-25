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

namespace lua53
{

template<bool B>
struct disable_crlf
{
    using analyze_t = pegtl::success::analyze_t;

    template< pegtl::apply_mode A, template< typename ... > class Action, template< typename ... > class Control, typename Input >
    static bool match(Input &, z8::code_fixer &f)
    {
        f.m_disable_crlf += B ? 1 : -1;
        return true;
    }
};

struct sep
{
    using analyze_t = lua53::sep_normal::analyze_t;

    template< pegtl::apply_mode A, template< typename ... > class Action, template< typename ... > class Control, typename Input >
    static bool match(Input & in, z8::code_fixer &f)
    {
        if (f.m_disable_crlf > 0)
            return lua53::sep_horiz::match(in);

        return lua53::sep_normal::match<A, Action, Control>(in, f);
    }
};

} // namespace lua53

namespace z8
{

//
// Actions executed during analysis
//

template<typename R>
struct analyze_action : pegtl::nothing<R> {};

template<>
struct analyze_action<lua53::short_if_body>
{
    static void apply(pegtl::action_input const &in, code_fixer &f)
    {
#if 0
        msg::info("short_if_body at line %ld:%ld(%ld): %s\n", in.line(), in.byte_in_line(), in.byte(), in.string().c_str());
#endif
        lol::ivec2 pos(in.byte(), in.byte() + in.size());
        // FIXME: push_unique is a hack; we need to ensure single step
        f.m_short_ifs.push_unique(pos);
    }
};

template<>
struct analyze_action<lua53::reassign_op>
{
    static void apply(pegtl::action_input const &in, code_fixer &f)
    {
#if 0
        msg::info("reassign_op %ld:%ld(%ld): %s\n", in.line(), in.byte_in_line(), in.byte(), in.string().c_str());
#endif
        f.m_reassign_ops.push(in.byte());
    }
};

template<>
struct analyze_action<lua53::reassign_body>
{
    static void apply(pegtl::action_input const &in, code_fixer &f)
    {
        lol::ivec3 pos(in.byte(), f.m_reassign_ops.pop(), in.byte() + in.size());
        // FIXME: push_unique is a hack; we need to ensure single step
        f.m_reassigns.push_unique(pos);
    }
};

template<>
struct analyze_action<lua53::operator_notequal>
{
    static void apply(pegtl::action_input const &in, code_fixer &f)
    {
        /* XXX: Try to remove elements that are now invalid because of
         * backtracking (see https://github.com/ColinH/PEGTL/issues/32).
         * To be honest, it’s probably me who don’t understand how to
         * do this properly. */
        while (f.m_notequals.count() &&
               f.m_notequals.last() >= (int)in.byte())
            f.m_notequals.pop();
        f.m_notequals.push(in.byte());
    }
};

template<>
struct analyze_action<lua53::cpp_comment>
{
    static void apply(pegtl::action_input const &in, code_fixer &f)
    {
        /* FIXME: see above for why this loop */
        while (f.m_cpp_comments.count() &&
               f.m_cpp_comments.last() >= (int)in.byte())
            f.m_cpp_comments.pop();
        f.m_cpp_comments.push(in.byte());
    }
};

//
// Actions executed during translation
//

code_fixer::code_fixer(String const &code)
  : m_code(code)
{
}

void code_fixer::bump(int offset, int delta)
{
    for (int &pos : m_notequals)
        if (pos >= offset)
            pos += delta;

    for (int &pos : m_cpp_comments)
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

    for (ivec2 &pos : m_short_ifs)
    {
        if (pos[0] >= offset)
            pos[0] += delta;
        if (pos[1] >= offset)
            pos[1] += delta;
    }
}

String code_fixer::fix()
{
    String code = m_code;

    /* PNG carts have a “if(_update60)_update…” code snippet added by PICO-8
     * for backwards compatibility. But some buggy versions apparently miss
     * a carriage return or space, leading to syntax errors or maybe this
     * code being lost in a comment. */
    int index = code.last_index_of("if(_update60)_update");
    if (index > 0 && code[index] != '\n')
        code = code.sub(0, index) + '\n' + code.sub(index);

#if 0
    /* Some useful debugging code when tweaking the grammar */
    msg::info("Checking grammar\n");
    pegtl::analyze< lua53::grammar >();
    int y = 0;
    for (auto line : code.split('\n'))
        msg::info("%4d: %s\n", y++, line.C());
    msg::info("Checking code\n");
    pegtl::trace_string<lua53::grammar, analyze_action>(code.C(), "code", *this);
#endif

    m_notequals.empty();
    m_cpp_comments.empty();
    m_reassigns.empty();
    m_short_ifs.empty();
    pegtl::parse_string<lua53::grammar, analyze_action>(code.C(), "code", *this);

    /* Fix if(x)y → if(x)then y end */
    for (ivec2 const &pos : m_short_ifs)
    {
        code = code.sub(0, pos[0])
             + " then "
             + code.sub(pos[0], pos[1] - pos[0])
             + " end "
             + code.sub(pos[1]);

        bump(pos[0], 6);
        bump(pos[1], 5);
    }

    /* Fix != → ~= */
    for (int const &pos : m_notequals)
    {
        ASSERT(code[pos] == '!' && code[pos + 1] == '=',
               "invalid operator %c%c", code[pos], code[pos + 1]);
        code[pos] = '~';
    }

    /* Fix // → -- */
    for (int const &pos : m_cpp_comments)
    {
        ASSERT(code[pos] == '/' && code[pos + 1] == '/',
               "invalid operator %c%c", code[pos], code[pos + 1]);
        code[pos] = code[pos + 1] = '-';
    }

    /* Fix a+=b → a=a+(b) etc. */
    for (ivec3 const &pos : m_reassigns)
    {
        ASSERT(code[pos[1]] == '+' ||
               code[pos[1]] == '-' ||
               code[pos[1]] == '*' ||
               code[pos[1]] == '/' ||
               code[pos[1]] == '%',
               "invalid operator %c%c", code[pos[1]], code[pos[1] + 1]);
        ASSERT(code[pos[1] + 1] == '=',
               "invalid operator %c%c", code[pos[1]], code[pos[1] + 1]);

#if 0
        String var = code.sub(pos[0], pos[1] - pos[0]);
        String op = code.sub(pos[1], 2);
        String arg = code.sub(pos[1] + 2, pos[2] - pos[1] - 2);
        msg::info("Reassignment %d/%d/%d: “%s” “%s” “%s”\n", pos[0], pos[1], pos[2], var.C(), op.C(), arg.C());
#endif

        /* 1. build the string ‘=a+(b)’ */
        String dst = "=" // FIXME: Lol Engine is missing +(char,String)
                   + code.sub(pos[0], pos[1] - pos[0])
                   + code[pos[1]]
                   + '(' + code.sub(pos[1] + 2, pos[2] - pos[1] - 2) + ')';

        /* 2. insert that string where ‘+=b’ is currently */
        code = code.sub(0, pos[1]) + dst + code.sub(pos[2]);
        /* XXX: this bump will mess up what’s between pos[1] and pos[2] */
        bump(pos[1] + 2, dst.count() - (pos[2] - pos[1]));
    }

    return code;
}

} // namespace z8

