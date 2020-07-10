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

#include <lol/pegtl>
#include <lol/3rdparty/pegtl/include/tao/pegtl/contrib/parse_tree.hpp>
#include <lol/3rdparty/pegtl/include/tao/pegtl/contrib/parse_tree_to_dot.hpp>
#include <lol/msg>

#include <string>  // std::string
#include <iomanip> // std::setw

#include "pico8.h"
#define WITH_PICO8 1
#include "grammar.h"

using namespace tao;

namespace z8::pico8
{

//
// Parse tree support
//

struct transform_expr : pegtl::parse_tree::apply< transform_expr >
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

using store_selector = pegtl::parse_tree::store_content::on<
    // others
    variable,
    keyword,
    name,
    numeral,
    bracket_expr,
    literal_string,
    unary_operators,
    compound_operators,
    operators_nine,
    operators_eight,
    operators_six,
    operators_two
>;

using transform_selector = transform_expr::on<
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
>;

using remove_selector = pegtl::parse_tree::remove_content::on<
    // statement :=
    assignments,
        assignment_variable_list,
        expr_list_must,
    compound_statement,
        // compound_operators,
    short_print,
#if 0
    if_do_statement,
#endif
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
    local_statement
>;

// The global AST selector
template<typename Rule>
struct ast_selector : pegtl::parse_tree::selector< Rule,
    store_selector, transform_selector, remove_selector > {};

// Special rule: discard a whole subtree
template< typename... Rule >
struct ast_selector< silent_at< Rule... > > : std::true_type
{
    template< typename Node, typename... States >
    static void transform( std::unique_ptr< Node >& n, States&&... )
    {
        n.reset();
    }
};

struct chunk {};

template<>
struct ast_selector< grammar > : std::true_type
{
    template< typename Node, typename... States >
    static void transform( std::unique_ptr< Node >& n, States&&... )
    {
        for (auto &c : n->children)
            c->template set_type<chunk>();
    }
};

std::string code::ast(std::string const &s)
{
    pegtl::string_input<> in(s, "p8");
    try
    {
        const auto root = pegtl::parse_tree::parse<grammar, ast_selector, pegtl::nothing>(in, parse_state{});
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
