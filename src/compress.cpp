//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2017—2018 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#pragma once

#include "compress.h"

#include <string>
#include <vector>
#include <iostream>
#include <streambuf>
#include <cstdint>
#include <cstdlib>
#include <regex>

extern "C" {
#define register /**/
#define adler32(...) 0
#include "zutil.h"
#include "zlib/deflate.c"
#include "zlib/trees.c"
#include "zlib.h"
extern z_const char * const z_errmsg[] = {};
}

namespace z8
{

std::string encode59(std::vector<uint8_t> const &v)
{
    char const *chr = "\n,i])v+=e%1*c579}f#k<lmax>0q/42368ghjnprwyz!{:;.~_do t[sub(";
    int const n = 47;
    int const p = 59;

    std::string ret;

    // Read all the data
    for (size_t pos = 0; pos < v.size() * 8; pos += n)
    {
        // Read a group of 47 bits
        uint64_t val = 0;
        for (size_t i = pos / 8; i <= (pos + n - 1) / 8 && i < v.size(); ++i)
            val |= (uint64_t)v[i] << ((i - pos / 8) * 8);
        val = (val >> (pos % 8)) & (((uint64_t)1 << n) - 1);

        // Convert those 47 bits to a string
        for (int i = 0; i < 8; ++i)
        {
            ret += chr[val % p];
            val /= p;
        }
    }

    // Remove trailing newlines
    while (ret.size() && ret.back() == '\n')
        ret.erase(ret.end() - 1);

    // If string starts with \n we need to add an extra \n for Lua
    if (ret.size() && ret[0] == '\n')
        ret = '\n' + ret;

#if 1
    // Workaround for a PICO-8 bug that freezes everything… 10 chars wasted!
    // fixed in 1.1.12: https://www.lexaloffle.com/bbs/?tid=31673
    // This should not happen because we are inside a string and nothing
    // needs to be parsed, but apparently the PICO-8 parser starts parsing
    // stuff after "]]" even if inside "[=[".
    ret = std::regex_replace(ret, std::regex("]]\n"), "XXX_1");
    ret = std::regex_replace(ret, std::regex("]]"), "XXX_2");

    // Workaround for another bug that messes with the parser
    // reported for 1.1.11g: https://www.lexaloffle.com/bbs/?tid=32155
    ret = std::regex_replace(ret, std::regex("\\[\\[\\[\n"), "YYY_1");
    ret = std::regex_replace(ret, std::regex("\\[\\[\\["), "YYY_2");
    ret = std::regex_replace(ret, std::regex("\\[\\["), "YYY_3");

    // Do the replacements described above
    // Of course if the above workaround is used, we need to take care
    // of the "[[\n" sequences we may have created.
    ret = std::regex_replace(ret, std::regex("YYY_1"), "[]]..'[['..[[\n\n");
    ret = std::regex_replace(ret, std::regex("YYY_2"), "[]]..'[['..[[");
    ret = std::regex_replace(ret, std::regex("YYY_3"), "[]]..[[[");
    // The newlines after "']]'" here are also required to avoid another bug,
    // reported for 1.1.11g: https://www.lexaloffle.com/bbs/?tid=32148
    ret = std::regex_replace(ret, std::regex("XXX_1"), "]]..']]'\n..[[\n\n");
    ret = std::regex_replace(ret, std::regex("XXX_2"), "]]..']]'\n..[[");

    // And finally, we cannot end with "]".
    if (ret.back() == ']')
        return "[[" + ret + "]..']'";

    return "[[" + ret + "]]";
#else
    // If ]] appears in string we need to escape it with [=[...]=] and so on
    std::string prefix;

    while ((ret + "]").find("]" + prefix + "]") != std::string::npos)
        prefix += "=";

    return "[" + prefix + "[" + ret + "]" + prefix + "]";
#endif
}

std::vector<uint8_t> compress(std::vector<uint8_t> &input)
{
    // Prepare a vector twice as big... we don't really care.
    std::vector<uint8_t> output(input.size() * 2 + 10);

    z_stream zs = {};
    zs.zalloc = [](void *, unsigned int n, unsigned int m) -> void * { return new char[n * m]; };
    zs.zfree = [](void *, void *p) -> void { delete[] (char *)p; };
    zs.next_in = input.data();
    zs.next_out = output.data();
    zs.avail_in = (uInt)input.size();
    zs.avail_out = (uInt)output.size();
    
    deflateInit(&zs, Z_BEST_COMPRESSION);
    deflate(&zs, Z_FINISH);
    // Strip first 2 bytes (deflate header) and last 4 bytes (checksum)
    output = std::vector<uint8_t>(output.begin() + 2, output.begin() + zs.total_out - 4);
    deflateEnd(&zs);

    return output;
}

} // namespace z8

