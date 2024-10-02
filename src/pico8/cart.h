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

#pragma once

#include <vector> // std::vector
#include <string> // std::string
#include <filesystem> // std::filesystem
#include "pico8/pico8.h"
#include "pico8/memory.h"

// The cart class
// ——————————————
// Represents a PICO-8 cartridge. Can load and unpack .p8 and .p8.png files, but also .lua
// and .js (from a PICO-8 web export). The VM can then load their content into memory.

namespace z8::pico8
{

class cart
{
public:
    cart()
    {}

    bool load(std::string const &filename);

    memory const &get_rom() const
    {
        return m_rom;
    }

    memory &get_rom()
    {
        return m_rom;
    }

    std::vector<uint8_t> &get_label()
    {
        return m_label;
    }

    std::string const &get_code() const
    {
        return m_code;
    }

    std::string const& get_filename() const
    {
        return m_filename;
    }

    std::string const& get_title() const
    {
        return m_title;
    }

    std::string const& get_author() const
    {
        return m_author;
    }


    bool has_file_changed() const
    {
#if !__NX__ && !__SCE__
        if (!m_file_time_init) return false;
        return m_file_time != std::filesystem::last_write_time(m_filename);
#else
        return false;
#endif
    }

    std::vector<uint8_t> get_compressed_code() const;
    std::vector<uint8_t> get_bin() const;
    bool save(std::string const& filename) const;
    void set_from_ram(memory const& ram, int in_dst, int in_src, int in_size);

    std::string preprocess_code() const;

private:
    bool load_png(std::string const &filename);
    bool load_p8(std::string const &filename);
    bool load_lua(std::string const &filename);
    bool load_js(std::string const &filename);

    bool save_p8(std::string const& filename) const;
    bool save_png(std::string const& filename) const;

    void set_bin(std::vector<uint8_t> const &data);
    void init_rom();
    void init_title();
    void init_filename(std::string filename);
    
    memory m_rom;
    std::vector<uint8_t> m_label;
    std::string m_code, m_lua;
    std::string m_filename;
    std::string m_title;
    std::string m_author;
    int m_version;

#if !__NX__ && !__SCE__
    bool m_file_time_init = false;
    std::filesystem::file_time_type m_file_time;
#endif
};

} // namespace z8::pico8

