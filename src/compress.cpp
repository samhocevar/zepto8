//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2017—2020 Sam Hocevar <sam@hocevar.net>
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

#include "compress.h"

#include <string>
#include <vector>
#include <iostream>
#include <streambuf>
#include <cstdint>
#include <cstdlib>
#include <regex>

extern "C" {
#define z_errmsg z_errmsg_gz8 // avoid conflicts with the real zlib
#define register /**/
#define adler32_gz8(...) 0 // not implemented
#include "gz8.h"
#include "zutil.h"
#include "zlib/deflate.c"
#include "zlib/trees.c"
#include "zlib.h"
extern z_const char * const z_errmsg[] = {};
#undef compress
}

namespace z8
{

std::string encode49(std::vector<uint8_t> const &v)
{
    char chr = '#';
    int const n = 28; // bits in a chunk
    int const p = 49; // alphabet size

    std::string ret;

    // Read all the data
    for (size_t pos = 0; pos < v.size() * 8; pos += n)
    {
        // Read a group of n bits
        uint64_t val = 0;
        for (size_t i = pos / 8; i <= (pos + n - 1) / 8 && i < v.size(); ++i)
            val |= (uint64_t)v[i] << ((i - pos / 8) * 8);
        val = (val >> (pos % 8)) & (((uint64_t)1 << n) - 1);

        // Convert those n bits to a string
        for (uint64_t k = p; k < 2 << n; k *= p)
        {
            ret += char(chr + val % p);
            val /= p;
        }
    }

    // Remove trailing zeroes
    while (ret.size() && ret.back() == chr)
        ret.erase(ret.end() - 1);

    return '"' + ret + '"';
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

