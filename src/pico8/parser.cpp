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
#include <lol/msg>

#include <string>

#include "pico8.h"
#define WITH_PICO8 1
#include "grammar.h"

using lol::ivec2;
using lol::ivec3;

using namespace tao;

namespace z8::pico8
{

struct parse_state
{
    int m_disable_crlf = 0;
};

// Special rule to disallow line breaks when matching special
// PICO-8 syntax (the one-line if(...)... construct)
template<bool B>
struct disable_crlf
{
    template< pegtl::apply_mode, pegtl::rewind_mode,
              template< typename ... > class Action,
              template< typename ... > class Control,
              typename Input >
    static bool match(Input &, parse_state &f)
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
    static bool match(Input & in, parse_state &f)
    {
        if (f.m_disable_crlf > 0)
            return sep_horiz::match(in);

        return sep_normal::match<A, R, Action, Control>(in, f);
    }
};

struct at_sol
{
    template< pegtl::apply_mode, pegtl::rewind_mode,
              template< typename ... > class Action,
              template< typename ... > class Control,
              typename Input >
    static bool match(Input & in, parse_state &)
    {
        return in.position().byte_in_line == 1;
    }
};

bool code::parse(std::string const &s)
{
    parse_state p;
    pegtl::string_input<> in(s, "p8");
    try
    {
        pegtl::parse<grammar>(in, p);
    }
    catch (std::exception &e)
    {
        lol::msg::error("parse error: %s\n", e.what());
    }
    return true;
}

} // namespace z8::pico8
