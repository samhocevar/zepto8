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

#include "pico8/pico8.h"
#include "pico8/vm.h"

namespace z8::pico8
{

using lol::msg;

std::string charset::decode(uint8_t ch)
{
    static char const *map[] =
    {
        "▮", "■", "□", "⁙", "⁘", "‖", "◀", "▶", "「", "」", "¥", "•", "、", "。", "゛", "゜",
        " ", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/",
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?",
        "@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
        "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\", "]", "^", "_",
        "`", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
        "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~", "○",
        "█", "▒", "🐱", "⬇", "░", "✽", "●", "♥", "☉", "웃", "⌂", "⬅", "😐", "♪", "🅾", "◆",
        "…", "➡", "★", "⧗", "⬆", "ˇ", "∧", "❎", "▤", "▥", "あ", "い", "う", "え", "お", "か",
        "き", "く", "け", "こ", "さ", "し", "す", "せ", "そ", "た", "ち", "つ", "て", "と", "な", "に",
        "ぬ", "ね", "の", "は", "ひ", "ふ", "へ", "ほ", "ま", "み", "む", "め", "も", "や", "ゆ", "よ",
        "ら", "り", "る", "れ", "ろ", "わ", "を", "ん", "っ", "ゃ", "ゅ", "ょ", "ア", "イ", "ウ", "エ",
        "オ", "カ", "キ", "ク", "ケ", "コ", "サ", "シ", "ス", "セ", "ソ", "タ", "チ", "ツ", "テ", "ト",
        "ナ", "ニ", "ヌ", "ネ", "ノ", "ハ", "ヒ", "フ", "ヘ", "ホ", "マ", "ミ", "ム", "メ", "モ", "ヤ",
        "ユ", "ヨ", "ラ", "リ", "ル", "レ", "ロ", "ワ", "ヲ", "ン", "ッ", "ャ", "ュ", "ョ", "◜", "◝",
    };

    return ch >= 16 ? map[ch - 16] : "";
}

uint8_t charset::encode(std::string const &str)
{
    return 0;
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

