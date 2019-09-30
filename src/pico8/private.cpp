//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2019 Sam Hocevar <sam@hocevar.net>
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

#include <lol/engine.h>

#include <locale>
#include <string>
#include <codecvt>
#include <cstring>

#include "pico8/pico8.h"
#include "pico8/vm.h"

namespace z8::pico8
{

using lol::msg;

std::string_view charset::pico8_to_utf8[256];
std::u32string_view charset::pico8_to_utf32[256];

// Map UTF-32 codepoints to 8-bit PICO-8 characters
std::map<char32_t, uint8_t> u32_to_pico8;

std::regex charset::match_utf8 = static_init();

std::regex charset::static_init()
{
    static std::u32string utf32_chars;
    static char const *utf8_chars =
        "\0\1\2\3\4\5\6\a\b\t\n\v\f\r\16\17▮■□⁙⁘‖◀▶「」¥•、。゛゜"
        " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNO"
        "PQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~○"
        "█▒🐱⬇️░✽●♥☉웃⌂⬅️😐♪🅾️◆…➡️★⧗⬆️ˇ∧❎▤▥あいうえおか"
        "きくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよ"
        "らりるれろわをんっゃゅょアイウエオカキクケコサシスセソタチツテト"
        "ナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲンッャュョ◜◝";

    utf32_chars.reserve(300); // for 256 chars + some U+FE0F selectors

    std::mbstate_t state {};
    char const *p = utf8_chars;

    for (int i = 0; i < 256; ++i)
    {
        char32_t ch32 = 0;
        size_t len = i ? std::mbrtoc32(&ch32, p, 6, &state) : 1;
        utf32_chars += ch32;
        if (ch32 == 0xfe0f)
        {
            // Oops! Previous char needed an emoji variation selector
            auto &s = pico8_to_utf8[--i];
            s = std::string_view(s.data(), s.length() + len);
            auto &s32 = pico8_to_utf32[i];
            s32 = std::u32string_view(s32.data(), 2);
        }
        else
        {
            pico8_to_utf32[i] = std::u32string_view(&utf32_chars.back(), 1);
            pico8_to_utf8[i] = std::string_view(p, len);
            u32_to_pico8[ch32] = i;
        }
        p += len;
    }

    std::string regex("^(\\0|");
    for (int i = 1; i < 256; ++i)
    {
        auto s = pico8_to_utf8[i];
        if (s.length() == 1 && ::strchr("^$\\.*+?()[]{}|", s[0]))
            regex += '\\';
        regex += s;
        regex += i == 255 ? ')' : '|';
    }

    return std::regex(regex);
}

std::string charset::encode(std::string const &str)
{
    std::string ret;

    for (auto p = str.begin(); p != str.end(); )
    {
        std::smatch(sm);
        if (std::regex_search(p, str.end(), sm, match_utf8))
        {
            ret += sm.str();
            p += sm.length();
        }
        else
        {
            ret += *p++;
        }
    }

    return ret;
}

void vm::private_stub(std::string str)
{
    msg::info("z8:stub:%s\n", str.c_str());
}

opt<bool> vm::private_cartdata(opt<std::string> str)
{
    // No argument given: we return whether there is data
    if (!str)
        return m_cartdata.size() > 0;

    if (!str->size())
    {
        // Empty argument given: get rid of cart data
        m_cartdata = "";
        return std::nullopt;
    }

    m_cartdata = *str;
    private_stub(lol::format("cartdata(\"%s\")", m_cartdata.c_str()));
    return false;
}

} // namespace z8::pico8

