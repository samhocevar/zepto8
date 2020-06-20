## Syntax 

PICO-8 uses Lua 5.2, however through various feature extensions its syntax has become closer to Lua 5.3.

The following PICO-8 syntax is adapted from [the official Lua 5.3 manual](https://www.lua.org/manual/5.3/manual.html#9).

```ebnf
chunk ::= block

block ::= {stat} [retstat]

stat ::= ";" |
         varlist "=" explist |
         var compop exp |
         functioncall |
         "?" [explist] |
         label |
         "break" |
         "goto" Name |
         "do" block "end" |
         "while" exp "do" block "end" |
         "while" "(" exp ")" block |
         "repeat" block "until" exp |
         "if" exp "then" block {"elseif" exp "then" block} ["else" block] "end" |
         "if" "(" exp ")" block ["else" block] |
         "for" Name "=" exp "," exp ["," exp] "do" block "end" |
         "for" namelist "in" explist "do" block "end" |
         "function" funcname funcbody |
         "local" "function" Name funcbody |
         "local" namelist ["=" explist]

retstat ::= "return" [explist] [";"]

label ::= "::" Name "::"

funcname ::= Name {"." Name} [":" Name]

varlist ::= var {"," var}

var ::= Name | prefixexp "[" exp "]" | prefixexp "." Name

namelist ::= Name {"," Name}

explist ::= exp {"," exp}

exp ::= "nil" | "false" | "true" | Numeral | LiteralString | "..." | functiondef |
        prefixexp | tableconstructor | exp binop exp | unop exp

prefixexp ::= var | functioncall | "(" exp ")"

functioncall ::= prefixexp args | prefixexp ":" Name args

args ::= "(" [explist] ")" | tableconstructor | LiteralString

functiondef ::= "function" funcbody

funcbody ::= "(" [parlist] ")" block "end"

parlist ::= namelist ["," "..."] | "..."

tableconstructor ::= "{" [fieldlist] "}"

fieldlist ::= field {fieldsep field} [fieldsep]

field ::= "[" exp "]" "=" exp | Name "=" exp | exp

fieldsep ::= "," | ";"

binop ::= "+" | "-" | "*" | "/" | "^" | "%" | "\" | "^^" |
          "&" | "|" | ">>" | ">>>" | "<<" | ">><" | "<<>" | ".." |
          "<" | "<=" | ">" | ">=" | "==" | "~=" | "and" | "or"

compop ::= "+=" | "-=" | "*=" | "/=" | "^=" | "%=" | "\=" | "^^=" |
           "&=" | "|=" | ">>=" | ">>>=" | "<<=" | ">><=" | "<<>=" | "..="

unop ::= "-" | "not" | "#" | "~" | "@" | "%” | "$"
```
