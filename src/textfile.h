//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2024 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#pragma once

#include "zepto8.h"

namespace z8
{

//
// Text File with a minimum delay between writes
//

class textfile
{
public:

    textfile();

    bool tick(bool force);
    bool read_save(std::string filepath, uint8_t* data);
    bool write_save(std::string filepath, uint8_t* data);
    bool read_config(std::string filepath, uint8_t* data);
    bool write_config(std::string filepath, uint8_t* data);

    void set_dirty() { m_is_dirty = true; }

private:
    bool m_is_dirty = false;
    int m_min_frames_between_saves = 640;
    int m_frames_since_last_save = 0;

};

} // namespace z8

