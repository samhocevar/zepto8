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

// The whole PICO-8 charset (256 characters)
static std::string const allchars = '\0' + std::string(
    "\1\2\3\4\5\6\7\10\11\12\13\14\15\16\17▮■□⁙⁘‖◀▶「」¥•、。゛゜"
    " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNO"
    "PQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~○"
    "█▒🐱⬇░✽●♥☉웃⌂⬅😐♪🅾◆…➡★⧗⬆ˇ∧❎▤▥あいうえおか"
    "きくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよ"
    "らりるれろわをんっゃゅょアイウエオカキクケコサシスセソタチツテト"
    "ナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲンッャュョ◜◝");

// Map 8-bit PICO-8 characters to UTF-32 codepoints
std::u32string const charset::pico8_to_u32 =
    // The reinterpret_cast is required because of an old Visual Studio bug
    reinterpret_cast<std::u32string const &>(
        std::wstring_convert<std::codecvt_utf8<int32_t>, int32_t>().from_bytes(allchars));

// Map UTF-32 codepoints to 8-bit PICO-8 characters
std::map<char32_t, uint8_t> const charset::u32_to_pico8 = []()
{
    std::map<char32_t, uint8_t> ret;
    for (int n = 0; n < 256; ++n)
        ret[pico8_to_u32[n]] = n;
    return ret;
}();

std::string charset::decode(uint8_t ch)
{
    return "";
}

std::string charset::encode(std::string const &str)
{
    std::string ret;
    std::mbstate_t state {};
    char const *p = str.c_str(), *end = p + str.size() + 1;
    char32_t ch32;

    while (size_t len = std::mbrtoc32(&ch32, p, end - p, &state))
    {
        auto ch8 = u32_to_pico8.find(ch32);
        if (len < 6 && ch8 != u32_to_pico8.end())
        {
            ret += ch8->second;
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

