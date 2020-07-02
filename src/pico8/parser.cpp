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

#include <string>  // std::string
#include <iomanip> // std::setw

#include "pico8.h"
#define WITH_PICO8 1
#include "grammar.h"

using namespace tao;

namespace z8::pico8
{

template<typename R> static int token_counter = 0;

// Most keywords cost 1 token
template<> int token_counter<key_and>      = 1;
template<> int token_counter<key_break>    = 1;
template<> int token_counter<key_do>       = 1;
template<> int token_counter<key_else>     = 1;
template<> int token_counter<key_elseif>   = 1;
template<> int token_counter<key_end>      = 0; // “end” is free
template<> int token_counter<key_false>    = 1;
template<> int token_counter<key_for>      = 1;
template<> int token_counter<key_function> = 1;
template<> int token_counter<key_goto>     = 1;
template<> int token_counter<key_if>       = 1;
template<> int token_counter<key_in>       = 1;
template<> int token_counter<key_local>    = 0; // “local” is free
template<> int token_counter<key_nil>      = 1;
template<> int token_counter<key_not>      = 0; // “not” already appears in unary_operators
template<> int token_counter<key_or>       = 1;
template<> int token_counter<key_repeat>   = 1;
template<> int token_counter<key_return>   = 1;
template<> int token_counter<key_then>     = 1;
template<> int token_counter<key_true>     = 1;
template<> int token_counter<key_until>    = 1;
template<> int token_counter<key_while>    = 1;

// Most terminals cost 1 token
template<> int token_counter<name>           = 1;
template<> int token_counter<literal_string> = 1;
template<> int token_counter<numeral>        = 1;
template<> int token_counter<semicolon>      = 0;
template<> int token_counter<tao::pegtl::ellipsis> = 1;
template<> int token_counter<table_constructor> = 1; // the “{}”
template<> int token_counter<table_field_one>   = 2; // the “[]” and “=” in “[x]=1”
template<> int token_counter<table_field_two>   = 1; // the “=” in “x=1”
template<> int token_counter<function_body>     = 1; // the “()”
template<> int token_counter<bracket_expr>      = 1; // the “()”
template<> int token_counter<function_args_one> = 1; // the “()”
template<> int token_counter<variable_tail_one> = 1; // the “[]” in “a[b]”
template<> int token_counter<variable_tail_two> = 0; // the “.” in “a.b” is free
template<> int token_counter<function_call_tail_one> = 0; // the “:” in “a:b()” is free
template<> int token_counter<short_print>       = 1; // the “?” in “?12,3,5”
template<> int token_counter<for_statement_one> = 1; // the “=” in “for x=1,2 do end”
template<> int token_counter<for_statement_two> = 0; // no additional cost
template<> int token_counter<assignments_one>   = 1; // the “=” in “a,b,c = 2,3,4”

template<> int token_counter<operators_eleven> = 1; // “^”
template<> int token_counter<operators_nine>   = 1; // “/” “\” etc.
template<> int token_counter<operators_eight>  = 1; // “-” “+”
template<> int token_counter<operators_seven>  = 1; // “..”
template<> int token_counter<operators_six>    = 1; // “<<” “>>” etc.
template<> int token_counter<operators_five>   = 1; // “&”
template<> int token_counter<operators_four>   = 1; // “^^”
template<> int token_counter<operators_three>  = 1; // “|”
template<> int token_counter<operators_two>    = 1; // “==” “<=” etc.

template<> int token_counter<free_unary_operators> = -1; // “-2” counts as one, “- 2” counts as two
template<> int token_counter<unary_operators>      = 1;
template<> int token_counter<compound_operators>   = 1;

template<typename R> struct tokenise_action
{
    template<typename Input>
    static void apply(const Input &, parse_state &ps)
    {
        //if (token_counter<R>)
        //    printf(" +%d: %s\n", token_counter<R>, std::string(in.string()).c_str());
        ps.tokens += token_counter<R>;
    }
};

int code::count_tokens(std::string const &s)
{
    pegtl::string_input<> in(s, "p8");
    try
    {
        parse_state ps;
        pegtl::parse<grammar, tokenise_action>(in, ps);
        return ps.tokens;
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
    return -1;
}

} // namespace z8::pico8
