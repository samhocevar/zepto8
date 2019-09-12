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

#pragma once

#include <lol/engine.h>

// Font dump code:
//
//  palset(1,255,255,255);
//  var t = [], s = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.,!()-";
//  for (c in s)
//  {
//      cls();
//      print(s[c]==","?1:0,0,s[c],1);
//      p = 0;
//      for (x = 0; x < 4; ++x)
//      for (y = 0; y < 8; ++y)
//          p += (pget(x,y) << y) << (x * 8);
//      t[c] = p;
//  }
//  
//  cls();
//  for (c in s) print(c % 4 * 32, flr(c / 4) * 7, t[c].toString(16),1);
//

namespace z8::raccoon
{

uint32_t font_data[] =
{
    // 0x20…0x2f: !(),-.
    0, 0x17, 0, 0, 0, 0, 0, 0, 0x211e, 0x1e21, 0, 0, 0x1020, 0x80808, 0x10, 0,
    // 0x30…0x3f: 0…9
    0x1f111f, 0x101f12, 0x17151d, 0x1f1511, 0x1f0407, 0x1d1517, 0x1d151f,
    0x1f0101, 0x1f151f, 0x1f1517, 0, 0, 0, 0, 0, 0,
    // 0x40…0x5f: A…Z
    0,
    0x1e051e,  0xa151f, 0x11110e,  0xe111f, 0x11151f,  0x1051f, 0x19110e,
    0x1f041f, 0x111f11,  0x11f11, 0x1b041f, 0x10101f, 0x1f021f, 0x1e011f,
     0xe110e,  0x2051f, 0x16190e, 0x1a051f,  0x91512,  0x11f01,  0xf100f,
     0xf180f, 0x1f018f, 0x1b041b,  0x71c07, 0x131519,
    0, 0, 0, 0, 0,
    // 0x60…0x7f: a…z
    0,
    0x1c1418,  0x8141f, 0x141408, 0x1f1408,   0x140c,    0x51e, 0x385408,
    0x18041f,     0x1d,   0x3d40, 0x14081f,     0x1f, 0x1c041c, 0x18041c,
     0x81408,  0x8147c, 0x7c1408,    0x418,  0x41c10,   0x120f,  0xc100c,
     0xc180c, 0x1c101c, 0x140814, 0x3c500c, 0x101c04,
    0, 0, 0, 0, 0,
};

};

