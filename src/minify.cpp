//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2018 Sam Hocevar <sam@hocevar.net>
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

#include <vector>
#include <iostream>
#include <streambuf>
#include <cstdint>
#include <cstdlib>
#include <regex>

#include "minify.h"

namespace z8
{

std::string minify(std::string const &input)
{
    std::regex re_spaces(" +");

    // Split input into lines
    std::vector<std::string> lines;
    std::regex re_crlf("\r*\n");
    std::copy(std::sregex_token_iterator(input.begin(), input.end(), re_crlf, -1),
              std::sregex_token_iterator(),
              std::back_inserter(lines));

    // Skip first three lines (they’re the PICO-8 cart header)
    lines.erase(lines.begin(), lines.begin() + 3);

    std::vector<std::tuple<std::string, std::string>> replaces;
    std::string result;

    // Process each line
    for (auto & line : lines)
    {
        // Parse special comments indicating possible replacements
        static std::regex re_replaces("^(.*--.*replaces: |.*)");
        static std::regex re_replace_pair(" *([^ ]+) *([^ ]+) *");
        std::vector<std::string> replaces_list;
        auto tmp = std::regex_replace(line, re_replaces, "");
        std::copy(std::sregex_token_iterator(tmp.begin(), tmp.end(), re_spaces, -1),
                  std::sregex_token_iterator(),
                  std::back_inserter(replaces_list));
        for (size_t i = 0; i + 1 < replaces_list.size(); i += 2)
            replaces.push_back(std::make_tuple(replaces_list[i], replaces_list[i + 1]));

        // Strip comments; if comment starts with "debug" then whole line is stripped
        static std::regex re_comment("(^.*-- *debug| *--).*");
        line = std::regex_replace(line, re_comment, "");

        // Ignore empty lines
        static std::regex re_empty("^ *$");
        if (std::regex_match(line, re_empty))
            continue;

        // Protect lines that contain +=, -= etc.
        static std::regex re_compound(".*[-+*/%]=.*");
        line = std::regex_replace(line, re_compound, "X $0 X");

        result += result.empty() ? line : " " + line;
    }

    // Rename all variables according to our rules
    for (auto & rep : replaces)
        result = std::regex_replace(result, std::regex("([^a-z_])(" + std::get<0>(rep) + ")([^a-z0-9_])"), "$1" + std::get<1>(rep) + "$3");

    // Strip multiple spaces
    result = std::regex_replace(result, re_spaces, " ");

    // Exploit Lua parsing rules
    result = std::regex_replace(result, std::regex("0 ([g-wyz])"), "0$1");
    result = std::regex_replace(result, std::regex("([1-9]) ([g-z])"), "$1$2");

    // Unprotect lines protected with X
    result = std::regex_replace(result, std::regex(" *X[ X]*"), " ");

    // Remove spaces before and after symbols
    result = std::regex_replace(result, std::regex(" *([[\\]<>(){}#+*%^/=:!~,-]) *"), "$1");

    return result;
}

} // namespace z8

