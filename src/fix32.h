//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2017 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#pragma once

#include <lol/engine.h>

namespace z8
{

struct fix32
{
    inline fix32()
    {}

    /* Convert from/to double */
    inline fix32(double d)
      : m_bits((int32_t)std::round(std::ldexp(d, 16)))
    {}

    inline operator double() const
    {
        return std::ldexp((double)m_bits, -16);
    }

    /* Parse 0b... number */
    static fix32 parse_binary(char const *str)
    {
        ASSERT(str[0] == '0' && str[1] == 'b');

        uint32_t bits = 0;
        for (str = str + 2; *str != '\0' && *str != '.'; ++str)
        {
            if (*str == '1')
                bits = (bits << 1) | 0x10000;
            else if (*str == '0')
                bits <<= 1;
            else
                break;
        }

        if (*str == '.')
        {
            for (int i = 1; i <= 16 && str[i] != '\0'; ++i)
                if (str[i] == '1')
                    bits |= 0x10000 >> i;
                else if (str[i] != '0')
                    break;
        }

        return frombits(bits);
    }

    /* Conversions up to int16_t are allowed */
    inline fix32(int8_t x) : m_bits(x << 16) {}
    inline fix32(uint8_t x) : m_bits(x << 16) {}
    inline fix32(int16_t x) : m_bits(x << 16) {}

    /* Anything above int16_t is disallowed because of precision loss */
    inline fix32(uint16_t x) = delete;
    inline fix32(int32_t x) = delete;
    inline fix32(uint32_t x) = delete;

    /* Explicit casts are all allowed */
    inline operator int8_t() const { return m_bits >> 16; }
    inline operator uint8_t() const { return m_bits >> 16; }
    inline operator int16_t() const { return m_bits >> 16; }
    inline operator uint16_t() const { return m_bits >> 16; }
    inline operator int32_t() const { return m_bits >> 16; }
    inline operator uint32_t() const { return m_bits >> 16; }
    inline operator int64_t() const { return m_bits >> 16; }
    inline operator uint64_t() const { return m_bits >> 16; }

    /* Directly initialise bits */
    static inline fix32 frombits(int32_t x)
    {
        fix32 ret; ret.m_bits = x; return ret;
    }

    inline int32_t bits() const { return m_bits; }

    /* Comparisons */
    bool operator ==(fix32 x) const { return m_bits == x.m_bits; }
    bool operator !=(fix32 x) const { return m_bits != x.m_bits; }
    bool operator  <(fix32 x) const { return m_bits  < x.m_bits; }
    bool operator  >(fix32 x) const { return m_bits  > x.m_bits; }
    bool operator <=(fix32 x) const { return m_bits <= x.m_bits; }
    bool operator >=(fix32 x) const { return m_bits >= x.m_bits; }

    /* Increments */
    fix32& operator ++() { m_bits += 0x10000; return *this; }
    fix32& operator --() { m_bits -= 0x10000; return *this; }
    fix32 operator ++(int) { fix32 ret = *this; ++*this; return ret; }
    fix32 operator --(int) { fix32 ret = *this; --*this; return ret; }

    /* Math operations */
    fix32 const &operator +() const { return *this; }
    fix32 operator -() const { return frombits(-m_bits); }
    fix32 operator ~() const { return frombits(~m_bits); }

    fix32 operator +(fix32 x) const { return frombits(m_bits + x.m_bits); }
    fix32 operator -(fix32 x) const { return frombits(m_bits - x.m_bits); }
    fix32 operator &(fix32 x) const { return frombits(m_bits & x.m_bits); }
    fix32 operator |(fix32 x) const { return frombits(m_bits | x.m_bits); }
    fix32 operator ^(fix32 x) const { return frombits(m_bits ^ x.m_bits); }

    fix32 operator *(fix32 x) const
    {
        return frombits((int64_t)m_bits * x.m_bits / 0x10000);
    }

    fix32 operator /(fix32 x) const
    {
        /* XXX: PICO-8 returns 0x8000.0001 instead of 0x8000.0000 */
        if (x.m_bits == 0)
            return frombits(m_bits >= 0 ? 0x7ffffff : 0x80000001);
        return frombits((int64_t)m_bits * 0x10000 / x.m_bits);
    }

    inline fix32& operator +=(fix32 x) { return *this = *this + x; }
    inline fix32& operator -=(fix32 x) { return *this = *this - x; }
    inline fix32& operator &=(fix32 x) { return *this = *this & x; }
    inline fix32& operator |=(fix32 x) { return *this = *this | x; }
    inline fix32& operator ^=(fix32 x) { return *this = *this ^ x; }
    inline fix32& operator *=(fix32 x) { return *this = *this * x; }
    inline fix32& operator /=(fix32 x) { return *this = *this / x; }

    /* Free functions */
    static fix32 abs(fix32 a) { return a.m_bits > 0 ? a : -a; }
    static fix32 min(fix32 a, fix32 b) { return a < b ? a : b; }
    static fix32 max(fix32 a, fix32 b) { return a > b ? a : b; }

    static fix32 ceil(fix32 x) { return -floor(-x); }
    static fix32 floor(fix32 x) { return frombits(x.m_bits & 0xffff0000); }

private:
    int32_t m_bits;
};

};

