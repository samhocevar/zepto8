//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2017 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#include <lol/engine.h>

#include <tao/pegtl.hpp>

#include "analyzer.h"
#define WITH_PICO8 1
#include "lua53-parse.h"

using lol::String;
using lol::ivec2;
using lol::ivec3;
using lol::msg;

using namespace tao;

namespace lua53
{

// Special rule to disallow line breaks when matching special
// PICO-8 syntax (the one-line if(...)... construct)
template<bool B>
struct disable_crlf
{
    using analyze_t = pegtl::success::analyze_t;

    template< pegtl::apply_mode, pegtl::rewind_mode,
              template< typename ... > class Action,
              template< typename ... > class Control,
              typename Input >
    static bool match(Input &, z8::analyzer &f)
    {
        f.m_disable_crlf += B ? 1 : -1;
        return true;
    }
};

struct sep
{
    using analyze_t = sep_normal::analyze_t;

    template< pegtl::apply_mode A, pegtl::rewind_mode R,
              template< typename ... > class Action,
              template< typename ... > class Control,
              typename Input >
    static bool match(Input & in, z8::analyzer &f)
    {
        if (f.m_disable_crlf > 0)
            return sep_horiz::match(in);

        return sep_normal::match<A, R, Action, Control>(in, f);
    }
};

struct query_at_sol
{
    using analyze_t = query::analyze_t;

    template< pegtl::apply_mode, pegtl::rewind_mode,
              template< typename ... > class Action,
              template< typename ... > class Control,
              typename Input >
    static bool match(Input & in, z8::analyzer &)
    {
        return in.position().byte_in_line == 0 && query::match(in);
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
struct analyze_action<lua53::short_print>
{
    template<typename Input>
    static void apply(Input const &in, analyzer &f)
    {
        auto pos = in.position();
        lol::ivec2 tmp(pos.byte, pos.byte + in.size());
        f.m_short_prints.push(tmp);
#if 0
        msg::info("short_print at %ld:%ld(%ld): %s\n", pos.line, pos.byte_in_line, pos.byte, in.string().c_str());
#endif
    }
};

template<>
struct analyze_action<lua53::name>
{
    template<typename Input>
    static void apply(Input const &in, analyzer &)
    {
        UNUSED(in);
#if 0
        auto pos = in.position();
        msg::info("name at %ld:%ld(%ld): '%s'\n", pos.line, pos.byte_in_line, pos.byte, in.string().c_str());
#endif
    }
};

template<>
struct analyze_action<lua53::if_do_trail>
{
    template<typename Input>
    static void apply(Input const &in, analyzer &f)
    {
        auto pos = in.position();
        lol::ivec2 tmp(pos.byte, pos.byte + in.size());
        f.m_if_do_trails.push(tmp);
#if 0
        msg::info("if_do_trail at %ld:%ld(%ld): %s\n", pos.line, pos.byte_in_line, pos.byte, in.string().c_str());
#endif
    }
};

template<>
struct analyze_action<lua53::short_if_body>
{
    template<typename Input>
    static void apply(Input const &in, analyzer &f)
    {
        auto pos = in.position();
        lol::ivec2 tmp(pos.byte, pos.byte + in.size());
        f.m_short_ifs.push(tmp);
#if 0
        msg::info("short_if_body at %ld:%ld(%ld): %s\n", pos.line, pos.byte_in_line, pos.byte, in.string().c_str());
#endif
    }
};

template<>
struct analyze_action<lua53::reassign_op>
{
    template<typename Input>
    static void apply(Input const &in, analyzer &f)
    {
        auto pos = in.position();
        f.m_reassign_ops.push(pos.byte);
#if 0
        msg::info("reassign_op at %ld:%ld(%ld): %s\n", pos.line, pos.byte_in_line, pos.byte, in.string().c_str());
#endif
    }
};

template<>
struct analyze_action<lua53::reassign_body>
{
    template<typename Input>
    static void apply(Input const &in, analyzer &f)
    {
        auto pos = in.position();
        lol::ivec3 tmp(pos.byte, f.m_reassign_ops.pop(), pos.byte + in.size());
        f.m_reassigns.push(tmp);
#if 0
        msg::info("reassign_body at %ld:%ld(%ld): %s\n", pos.line, pos.byte_in_line, pos.byte, in.string().c_str());
#endif
    }
};

template<>
struct analyze_action<lua53::operator_notequal>
{
    template<typename Input>
    static void apply(Input const &in, analyzer &f)
    {
        f.m_notequals.push(in.position().byte);
    }
};

template<>
struct analyze_action<lua53::cpp_comment>
{
    template<typename Input>
    static void apply(Input const &in, analyzer &f)
    {
        auto pos = in.position();
#if 0
        msg::info("cpp comment at %ld:%ld(%ld): %s\n", pos.line, pos.byte_in_line, pos.byte, in.string().c_str());
#endif
        // We need push_unique because of backtracking; it’s not serious
        // enough for us to fix the grammar, so we don’t really care.
        f.m_cpp_comments.push_unique(pos.byte);
    }
};

//
// Actions executed during translation
//

analyzer::analyzer(String const &code)
  : m_code(code)
{
}

void analyzer::bump(int offset, int delta)
{
    for (int &pos : m_notequals)
        if (pos >= offset) pos += delta;

    for (int &pos : m_cpp_comments)
        if (pos >= offset) pos += delta;

    for (ivec2 &pos : m_if_do_trails)
    {
        if (pos[0] >= offset) pos[0] += delta;
        if (pos[1] >= offset) pos[1] += delta;
    }

    for (ivec3 &pos : m_reassigns)
    {
        if (pos[0] >= offset) pos[0] += delta;
        if (pos[1] >= offset) pos[1] += delta;
        if (pos[2] > offset) pos[2] += delta;
    }

    for (ivec2 &pos : m_short_prints)
    {
        if (pos[0] >= offset) pos[0] += delta;
        if (pos[1] > offset) pos[1] += delta;
    }

    for (ivec2 &pos : m_short_ifs)
    {
        if (pos[0] >= offset) pos[0] += delta;
        if (pos[1] > offset) pos[1] += delta;
    }
}

String analyzer::fix()
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
    m_short_prints.empty();
    m_short_ifs.empty();
    m_if_do_trails.empty();
    pegtl::memory_input<> in(code.C(), "code");
    pegtl::parse<lua53::grammar, analyze_action>(in, *this);

    /* Fix if(x)do → if(x)then do end */
    for (ivec2 const &pos : m_if_do_trails)
    {
        code = code.sub(0, pos[0])
             + "then "
             + code.sub(pos[0], pos[1] - pos[0])
             + " end"
             + code.sub(pos[1]);

        bump(pos[0], 5); // len("then ")
        bump(pos[1], 4); // len(" end")
    }

    /* Fix if(x)y → if(x)then y end */
    for (ivec2 const &pos : m_short_ifs)
    {
        code = code.sub(0, pos[0])
             + " then "
             + code.sub(pos[0], pos[1] - pos[0])
             + " end "
             + code.sub(pos[1]);

        bump(pos[0], 6); // len(" then ")
        bump(pos[1], 5); // len(" end ")
    }

    /* Fix ?… → print(…) */
    for (ivec2 const &pos : m_short_prints)
    {
        ASSERT(code[pos[0]] == '?')
        code = code.sub(0, pos[0])
             + "print("
             + code.sub(pos[0] + 1, pos[1] - pos[0] - 1)
             + ")"
             + code.sub(pos[1]);

        bump(pos[0], 5); // len("print(") - len("?")
        bump(pos[1], 1); // len(")")
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
               "invalid syntax %c%c", code[pos], code[pos + 1]);
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

        /* Handle the weird trailing comma bug as reported on
         * https://www.lexaloffle.com/bbs/?tid=29750 */
        int hack = code[pos[2] - 1] == ',';

        /* 1. build the string ‘=a+(b)’ */
        String dst = "=" // FIXME: Lol Engine is missing +(char,String)
                   + code.sub(pos[0], pos[1] - pos[0])
                   + code[pos[1]]
                   + '('
                   + code.sub(pos[1] + 2, pos[2] - pos[1] - 2 - hack)
                   + ')';

        /* 2. insert that string where ‘+=b’ is currently */
        code = code.sub(0, pos[1]) + dst + code.sub(pos[2]);
        /* XXX: this bump will mess up what’s between pos[1] and pos[2] */
        bump(pos[1] + 2, dst.count() - (pos[2] - pos[1]));
    }

    return code;
}

} // namespace z8

