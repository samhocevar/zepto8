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

#include <lol/vector>
#include <lol/pegtl>

#include <string>
#include <regex>

#include "analyzer.h"
#define WITH_PICO8 1
#include "lua53-parse.h"

using lol::ivec2;
using lol::ivec3;

using namespace tao;

namespace lua53
{

// Special rule to disallow line breaks when matching special
// PICO-8 syntax (the one-line if(...)... construct)
template<bool B>
struct disable_crlf
{
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

std::string analyzer::fix(std::string const &code)
{
    /* PNG carts have a “if(_update60)_update…” code snippet added by PICO-8
     * for backwards compatibility. But some buggy versions apparently miss
     * a carriage return or space, leading to syntax errors or maybe this
     * code being lost in a comment. */
    static std::regex pattern("if(_update60)_update=function()_update60()_update_buttons()_update60()end");
    return std::regex_replace(code, pattern, "");
}

} // namespace z8

