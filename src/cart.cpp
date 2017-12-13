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

#include "tao/pegtl.hpp"

#include "zepto8.h"
#include "cart.h"

namespace z8
{

using lol::array;
using lol::ivec2;
using lol::msg;
using lol::u8vec4;
using lol::PixelFormat;

using namespace tao;

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

static uint8_t const *compress_lut = nullptr;
static char const *decompress_lut = "\n 0123456789abcdefghijklmnopqrstuvwxyz!#%(){}[]<>+=/*:;.,~_";

bool cart::load_png(char const *filename)
{
    // Open cartridge as PNG image
    lol::image img;
    img.load(filename);
    ivec2 size = img.size();

    if (size.x * size.y < (int)sizeof(m_rom) + 1)
        return false;

    u8vec4 const *pixels = img.lock<PixelFormat::RGBA_8>();

    // Retrieve cartridge data from lower image bits
    uint8_t version = 0;
    for (int n = 0; n < (int)sizeof(m_rom) + 1; ++n)
    {
        u8vec4 p = pixels[n] * 64;
        uint8_t byte = p.a + p.r / 4 + p.g / 16 + p.b / 64;
        (n < (int)sizeof(m_rom) ? m_rom[n] : version) = byte;
    }

    // Retrieve label from image pixels
    if (size.x >= LABEL_WIDTH + LABEL_X && size.y >= LABEL_HEIGHT + LABEL_Y)
    {
        m_label.resize(LABEL_WIDTH * LABEL_HEIGHT / 2);
        for (int y = 0; y < LABEL_HEIGHT; ++y)
        for (int x = 0; x < LABEL_WIDTH; ++x)
        {
            lol::u8vec4 p = pixels[(y + LABEL_Y) * size.x + (x + LABEL_X)];
            uint8_t c = z8::palette::best(p);
            if (x & 1)
                m_label[(y * LABEL_WIDTH + x) / 2] += c << 4;
            else
                m_label[(y * LABEL_WIDTH + x) / 2] = c;
        }
    }

    img.unlock(pixels);

    // Retrieve code, with optional decompression
    if (version == 0 || m_rom.code[0] != ':' || m_rom.code[1] != 'c'
                     || m_rom.code[2] != ':' || m_rom.code[3] != '\0')
    {
        int length = 0;
        while (length < (int)sizeof(m_rom.code) && m_rom.code[length] != '\0')
            ++length;

        m_code.resize(length);
        memcpy(m_code.C(), &m_rom.code, length);
        m_code[length] = '\0';
    }
    else if (version == 1 || version >= 5)
    {
        // Expected data length (including trailing zero)
        int length = m_rom.code[4] * 256
                   + m_rom.code[5];

        m_code.resize(0);
        for (int i = 8; i < (int)sizeof(m_rom.code) && m_code.count() < length; ++i)
        {
            if (m_rom.code[i] >= 0x3c)
            {
                int a = (m_rom.code[i] - 0x3c) * 16 + (m_rom.code[i + 1] & 0xf);
                int b = m_rom.code[i + 1] / 16 + 2;
                if (m_code.count() >= a)
                    while (b--)
                        m_code += m_code[m_code.count() - a];
                ++i;
            }
            else
            {
                m_code += m_rom.code[i] ? decompress_lut[m_rom.code[i] - 1]
                                        : m_rom.code[++i];
            }
        }

        if (length != (int)m_code.count())
            msg::warn("expected %d code bytes, got %d\n", length, (int)m_code.count());
    }

    // Remove possible trailing zeroes
    m_code.resize(strlen(m_code.C()));

    // Invalidate code cache
    m_lua.resize(0);

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

    struct r_bom : pegtl::opt<TAOCPP_PEGTL_STRING("\xef\xbb\xbf")> {};

    struct r_lua : TAOCPP_PEGTL_STRING("__lua__") {};
    struct r_gfx : TAOCPP_PEGTL_STRING("__gfx__") {};
    struct r_gff : TAOCPP_PEGTL_STRING("__gff__") {};
    struct r_map : TAOCPP_PEGTL_STRING("__map__") {};
    struct r_sfx : TAOCPP_PEGTL_STRING("__sfx__") {};
    struct r_mus : TAOCPP_PEGTL_STRING("__music__") {};
    struct r_lab : TAOCPP_PEGTL_STRING("__label__") {};

    struct r_section_name : pegtl::sor<r_lua,
                                       r_gfx,
                                       r_gff,
                                       r_map,
                                       r_sfx,
                                       r_mus,
                                       r_lab> {};
    struct r_section_line : pegtl::seq<r_section_name, pegtl::eolf> {};

    struct r_data_line : pegtl::until<pegtl::eolf> {};
    struct r_data : pegtl::star<pegtl::not_at<r_section_line>,
                                pegtl::not_at<pegtl::eof>,
                                r_data_line> {};

    struct r_section : pegtl::seq<r_section_line, r_data> {};
    struct r_version : pegtl::star<pegtl::digit> {};

    struct r_header: pegtl::seq<TAOCPP_PEGTL_STRING("pico-8 cartridge"), pegtl::until<pegtl::eol>,
                                TAOCPP_PEGTL_STRING("version "), r_version, pegtl::until<pegtl::eol>> {};
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
        lab,
    };

    section m_current_section;
    lol::map<int8_t, lol::array<uint8_t>> m_sections;
    lol::String m_code;

    //
    // Actual reader
    //

    void parse(char const *str)
    {
        pegtl::memory_input<> in(str, "p8");
        pegtl::parse<r_file, action>(in, *this);
    }
};

template<>
struct p8_reader::action<p8_reader::r_version>
{
    template<typename Input>
    static void apply(Input const &in, p8_reader &r)
    {
        r.m_version = std::atoi(in.string().c_str());
    }
};

template<>
struct p8_reader::action<p8_reader::r_section_name>
{
    template<typename Input>
    static void apply(Input const &in, p8_reader &r)
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
        else if (in.string().find("label") != std::string::npos)
            r.m_current_section = section::lab;
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
    template<typename Input>
    static void apply(Input const &in, p8_reader &r)
    {
        if (r.m_current_section == section::lua)
        {
            // Copy the code verbatim
            r.m_code = lol::String(in.string().c_str());
        }
        else
        {
            bool must_swap = r.m_current_section == section::gfx
                          || r.m_current_section == section::lab;

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

    memset(&m_rom, 0, sizeof(m_rom));

    auto const &gfx = reader.m_sections[(int8_t)p8_reader::section::gfx];
    auto const &gff = reader.m_sections[(int8_t)p8_reader::section::gff];
    auto const &map = reader.m_sections[(int8_t)p8_reader::section::map];
    auto const &sfx = reader.m_sections[(int8_t)p8_reader::section::sfx];
    auto const &mus = reader.m_sections[(int8_t)p8_reader::section::mus];
    auto const &lab = reader.m_sections[(int8_t)p8_reader::section::lab];

    msg::info("code: %d gfx: %d/%d gff: %d/%d map: %d/%d "
              "sfx: %d/%d mus: %d/%d lab: %d/%d\n", m_code.count(),
              gfx.count(), (int)sizeof(m_rom.gfx),
              gff.count(), (int)sizeof(m_rom.gfx_props),
              map.count(), (int)(sizeof(m_rom.map) + sizeof(m_rom.map2)),
              sfx.count() / (4 + 80) * (4 + 64), (int)sizeof(m_rom.sfx),
              mus.count() / 5 * 4, (int)sizeof(m_rom.song),
              lab.count(), LABEL_WIDTH * LABEL_HEIGHT / 2);

    // The optional second chunk of gfx is contiguous, we can copy it directly
    memcpy(&m_rom.gfx, gfx.data(), lol::min((int)sizeof(m_rom.gfx), gfx.count()));

    memcpy(&m_rom.gfx_props, gff.data(), lol::min((int)sizeof(m_rom.gfx_props), gff.count()));

    // Map data + optional second chunk
    memcpy(&m_rom.map, map.data(), lol::min((int)sizeof(m_rom.map), map.count()));
    if (map.count() > (int)sizeof(m_rom.map))
    {
        int map2_count = lol::min((int)sizeof(m_rom.map2),
                                  map.count() - (int)sizeof(m_rom.map));
        memcpy(&m_rom.map2, map.data() + (int)sizeof(m_rom.map), map2_count);
    }

    // Song data is encoded slightly differently
    int song_count = lol::min((int)sizeof(m_rom.song) / 4,
                              mus.count() / 5);
    for (int i = 0; i < song_count; ++i)
    {
        m_rom.song[i * 4 + 0] = mus[i * 5 + 1] + ((mus[i * 5] << 7) & 0x80);
        m_rom.song[i * 4 + 1] = mus[i * 5 + 2] + ((mus[i * 5] << 6) & 0x80);
        m_rom.song[i * 4 + 2] = mus[i * 5 + 3] + ((mus[i * 5] << 5) & 0x80);
        m_rom.song[i * 4 + 3] = mus[i * 5 + 4] + ((mus[i * 5] << 4) & 0x80);
    }

    // SFX data is packed
    int sfx_count = lol::min((int)sizeof(m_rom.sfx) / (4 + 32 * 2),
                             sfx.count() / (4 + 32 * 5 / 2));
    for (int i = 0; i < sfx_count; ++i)
    {
        // FIXME move this to the parser maybe?
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
                         | ((ins & 0x0000f) << 4); // effect

            m_rom.sfx[i].notes[j][0] = dst >> 8;
            m_rom.sfx[i].notes[j][1] = dst & 0x00ff;
        }

        m_rom.sfx[i].editor_mode = sfx[i * (4 + 32 * 5 / 2) + 0];
        m_rom.sfx[i].speed       = sfx[i * (4 + 32 * 5 / 2) + 1];
        m_rom.sfx[i].loop_start  = sfx[i * (4 + 32 * 5 / 2) + 2];
        m_rom.sfx[i].loop_end    = sfx[i * (4 + 32 * 5 / 2) + 3];
    }

    // Optional cartridge label
    m_label.resize(lol::min(lab.count(), LABEL_WIDTH * LABEL_HEIGHT / 2));
    memcpy(m_label.data(), lab.data(), m_label.count());

    // Invalidate code cache
    m_lua.resize(0);

    return true;
}

lol::image cart::get_png() const
{
    lol::image ret;
    ret.load("data/blank.png");

    ivec2 size = ret.size();

    u8vec4 *pixels = ret.lock<PixelFormat::RGBA_8>();

    /* Apply label */
    if (m_label.count() >= LABEL_WIDTH * LABEL_HEIGHT / 2)
    {
        for (int y = 0; y < LABEL_HEIGHT; ++y)
        for (int x = 0; x < LABEL_WIDTH; ++x)
        {
            uint8_t col = (m_label[(y * LABEL_WIDTH + x) / 2] >> (4 * (x & 1))) & 0xf;
            pixels[(y + LABEL_Y) * size.x + (x + LABEL_X)] = palette::get(col);
        }
    }

    /* Create ROM data */
    lol::array<uint8_t> const &rom = get_bin();

    /* Write ROM to lower image bits */
    for (int n = 0; n < rom.count(); ++n)
    {
        u8vec4 p(rom[n] & 0x30, rom[n] & 0x0c, rom[n] & 0x03, rom[n] & 0xc0);
        pixels[n] = pixels[n] / 4 * 4 + p / u8vec4(16, 4, 1, 64);
    }

    ret.unlock(pixels);

    return ret;
}

lol::array<uint8_t> cart::get_compressed_code() const
{
    lol::array<uint8_t> ret;

    /* Ensure the compression LUT is initialised */
    if (!compress_lut)
    {
        uint8_t *tmp = new uint8_t[256];
        memset(tmp, 0, 256);
        for (int i = 0; i < 0x3b; ++i)
            tmp[(uint8_t)decompress_lut[i]] = i + 1;
        compress_lut = tmp;
    }

    /* FIXME: PICO-8 appears to be adding an implicit \n at the
     * end of the code, and ignoring it when compressing code. So
     * for the moment we write one char too many. */
    for (int i = 0; i < m_code.count(); ++i)
    {
        /* Look behind for possible patterns */
        int best_j = 0, best_len = 0;
        for (int j = lol::max(i - 3135, 0); j < i; ++j)
        {
            int end = lol::min(m_code.count() - j, 17);

            /* XXX: official PICO-8 stops at i - j, despite being able
             * to support m_code.count() - j, it seems. */
            end = lol::min(end, i - j);

            for (int k = 0; ; ++k)
            {
                if (k >= end || m_code[j + k] != m_code[i + k])
                {
                    if (k > best_len)
                    {
                        best_j = j;
                        best_len = k;
                    }
                    break;
                }
            }
        }

        uint8_t byte = (uint8_t)m_code[i];

        /* If best length is 2, it may or may not be interesting to emit a back
         * reference. Most of the time, if the first character fits in a single
         * byte, we should emit it directly. And if the first character needs
         * to be escaped, we should always emit a back reference of length 2.
         *
         * However there is at least one suboptimal case with preexisting
         * sequences “ab”, “b-”, and “-c-”. Then the sequence “ab-c-” can be
         * encoded as “a” “b-” “c-” (5 bytes) or as “ab” “-c-” (4 bytes).
         * Clearly it is more interesting to encode “ab” as a two-byte back
         * reference. But since this case is statistically rare, we ignore
         * it and accept that compression could be more efficient.
         *
         * If it can be a relief, PICO-8 is a lot less good than us at this. */
        if (compress_lut[byte] && best_len <= 2)
        {
            ret << compress_lut[byte];
        }
        else if (best_len >= 2)
        {
            int a = 0x3c + (i - best_j) / 16;
            int b = ((i - best_j) & 0xf) + (best_len - 2) * 16;
            ret << a << b;
            i += best_len - 1;
        }
        else
        {
            ret << '\0' << byte;
        }
    }

    msg::info("compressed code (%d bytes, max %d)\n",
              ret.count(), (int)sizeof(m_rom.code) - 8);

    return ret;
}

lol::array<uint8_t> cart::get_bin() const
{
    int const data_size = offsetof(memory, code);

    /* Create ROM data */
    lol::array<uint8_t> ret;

    ret.resize(data_size);
    memcpy(ret.data(), &m_rom, data_size);

    ret << ':' << 'c' << ':' << '\0';
    ret << (m_code.count() >> 8);
    ret << (m_code.count() & 0xff);
    ret << 0 << 0; /* FIXME: what is this? */
    ret += get_compressed_code();

    int max_len = (int)sizeof(m_rom.code);
    msg::info("compressed code length: %d/%d\n", ret.count() - data_size, max_len);

    ret << EXPORT_VERSION;

    return ret;
}

lol::String cart::get_p8() const
{
    lol::String ret = "pico-8 cartridge // http://www.pico-8.com\n";
    ret += lol::String::format("version %d\n", EXPORT_VERSION);

    ret += "__lua__\n";
    ret += get_code();
    if (ret.last() != '\n')
        ret += '\n';

    ret += "__gfx__\n";
    for (int i = 0; i < (int)sizeof(m_rom.gfx); ++i)
    {
        ret += lol::String::format("%02x", uint8_t(m_rom.gfx[i] * 0x101 / 0x10));
        if ((i + 1) % 64 == 0)
            ret += '\n';
    }

    if (m_label.count() >= LABEL_WIDTH * LABEL_HEIGHT / 2)
    {
        ret += "__label__\n";
        for (int i = 0; i < LABEL_WIDTH * LABEL_HEIGHT / 2; ++i)
        {
            ret += lol::String::format("%02x", uint8_t(m_label.data()[i] * 0x101 / 0x10));
            if ((i + 1) % (LABEL_WIDTH / 2) == 0)
                ret += '\n';
        }
        ret += '\n';
    }

    ret += "__gff__\n";
    for (int i = 0; i < (int)sizeof(m_rom.gfx_props); ++i)
    {
        ret += lol::String::format("%02x", m_rom.gfx_props[i]);
        if ((i + 1) % 128 == 0)
            ret += '\n';
    }

    // Only serialise m_rom.map, because m_rom.map2 overlaps with m_rom.gfx
    // which has already been serialised.
    ret += "__map__\n";
    for (int i = 0; i < (int)sizeof(m_rom.map); ++i)
    {
        ret += lol::String::format("%02x", m_rom.map[i]);
        if ((i + 1) % 128 == 0)
            ret += '\n';
    }

    ret += "__sfx__\n";
    for (int n = 0; n < (int)sizeof(m_rom.sfx); n += 68)
    {
        uint8_t const *data = (uint8_t const *)&m_rom.sfx[n];
        ret += lol::String::format("%02x%02x%02x%02x", data[64], data[65], data[66], data[67]);
        for (int j = 0; j < 64; j += 2)
        {
            int pitch = data[j] & 0x3f;
            int instrument = ((data[j + 1] << 2) & 0x4) | (data[j] >> 6);
            int volume = (data[j + 1] >> 1) & 0x7;
            int effect = (data[j + 1] >> 4) & 0xf;
            ret += lol::String::format("%02x%1x%1x%1x", pitch, instrument, volume, effect);
        }
        ret += '\n';
    }

    ret += "__music__\n";
    for (int n = 0; n < (int)sizeof(m_rom.song); n += 4)
    {
        uint8_t const *data = &m_rom.song[n];
        int flags = (data[0] >> 7) | ((data[1] >> 6) & 0x2)
                     | ((data[2] >> 5) & 0x4) | ((data[3] >> 4) & 0x8);
        ret += lol::String::format("%02x %02x%02x%02x%02x\n", flags,
                                   data[0] & 0x7f, data[1] & 0x7f,
                                   data[2] & 0x7f, data[3] & 0x7f);
    }

    ret += '\n';

    return ret;
}

} // namespace z8

