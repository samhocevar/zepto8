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

    /* Convert from int16_t and to int */
    inline fix32(int16_t x) : m_bits(x << 16) {}
    inline operator int() const { return m_bits >> 16; }

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
        return frombits(((int64_t)m_bits * x.m_bits) >> 16);
    }

    fix32 operator /(fix32 x) const
    {
        /* XXX: PICO-8 returns 0x8000.0001 instead of 0x8000.0000 */
        if (x.m_bits == 0)
            return frombits(m_bits >= 0 ? 0x7ffffff : 0x80000001);
        return frombits(((int64_t)m_bits / x.m_bits) >> 16);
    }

    inline fix32& operator +=(fix32 x) { return *this = *this + x; }
    inline fix32& operator -=(fix32 x) { return *this = *this - x; }
    inline fix32& operator &=(fix32 x) { return *this = *this & x; }
    inline fix32& operator |=(fix32 x) { return *this = *this | x; }
    inline fix32& operator ^=(fix32 x) { return *this = *this ^ x; }
    inline fix32& operator *=(fix32 x) { return *this = *this * x; }
    inline fix32& operator /=(fix32 x) { return *this = *this / x; }

    /* Free functions */
    static fix32 abs(fix32 const &a) { return a.m_bits > 0 ? a : -a; }
    static fix32 min(fix32 const &a, fix32 const &b) { return a < b ? a : b; }
    static fix32 max(fix32 const &a, fix32 const &b) { return a > b ? a : b; }

    static fix32 ceil(fix32 const &x)
    {
        return frombits((x.m_bits + 0xffff) & 0xffff0000);
    }

    static fix32 floor(fix32 const &x)
    {
        return frombits(x.m_bits & 0xffff0000);
    }

private:
    int32_t m_bits;
};

};

