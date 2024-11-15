//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2017–2024 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#include "textfile.h"
#include <lol/file> // lol::file
#if __NX__
#include <nn/fs.h>
#include <nn/oe.h>
#elif !__SCE__
#include <filesystem>
#endif
#include <sstream> // std::stringstream

namespace z8
{

textfile::textfile()
{
    
}

bool textfile::tick(bool force)
{
    m_frames_since_last_save += 1;
    if (!force)
    {
        if (!m_is_dirty) return false;
        if (m_frames_since_last_save < m_min_frames_between_saves) return false;
    }
    m_frames_since_last_save = 0;
    m_is_dirty = false;

    return true;
}

bool textfile::read_save(std::string filepath, uint8_t* data)
{
    // todo: verify cartdata is not empty
    std::string s;
    if (!lol::file::read(filepath, s))
        return false;

    auto ss = std::stringstream(s);

    int j = 0;
    for (std::string line; std::getline(ss, line, '\n');)
    {
        if (j >= 8) break;
        for (int i = 0; i < 32; ++i)
        {
            std::string sub = line.substr(i * 2, 2);
            unsigned int x = std::stoul(sub, nullptr, 16);
            int gindex = i + j * 32;
            // pico 8 store the numbers in reverse order from ram
            int index = (gindex & ~0x3) + 3 - gindex % 4;
            data[index] = x & 0xff;
        }
        j++;
    }

    return true;
}

bool textfile::write_save(std::string filepath, uint8_t* data)
{
    std::string content;
    for (int i = 0; i < 256; ++i)
    {
        char hex[3];
        // pico 8 store the numbers in reverse order from ram
        int index = (i & ~0x3) + 3 - i % 4;
        std::snprintf(hex, sizeof(hex), "%02x", data[index]);
        content += hex;
        if (i % 32 == 31)
        {
            content += "\n";
        }
    }

    // todo: verify cartdata is not empty
    if (!lol::file::write(filepath, content))
        return false;

    return true;
}

} // namespace z8
