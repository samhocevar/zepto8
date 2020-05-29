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

#include <lol/algo/suffix_array> // lol::suffix_array
#include <unordered_map> // std::unordered_map
#include <lol/msg> // lol::msg
#include <cstring> // std::memchr
#include <regex>   // std::regex
#include <vector>  // std::vector
#include <array>   // std::array

#if 0
#define TRACE(...) lol::msg::info(__VA_ARGS__)
#else
#define TRACE(...) (void)0
#endif

namespace z8::pico8
{

using lol::msg;

static std::string decompress_new(uint8_t const *input);
static std::string decompress_old(uint8_t const *input);
static std::vector<uint8_t> compress_new(std::string const &input);
static std::vector<uint8_t> compress_old(std::string const &input);

// Move to front structure
struct move_to_front
{
    move_to_front()
    {
        reset();
    }

    void reset()
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

std::vector<uint8_t> code::compress(std::string const &input,
                                    format fmt /* = format::pxa */)
{
    switch (fmt)
    {
        case format::old: return compress_old(input);
        case format::pxa: return compress_new(input);
        case format::best:
        default:
        {
            auto ret = compress_old(input);
            auto b = compress_new(input);
            if (b.size() < ret.size())
                ret = b;
            if (ret.size() <= input.length())
                return ret;
            [[fallthrough]];
        }
        case format::store:
        {
            std::vector<uint8_t> ret(input.begin(), input.end());
            ret.push_back(0);
            return ret;
        }
    }
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

    TRACE("# Size: %d (%04x)\n", int(compressed), int(compressed));

    while (ret.size() < length && pos < compressed * 8)
    {
        auto oldpos = pos;

        if (get_bits(1))
        {
            int nbits = 4;
            while (get_bits(1))
                ++nbits;
            int n = get_bits(nbits) + (1 << nbits) - 16;
            uint8_t ch = mtf.get(n);
            if (!ch)
                break;
            TRACE("%04x [%d] $%d\n", int(ret.size()), int(pos-oldpos), ch);
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

            TRACE("%04x [%d] %d@-%d\n", int(ret.size()), int(pos-oldpos), len, offset);
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
    auto put_bits = [&pos, &ret](size_t count, uint32_t n) -> void
    {
        for (size_t i = 0; i < count; ++i, ++pos)
        {
            if (ret.size() * 8 <= pos)
                ret.push_back(0);
            ret.back() |= ((n >> i) & 1) << (pos & 0x7);
        }
    };

    // Describe a transition between two characters; if length is 1 we emit
    // a single character, otherwise it is a back reference.
    struct node
    {
        // Here, cost is only for debugging purposes
        int length = 0, offset = 0, cost = 0;
        // Sum of costs leading to this node
        int weight = INT_MAX;
        // Next and previous nodes
        int next = -1, prev = -1;
    };

    // The transitions between characters is a directed acyclic graph with
    // weights equal to the cost in bits of each encoding. We can solve
    // single-source shortest path on it to find a very good solution to the
    // compression problem.
    std::vector<node> nodes(input.length() + 1);
    nodes[0].weight = 0;

    auto relax_node = [&nodes](size_t i, size_t len, int offset, int cost)
    {
        if (nodes[i].weight + cost < nodes[i + len].weight)
        {
            nodes[i + len].weight = nodes[i].weight + cost;
            nodes[i + len].prev = int(i);
            nodes[i + len].length = int(len);
            nodes[i + len].offset = offset;
            nodes[i + len].cost = cost;
        }
    };

    // Create suffix array, inverse suffix array, and LCP array for input
    auto sar = lol::suffix_array<>{ input };
    auto isar = std::vector<size_t>(sar.size());
    for (size_t i = 0; i < sar.size(); ++i)
        isar[sar.nth_element(i)] = i;
    auto lcp = std::vector<size_t>(sar.size());
    sar.longest_common_prefix_array(lcp.begin());

    move_to_front mtf;

    // First pass: estimate costs in bits for each node
    for (size_t i = 0; i < input.length(); ++i)
    {
        // Cost of emitting a single character. This is not the actual cost
        // because the MtF state depends on how we ended on this node of the
        // graph, but I can’t find a better way for now.
        int n = mtf.find(input[i]);
        int cost = 2 * compress_bits[n >> 4] - 2;
        mtf.get(n);
        relax_node(i, 1, -1, cost);

        // Cost of emitting a back reference. We find the current suffix in
        // the suffix array by using the inverse suffix array, and proceed to
        // scan in both directions for compatible suffixes as long as we have
        // at least 3 good characters in the suffix.
        size_t start = isar[i];

        for (auto forward : { 0, 1 })
        {
            size_t suffix = start;
            size_t current_len = std::max(lcp[suffix], lcp[suffix ? suffix - 1 : 0]);
            bool done1024 = false, done32 = false;

            while (suffix > 0 && suffix + 1 < sar.size())
            {
                suffix += forward ? 1 : -1;
                current_len = std::min(current_len, lcp[suffix - forward]);

                // Stop when the common suffix length becomes too small.
                // TODO: when current_len becomes significantly smaller than
                // its initial value, it may be time to exit the loop, because
                // the odds that we find a better back reference become too
                // small.
                if (current_len < 3)
                    break;

                // If we already tested 50 back references for this suffix,
                // stop there, otherwise we may use too much CPU.
                if (suffix + 50 < start || start + 50 < suffix)
                    break;

                size_t j = sar.nth_element(suffix);

                // Only look at valid back references
                if (j + 32768 < i || j >= i)
                    continue;

                int offset = int(i - j);
                int cost = 11;

                // If we already emitted one of these back reference lengths,
                // there is no need to do it again.
                if (offset > 1024)
                {
                    if (done1024)
                        continue;
                    done1024 = true;
                    cost = 20;
                }
                else if (offset > 32)
                {
                    if (done32)
                        continue;
                    done32 = done1024 = true;
                    cost = 16;
                }

                // We try to emit a back reference of size L, but also of
                // size L-1, L-2… down to L-7. This is O(1) and has been shown
                // to help in some edge cases. For instance two back references
                // of lengths 10 and 8 cost 35 bits, but lengths 9 and 9 cost
                // 32 bits, so the greedy approach is not always optimal.
                for (size_t len = current_len; len + 7 >= current_len && len >= 3; --len)
                    relax_node(i, len, offset, cost + int(len - 3) / 7 * 3);

                // We can’t do better than a reference of offset ≤ 32 because
                // no subsequent attempt can beat current_len.
                if (offset <= 32)
                    break;
            }
        }
    }

    // Link every predecessor to their successor in the transition array
    for (int i = int(input.length()); ; )
    {
        int prev = nodes[i].prev;
        if (prev < 0)
            break;
        nodes[prev].next = i;
        i = prev;
    }

    // Emit the final compression bitstream
    mtf.reset();

    for (int i = 0; nodes[i].next >= 0; i = nodes[i].next)
    {
        // The transition information is in the _next_ node
        auto const &t = nodes[nodes[i].next];

        if (t.length == 1)
        {
            int n = mtf.find(input[i]);
            int bits = compress_bits[n >> 4];
            TRACE("%04x [%d%+d] $%d\n", int(i), t.cost, 2 * bits - 2 - t.cost, input[i]);

            put_bits(bits - 2, ((1 << (bits - 3)) - 1));
            put_bits(bits, n - (1 << bits) + 16);
            mtf.get(n);
        }
        else
        {
            int len = t.length - 3;
            int off = t.offset - 1;
            TRACE("%04x [%d] %d@-%d\n", int(i), t.cost, t.length, t.offset);

            if (off < 32)
                put_bits(8, 0x6 | (off << 3));
            else if (off < 1024)
                put_bits(13, 0x2 | (off << 3));
            else
                put_bits(17, 0x0 | (off << 2));

            while (len >= 0)
            {
                put_bits(3, std::min(len, 7));
                len -= 7;
            }
        }
    }

    // Adjust compressed size
    size_t compressed = (pos + 7) / 8;
    ret[6] = uint8_t(compressed >> 8);
    ret[7] = uint8_t(compressed);

    TRACE("# Size: %d (%04x)\n", int(compressed), int(compressed));

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
            int k = 0, end = std::min((int)input.length() - j, 17);

            // XXX: official PICO-8 stops at i - j, despite being able
            // to support input.length() - j, it seems.
            end = std::min(end, i - j);

            while (k < end && input[j + k] == input[i + k])
                ++k;

            if (k >= best_len)
            {
                best_j = j;
                best_len = k;
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

