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

bool cart::load(char const *filename)
{
    if (load_p8(filename) || load_png(filename))
    {
        // Dump code to stdout
        //msg::info("Cartridge code:\n");
        //printf("%s", m_code.C());

        return true;
    }

    return false;
}

bool cart::load_png(char const *filename)
{
    // Open cartridge as PNG image
    lol::Image img;
    img.Load(filename);
    ivec2 size = img.GetSize();
    int count = size.x * size.y;
    m_rom.resize(count);

    // Retrieve cartridge data from lower image bits
    u8vec4 const *pixels = img.Lock<PixelFormat::RGBA_8>();
    for (int n = 0; n < count; ++n)
    {
        u8vec4 p = pixels[n] * 64;
        m_rom[n] = p.a + p.r / 4 + p.g / 16 + p.b / 64;
    }
    img.Unlock(pixels);

    // Retrieve code, with optional decompression
    int version = m_rom[SIZE_MEMORY];
    msg::info("Found cartridge version %d\n", version);
    if (version == 0 || m_rom[OFFSET_CODE] != ':'
                     || m_rom[OFFSET_CODE + 1] != 'c'
                     || m_rom[OFFSET_CODE + 2] != ':'
                     || m_rom[OFFSET_CODE + 3] != '\0')
    {
        int length = 0;
        while (OFFSET_CODE + length < SIZE_MEMORY
                && m_rom[OFFSET_CODE + length] != '\0')
            ++length;

        m_code.resize(length);
        memcpy(m_code.C(), m_rom.data() + OFFSET_CODE, length);
        m_code[length] = '\0';
    }
    else if (version == 1 || version >= 5)
    {
        // Expected data length (including trailing zero)
        int length = m_rom[OFFSET_CODE + 4] * 256
                   + m_rom[OFFSET_CODE + 5];

        m_code.resize(0);
        for (int i = OFFSET_CODE + 8; i < m_rom.count() && m_code.count() < length; ++i)
        {
            if(m_rom[i] >= 0x3c)
            {
                int a = (m_rom[i] - 0x3c) * 16 + (m_rom[i + 1] & 0xf);
                int b = m_rom[i + 1] / 16 + 2;
                while (b--)
                    m_code += m_code[m_code.count() - a];
                ++i;
            }
            else
            {
                static char const *lut = "#\n 0123456789abcdef"
                    "ghijklmnopqrstuvwxyz!#%(){}[]<>+=/*:;.,~_";

                m_code += m_rom[i] ? lut[m_rom[i]] : m_rom[++i];
            }
        }
        msg::info("Expected %d bytes, got %d\n", length, (int)m_code.count());
    }

    // Remove possible trailing zeroes
    m_code.resize(strlen(m_code.C()));

    m_version = version;

    return true;
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
    struct r_mus : pegtl_string_t("__music__") {};

    struct r_section_name : pegtl::sor<r_lua,
                                       r_gfx,
                                       r_gff,
                                       r_map,
                                       r_sfx,
                                       r_mus> {};
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

    int m_version = -1;

    enum class section : int8_t
    {
        error = -1,
        header = 0,
        lua,
        gfx,
        gff,
        map,
        sfx,
        mus,
    };

    section m_current_section;
    lol::map<int8_t, lol::array<uint8_t>> m_sections;
    lol::String m_code;

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
            r.m_current_section = section::lua;
        else if (in.string().find("gfx") != std::string::npos)
            r.m_current_section = section::gfx;
        else if (in.string().find("gff") != std::string::npos)
            r.m_current_section = section::gff;
        else if (in.string().find("map") != std::string::npos)
            r.m_current_section = section::map;
        else if (in.string().find("sfx") != std::string::npos)
            r.m_current_section = section::sfx;
        else if (in.string().find("music") != std::string::npos)
            r.m_current_section = section::mus;
        else
        {
            msg::info("unknown section name %s\n", in.string().c_str());
            r.m_current_section = section::error;
        }
    }
};

template<>
struct p8_reader::action<p8_reader::r_data>
{
    static void apply(pegtl::action_input const &in, p8_reader &r)
    {
        if (r.m_current_section == section::lua)
        {
            // Copy the code verbatim
            r.m_code = lol::String(in.string().c_str());
        }
        else
        {
            bool must_swap = r.m_current_section == section::gfx;

            // Decode hexadecimal data from this section
            auto &section = r.m_sections[(int8_t)r.m_current_section];
            for (uint8_t const *parser = (uint8_t const *)in.begin(); parser < (uint8_t const *)in.end(); ++parser)
            {
                if ((parser[0] >= 'a' && parser[0] <= 'f')
                     || (parser[0] >= 'A' && parser[0] <= 'F')
                     || (parser[0] >= '0' && parser[0] <= '9'))
                {
                    char str[3] = { (char)parser[must_swap ? 1 : 0],
                                    (char)parser[must_swap ? 0 : 1], '\0' };
                    section << strtoul(str, nullptr, 16);
                    ++parser;
                }
            }
        }
    }
};

bool cart::load_p8(char const *filename)
{
    lol::String s;
    lol::File f;
    for (auto candidate : lol::sys::get_path_list(filename))
    {
        f.Open(candidate, lol::FileAccess::Read);
        if (f.IsValid())
        {
            s = f.ReadString();
            f.Close();

            msg::debug("loaded file %s\n", candidate.C());
            break;
        }
    }

    if (s.count() == 0)
        return false;

    p8_reader reader;
    reader.parse(s.C());

    if (reader.m_version < 0)
        return false;

    m_code = reader.m_code;
    msg::info("code length: %d\n", m_code.count());

    m_rom.resize(0x8000);
    memset(m_rom.data(), 0, m_rom.bytes());

    auto const &gfx = reader.m_sections[(int8_t)p8_reader::section::gfx];
    auto const &gff = reader.m_sections[(int8_t)p8_reader::section::gff];
    auto const &map = reader.m_sections[(int8_t)p8_reader::section::map];
    auto const &sfx = reader.m_sections[(int8_t)p8_reader::section::sfx];
    auto const &mus = reader.m_sections[(int8_t)p8_reader::section::mus];

    msg::info("gfx size: %d / %d\n", gfx.count(), SIZE_GFX + SIZE_GFX2);
    msg::info("gff size: %d / %d\n", gff.count(), SIZE_GFX_PROPS);
    msg::info("map size: %d / %d\n", map.count(), SIZE_MAP + SIZE_MAP2);
    msg::info("sfx size: %d / %d\n", sfx.count() / (4 + 80) * (4 + 64), SIZE_SFX);
    msg::info("mus size: %d / %d\n", mus.count() / 5 * 4, SIZE_SONG);

    // The optional second chunk of gfx is contiguous, we can copy it directly
    memcpy(m_rom.data() + OFFSET_GFX, gfx.data(), lol::min(SIZE_GFX + SIZE_GFX2, gfx.count()));

    memcpy(m_rom.data() + OFFSET_GFX_PROPS, gff.data(), lol::min(SIZE_GFX_PROPS, gff.count()));

    // Map data + optional second chunk
    memcpy(m_rom.data() + OFFSET_MAP, map.data(), lol::min(SIZE_MAP, map.count()));
    if (map.count() > SIZE_MAP)
        memcpy(m_rom.data() + OFFSET_MAP2, map.data() + SIZE_MAP, lol::min(SIZE_MAP2, map.count() - SIZE_MAP));

    // Song data is encoded slightly differently
    for (int i = 0; i < lol::min(SIZE_SONG / 4, mus.count() / 5); ++i)
    {
        m_rom[OFFSET_SONG + i * 4 + 0] = mus[i * 5 + 1] + ((mus[i * 5] << 7) & 0x80);
        m_rom[OFFSET_SONG + i * 4 + 1] = mus[i * 5 + 2] + ((mus[i * 5] << 6) & 0x80);
        m_rom[OFFSET_SONG + i * 4 + 2] = mus[i * 5 + 3] + ((mus[i * 5] << 5) & 0x80);
        m_rom[OFFSET_SONG + i * 4 + 3] = mus[i * 5 + 4] + ((mus[i * 5] << 4) & 0x80);
    }

    // SFX data is packed
    for (int i = 0; i < lol::min(SIZE_SFX / (4 + 32 * 2), sfx.count() / (4 + 32 * 5 / 2)); ++i)
    {
        // FIXME move this to the parser
        for (int j = 0; j < 32; ++j)
        {
            uint32_t ins = (sfx[4 + i * (4 + 80) + j * 5 / 2 + 0] << 16)
                         | (sfx[4 + i * (4 + 80) + j * 5 / 2 + 1] << 8)
                         | (sfx[4 + i * (4 + 80) + j * 5 / 2 + 2]);
            // We read unaligned data; must realign it if j is odd
            ins = (j & 1) ? ins & 0xfffff : ins >> 4;

            uint16_t dst = ((ins & 0x3f000) >> 4)  // pitch
                         | ((ins & 0x00400) >> 10) // instrument (part 1)
                         | ((ins & 0x00300) << 6)  // instrument (part 2)
                         | ((ins & 0x00070) >> 3)  // volume
                         | ((ins & 0x00007) << 4); // effect

            m_rom[OFFSET_SFX + i * (4 + 64) + j * 2 + 0] = dst >> 8;
            m_rom[OFFSET_SFX + i * (4 + 64) + j * 2 + 1] = dst & 0x00ff;
        }

        m_rom[OFFSET_SFX + i * (4 + 64) + 64 + 0] = sfx[i * (4 + 32 * 5 / 2) + 0];
        m_rom[OFFSET_SFX + i * (4 + 64) + 64 + 1] = sfx[i * (4 + 32 * 5 / 2) + 1];
        m_rom[OFFSET_SFX + i * (4 + 64) + 64 + 2] = sfx[i * (4 + 32 * 5 / 2) + 2];
        m_rom[OFFSET_SFX + i * (4 + 64) + 64 + 3] = sfx[i * (4 + 32 * 5 / 2) + 3];
    }

    return true;
}

} // namespace z8

