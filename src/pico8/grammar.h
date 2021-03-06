// Copyright (c) 2015-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_SRC_EXAMPLES_PEGTL_LUA53_HPP
#define TAO_PEGTL_SRC_EXAMPLES_PEGTL_LUA53_HPP

#include <lol/pegtl>
#include <lol/3rdparty/pegtl/include/tao/pegtl/contrib/analyze.hpp>
#include <lol/3rdparty/pegtl/include/tao/pegtl/contrib/raw_string.hpp>
#include <lol/3rdparty/pegtl/include/tao/pegtl/contrib/uint8.hpp>

#if WITH_PICO8
namespace z8::pico8
#else
namespace lua53
#endif
{
   // PEGTL grammar for the Lua 5.3.0 lexer and parser.
   //
   // The grammar here is not very similar to the grammar
   // in the Lua reference documentation on which it is based
   // which is due to multiple causes.
   //
   // The main difference is that this grammar includes really
   // "everything", not just the structural parts from the
   // reference documentation:
   // - The PEG-approach combines lexer and parser; this grammar
   //   handles comments and tokenisation.
   // - The operator precedence and associativity are reflected
   //   in the structure of this grammar.
   // - All details for all types of literals are included, with
   //   escape-sequences for literal strings, and long literals.
   //
   // The second necessary difference is that all left-recursion
   // had to be eliminated.
   //
   // In some places the grammar was optimised to require as little
   // back-tracking as possible, most prominently for expressions.
   // The original grammar contains the following production rules:
   //
   //   prefixexp ::= var | functioncall | ‘(’ exp ‘)’
   //   functioncall ::=  prefixexp args | prefixexp ‘:’ Name args
   //   var ::=  Name | prefixexp ‘[’ exp ‘]’ | prefixexp ‘.’ Name
   //
   // We need to eliminate the left-recursion, and we also want to
   // remove the ambiguity between function calls and variables,
   // i.e. the fact that we can have expressions like
   //
   //   ( a * b ).c()[ d ].e:f()
   //
   // where only the last element decides between function call and
   // variable, making it necessary to parse the whole thing again
   // if we chose wrong at the beginning.
   // First we eliminate prefixexp and obtain:
   //
   //   functioncall ::=  ( var | functioncall | ‘(’ exp ‘)’ ) ( args | ‘:’ Name args )
   //   var ::=  Name | ( var | functioncall | ‘(’ exp ‘)’ ) ( ‘[’ exp ‘]’ | ‘.’ Name )
   //
   // Next we split function_call and variable into a first part,
   // a "head", or how they can start, and a second part, the "tail",
   // which, in a sequence like above, is the final deciding part:
   //
   //   vartail ::= '[' exp ']' | '.' Name
   //   varhead ::= Name | '(' exp ')' vartail
   //   functail ::= args | ':' Name args
   //   funchead ::= Name | '(' exp ')'
   //
   // This allows us to rewrite var and function_call as follows.
   //
   //   var ::= varhead { { functail } vartail }
   //   function_call ::= funchead [ { vartail } functail ]
   //
   // Finally we can define a single expression that takes care
   // of var, function_call, and expressions in a bracket:
   //
   //   chead ::= '(' exp ')' | Name
   //   combined ::= chead { functail | vartail }
   //
   // Such a combined expression starts with a bracketed
   // expression or a name, and continues with an arbitrary
   // number of functail and/or vartail parts, all in a one
   // grammar rule without back-tracking.
   //
   // The rule expr_thirteen below implements "combined".
   //
   // Another issue of interest when writing a PEG is how to
   // manage the separators, the white-space and comments that
   // can occur in many places; in the classical two-stage
   // lexer-parser approach the lexer would have taken care of
   // this, but here we use the PEG approach that combines both.
   //
   // In the following grammar most rules adopt the convention
   // that they take care of "internal padding", i.e. spaces
   // and comments that can occur within the rule, but not
   // "external padding", i.e. they don't start or end with
   // a rule that "eats up" all extra padding (spaces and
   // comments). In some places, where it is more efficient,
   // right padding is used.

#if WITH_PICO8
   struct parse_state
   {
       // Allow to disable CRLF within a rule when > 0
       int nocrlf = 0;

       // Track leading blanks at each start of line
       size_t blank_line = 1, blank_byte = 1;

       // Count tokens
       int tokens = 0;
   };

   // This rule will toggle a special state where line breaks can be
   // forbidden within a rule.
   template<bool B> struct disable_crlf
   {
      using subs_t = tao::pegtl::type_list<>;

      template< tao::pegtl::apply_mode, tao::pegtl::rewind_mode,
                template< typename ... > class Action,
                template< typename ... > class Control,
                typename Input, typename... States >
      static bool match(Input &, parse_state &ps, States &&...)
      {
         ps.nocrlf += B ? 1 : -1;
         return true;
      }
   };

   // Convenience rule to match a sequence on a single line
   template< typename... R >
   struct one_line_seq : tao::pegtl::seq< disable_crlf< true >,
                                          tao::pegtl::sor< tao::pegtl::try_catch< tao::pegtl::seq< R... > >,
                                                           tao::pegtl::seq< disable_crlf< false >, tao::pegtl::failure > >,
                                          disable_crlf< false > > {};

   // Convenience rule to make sure the subtree is ignored from AST; useful with at<>
   template< typename... R > struct silent_at : tao::pegtl::at< R... > {};
#endif

   // clang-format off
   struct short_comment : tao::pegtl::until< tao::pegtl::eolf > {};
   struct long_string : tao::pegtl::raw_string< '[', '=', ']' > {};
#if WITH_PICO8
   // Emulate PICO-8 parsing “bugs”:
   //  - no support for raw string literals of level > 0, i.e. [[abc]] is
   //    valid but [=[abc]=] isn’t, so we don’t use tao::pegtl::raw_string for
   //    comments and roll out or own simpler implementation instead.
   //  - PICO-8 allows nesting raw string literals, despite these being
   //    explicitly disallowed by the Lua specification (“ends at the first
   //    closing long bracket”), so we capture nested raw strings, too.
   //  - any syntax error within a long comment will abort parsing and treat
   //    the first line as a normal comment line, so we don’t raise a general
   //    failure unlike tao::pegtl::raw_string.
   struct long_comment_open : tao::pegtl::string< '[', '[' > {};
   struct long_comment_close : tao::pegtl::string< ']', ']' > {};
   struct long_comment_string : tao::pegtl::seq< long_comment_open,
                                                 tao::pegtl::star< tao::pegtl::not_at< long_comment_close >,
                                                                   tao::pegtl::sor< long_comment_string, tao::pegtl::any > >,
                                                 long_comment_close > {};
   struct comment : tao::pegtl::disable< tao::pegtl::two< '-' >, tao::pegtl::sor< long_comment_string, short_comment > > {};
#else
   struct comment : tao::pegtl::disable< tao::pegtl::two< '-' >, tao::pegtl::sor< long_string, short_comment > > {};
#endif

#if WITH_PICO8
   // PICO-8 accepts “//” as a comment prefix, too, but doesn’t accept
   // multiline long strings in this variation
   struct cpp_comment : tao::pegtl::disable< tao::pegtl::two< '/' >, short_comment > {};

   // We need to be able to switch separator parsing to single-line
   struct sep_vertical : tao::pegtl::sor< tao::pegtl::one< '\r', '\n' >, comment, cpp_comment > {};
   struct sep_horizontal : tao::pegtl::seq< tao::pegtl::not_at< sep_vertical >, tao::pegtl::ascii::space > {};

   // Separator matching rule. Due to the context-aware nature of the PICO-8 grammar, this rule
   // is a bit complex. When disable_crlf<> is active, it only matches sep_horizontal. Otherwise
   // it will also try to match sep_vertical first.
   struct sep
   {
      using subs_t = tao::pegtl::type_list< sep_vertical, sep_horizontal >;

      template< tao::pegtl::apply_mode A, tao::pegtl::rewind_mode R,
                template< typename ... > class Action,
                template< typename ... > class Control,
                typename Input, typename... States >
      static bool match(Input &in, parse_state &ps, States &&...st)
      {
         if (ps.nocrlf == 0 && sep_vertical::match<A, R, Action, Control>(in, ps, st...))
         {
            ps.blank_byte = in.position().byte_in_line;
            ps.blank_line = in.position().line;
            return true;
         }

         auto byte = in.position().byte_in_line;
         auto line = in.position().line;
         bool ret = sep_horizontal::match<A, R, Action, Control>(in, ps, st...);
         if (ret && byte == ps.blank_byte && line == ps.blank_line)
         {
            ps.blank_byte = in.position().byte_in_line;
            ps.blank_line = in.position().line;
         }
         return ret;
      }
   };
#else
   struct sep : tao::pegtl::sor< tao::pegtl::ascii::space, comment > {};
#endif
   struct seps : tao::pegtl::star< sep > {};

   struct str_and : TAO_PEGTL_STRING( "and" ) {};
   struct str_break : TAO_PEGTL_STRING( "break" ) {};
   struct str_do : TAO_PEGTL_STRING( "do" ) {};
   struct str_else : TAO_PEGTL_STRING( "else" ) {};
   struct str_elseif : TAO_PEGTL_STRING( "elseif" ) {};
   struct str_end : TAO_PEGTL_STRING( "end" ) {};
   struct str_false : TAO_PEGTL_STRING( "false" ) {};
   struct str_for : TAO_PEGTL_STRING( "for" ) {};
   struct str_function : TAO_PEGTL_STRING( "function" ) {};
   struct str_goto : TAO_PEGTL_STRING( "goto" ) {};
   struct str_if : TAO_PEGTL_STRING( "if" ) {};
   struct str_in : TAO_PEGTL_STRING( "in" ) {};
   struct str_local : TAO_PEGTL_STRING( "local" ) {};
   struct str_nil : TAO_PEGTL_STRING( "nil" ) {};
   struct str_not : TAO_PEGTL_STRING( "not" ) {};
   struct str_or : TAO_PEGTL_STRING( "or" ) {};
   struct str_repeat : TAO_PEGTL_STRING( "repeat" ) {};
   struct str_return : TAO_PEGTL_STRING( "return" ) {};
   struct str_then : TAO_PEGTL_STRING( "then" ) {};
   struct str_true : TAO_PEGTL_STRING( "true" ) {};
   struct str_until : TAO_PEGTL_STRING( "until" ) {};
   struct str_while : TAO_PEGTL_STRING( "while" ) {};

   // Note that 'elseif' precedes 'else' in order to prevent only matching
   // the "else" part of an "elseif" and running into an error in the
   // 'keyword' rule.

   template< typename Key >
   struct key : tao::pegtl::seq< Key, tao::pegtl::not_at< tao::pegtl::identifier_other > > {};

   struct sor_keyword : tao::pegtl::sor< str_and, str_break, str_do, str_elseif, str_else, str_end, str_false, str_for, str_function, str_goto, str_if, str_in, str_local, str_nil, str_not, str_repeat, str_return, str_then, str_true, str_until, str_while > {};

   struct key_and : key< str_and > {};
   struct key_break : key< str_break > {};
   struct key_do : key< str_do > {};
   struct key_else : key< str_else > {};
   struct key_elseif : key< str_elseif > {};
   struct key_end : key< str_end > {};
   struct key_false : key< str_false > {};
   struct key_for : key< str_for > {};
   struct key_function : key< str_function > {};
   struct key_goto : key< str_goto > {};
   struct key_if : key< str_if > {};
   struct key_in : key< str_in > {};
   struct key_local : key< str_local > {};
   struct key_nil : key< str_nil > {};
   struct key_not : key< str_not > {};
   struct key_or : key< str_or > {};
   struct key_repeat : key< str_repeat > {};
   struct key_return : key< str_return > {};
   struct key_then : key< str_then > {};
   struct key_true : key< str_true > {};
   struct key_until : key< str_until > {};
   struct key_while : key< str_while > {};

   struct keyword : key< sor_keyword > {};

   template< typename R >
   struct pad : tao::pegtl::pad< R, sep > {};

#if WITH_PICO8
   using identifier_first = tao::pegtl::uint8::mask_ranges< 0xff, 0x10, 0x1f, 0x7f, 0xff, 'A', 'Z', 'a', 'z', '_', '_' >;
   using identifier_second = tao::pegtl::uint8::mask_ranges< 0xff, 0x10, 0x1f, 0x7f, 0xff, 'A', 'Z', 'a', 'z', '_', '_', '0', '9' >;
   using identifier = tao::pegtl::seq< identifier_first, tao::pegtl::star< identifier_second > >;
   struct name : tao::pegtl::seq< tao::pegtl::not_at< keyword >, identifier > {};
#else
   struct name : tao::pegtl::seq< tao::pegtl::not_at< keyword >, tao::pegtl::identifier > {};
#endif

   struct single : tao::pegtl::one< 'a', 'b', 'f', 'n', 'r', 't', 'v', '\\', '"', '\'', '0', '\n' > {};
   struct spaces : tao::pegtl::seq< tao::pegtl::one< 'z' >, tao::pegtl::star< tao::pegtl::space > > {};
   struct hexbyte : tao::pegtl::if_must< tao::pegtl::one< 'x' >, tao::pegtl::xdigit, tao::pegtl::xdigit > {};
   struct decbyte : tao::pegtl::if_must< tao::pegtl::digit, tao::pegtl::rep_opt< 2, tao::pegtl::digit > > {};
   struct unichar : tao::pegtl::if_must< tao::pegtl::one< 'u' >, tao::pegtl::one< '{' >, tao::pegtl::plus< tao::pegtl::xdigit >, tao::pegtl::one< '}' > > {};
   struct escaped : tao::pegtl::if_must< tao::pegtl::one< '\\' >, tao::pegtl::sor< hexbyte, decbyte, unichar, single, spaces > > {};
   struct regular : tao::pegtl::not_one< '\r', '\n' > {};
   struct character : tao::pegtl::sor< escaped, regular > {};

   template< char Q >
   struct short_string : tao::pegtl::if_must< tao::pegtl::one< Q >, tao::pegtl::until< tao::pegtl::one< Q >, character > > {};
   struct literal_string : tao::pegtl::sor< short_string< '"' >, short_string< '\'' >, long_string > {};

   template< typename E >
   struct exponent : tao::pegtl::opt_must< E, tao::pegtl::opt< tao::pegtl::one< '+', '-' > >, tao::pegtl::plus< tao::pegtl::digit > > {};

   template< typename D, typename E >
   struct numeral_three : tao::pegtl::seq< tao::pegtl::if_must< tao::pegtl::one< '.' >, tao::pegtl::plus< D > >, exponent< E > > {};
   template< typename D, typename E >
   struct numeral_two : tao::pegtl::seq< tao::pegtl::plus< D >, tao::pegtl::opt< tao::pegtl::one< '.' >, tao::pegtl::star< D > >, exponent< E > > {};
   template< typename D, typename E >
   struct numeral_one : tao::pegtl::sor< numeral_two< D, E >, numeral_three< D, E > > {};

   struct decimal : numeral_one< tao::pegtl::digit, tao::pegtl::one< 'e', 'E' > > {};
#if WITH_PICO8
   // PICO-8 does not support hexadecimal floats (with P exponent) but it supports binary digits
   struct hexadecimal : tao::pegtl::if_must< tao::pegtl::istring< '0', 'x' >, numeral_one< tao::pegtl::xdigit, tao::pegtl::failure > > {};
   struct binary : tao::pegtl::if_must< tao::pegtl::istring< '0', 'b' >, numeral_one< tao::pegtl::one< '0', '1' >, tao::pegtl::failure > > {};
   struct numeral : tao::pegtl::sor< hexadecimal, binary, decimal > {};
#else
   struct hexadecimal : tao::pegtl::if_must< tao::pegtl::istring< '0', 'x' >, numeral_one< tao::pegtl::xdigit, tao::pegtl::one< 'p', 'P' > > > {};
   struct numeral : tao::pegtl::sor< hexadecimal, decimal > {};
#endif

   struct label_statement : tao::pegtl::if_must< tao::pegtl::two< ':' >, seps, name, seps, tao::pegtl::two< ':' > > {};
   struct goto_statement : tao::pegtl::if_must< key_goto, seps, name > {};

   struct statement;
   struct expression;

   struct name_list : tao::pegtl::list< name, tao::pegtl::one< ',' >, sep > {};
   struct name_list_must : tao::pegtl::list_must< name, tao::pegtl::one< ',' >, sep > {};
   struct expr_list_must : tao::pegtl::list_must< expression, tao::pegtl::one< ',' >, sep > {};

   struct statement_return : tao::pegtl::seq< tao::pegtl::pad_opt< expr_list_must, sep >, tao::pegtl::opt< tao::pegtl::one< ';' >, seps > > {};

   template< typename E >
   struct statement_list : tao::pegtl::seq< seps, tao::pegtl::until< tao::pegtl::sor< E, tao::pegtl::if_must< key_return, statement_return, E > >, statement, seps > > {};

   template< char O, char... N >
   struct op_one : tao::pegtl::seq< tao::pegtl::one< O >, tao::pegtl::at< tao::pegtl::not_one< N... > > > {};
   template< char O, char P, char... N >
   struct op_two : tao::pegtl::seq< tao::pegtl::string< O, P >, tao::pegtl::at< tao::pegtl::not_one< N... > > > {};
#if WITH_PICO8
   template< char O, char P, char Q, char... N >
   struct op_three : tao::pegtl::seq< tao::pegtl::string< O, P, Q >, tao::pegtl::at< tao::pegtl::not_one< N... > > > {};
#endif

   struct table_field_one : tao::pegtl::if_must< tao::pegtl::one< '[' >, seps, expression, seps, tao::pegtl::one< ']' >, seps, tao::pegtl::one< '=' >, seps, expression > {};
   struct table_field_two : tao::pegtl::if_must< tao::pegtl::seq< name, seps, op_one< '=', '=' > >, seps, expression > {};
   struct table_field : tao::pegtl::sor< table_field_one, table_field_two, expression > {};
   struct table_field_list : tao::pegtl::list_tail< table_field, tao::pegtl::one< ',', ';' >, sep > {};
   struct table_constructor : tao::pegtl::if_must< tao::pegtl::one< '{' >, tao::pegtl::pad_opt< table_field_list, sep >, tao::pegtl::one< '}' > > {};

   struct parameter_list_one : tao::pegtl::seq< name_list, tao::pegtl::opt_must< pad< tao::pegtl::one< ',' > >, tao::pegtl::ellipsis > > {};
   struct parameter_list : tao::pegtl::sor< tao::pegtl::ellipsis, parameter_list_one > {};

   struct function_body : tao::pegtl::seq< tao::pegtl::one< '(' >, tao::pegtl::pad_opt< parameter_list, sep >, tao::pegtl::one< ')' >, seps, statement_list< key_end > > {};
   struct function_literal : tao::pegtl::if_must< key_function, seps, function_body > {};

   struct bracket_expr : tao::pegtl::if_must< tao::pegtl::one< '(' >, seps, expression, seps, tao::pegtl::one< ')' > > {};

   struct function_args_one : tao::pegtl::if_must< tao::pegtl::one< '(' >, tao::pegtl::pad_opt< expr_list_must, sep >, tao::pegtl::one< ')' > > {};
   struct function_args : tao::pegtl::sor< function_args_one, table_constructor, literal_string > {};

   struct variable_tail_one : tao::pegtl::if_must< tao::pegtl::one< '[' >, seps, expression, seps, tao::pegtl::one< ']' > > {};
   struct variable_tail_two : tao::pegtl::if_must< tao::pegtl::seq< tao::pegtl::not_at< tao::pegtl::two< '.' > >, tao::pegtl::one< '.' > >, seps, name > {};
   struct variable_tail : tao::pegtl::sor< variable_tail_one, variable_tail_two > {};

   struct function_call_tail_one : tao::pegtl::if_must< tao::pegtl::seq< tao::pegtl::not_at< tao::pegtl::two< ':' > >, tao::pegtl::one< ':' > >, seps, name, seps, function_args > {};
   struct function_call_tail : tao::pegtl::sor< function_args, function_call_tail_one > {};

   struct variable_head_one : tao::pegtl::seq< bracket_expr, seps, variable_tail > {};
   struct variable_head : tao::pegtl::sor< name, variable_head_one > {};

   struct function_call_head : tao::pegtl::sor< name, bracket_expr > {};

   struct variable : tao::pegtl::seq< variable_head, tao::pegtl::star< tao::pegtl::star< seps, function_call_tail >, seps, variable_tail > > {};
   struct function_call : tao::pegtl::seq< function_call_head, tao::pegtl::plus< tao::pegtl::until< tao::pegtl::seq< seps, function_call_tail >, seps, variable_tail > > > {};

   template< typename S, typename O >
   struct left_assoc : tao::pegtl::seq< S, seps, tao::pegtl::star_must< O, seps, S, seps > > {};
   template< typename S, typename O >
   struct right_assoc : tao::pegtl::seq< S, seps, tao::pegtl::opt_must< O, seps, right_assoc< S, O > > > {};

#if WITH_PICO8
   // Unary operators immediately followed by a numeral (without spaces) cost zero tokens, so we
   // need to treat them specially when counting tokens.
   struct free_unary_operators : tao::pegtl::seq< tao::pegtl::one< '-', '~' >, // “-” or “~”
                                                  silent_at< numeral > > {};
   struct unary_operators : tao::pegtl::sor< op_one< '-', '-', '=' >, // “-” but not “--” or “-=”
                                             op_one< '%', '=' >,      // “%” but not “%=”
                                             tao::pegtl::one< '@' >,  // “@”
                                             tao::pegtl::one< '$' >,  // “$”
#else
   struct unary_operators : tao::pegtl::sor< tao::pegtl::one< '-' >,
#endif
                                             tao::pegtl::one< '#' >,
                                             op_one< '~', '=' >,
                                             key_not > {};

   struct expr_ten;
   struct expr_thirteen : tao::pegtl::seq< tao::pegtl::sor< bracket_expr, name >, tao::pegtl::star< seps, tao::pegtl::sor< function_call_tail, variable_tail > > > {};
   struct expr_twelve : tao::pegtl::sor< key_nil,
                                         key_true,
                                         key_false,
                                         tao::pegtl::ellipsis,
                                         numeral,
                                         literal_string,
                                         function_literal,
                                         expr_thirteen,
                                         table_constructor > {};
#if WITH_PICO8
   // PICO-8 has extra operators such as “<<>” or “^^” so we need to
   // disambiguate more carefully when matching. Also all binary operators
   // have a compound assignment version, so avoid matching “=”.
   struct operators_eleven : op_one< '^', '^', '=' > {};
   struct expr_eleven : tao::pegtl::seq< expr_twelve, seps, tao::pegtl::opt< operators_eleven, seps, expr_ten, seps > > {};
   struct unary_apply : tao::pegtl::if_must< tao::pegtl::sor< unary_operators,
                                                              free_unary_operators >,
                                             seps, expr_ten, seps > {};
   struct expr_ten : tao::pegtl::sor< unary_apply, expr_eleven > {};
   struct operators_nine : tao::pegtl::sor< op_one< '/', '/', '=' >, // “/” but not “//” or “/=”
                                            op_one< '\\', '=' >,     // “\” but not “\=”
                                            op_one< '*', '=' >,      // “*” but not “*=”
                                            op_one< '%', '=' > > {}; // “%” but not “%=”
   struct expr_nine : left_assoc< expr_ten, operators_nine > {};
   struct operators_eight : tao::pegtl::sor< op_one< '-', '-', '=' >, // “-” but not “--” or “-=”
                                             op_one< '+', '=' > > {}; // “+” but not “+=”
   struct expr_eight : left_assoc< expr_nine, operators_eight > {};
   struct operators_seven : op_two< '.', '.', '.', '=' > {}; // “..” but not “...” or “..=”
   struct expr_seven : right_assoc< expr_eight, operators_seven > {};
   struct operators_six : tao::pegtl::sor< op_three< '<', '<', '>', '=' >, // “<<>” but not “<<>=”
                                           op_three< '>', '>', '<', '=' >, // “>><” but not “>><=”
                                           op_three< '>', '>', '>', '=' >, // “>>>” but not “>>>=”
                                           op_two< '<', '<', '=' >,        // “<<” but not “<<=”
                                           op_two< '>', '>', '=' > > {};   // “>>” but not “>>=”
   struct expr_six : left_assoc< expr_seven, operators_six > {};
   struct operators_five : op_one< '&', '=' > {}; // “&” but not “&=”
   struct expr_five : left_assoc< expr_six, operators_five > {};
   struct operators_four : op_two< '^', '^', '=' > {}; // “^^” but not “^^=”
   struct expr_four : left_assoc< expr_five, operators_four > {};
   struct operators_three : op_one< '|', '=' > {}; // “|” but not “|=”
   struct expr_three : left_assoc< expr_four, operators_three > {};
#else
   struct expr_eleven : tao::pegtl::seq< expr_twelve, seps, tao::pegtl::opt< tao::pegtl::one< '^' >, seps, expr_ten, seps > > {};
   struct unary_apply : tao::pegtl::if_must< unary_operators, seps, expr_ten, seps > {};
   struct expr_ten : tao::pegtl::sor< unary_apply, expr_eleven > {};
   struct operators_nine : tao::pegtl::sor< tao::pegtl::two< '/' >,
                                            tao::pegtl::one< '/' >,
                                            tao::pegtl::one< '*' >,
                                            tao::pegtl::one< '%' > > {};
   struct expr_nine : left_assoc< expr_ten, operators_nine > {};
   struct operators_eight : tao::pegtl::sor< tao::pegtl::one< '+' >,
                                             tao::pegtl::one< '-' > > {};
   struct expr_eight : left_assoc< expr_nine, operators_eight > {};
   struct expr_seven : right_assoc< expr_eight, op_two< '.', '.', '.' > > {};
   struct operators_six : tao::pegtl::sor< tao::pegtl::two< '<' >,
                                           tao::pegtl::two< '>' > > {};
   struct expr_six : left_assoc< expr_seven, operators_six > {};
   struct expr_five : left_assoc< expr_six, tao::pegtl::one< '&' > > {};
   struct expr_four : left_assoc< expr_five, op_one< '~', '=' > > {};
   struct expr_three : left_assoc< expr_four, tao::pegtl::one< '|' > > {};
#endif
   struct operators_two : tao::pegtl::sor< tao::pegtl::two< '=' >,
                                           tao::pegtl::string< '<', '=' >,
                                           tao::pegtl::string< '>', '=' >,
                                           op_one< '<', '<' >,
                                           op_one< '>', '>' >,
#if WITH_PICO8
                                           tao::pegtl::string< '!', '=' >, // “!=”
#endif
                                           tao::pegtl::string< '~', '=' > > {};
   struct expr_two : left_assoc< expr_three, operators_two > {};
   struct expr_one : left_assoc< expr_two, key_and > {};
   struct expression : left_assoc< expr_one, key_or > {};

   struct do_statement : tao::pegtl::if_must< key_do, statement_list< key_end > > {};
   struct while_statement : tao::pegtl::if_must< key_while, seps, expression, seps, key_do, statement_list< key_end > > {};
   struct repeat_statement : tao::pegtl::if_must< key_repeat, statement_list< key_until >, seps, expression > {};

   struct at_elseif_else_end : tao::pegtl::sor< tao::pegtl::at< key_elseif >, tao::pegtl::at< key_else >, tao::pegtl::at< key_end > > {};
   struct elseif_statement : tao::pegtl::if_must< key_elseif, seps, expression, seps, key_then, statement_list< at_elseif_else_end > > {};
   struct else_statement : tao::pegtl::if_must< key_else, statement_list< key_end > > {};
   struct if_statement : tao::pegtl::if_must< key_if, seps, expression, seps, key_then, statement_list< at_elseif_else_end >, seps, tao::pegtl::until< tao::pegtl::sor< else_statement, key_end >, elseif_statement, seps > > {};

#if WITH_PICO8
   // From the PICO-8 documentation:
   //
   // 1. IF THEN END statements on a single line can be expressed without the THEN & END
   //
   // IF (NOT B) I=1 J=2
   //
   // -- is equivalent to: IF (NOT B) THEN I=1 J=2 END
   // -- note that the condition must be surrounded by brackets.
   //
   // From Sam’s observations, this is valid:
   //  for i=0,9 do if(i>5) print(i) end
   //
   // But this is no longer valid as of 0.2.0i:
   //  for i=0,9 do if(i>5) for j=0,9 do if(j>5) print(j) end end
   //
   // Parsing strategy is to match “if”, verify that this is not a standard
   // “if-then”, temporarily disable CRLF, match “if (expression)”, then match
   // either:
   //  - a return statement
   //  - at least one statement followed by an optional return statement
   // Once the match is performed, we re-enable CRLF (even when rule failed).
   //
   // We also support “if (...) ... else” which does not do what the developer
   // thought, because “else” is ignored. Found in cartridge 14948 at least.
   // It is implemented by making short_if_tail optional after “else”.
   struct not_at_expr_then : tao::pegtl::not_at< tao::pegtl::seq< seps, expression, seps, key_then > > {};

   struct short_if_tail_two : tao::pegtl::if_must< key_return, statement_return > {};
   struct short_if_tail_one : tao::pegtl::seq< statement, seps,
                                               tao::pegtl::until< tao::pegtl::sor< tao::pegtl::eof, tao::pegtl::not_at< statement > >, statement, seps >,
                                               tao::pegtl::opt< short_if_tail_two > > {};
   struct short_if_tail : tao::pegtl::seq< seps, tao::pegtl::sor< short_if_tail_two, short_if_tail_one > > {};
   struct short_if_body : tao::pegtl::seq< short_if_tail, seps, tao::pegtl::opt< key_else, seps, tao::pegtl::opt< short_if_tail > > > {};

   struct short_if_statement : tao::pegtl::seq< key_if, not_at_expr_then,
                                                one_line_seq< seps, tao::pegtl::try_catch< bracket_expr >, seps, short_if_body > > {};

   struct short_while_body : tao::pegtl::seq< statement, seps,
                                              tao::pegtl::until< tao::pegtl::sor< tao::pegtl::eof, tao::pegtl::not_at< statement > >, statement, seps > > {};
   struct short_while_statement : tao::pegtl::seq< key_while,
                                                   one_line_seq< seps,
                                                                 tao::pegtl::try_catch< bracket_expr >,
                                                                 seps,
                                                                 short_while_body > > {};

#if 0
   // Undocumented feature: if (...) do
   //
   // This is actually a side effect of the short if statements; the code
   // gets translated to “if (...) then do end”, which simply needs to be
   // terminated by an endif somewhere.
   //
   // FIXME: rework this so as to not require backtracking
   struct if_do_trail : key_do {};
   struct if_do_statement : tao::pegtl::seq< key_if, not_at_expr_then,
                                             // The “do” must be at end of line or at a comment
                                             one_line_seq< seps, tao::pegtl::try_catch< bracket_expr >, seps, if_do_trail, seps, tao::pegtl::sor< tao::pegtl::at< comment >, tao::pegtl::eolf > >,
                                             // Same as the end of the actual “if” statement
                                             statement_list< at_elseif_else_end >, seps, tao::pegtl::until< tao::pegtl::sor< else_statement, key_end >, elseif_statement, seps > > {};
#endif

   // Undocumented feature: short print
   //
   // If a line starts with “?” then the rest of the line is used as arguments to the print()
   // function. The line can start with whitespace; struct at_sol takes care of that.
   struct at_sol
   {
      using subs_t = tao::pegtl::type_list<>;

      template< tao::pegtl::apply_mode, tao::pegtl::rewind_mode,
                template< typename ... > class Action,
                template< typename ... > class Control,
                typename Input, typename... States >
      static bool match(Input &in, parse_state &f, States &&...)
      {
         return f.blank_line == in.position().line
                 && f.blank_byte == in.position().byte_in_line;
      }
   };

   struct short_print : tao::pegtl::seq< at_sol,
                                         tao::pegtl::one< '?' >,
                                         one_line_seq< seps, tao::pegtl::try_catch< expr_list_must >, seps >,
                                         tao::pegtl::at< tao::pegtl::sor< tao::pegtl::eolf, comment, cpp_comment > > > {};
#endif

   struct for_statement_one : tao::pegtl::seq< tao::pegtl::one< '=' >, seps, expression, seps, tao::pegtl::one< ',' >, seps, expression, tao::pegtl::pad_opt< tao::pegtl::if_must< tao::pegtl::one< ',' >, seps, expression >, sep > > {};
   struct for_statement_two : tao::pegtl::seq< tao::pegtl::opt_must< tao::pegtl::one< ',' >, seps, name_list_must, seps >, key_in, seps, expr_list_must, seps > {};
   struct for_statement : tao::pegtl::if_must< key_for, seps, name, seps, tao::pegtl::sor< for_statement_one, for_statement_two >, key_do, statement_list< key_end > > {};

#if WITH_PICO8
   struct compound_var : variable {};
   struct compop : tao::pegtl::sor< tao::pegtl::string< '+', '=' >,
                                    tao::pegtl::string< '-', '=' >,
                                    tao::pegtl::string< '*', '=' >,
                                    tao::pegtl::string< '/', '=' >,
                                    tao::pegtl::string< '%', '=' >,
                                    tao::pegtl::string< '^', '=' >,
                                    tao::pegtl::string< '\\', '=' >,
                                    tao::pegtl::string< '&', '=' >,
                                    tao::pegtl::string< '|', '=' >,
                                    tao::pegtl::string< '^', '^', '=' >,
                                    tao::pegtl::string< '<', '<', '=' >,
                                    tao::pegtl::string< '>', '>', '=' >,
                                    tao::pegtl::string< '>', '>', '>', '=' >,
                                    tao::pegtl::string< '>', '>', '<', '=' >,
                                    tao::pegtl::string< '<', '<', '>', '=' >,
                                    tao::pegtl::string< '.', '.', '=' > > {};
   // We could use one_line_seq to try to emulate the PICO-8 parser: it seems
   // to first validate the syntax on a single line, but still allow multiline
   // code at runtime. For instance this is not valid:
   //   a += b +
   //        c
   // But this is valid because “a += b” is valid:
   //   a += b
   //        + c
   // However this is valid, too:
   //   a += (b
   //        + c)
   // There are also several other bugs that we used to try to tackle using a few hacks, for
   // instance eating a trailing comma, but I eventually decided against it. Most of them were
   // reported to https://www.lexaloffle.com/bbs/?tid=29750
   struct compound_body : tao::pegtl::seq< compound_var, seps, compop, seps, expression > {};
   struct compound_statement : tao::pegtl::seq< tao::pegtl::opt< key_local, seps >, compound_body > {};
#endif
   struct assignment_variable_list : tao::pegtl::list_must< variable, tao::pegtl::one< ',' >, sep > {};
   struct assignments_one : tao::pegtl::if_must< tao::pegtl::one< '=' >, seps, expr_list_must > {};
   struct assignments : tao::pegtl::seq< assignment_variable_list, seps, assignments_one > {};
   struct function_name : tao::pegtl::seq< tao::pegtl::list< name, tao::pegtl::one< '.' >, sep >, seps, tao::pegtl::opt_must< tao::pegtl::one< ':' >, seps, name, seps > > {};
   struct function_definition : tao::pegtl::if_must< key_function, seps, function_name, function_body > {};

   struct local_function : tao::pegtl::if_must< key_function, seps, name, seps, function_body > {};
   struct local_variables : tao::pegtl::if_must< name_list_must, seps, tao::pegtl::opt< assignments_one > > {};
   struct local_statement : tao::pegtl::if_must< key_local, seps, tao::pegtl::sor< local_function, local_variables > > {};

#if WITH_PICO8
   template< typename... R >
   using ensure = tao::pegtl::seq< silent_at< R... >, R... >;
#endif

   struct semicolon : tao::pegtl::one< ';' > {};
   struct statement : tao::pegtl::sor< semicolon,
#if WITH_PICO8
                                       // These three have conflicting prefixes
                                       ensure< assignments >,
                                       ensure< function_call >,
                                       ensure< compound_statement >,
                                       short_print,
#if 0
                                       if_do_statement,
#endif
                                       // These two conflict with the non-short versions
                                       ensure< short_if_statement >,
                                       ensure< short_while_statement >,
#else
                                       assignments,
                                       function_call,
#endif
                                       label_statement,
                                       key_break,
                                       goto_statement,
                                       do_statement,
                                       while_statement,
                                       repeat_statement,
                                       if_statement,
                                       for_statement,
                                       function_definition,
                                       local_statement > {};

#if WITH_PICO8
   // FIXME: we try to capture the first two comment lines, but what
   // to do if they are actually a multiline / long comment?
   struct header_comment : tao::pegtl::disable< tao::pegtl::two< '-' >, tao::pegtl::not_at< long_comment_string >, short_comment > {};
   struct grammar : tao::pegtl::must< tao::pegtl::opt< header_comment >,
                                      tao::pegtl::opt< header_comment >,
                                      statement_list< tao::pegtl::eof > > {};
#else
   struct grammar : tao::pegtl::must< tao::pegtl::opt< tao::pegtl::shebang >, statement_list< tao::pegtl::eof > > {};
#endif
   // clang-format on

}  // namespace lua53

#endif
