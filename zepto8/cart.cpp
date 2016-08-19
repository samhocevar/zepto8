//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#include <lol/engine.h>

#include "pegtl.hh"

#include "zepto8.h"
#include "cart.h"

namespace z8
{

using lol::array;
using lol::ivec2;
using lol::msg;
using lol::u8vec4;
using lol::PixelFormat;

void cart::load_png(char const *filename)
{
    // Open cartridge as PNG image
    lol::Image img;
    img.Load(filename);
    ivec2 size = img.GetSize();
    int count = size.x * size.y;
    m_data.resize(count);

    // Retrieve cartridge data from lower image bits
    u8vec4 const *pixels = img.Lock<PixelFormat::RGBA_8>();
    for (int n = 0; n < count; ++n)
    {
        u8vec4 p = pixels[n] * 64;
        m_data[n] = p.a + p.r / 4 + p.g / 16 + p.b / 64;
    }
    img.Unlock(pixels);

    // Retrieve code, with optional decompression
    int version = m_data[OFFSET_VERSION];
    msg::info("Found cartridge version %d\n", version);
    if (version == 0 || m_data[OFFSET_CODE] != ':'
                     || m_data[OFFSET_CODE + 1] != 'c'
                     || m_data[OFFSET_CODE + 2] != ':'
                     || m_data[OFFSET_CODE + 3] != '\0')
    {
        int length = 0;
        while (OFFSET_CODE + length < OFFSET_VERSION
                && m_data[OFFSET_CODE + length] != '\0')
            ++length;

        m_code.resize(length);
        memcpy(m_code.C(), m_data.data() + OFFSET_CODE, length);
        m_code[length] = '\0';
    }
    else if (version == 1 || version >= 5)
    {
        // Expected data length
        int length = m_data[OFFSET_CODE + 4] * 256
                   + m_data[OFFSET_CODE + 5];

        m_code.resize(0);
        for (int i = OFFSET_CODE + 8; i < m_data.count() && m_code.count() < length; ++i)
        {
            if(m_data[i] >= 0x3c)
            {
                int a = (m_data[i] - 0x3c) * 16 + (m_data[i + 1] & 0xf);
                int b = m_data[i + 1] / 16 + 2;
                while (b--)
                    m_code += m_code[m_code.count() - a];
                ++i;
            }
            else
            {
                static char const *lut = "#\n 0123456789abcdef"
                    "ghijklmnopqrstuvwxyz!#%(){}[]<>+=/*:;.,~_";

                m_code += m_data[i] ? lut[m_data[i]] : m_data[++i];
            }
        }
        msg::info("Expected %d bytes, got %d\n", length, (int)m_code.count());
    }

    m_version = version;

    // Dump code to stdout
    //msg::info("Cartridge code:\n");
    //printf("%s", m_code.C());
}

//
// A speacial parser object for the .p8 format
//

struct p8_reader
{
    //
    // Grammar rules
    //

    struct r_bom : pegtl::opt<pegtl_string_t("\xef\xbb\xbf")> {};

    struct r_lua : pegtl_string_t("__lua__") {};
    struct r_gfx : pegtl_string_t("__gfx__") {};
    struct r_gff : pegtl_string_t("__gff__") {};
    struct r_map : pegtl_string_t("__map__") {};
    struct r_sfx : pegtl_string_t("__sfx__") {};
    struct r_music : pegtl_string_t("__music__") {};

    struct r_section_name : pegtl::sor<r_lua,
                                       r_gfx,
                                       r_gff,
                                       r_map,
                                       r_sfx,
                                       r_music> {};
    struct r_section_line : pegtl::seq<r_section_name, pegtl::eol> {};

    struct r_data_line : pegtl::seq<pegtl::not_at<r_section_line>,
                                    pegtl::until<pegtl::eolf>> {};
    struct r_data : pegtl::star<r_data_line> {};

    struct r_section : pegtl::seq<r_section_line, r_data> {};
    struct r_version : pegtl::star<pegtl::digit> {};

    struct r_header: pegtl::seq<pegtl_string_t("pico-8 cartridge"), pegtl::until<pegtl::eol>,
                                pegtl_string_t("version "), r_version, pegtl::until<pegtl::eol>> {};
    struct r_file : pegtl::seq<r_bom, r_header, pegtl::star<r_section>, pegtl::eof> {};

    //
    // Grammar actions
    //

    template<typename R>
    struct action : pegtl::nothing<R> {};

    //
    // Parser state
    //

    int m_version;

    enum class section : int8_t
    {
        error = -1,
        header = 0,
        lua,
        gfx,
        gff,
        map,
        sfx,
        music,
    };

    section m_section;

    //
    // Actual reader
    //

    void parse(char const *str)
    {
        pegtl::parse_string<r_file, action>(str, "p8", *this);
    }
};

template<>
struct p8_reader::action<p8_reader::r_version>
{
    static void apply(pegtl::action_input const &in, p8_reader &r)
    {
        r.m_version = std::atoi(in.string().c_str());
    }
};

template<>
struct p8_reader::action<p8_reader::r_section_name>
{
    static void apply(pegtl::action_input const &in, p8_reader &r)
    {
        if (in.string().find("lua") != std::string::npos)
            r.m_section = section::lua;
        else if (in.string().find("gfx") != std::string::npos)
            r.m_section = section::gfx;
        else if (in.string().find("gff") != std::string::npos)
            r.m_section = section::gff;
        else if (in.string().find("map") != std::string::npos)
            r.m_section = section::map;
        else if (in.string().find("sfx") != std::string::npos)
            r.m_section = section::sfx;
        else if (in.string().find("music") != std::string::npos)
            r.m_section = section::music;
        else
            r.m_section = section::error;
    }
};

template<>
struct p8_reader::action<p8_reader::r_data>
{
    static void apply(pegtl::action_input const &in, p8_reader &r)
    {
        UNUSED(in, r);
        //printf("DATA: %s", in.string().c_str());
    }
};

void cart::load_p8(char const *filename)
{
    lol::File f;
    for (auto candidate : lol::System::GetPathList(filename))
    {
        f.Open(candidate, lol::FileAccess::Read);
        if (f.IsValid())
        {
            lol::String s = f.ReadString();
            f.Close();

            msg::debug("loaded file %s\n", candidate.C());

            p8_reader reader;
            reader.parse(s.C());

            break;
        }
    }
}

} // namespace z8

