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

#include <lol/vector> // lol::ivec2
#include <lol/msg>    // lol::msg
#include <cstring>    // std::memchr
#include <regex>      // std::regex
#include <vector>     // std::vector
#include <array>      // std::array

namespace z8::pico8
{

using lol::ivec2;
using lol::msg;
using lol::u8vec4;

static std::string decompress_new(uint8_t const *input);
static std::string decompress_old(uint8_t const *input);
static std::vector<uint8_t> compress_new(std::string const &input);
static std::vector<uint8_t> compress_old(std::string const &input);

// Move to front structure
struct move_to_front
{
    move_to_front()
    {
        for (int n = 0; n < 256; ++n)
            state[n] = uint8_t(n);
    }

    // Get the nth byte and move it to front
    uint8_t get(int n)
    {
        std::rotate(state.begin(), state.begin() + n, state.begin() + n + 1);
        return state.front();
    }

    // Find index of a given character
    int find(uint8_t ch)
    {
        auto val = std::find(state.begin(), state.end(), ch);
        return int(std::distance(state.begin(), val));
    }

private:
    std::array<uint8_t, 256> state;
};

std::string code::decompress(uint8_t const *input)
{
    if (input[0] == '\0' && input[1] == 'p' &&
        input[2] == 'x' && input[3] == 'a')
        return decompress_new(input);

    if (input[0] == ':' && input[1] == 'c' &&
        input[2] == ':' && input[3] == '\0')
        return decompress_old(input);

    auto end = (uint8_t const *)std::memchr(input, '\0', sizeof(pico8::memory::code));
    auto len = end ? size_t(end - input) : sizeof(pico8::memory::code);
    return std::string((char const *)input, len);
}

std::vector<uint8_t> code::compress(std::string const &input)
{
    auto a = compress_old(input);
    auto b = compress_new(input);
    return a.size() < b.size() ? a : b;
}

static uint8_t const *compress_lut = nullptr;
static char const *decompress_lut = "\n 0123456789abcdefghijklmnopqrstuvwxyz!#%(){}[]<>+=/*:;.,~_";

static std::string decompress_new(uint8_t const *input)
{
    size_t length = input[4] * 256 + input[5];
    size_t compressed = input[6] * 256 + input[7];
    assert(compressed <= sizeof(pico8::memory::code));

    size_t pos = size_t(8) * 8; // stream position in bits
    auto get_bits = [&](size_t count) -> uint32_t
    {
        uint32_t n = 0;
        for (size_t i = 0; i < count; ++i, ++pos)
            n |= ((input[pos >> 3] >> (pos & 0x7)) & 0x1) << i;
        return n;
    };

    move_to_front mtf;
    std::string ret;

    while (ret.size() < length && pos < compressed * 8)
    {
        if (get_bits(1))
        {
            int nbits = 4;
            while (get_bits(1))
                ++nbits;
            int n = get_bits(nbits) + (1 << nbits) - 16;
            uint8_t ch = mtf.get(n);
            if (!ch)
                break;
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

    return ret;
}

static std::string decompress_old(uint8_t const *input)
{
    std::string ret;

    // Expected data length (including trailing zero)
    size_t length = input[4] * 256 + input[5];

    ret.resize(0);
    for (size_t i = 8; i < sizeof(pico8::memory::code) && ret.length() < length; ++i)
    {
        if (input[i] >= 0x3c)
        {
            size_t a = (input[i] - 0x3c) * 16 + (input[i + 1] & 0xf);
            size_t b = input[i + 1] / 16 + 2;
            if (ret.length() >= a)
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

    if (length != ret.length())
        msg::warn("expected %d code bytes, got %d\n", int(length), int(ret.length()));

    // Remove possible trailing zeroes
    ret.resize(strlen(ret.c_str()));

    // Some old PNG carts have a “if(_update60)_update…” code snippet added by
    // PICO-8 for backwards compatibility. But some buggy versions apparently
    // miss a carriage return or space, leading to syntax errors. Remove it.
    static std::regex junk("if(_update60)_update=function()_update60([)_update_buttons(]*)_update60()end$");
    return std::regex_replace(ret, junk, "");
}


static std::vector<uint8_t> compress_new(std::string const& input)
{
    static int compress_bits[16] =
    {
        // this[n/16] is the number of bits required to encode n
        4, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 8
    };

    std::vector<uint8_t> ret;

    ret.insert(ret.end(),
    {
        '\0', 'p', 'x', 'a',
        uint8_t(input.length() >> 8), uint8_t(input.length()),
        0, 0,
    });

    size_t pos = ret.size() * 8; // stream position in bits
    auto put_bits = [&](size_t count, uint32_t n) -> void
    {
        for (size_t i = 0; i < count; ++i, ++pos)
        {
            if (ret.size() * 8 <= pos)
                ret.push_back(0);
            ret.back() |= ((n >> i) & 1) << (pos & 0x7);
        }
    };

    move_to_front mtf;

    for (size_t i = 0; i < input.length(); ++i)
    {
        uint8_t ch = input[i];

        // Find char in mtf structure; cost for a single char
        // will be 2*bits-2
        int n = mtf.find(ch);
        int bits = compress_bits[n >> 4];

        // Look behind for possible patterns
        int best_off = 0, best_len = 1, best_cost = 100000;
        for (int j = std::max(int(i) - 32768, 0); j < i; ++j)
        {
            int end = int(input.length()) - j;

            for (int k = 0; ; ++k)
            {
                if (k >= end || input[j + k] != input[i + k])
                {
                    if (k > 0)
                    {
                        int offset = int(i - j);
                        int cost = (offset <= 32 ? 8 : offset <= 1024 ? 13 : 17)
                                 + ((k + 6) / 7);
                        // if (cost/k <= best_cost/best_len)
                        if (cost * best_len <= best_cost * k)
                        {
                            best_off = offset;
                            best_len = k;
                            best_cost = cost;
                        }
                    }
                    break;
                }
            }
        }

        // Choose between back reference and single char; it’s unfortunate
        // that we can’t use back references of length 1 or 2 because there
        // may be cases when we don’t want to pollute the mtf structure.
        if (best_len < 3 || (2 * bits - 2) * best_len < best_cost)
        {
            put_bits(bits - 2, ((1 << (bits - 3)) - 1));
            put_bits(bits, n - (1 << bits) + 16);
            mtf.get(n);
        }
        else
        {
            i += best_len - 1;
            best_len -= 3;
            best_off -= 1;

            if (best_off < 32)
                put_bits(8, 0x6 | (best_off << 3));
            else if (best_off < 1024)
                put_bits(13, 0x2 | (best_off << 3));
            else
                put_bits(17, 0x0 | (best_off << 2));

            while (best_len >= 0)
            {
                put_bits(3, std::min(best_len, 7));
                best_len -= 7;
            }
        }
    }

    // Adjust compressed size
    size_t compressed = (pos + 7) / 8;
    ret[6] = uint8_t(compressed >> 8);
    ret[7] = uint8_t(compressed);

    return ret;
}

static std::vector<uint8_t> compress_old(std::string const &input)
{
    std::vector<uint8_t> ret;

    ret.insert(ret.end(),
    {
        ':', 'c', ':', '\0',
        (uint8_t)(input.length() >> 8), (uint8_t)input.length(),
        0, 0 // FIXME: what is this?
    });

    // Ensure the compression LUT is initialised
    if (!compress_lut)
    {
        uint8_t *tmp = new uint8_t[256];
        memset(tmp, 0, 256);
        for (int i = 0; i < 0x3b; ++i)
            tmp[(uint8_t)decompress_lut[i]] = i + 1;
        compress_lut = tmp;
    }

    // FIXME: PICO-8 appears to be adding an implicit \n at the
    // end of the code, and ignoring it when compressing code. So
    // for the moment we write one char too many.
    for (int i = 0; i < (int)input.length(); ++i)
    {
        // Look behind for possible patterns
        int best_j = 0, best_len = 0;
        for (int j = std::max(i - 3135, 0); j < i; ++j)
        {
            int end = std::min((int)input.length() - j, 17);

            // XXX: official PICO-8 stops at i - j, despite being able
            // to support input.length() - j, it seems.
            end = std::min(end, i - j);

            for (int k = 0; ; ++k)
            {
                if (k >= end || input[j + k] != input[i + k])
                {
                    if (k >= best_len)
                    {
                        best_j = j;
                        best_len = k;
                    }
                    break;
                }
            }
        }

        uint8_t byte = (uint8_t)input[i];

        // If best length is 2, it may or may not be interesting to emit a back
        // reference. Most of the time, if the first character fits in a single
        // byte, we should emit it directly. And if the first character needs
        // to be escaped, we should always emit a back reference of length 2.
        //
        // However there is at least one suboptimal case with preexisting
        // sequences “ab”, “b-”, and “-c-”. Then the sequence “ab-c-” can be
        // encoded as “a” “b-” “c-” (5 bytes) or as “ab” “-c-” (4 bytes).
        // Clearly it is more interesting to encode “ab” as a two-byte back
        // reference. But since this case is statistically rare, we ignore
        // it and accept that compression could be more efficient.
        //
        // If it can be a relief, PICO-8 is a lot less good than us at this.
        if (compress_lut[byte] && best_len <= 2)
        {
            ret.push_back(compress_lut[byte]);
        }
        else if (best_len >= 2)
        {
            uint8_t a = 0x3c + (i - best_j) / 16;
            uint8_t b = ((i - best_j) & 0xf) + (best_len - 2) * 16;
            ret.insert(ret.end(), { a, b });
            i += best_len - 1;
        }
        else
        {
            ret.insert(ret.end(), { '\0', byte });
        }
    }

    return ret;
}

} // namespace z8::pico8

