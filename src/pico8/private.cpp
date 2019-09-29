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
#include <codecvt>

#include "pico8/pico8.h"
#include "pico8/vm.h"

namespace z8::pico8
{

using lol::msg;

struct charset_data
{
    static charset_data const &get()
    {
        static charset_data data {};
        return data;
    }

    // Map UTF-32 codepoints to 8-bit PICO-8 characters
    std::map<char32_t, uint8_t> u32_to_pico8;

    // Map 8-bit PICO-8 characters to UTF-32 codepoints
    char32_t pico8_to_u32[256];

    // Map 8-bit PICO-8 characters to UTF-8 string views
    std::string_view pico8_to_u8[256];

private:
    charset_data()
    {
        char const *all_chars =
            "\0\1\2\3\4\5\6\a\b\t\n\v\f\r\16\17▮■□⁙⁘‖◀▶「」¥•、。゛゜"
            " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNO"
            "PQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~○"
            "█▒🐱⬇░✽●♥☉웃⌂⬅😐♪🅾◆…➡★⧗⬆ˇ∧❎▤▥あいうえおか"
            "きくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよ"
            "らりるれろわをんっゃゅょアイウエオカキクケコサシスセソタチツテト"
            "ナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲンッャュョ◜◝";

        std::mbstate_t state {};
        char const *p = all_chars;

        for (int i = 0; i < 256; ++i)
        {
            char32_t ch32;
            size_t len = i ? std::mbrtoc32(&ch32, p, 6, &state) : 1;
            pico8_to_u32[i] = i ? ch32 : 0;
            pico8_to_u8[i] = std::string_view(p, len);
            p += len;

            u32_to_pico8[ch32] = i;
        }
    }
};

std::string_view charset::p8_to_utf8(uint8_t ch)
{
    return charset_data::get().pico8_to_u8[ch];
}

char32_t charset::p8_to_utf32(uint8_t ch)
{
    return charset_data::get().pico8_to_u32[ch];
}

std::string charset::encode(std::string const &str)
{
    std::string ret;
    std::mbstate_t state {};
    char const *p = str.c_str(), *end = p + str.size() + 1;
    char32_t ch32;

    auto const &lut = charset_data::get().u32_to_pico8;

    while (size_t len = std::mbrtoc32(&ch32, p, end - p, &state))
    {
        auto ch = lut.find(ch32);
        if (len <= 6 && ch != lut.end())
        {
            ret += ch->second;
            p += len;
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

