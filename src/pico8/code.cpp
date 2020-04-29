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

#include "zepto8.h"
#include "pico8/cart.h"
#include "pico8/pico8.h"

#include <lol/engine.h>
#include <lol/pegtl>
#include <regex>

namespace z8::pico8
{

using lol::ivec2;
using lol::msg;
using lol::u8vec4;
using lol::PixelFormat;

static uint8_t const *compress_lut = nullptr;
static char const *decompress_lut = "\n 0123456789abcdefghijklmnopqrstuvwxyz!#%(){}[]<>+=/*:;.,~_";

std::string code::decompress(uint8_t const *input)
{
    std::string ret;

    // Retrieve code, with optional decompression
    if (input[0] == '\0' && input[1] == 'p' &&
        input[2] == 'x' && input[3] == 'a')
    {
        // Move to front structure, reversed
        std::vector<uint8_t> mtf(256);
        for (int i = 0; i < 256; ++i)
            mtf[i] = uint8_t(255 - i);

        int length = input[4] * 256 + input[5];
        int compressed = input[6] * 256 + input[7];
        assert(compressed <= int(sizeof(pico8::memory::code)));

        int offset = 8 * 8; // offset in bits
        auto get_bits = [&](int count) -> uint32_t
        {
            uint32_t ret = 0;
            for (int i = 0; i < count; ++i, ++offset)
                ret |= ((input[offset >> 3] >> (offset & 0x7)) & 0x1) << i;
            return ret;
        };

        while (int(ret.size()) < length && offset < compressed * 8)
        {
            if (get_bits(1))
            {
                int nbits = 4;
                while (get_bits(1))
                    ++nbits;
                int n = get_bits(nbits) + (1 << nbits) - 16;
                uint8_t ch = mtf[255 - n];
                mtf.erase(mtf.begin() + 255 - n);
                mtf.push_back(ch);
                ret.push_back(char(ch));
            }
            else
            {
                int nbits = get_bits(1) ? get_bits(1) ? 5 : 10 : 15;
                int offset = get_bits(nbits) + 1;

                int n, len = 3;
                do
                    len += (n = get_bits(3));
                while (n == 7);

                for (int i = 0; i < len; ++i)
                    ret.push_back(ret[ret.size() - offset]);
            }
        }
    }
    else if (input[0] == ':' && input[1] == 'c' &&
             input[2] == ':' && input[3] == '\0')
    {
        // Expected data length (including trailing zero)
        int length = input[4] * 256 + input[5];

        ret.resize(0);
        for (int i = 8; i < (int)sizeof(pico8::memory::code) && (int)ret.length() < length; ++i)
        {
            if (input[i] >= 0x3c)
            {
                int a = (input[i] - 0x3c) * 16 + (input[i + 1] & 0xf);
                int b = input[i + 1] / 16 + 2;
                if ((int)ret.length() >= a)
                    while (b--)
                        ret += ret[ret.length() - a];
                ++i;
            }
            else
            {
                ret += input[i] ? decompress_lut[input[i] - 1]
                                        : input[++i];
            }
        }

        if (length != (int)ret.length())
            msg::warn("expected %d code bytes, got %d\n", length, (int)ret.length());
    }
    else
    {
        int length = 0;
        while (length < (int)sizeof(pico8::memory::code) && input[length] != '\0')
            ++length;

        ret.resize(length);
        memcpy(&ret[0], &input, length);
    }

    // Remove possible trailing zeroes
    ret.resize(strlen(ret.c_str()));

    return ret;
}

} // namespace z8::pico8

