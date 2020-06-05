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
#include <lol/3rdparty/pegtl/include/tao/pegtl/contrib/parse_tree.hpp>
#include <lol/3rdparty/pegtl/include/tao/pegtl/contrib/parse_tree_to_dot.hpp>
#include <lol/msg>

#include <string>  // std::string
#include <iomanip> // std::setw

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
    // Track leading blanks at each start of line
    size_t m_blank_line = 1, m_blank_byte = 1;
};

// Special rule to disallow line breaks when matching special
// PICO-8 syntax (the one-line if(...)... construct)
template<bool B>
struct disable_crlf
{
    using subs_t = pegtl::type_list<>;

    template< pegtl::apply_mode, pegtl::rewind_mode,
              template< typename ... > class Action,
              template< typename ... > class Control,
              typename Input, typename... States >
    static bool match(Input &, parse_state &f, States &&...)
    {
        f.m_disable_crlf += B ? 1 : -1;
        return true;
    }
};

struct sep
{
    using subs_t = pegtl::type_list< sep_horiz, sep_normal >;

    template< pegtl::apply_mode A, pegtl::rewind_mode R,
              template< typename ... > class Action,
              template< typename ... > class Control,
              typename Input, typename... States >
    static bool match(Input &in, parse_state &f, States &&...st)
    {
        if (f.m_disable_crlf > 0)
            return sep_horiz::match(in);

        auto byte = in.position().byte_in_line;
        auto line = in.position().line;
        bool ret = sep_normal::match<A, R, Action, Control>(in, f, st...);
        if (ret)
        {
            if ((byte == f.m_blank_byte && line == f.m_blank_line)
                 || line != in.position().line)
            {
                f.m_blank_byte = in.position().byte_in_line;
                f.m_blank_line = in.position().line;
            }
        }
        return ret;
    }
};

struct at_sol
{
    using subs_t = pegtl::type_list<>;

    template< pegtl::apply_mode, pegtl::rewind_mode,
              template< typename ... > class Action,
              template< typename ... > class Control,
              typename Input, typename... States >
    static bool match(Input &in, parse_state &f, States &&...)
    {
        return f.m_blank_line == in.position().line
                && f.m_blank_byte == in.position().byte_in_line;
    }
};

bool code::parse(std::string const &s)
{
    parse_state p;
    pegtl::string_input<> in(s, "p8");
    try
    {
        pegtl::parse<grammar>(in, p);
        return true;
    }
    catch (pegtl::parse_error const &e)
    {
        auto const pos = e.positions().front();
        std::cerr << e.what() << '\n'
                  << in.line_at(pos) << '\n'
                  << std::setw(pos.byte_in_line) << '^' << '\n';
    }
    catch (std::exception const &e)
    {
        lol::msg::error("parse error: %s\n", e.what());
    }
    return false;
}

//
// Parse tree support
//

struct transform_expr
  : pegtl::parse_tree::apply< transform_expr >
{
    template< typename Node, typename... States >
    static void transform( std::unique_ptr< Node >& n, States&&... )
    {
        // Collapse if only one child
        if (n->children.size() == 1)
        {
            n = std::move(n->children.back());
            return;
        }
    }
};

template<typename Rule>
struct selector : pegtl::parse_tree::selector<
    Rule,
    pegtl::parse_tree::store_content::on<
        // statement
        assignments,
            assignment_variable_list,
            expr_list_must,
        compound_statement,
            compound_op,
        short_print,
        if_do_statement,
        short_if_statement,
        short_while_statement,
        function_call,
        label_statement,
        key_break,
        goto_statement,
        do_statement,
        while_statement,
        repeat_statement,
        if_statement,
        for_statement,
        function_definition,
        local_statement,
        // others
        variable,
        keyword,
        name,
        numeral,
        bracket_expr,
        literal_string,
        unary_operators,
        operators_nine,
        operators_eight,
        operators_six,
        operators_two
        >,
    transform_expr::on<
        expr_eleven,
        expr_nine,
        expr_eight,
        expr_seven,
        expr_six,
        expr_five,
        expr_four,
        expr_three,
        expr_two,
        expr_one,
        expression
        >,
    pegtl::parse_tree::remove_content::on<
        > >
{
};

std::string code::ast(std::string const &s)
{
    parse_state p;
    pegtl::string_input<> in(s, "p8");
    try
    {
        const auto root = pegtl::parse_tree::parse<grammar, selector, pegtl::nothing>(in, p);
        pegtl::parse_tree::print_dot(std::cout, *root);
    }
    catch (pegtl::parse_error const &e)
    {
        auto const pos = e.positions().front();
        std::cerr << e.what() << '\n'
                  << in.line_at(pos) << '\n'
                  << std::setw(pos.byte_in_line) << '^' << '\n';
    }
    catch (std::exception const &e)
    {
        std::cerr << e.what() << '\n';
    }
    return "";
}

} // namespace z8::pico8
