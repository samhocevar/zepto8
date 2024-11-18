//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016–2024 Sam Hocevar <sam@hocevar.net>
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

#include <lol/math>  // lol::round, lol::mix
#include <algorithm> // std::swap
#include <cmath>     // std::min, std::max

#include "pico8/vm.h"
#include "bios.h"

namespace z8::pico8
{

/* Return color bits for use with set_pixel().
 *  - bits 0x0000ffff: fillp pattern
 *  - bits 0x000f0000: default color (palette applied)
 *  - bits 0x00f00000: color for patterns (palette applied)
 *  - bit  0x01000000: transparency for patterns */
uint32_t vm::to_color_bits(opt<fix32> c)
{
    auto &ds = m_ram.draw_state;

    if (c)
    {
        /* From the PICO-8 documentation:
         *  -- bit  0x1000.0000 means the non-colour bits should be observed
         *  -- bit  0x0100.0000 transparency bit
         *  -- bits 0x00FF.0000 are the usual colour bits
         *  -- bits 0x0000.FFFF are interpreted as the fill pattern */
        fix32 col = *c;
        ds.pen = (col.bits() >> 16) & 0xff;

        if (col.bits() & 0x1000'0000 && ds.fillp_flag) // 0x5f34
        {
            ds.fillp[0] = col.bits();
            ds.fillp[1] = col.bits() >> 8;
            ds.fillp_trans = (col.bits() >> 24) & 0x1;
        }
    }

    return raw_to_bits(ds.pen);
}

// From raw color to color bits, doesn't modify pen
uint32_t vm::raw_to_bits(uint8_t c) const
{
    auto& ds = m_ram.draw_state;

    int32_t bits = 0;

    bits |= ds.fillp[0];
    bits |= ds.fillp[1] << 8;
    bits |= ds.fillp_trans << 24;

    uint8_t c1 = c & 0xf;
    uint8_t c2 = (c >> 4) & 0xf;

    bits |= (ds.draw_palette[c1] & 0xf) << 16;
    bits |= (ds.draw_palette[c2] & 0xf) << 20;

    return bits;
}

uint8_t vm::get_pixel(int16_t x, int16_t y) const
{
    auto &ds = m_ram.draw_state;

    if (x < ds.clip.x1 || x >= ds.clip.x2 || y < ds.clip.y1 || y >= ds.clip.y2)
        return 0;

    return get_current_screen().get(x, y);
}

void vm::set_pixel(int16_t x, int16_t y, uint32_t color_bits)
{
    auto &ds = m_ram.draw_state;
    auto &hw = m_ram.hw_state;

    if (x < ds.clip.x1 || x >= ds.clip.x2 || y < ds.clip.y1 || y >= ds.clip.y2)
        return;

    uint8_t color = (color_bits >> 16) & 0xf;

    // This is where the fillp pattern is actually handled
    if ((color_bits >> (15 - (x & 3) - 4 * (y & 3))) & 0x1)
    {
        if (color_bits & 0x100'0000) // Special transparency bit
            return;
        color = (color_bits >> 20) & 0xf;
    }

    if (hw.bit_mask)
    {
        uint8_t old = get_current_screen().get(x, y);
        color = (old & ~(hw.bit_mask & 7))
              | (color & (hw.bit_mask & 7) & (hw.bit_mask >> 4));
    }

    get_current_screen().set(x, y, color);
}

void vm::hline(int16_t x1, int16_t x2, int16_t y, uint32_t color_bits)
{
    using std::min, std::max;

    auto &ds = m_ram.draw_state;
    auto &hw = m_ram.hw_state;

    if (y < ds.clip.y1 || y >= ds.clip.y2)
        return;

    if (x1 > x2)
        std::swap(x1, x2);

    x1 = max(x1, (int16_t)ds.clip.x1);
    x2 = min(x2, (int16_t)(ds.clip.x2 - 1));

    if (x1 > x2)
        return;

    // Cannot use shortcut code when fillp or bitplanes are active
    if ((color_bits & 0xffff) || hw.bit_mask)
    {
        for (int16_t x = x1; x <= x2; ++x)
            set_pixel(x, y, color_bits);
    }
    else
    {
        uint8_t *p = get_current_screen().data[y];
        uint8_t color = (color_bits >> 16) & 0xf;

        if (x1 & 1)
        {
            p[x1 / 2] = (p[x1 / 2] & 0x0f) | (color << 4);
            ++x1;
        }

        if ((x2 & 1) == 0)
        {
            p[x2 / 2] = (p[x2 / 2] & 0xf0) | color;
            --x2;
        }

        ::memset(p + x1 / 2, color * 0x11, (x2 - x1 + 1) / 2);
    }
}

void vm::vline(int16_t x, int16_t y1, int16_t y2, uint32_t color_bits)
{
    using std::min, std::max;

    auto &ds = m_ram.draw_state;
    auto &hw = m_ram.hw_state;

    if (x < ds.clip.x1 || x >= ds.clip.x2)
        return;

    if (y1 > y2)
        std::swap(y1, y2);

    y1 = max(y1, (int16_t)ds.clip.y1);
    y2 = min(y2, (int16_t)(ds.clip.y2 - 1));

    if (y1 > y2)
        return;

    // Cannot use shortcut code when fillp or bitplanes are active
    if ((color_bits & 0xffff) || hw.bit_mask)
    {
        for (int16_t y = y1; y <= y2; ++y)
            set_pixel(x, y, color_bits);
    }
    else
    {
        uint8_t mask = (x & 1) ? 0x0f : 0xf0;
        uint8_t color = (color_bits >> 16) & 0xf;
        uint8_t p = (x & 1) ? color << 4 : color;

        for (int16_t y = y1; y <= y2; ++y)
        {
            auto &data = get_current_screen().data[y][x / 2];
            data = (data & mask) | p;
        }
    }
}


//
// Text
//

tup<uint8_t, uint8_t, uint8_t> vm::api_cursor(uint8_t x, uint8_t y, opt<uint8_t> in_c)
{
    auto &ds = m_ram.draw_state;
    uint8_t c = in_c ? *in_c : ds.pen;

    std::swap(ds.cursor.x, x);
    std::swap(ds.cursor.y, y);
    std::swap(ds.pen, c);

    ds.print_start_x = ds.cursor.x;

    return std::make_tuple(x, y, c);
}

uint8_t get_p8scii_value(uint8_t ch)
{
    if (ch >= 0x30 && ch <= 0x39) return ch - 0x30; // 0 to 9
    if (ch >= 0x61 && ch <= 0x99) return ch - 0x57; // a to z + extras
    return 0;
}

fix32 get_p8scii_cursor_value(uint8_t ch)
{
    return get_p8scii_value(ch) - fix32(16);
}


void vm::scrool_screen(int16_t scroll_amount)
{
    // FIXME: is this affected by the camera?
    uint8_t* s = get_current_screen().data[0];
    memmove(s, s + scroll_amount * 64, sizeof(get_current_screen()) - scroll_amount * 64);
    ::memset(s + sizeof(get_current_screen()) - scroll_amount * 64, 0, scroll_amount * 64);
}

tup<opt<fix32>, opt<fix32> > vm::api_print(opt<rich_string> str, opt<fix32> opt_x, opt<fix32> opt_y,
                   opt<fix32> c)
{
    auto &ds = m_ram.draw_state;
    auto &font = m_ram.custom_font;

    if (!str)
        return std::make_tuple(std::nullopt, std::nullopt); // return 0 arguments

    // The presence of y indicates whether mode is print(s,c) or print(s,x,y,c)
    bool has_coords = !!opt_y;
    // FIXME: make x and y int16_t instead?
    fix32 x = has_coords ? *opt_x : fix32(ds.cursor.x);
    fix32 y = has_coords ? *opt_y : fix32(ds.cursor.y);

    // get background color
    uint32_t background_bits = raw_to_bits(0);

    // FIXME: we ignore fillp here, but should we set it in to_color_bits()?
    uint32_t color_bits = to_color_bits(has_coords ? c : opt_x) & 0xf'0000;
    
    fix32 initial_x = has_coords ? *opt_x : fix32(ds.print_start_x);
    fix32 initial_y = y;

    // p8scii todo:
    // skipping frames \^1 -> \^9
    // delaying between char \^d
    // drawing one off characters \^. and \^:
    // poking \^@address and \^!address

    print_state_t print_state = m_ram.hw_state.print_state;
    if (!print_state.active)
    {
        // if not active, use default values:
        print_state.padding = true;
        print_state.wide = false;
        print_state.tall = false;
        print_state.solid = false;
        print_state.invert = false;
        print_state.dotty = false;
        print_state.custom = false;
    }
    
    int16_t font_width = print_state.custom ? font.width : 4;
    int16_t font_extwidth = print_state.custom ? font.extended_width : 8;
    int16_t font_height = print_state.custom ? font.height : 6;

    int16_t base_height = print_state.char_h == 0 ? font_height : print_state.char_h;
    int16_t height = 0;
    bool first_character = true;
    int16_t width = print_state.char_w == 0 ? font_width : print_state.char_w;
    int16_t width_2 = print_state.char_w2 == 0 ? font_extwidth : print_state.char_w2;
    
    int16_t last_character_width = width; // used with backspace
    fix32 tab_size = print_state.tab_w == 0 ? 16 : (print_state.tab_w + 1) * 4;
    bool new_line_at_end = !ds.misc_features.no_newlines;
    fix32 wrap_limit = 128;
    fix32 character_repeat = 0;
    bool decoration_active = false;
    fix32 decoraction_cursor_x = 0;
    fix32 decoraction_cursor_y = 0;

    bool draw_one_off=false;
    uint8_t one_off_glyph[8];

    for (auto chi = str.value().begin(); chi != str.value().end(); )
    {
        uint8_t ch = *chi;
        if (ch == '\0')
        {
            new_line_at_end = false; // if there is an explicite \0, don't skip at the end
            break;
        }

        switch (ch)
        {
        case '\n':
            if (first_character) height = font_height;
            x = initial_x;
            y += fix32(height);
            height = base_height;
            break;
        case '\r':
            x = initial_x;
            break;
        case '\b':
            x -= last_character_width;
            break;
        case '\t':
            x += tab_size - ((x) % tab_size);
            break;
        case '\f': // foreground color
            if (++chi == str.value().end()) break;
            color_bits = to_color_bits( get_p8scii_value(*chi) );
            break;
        case '\v': // decoration characters
        {
            if (++chi == str.value().end()) break;
            decoration_active = true;
            decoraction_cursor_x = x;
            decoraction_cursor_y = y; 
            uint8_t decoration_position = get_p8scii_value(*chi);
            x += (decoration_position % 4) - 2 - last_character_width;
            y += (decoration_position >> 2) - 8;
            break;
        }
        case '\x1': // repeating characters
            if (++chi == str.value().end())
            {
                new_line_at_end = false;
                break;
            }
            character_repeat = get_p8scii_value(*chi);
            if (++chi == str.value().end())
            {
                // no character to repeat, jump lines
                new_line_at_end = false;
                x = initial_x;
                y += fix32(height * character_repeat);
                break;
            }
            break;
        case '\x2': // background color \#
            if (++chi == str.value().end()) break;
            print_state.solid = true;
            background_bits = raw_to_bits( get_p8scii_value(*chi) );
            break;
        case '\x3': // shift cursor horizontal \-
            if (++chi == str.value().end()) break;
            x += get_p8scii_cursor_value(*chi);
            break;
        case '\x4': // shift cursor vertical \|
            if (++chi == str.value().end()) break;
            y += get_p8scii_cursor_value(*chi);
            break;
        case '\x5': // shift cursor \+
            if (++chi == str.value().end()) break;
            x += get_p8scii_cursor_value(*chi);
            if (++chi == str.value().end()) break;
            y += get_p8scii_cursor_value(*chi);
            break;
        case '\xe': // font switch to custom
            print_state.custom = 1;
            
            font_width = print_state.custom ? font.width : 4;
            font_extwidth = print_state.custom ? font.extended_width : 8;
            font_height = print_state.custom ? font.height : 6;

            base_height = print_state.char_h == 0 ? font_height : print_state.char_h;
            width = print_state.char_w == 0 ? font_width : print_state.char_w;
            width_2 = print_state.char_w2 == 0 ? font_extwidth : print_state.char_w2;
            break;
        case '\xf': // font switch back
            print_state.custom = 0;
            
            font_width = print_state.custom ? font.width : 4;
            font_extwidth = print_state.custom ? font.extended_width : 8;
            font_height = print_state.custom ? font.height : 6;
            
            base_height = print_state.char_h == 0 ? font_height : print_state.char_h;
            width = print_state.char_w == 0 ? font_width : print_state.char_w;
            width_2 = print_state.char_w2 == 0 ? font_extwidth : print_state.char_w2;
            break;
        case '\a': // audio
        {
            ++chi;
            // search a non-playing channel and mark playing sfx between 60 and 63
            int16_t sfx_channel = -1;
            bool sfx_playing[4] = {};
            for (uint8_t audio_channel = 0; audio_channel <= 3; ++audio_channel)
            {
                int16_t csfx = m_state.channels[audio_channel].main_sfx.sfx;
                if (csfx == -1)
                {
                    if (sfx_channel < 0) sfx_channel = audio_channel;
                }
                else if(csfx >= 60 && csfx <= 63)
                {
                    sfx_playing[csfx - 60] = true;
                }
            }
            // search for first non-playing sfx starting from 60
            int16_t reserved_sfx = -1;
            for (int16_t csfx = 3; csfx >= 0; --csfx)
            {
                if (!sfx_playing[csfx])
                {
                    reserved_sfx = csfx;
                    break;
                }
            }
            if (reserved_sfx < 0)
            {
                // no free sfx found
                reserved_sfx = 0;
            }

            uint8_t sfx_index = 60 + reserved_sfx;

            // specific sfx index
            if (chi != str.value().end() && ((*chi) >= '0' && (*chi) <= '9'))
            {
                sfx_index = get_p8scii_value((*chi));
                ++chi;
                if (chi != str.value().end() && ((*chi) >= '0' && (*chi) <= '9'))
                {
                    sfx_index = sfx_index * 10 + get_p8scii_value((*chi));
                    ++chi;
                }
            }

            sfx_t& sfx = m_ram.sfx[sfx_index];
            sfx = {};
            sfx.speed = 4;

            // header controls, global to the sfx
            while (chi != str.value().end() && (*chi) != ' ')
            {
                bool finished_header = false;
                uint8_t cha = *chi;
                switch (cha)
                {
                case 'l': // loop
                    if (++chi == str.value().end()) break;
                    sfx.loop_start = get_p8scii_value(*chi);
                    if (++chi == str.value().end()) break;
                    sfx.loop_end = get_p8scii_value(*chi);
                    break;
                case 's': // speed
                    if (++chi == str.value().end()) break;
                    sfx.speed = get_p8scii_value(*chi);
                    break;
                case 'z': // special effects
                    if (++chi == str.value().end()) break;
                    sfx.filters = get_p8scii_value(*chi); // todo: is this just a direct copy?
                    break;
                default:
                    finished_header = true;
                    break;
                }

                if (finished_header)
                {
                    break;
                }
                if (chi != str.value().end()) ++chi;
            }

            if (chi == str.value().end())
            {
                // no notes have been given, play a single c
                sfx.speed = 16;
                sfx.notes[0] = { 46, 0, 5, 0 };
            }

            // notes controls
            uint8_t sfx_time = 0;
            uint8_t sfx_instr = 5;
            uint8_t sfx_vol = 5;
            uint8_t sfx_fx = 0;
            uint8_t sfx_octave = 3;
            while (chi != str.value().end() && (*chi) != ' ')
            {
                uint8_t cha = *chi;
                switch (cha)
                {
                case 'i':
                    if (++chi == str.value().end()) break;
                    sfx_instr = get_p8scii_value(*chi) % 8;
                    break;
                case 'x':
                    if (++chi == str.value().end()) break;
                    sfx_fx = get_p8scii_value(*chi) % 8;
                    break;
                case 'v':
                    if (++chi == str.value().end()) break;
                    sfx_vol = get_p8scii_value(*chi) % 8;
                    break;
                case '>':
                    sfx_vol = std::min<uint8_t>(sfx_vol + 1, 7);
                    break;
                case '<':
                    sfx_vol = std::max<uint8_t>(sfx_vol - 1, 0);
                    break;
                case '.':
                    ++sfx_time;
                    break;
                default:
                    {
                        const std::string keys = "c d ef g a b";
                        size_t found = keys.find(cha);
                        int16_t newkey = 0;
                        if (found != keys.npos)
                        {
                            newkey = found;
                        }
                        auto sign_chi = (chi + 1);
                        if (sign_chi != str.value().end() && ((*sign_chi) == '-' || (*sign_chi) == '#'))
                        {
                            newkey += ((*sign_chi) == '#') ? 1 : -1;
                            ++chi;
                        }
                        auto octave_chi = (chi + 1);
                        if (octave_chi != str.value().end() && ((*octave_chi) >= '0' && (*octave_chi) <= '5'))
                        {
                            sfx_octave = get_p8scii_value((*octave_chi));
                            ++chi;
                        }
                        uint8_t notekey = std::min<uint8_t>(std::max<uint8_t>(newkey + sfx_octave * 12, 0), 63);
                        sfx.notes[sfx_time] = { notekey, sfx_instr, sfx_vol, sfx_fx };
                        ++sfx_time;
                    }
                    break;
                }

                if (chi != str.value().end()) ++chi;
                if (sfx_time >= 32) break; // too many notes
            }
            api_sfx(sfx_index, sfx_channel, 0, 0);
            break;
        }
        case 6: // \^
        {
            if (++chi == str.value().end()) break;
            bool command = true;
            if (*chi == '-')
            {
                command = false;
                if (++chi == str.value().end()) break;
            }
            bool skip_character_draw = true;
            uint8_t style_control = *chi;
            switch (style_control)
            {
            case 'w': print_state.wide = command; break;
            case 't': print_state.tall = command; break;
            case '=': print_state.dotty = command; break;
            case 'p': print_state.wide = print_state.tall = print_state.dotty = command; break;
            case 'i': print_state.invert = command; break;
            case 'b': print_state.padding = command; break;
            case '#': print_state.solid = command; break;
            case 'g': x = initial_x; y = initial_y;  break; // return home
            case 'h': initial_x = x; initial_y = y;  break; // update home
            case 'j': // absolute cursor
                if (++chi == str.value().end()) break;
                x = get_p8scii_value(*chi) * fix32(4);
                if (++chi == str.value().end()) break;
                y = get_p8scii_value(*chi) * fix32(4);
                break;
            case 's': // tab stop width
                if (++chi == str.value().end()) break;
                tab_size = get_p8scii_value(*chi) * fix32(4);
                break;
            case 'r': // right side print border
                if (++chi == str.value().end()) break;
                wrap_limit = get_p8scii_value(*chi) * fix32(4);
                break;
            case 'c': // clear screen
                if (++chi == str.value().end()) break;
                api_cls(get_p8scii_value(*chi));
                // also reset local print cursor
                x = fix32(ds.cursor.x);
                y = fix32(ds.cursor.y);
                break;
            case 'x':
                if (++chi == str.value().end()) break;
                width = get_p8scii_value(*chi);
                width_2 = width + (print_state.custom ? 0 : 4);
                break;
            case 'y':
                if (++chi == str.value().end()) break;
                base_height = get_p8scii_value(*chi);
                break;
            case '.':
            {
                auto chicp = chi;
                bool had_issue = false;
                memset(&one_off_glyph, 0, sizeof(uint8_t) * 8);
                for (int glyphline = 0; glyphline < 8; ++glyphline)
                {
                    if (++chicp == str.value().end()) { had_issue = true; break; }
                    one_off_glyph[glyphline] = *chicp;
                }
                if (!had_issue)
                {
                    skip_character_draw = false;
                    ch = 0;
                    draw_one_off = true;
                    chi = chicp;
                }
                break;
            }
            case ':':
            {
                auto chicp = chi;
                bool had_issue = false;
                memset(&one_off_glyph, 0, sizeof(uint8_t)*8);
                for (int glyphline = 0; glyphline < 16; ++glyphline)
                {
                    if (++chicp == str.value().end()) { had_issue = true; break; }
                    uint8_t curline = get_p8scii_value(*chicp);
                    if (curline > 15) curline = 0;
                    one_off_glyph[glyphline / 2] += curline << (glyphline % 2 ? 0 : 4);
                }
                if (!had_issue)
                {
                    skip_character_draw = false;
                    ch = 0;
                    draw_one_off = true;
                    chi = chicp;
                }
                break;
            }
            }
            if (skip_character_draw) break;
            [[fallthrough]];
        }
        // case 6 must be just before default, as we sometimes don't break
        default:
            {
                int16_t wide_scale = print_state.wide ? 2 : 1;
                int16_t tall_scale = print_state.tall ? 2 : 1;
                
                font_width = print_state.custom ? font.width : 4;
                font_extwidth = print_state.custom ? font.extended_width : 8;
                font_height = print_state.custom ? font.height : 6;
                int16_t default_height = print_state.custom ? std::min(int(font.height), 8) : 5;

                int offset = ch < 0x80 ? ch : 2 * ch - 0x80;
                int font_x = offset % 32 * 4;
                int font_y = offset / 32 * 6;

                int16_t w = ch < 0x80 ? std::min<int16_t>(width, font_width) : std::min<int16_t>(width_2, font_extwidth);
                int16_t draw_width = ch < 0x80 ? width : width_2;

                int16_t h = std::min<int16_t>(base_height, default_height);
                int16_t draw_height = base_height;

                if (draw_one_off)
                {
                    font_width = 8;
                    font_extwidth = 8;
                    font_height = 8;
                    default_height = 8;
                    w = 8;
                    h = 8;
                    draw_width = 8;
                    draw_height = 8;
                }

                int16_t vertical_offset = 0;
                // per-character size adjustments
                if (!draw_one_off && print_state.custom && font.size_adjustments != 0)
                {
                    int16_t addr = 0X5600 + ch / 2;
                    int8_t adj = m_ram[addr];
                    if (ch % 2) {
                        adj >>= 4;
                    }
                    draw_width += ((adj & 0x7) + 4) % 8 - 4;
                    if (adj & 0x8) vertical_offset = 1;
                }

                if (ds.misc_features.char_wrap && x + fix32(draw_width) > wrap_limit)
                {
                    // go new line
                    x = initial_x;
                    y += fix32(height);
                }

                height = std::max<int16_t>(height, print_state.tall ? draw_height * 2 : draw_height);
    
                // check if we need to scroll before each character, so it can fit
                if (!has_coords && !ds.misc_features.no_printscroll && new_line_at_end)
                {
                    int16_t final_y = int16_t(y) + height - 128;
                    if (final_y > 0)
                    {
                        scrool_screen(final_y);
                        y -= fix32(final_y);
                    }
                }

                int16_t base_x = (int16_t)x - ds.camera.x + print_state.offset_x;
                int16_t base_y = (int16_t)y - ds.camera.y + print_state.offset_y;
                if (!draw_one_off && print_state.custom)
                {
                    base_x += font.offset.x;
                    base_y += font.offset.y + vertical_offset;
                }

                #if __NX__
                // special picto button A and B on switch
                if (ch == 142 || ch == 151)
                {
                    font_x = ch == 142 ? 0 : 8;
                    font_y = 72;
                    base_y -= 1;
                    h = 7;
                    draw_height = 7;
                }
                #endif
                    
                auto& g = draw_one_off ? one_off_glyph : font.glyphs[ch - 1];

                for (int16_t dy = print_state.padding ? -1 : 0; dy < draw_height; ++dy)
                    for (int16_t dx = print_state.padding ? -1 : 0; dx < draw_width; ++dx)
                    {
                        int16_t screen_x = base_x + dx * wide_scale;
                        int16_t screen_y = base_y + dy * tall_scale;

                        bool is_on = false;
                        if (dy >= 0 && dy < h && dx >= 0 && dx < w)
                        {
                            // inside the font
                            if (draw_one_off || print_state.custom)
                            {                
                                auto font_line = g[dy];
                                uint8_t gl = font_line >>= dx;
                                is_on = gl & 0x1;
                            }
                            else
                            {
                                is_on = (bool)m_bios->get_spixel(font_x + dx, font_y + dy);
                            }
                        }
                        if (is_on != print_state.invert)
                        {
                            set_pixel(screen_x, screen_y, color_bits);
                            if (!print_state.dotty)
                            {
                                if (print_state.wide) set_pixel(screen_x + 1, screen_y, color_bits);
                                if (print_state.tall) set_pixel(screen_x, screen_y + 1, color_bits);
                                if (print_state.wide && print_state.tall) set_pixel(screen_x + 1, screen_y + 1, color_bits);
                            }
                            else if (print_state.solid)
                            {
                                if (print_state.wide) set_pixel(screen_x + 1, screen_y, background_bits);
                                if (print_state.tall) set_pixel(screen_x, screen_y + 1, background_bits);
                                if (print_state.wide && print_state.tall) set_pixel(screen_x + 1, screen_y + 1, background_bits);
                            }
                        }
                        else if(print_state.solid)
                        {
                            set_pixel(screen_x, screen_y, background_bits);
                            if (print_state.wide) set_pixel(screen_x + 1, screen_y, background_bits);
                            if (print_state.tall) set_pixel(screen_x, screen_y + 1, background_bits);
                            if (print_state.wide && print_state.tall) set_pixel(screen_x + 1, screen_y + 1, background_bits);
                        }
                    }

                last_character_width = draw_width * wide_scale;
                x += fix32(last_character_width);
                
                first_character = false;
                if (decoration_active)
                {
                    decoration_active = false;
                    x = decoraction_cursor_x;
                    y = decoraction_cursor_y;
                }

                draw_one_off = false;
            }
            break;
        }

        // go to next character
        if (chi != str.value().end() && character_repeat == 0) ++chi;
        if (character_repeat > fix32(0)) --character_repeat;
    }

    fix32 line_offset = fix32(new_line_at_end ? std::max(height, base_height) : 0);
    
    // check if we need to scroll because of a newline
    if (!has_coords && !ds.misc_features.no_printscroll && new_line_at_end)
    {
        int16_t final_y = int16_t(y) + height + 6 - 128;
        if (final_y > 0)
        {
            scrool_screen(6);
            y -= fix32(6);
        }
    }

    // FIXME: should a multiline print update y?
    ds.cursor.x = (uint8_t)(new_line_at_end ? initial_x : x);
    ds.cursor.y = (uint8_t)(y + line_offset);

    ds.print_start_x = (uint8_t)initial_x;

    return std::make_tuple(x, y + fix32(height));
}

//
// Graphics
//

tup<int16_t, int16_t> vm::api_camera(int16_t x, int16_t y)
{
    auto &ds = m_ram.draw_state;

    auto prev = ds.camera;
    ds.camera.x = x;
    ds.camera.y = y;
    return std::make_tuple(prev.x, prev.y);
}

void vm::api_circ(int16_t x, int16_t y, int16_t r, opt<fix32> c)
{
    auto &ds = m_ram.draw_state;

    x -= ds.camera.x;
    y -= ds.camera.y;

    if (x + r < 0 || x - r >= 128 || y + r < 0 || y - r >= 128) return;

    uint32_t color_bits = to_color_bits(c);
    // seems to come from https://rosettacode.org/wiki/Bitmap/Midpoint_circle_algorithm#BASIC256
    for (int16_t dx = r, dy = 0, err = 0; dx >= dy; )
    {
        set_pixel(x + dx, y + dy, color_bits);
        set_pixel(x + dy, y + dx, color_bits);
        set_pixel(x - dy, y + dx, color_bits);
        set_pixel(x - dx, y + dy, color_bits);
        set_pixel(x - dx, y - dy, color_bits);
        set_pixel(x - dy, y - dx, color_bits);
        set_pixel(x + dy, y - dx, color_bits);
        set_pixel(x + dx, y - dy, color_bits);

        dy += 1;
        if (err < r - 1)
        {
            err += 1 + 2 * dy;
        }
        else
        {
            dx -= 1;
            err += 1 + 2 * (dy-dx);
        }
    }
}

void vm::api_circfill(int16_t x, int16_t y, int16_t r, opt<fix32> c)
{
    auto &ds = m_ram.draw_state;

    x -= ds.camera.x;
    y -= ds.camera.y;

    if (x + r < 0 || x - r >= 128 || y + r < 0 || y - r >= 128) return;

    uint32_t color_bits = to_color_bits(c);
    // seems to come from https://rosettacode.org/wiki/Bitmap/Midpoint_circle_algorithm#BASIC256
    for (int16_t dx = r, dy = 0, err = 0; dx >= dy; )
    {
        // Some minor overdraw here when dx == 0 or dx == dy, but nothing serious
        hline(x - dx, x + dx, y - dy, color_bits);
        hline(x - dx, x + dx, y + dy, color_bits);
        hline(x - dy, x + dy, y - dx, color_bits);
        hline(x - dy, x + dy, y + dx, color_bits);

        dy += 1;
        if (err < r - 1)
        {
            err += 1 + 2 * dy;
        }
        else
        {
            dx -= 1;
            err += 1 + 2 * (dy - dx);
        }
    }
}

tup<uint8_t, uint8_t, uint8_t, uint8_t> vm::api_clip(int16_t x, int16_t y,
                                                     int16_t w, opt<int16_t> h, bool intersect)
{
    auto &ds = m_ram.draw_state;
    uint8_t x1 = intersect ? ds.clip.x1 : 0;
    uint8_t y1 = intersect ? ds.clip.y1 : 0;
    uint8_t x2 = intersect ? ds.clip.x2 : 128;
    uint8_t y2 = intersect ? ds.clip.y2 : 128;

    // All three arguments are required for the non-default behaviour
    if (h)
    {
        x2 = uint8_t(std::min(int16_t(x2), int16_t(x + std::max(w, int16_t(0)))));
        y2 = uint8_t(std::min(int16_t(y2), int16_t(y + std::max(*h, int16_t(0)))));
        x1 = uint8_t(std::max(int16_t(x1), x));
        y1 = uint8_t(std::max(int16_t(y1), y));
    }

    std::swap(ds.clip.x1, x1);
    std::swap(ds.clip.y1, y1);
    std::swap(ds.clip.x2, x2);
    std::swap(ds.clip.y2, y2);

    return std::make_tuple(x1, y1, x2, y2);
}

void vm::api_cls(uint8_t c)
{
    ::memset(&get_current_screen(), c % 0x10 * 0x11, sizeof(get_current_screen()));

    // Documentation: “Clear the screen and reset the clipping rectangle”.
    auto &ds = m_ram.draw_state;
    ds.cursor.x = ds.cursor.y = 0;
    ds.print_start_x = 0;
    ds.clip.x1 = ds.clip.y1 = 0;
    ds.clip.x2 = ds.clip.y2 = 128;
}

uint8_t vm::api_color(opt<uint8_t> c)
{
    uint8_t col = c ? *c : 6;
    std::swap(col, m_ram.draw_state.pen);
    return col;
}

fix32 vm::api_fillp(fix32 fillp)
{
    auto &ds = m_ram.draw_state;

    int32_t prev = (ds.fillp[0] << 16)
                 | (ds.fillp[1] << 24)
                 | ((ds.fillp_trans & 1) << 15);
    ds.fillp[0] = fillp.bits() >> 16;
    ds.fillp[1] = fillp.bits() >> 24;
    ds.fillp_trans = (fillp.bits() >> 15) & 1;
    return fix32::frombits(prev);
}

opt<var<int16_t, bool>> vm::api_fget(opt<int16_t> n, opt<int16_t> f)
{
    if (!n)
        return std::nullopt; // return 0 arguments

    int16_t bits = 0;
    if (*n >= 0 && *n < (int)sizeof(m_ram.gfx_flags))
        bits = m_ram.gfx_flags[*n];

    if (f)
        return (bool)(bits & (1 << *f)); // return a boolean

    return bits; // return a number
}

void vm::api_fset(opt<int16_t> n, opt<int16_t> f, opt<bool> b)
{
    if (!n || !f || *n < 0 || *n >= (int16_t)sizeof(m_ram.gfx_flags))
        return;

    uint8_t &data = m_ram.gfx_flags[*n];

    if (!b)
        data = (uint8_t)*f;
    else if (*b)
        data |= 1 << *f;
    else
        data &= ~(1 << *f);
}

void vm::api_line(opt<fix32> arg0, opt<fix32> arg1, opt<fix32> arg2,
                  opt<fix32> arg3, opt<fix32> arg4)
{
    using std::abs, std::min, std::max;

    auto &ds = m_ram.draw_state;

    uint32_t color_bits;

    if (!arg0 || !arg1)
    {
        ds.polyline_flag |= 0x1;
        color_bits = to_color_bits(arg0);
        return;
    }

    int16_t x0, y0, x1, y1;

    if (!arg2 || !arg3)
    {
        // Polyline mode (with 2 or 3 arguments)
        x0 = ds.polyline.x;
        y0 = ds.polyline.y;
        x1 = int16_t(*arg0);
        y1 = int16_t(*arg1);
        color_bits = to_color_bits(arg2);
    }
    else
    {
        // Standard mode
        x0 = int16_t(*arg0);
        y0 = int16_t(*arg1);
        x1 = int16_t(*arg2);
        y1 = int16_t(*arg3);
        color_bits = to_color_bits(arg4);
    }

    // Store polyline state
    ds.polyline.x = x1;
    ds.polyline.y = y1;

    // If this is the polyline bootstrap, do nothing
    if (ds.polyline_flag & 0x1)
    {
        ds.polyline_flag &= ~0x1;
        return;
    }

    x0 -= ds.camera.x; y0 -= ds.camera.y;
    x1 -= ds.camera.x; y1 -= ds.camera.y;

    bool horiz = abs(x1 - x0) >= abs(y1 - y0);
    int16_t dx = x0 <= x1 ? 1 : -1;
    int16_t dy = y0 <= y1 ? 1 : -1;

    // Clamping helps with performance even if pixels are clipped later
    int16_t x = lol::clamp(int(x0), -1, 128);
    int16_t y = lol::clamp(int(y0), -1, 128);
    int16_t xend = lol::clamp(int(x1), -1, 128);
    int16_t yend = lol::clamp(int(y1), -1, 128);

    for (;;)
    {
        set_pixel(x, y, color_bits);

        if (horiz)
        {
            if (x == xend)
                break;
            x += dx;
            y = (int16_t)lol::round(lol::mix((double)y0, (double)y1, (double)(x - x0) / (x1 - x0)));
        }
        else
        {
            if (y == yend)
                break;
            y += dy;
            x = (int16_t)lol::round(lol::mix((double)x0, (double)x1, (double)(y - y0) / (y1 - y0)));
        }
    }
}

void vm::api_tline(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                   fix32 mx, fix32 my, opt<fix32> in_mdx, opt<fix32> in_mdy, int16_t layer)
{
    using std::abs, std::min;

    auto &ds = m_ram.draw_state;

    // mdx, mdy default to 1/8, 0
    fix32 mdx = in_mdx ? *in_mdx : fix32::frombits(0x2000);
    fix32 mdy = in_mdy ? *in_mdy : fix32(0);

    fix32 loopx = ds.tline.mask.x != 0 ? fix32(ds.tline.mask.x) : fix32(128);
    fix32 loopy = ds.tline.mask.y != 0 ? fix32(ds.tline.mask.y) : fix32(128);

    mx = mx % loopx;
    my = my % loopy;

    x0 -= ds.camera.x; y0 -= ds.camera.y;
    x1 -= ds.camera.x; y1 -= ds.camera.y;

    bool horiz = abs(x1 - x0) >= abs(y1 - y0);
    int16_t dx = horiz ? x0 <= x1 ? 1 : -1 : 0;
    int16_t dy = horiz ? 0 : y0 <= y1 ? 1 : -1;

    // Clamping helps with performance even if pixels are clipped later
    int16_t x = lol::clamp(int(x0), -1, 128);
    int16_t y = lol::clamp(int(y0), -1, 128);
    int16_t xend = lol::clamp(int(x1), -1, 128);
    int16_t yend = lol::clamp(int(y1), -1, 128);

    // Advance texture coordinates; do it in steps to avoid overflows
    int16_t delta = abs(horiz ? x - x0 : y - y0);
    while (delta)
    {
        int16_t step = min(int16_t(8192), delta);
        mx = (mx + mdx * fix32(step)) % loopx;
        my = (my + mdy * fix32(step)) % loopy;
        delta -= step;
    }

    int16_t map_size_x = get_map_size_x();
    int16_t map_size_y = get_map_size_y(map_size_x);

    u4mat2<128, 128>& gfx = m_ram.get_gfx();
    for (;;)
    {
        // Find sprite in map memory
        int sx = (ds.tline.offset.x + int(mx)) % map_size_x;
        int sy = (ds.tline.offset.y + int(my)) % map_size_y;
        uint8_t sprite = m_ram.map[map_size_x * sy + sx];
        uint8_t bits = m_ram.gfx_flags[sprite];

        // If found, draw pixel
        if ((sprite || ds.misc_features.sprite_zero) && (!layer || (bits & layer)))
        {
            int col = gfx.get(sprite % 16 * 8 + (int(mx << 3) & 0x7),
                              sprite / 16 * 8 + (int(my << 3) & 0x7));
            if ((ds.draw_palette[col] & 0xf0) == 0)
            {
                uint32_t color_bits = (ds.draw_palette[col] & 0xf) << 16;
                set_pixel(x, y, color_bits);
            }
        }

        // Advance source coordinates
        mx = (mx + mdx) % loopx;
        my = (my + mdy) % loopy;

        // Advance destination coordinates
        if (horiz)
        {
            if (x == xend)
                break;
            x += dx;
            y = (int16_t)lol::round(lol::mix((double)y0, (double)y1, (double)(x - x0) / (x1 - x0)));
        }
        else
        {
            if (y == yend)
                break;
            y += dy;
            x = (int16_t)lol::round(lol::mix((double)x0, (double)x1, (double)(y - y0) / (y1 - y0)));
        }
    }
}

int16_t vm::get_map_size_x()
{
    return m_ram.hw_state.mapping_map_width == 0 ? 256 : m_ram.hw_state.mapping_map_width;
}

int16_t vm::get_map_size_y(int16_t map_size_x)
{
    return (m_ram.hw_state.mapping_map >= 0x80 ? ((0x100 - m_ram.hw_state.mapping_map) << 8) : 8192) / map_size_x;
}

// Tested on PICO-8 1.1.12c: fractional part of all arguments is ignored.
void vm::api_map(int16_t cel_x, int16_t cel_y, int16_t sx, int16_t sy,
                 opt<int16_t> in_cel_w, opt<int16_t> in_cel_h, int16_t layer)
{
    auto &ds = m_ram.draw_state;

    sx -= ds.camera.x;
    sy -= ds.camera.y;

    int16_t map_size_x = get_map_size_x();
    int16_t map_size_y = get_map_size_y(map_size_x);
    int16_t map_max_y = m_ram.hw_state.mapping_map >= 0x80 ? map_size_y : map_size_y / 2;

    // PICO-8 documentation: “If cel_w and cel_h are not specified,
    // defaults to 128,32”.
    bool no_size = !in_cel_w && !in_cel_h;
    int16_t src_w = (no_size ? map_size_x : *in_cel_w) * 8;
    int16_t src_h = (no_size ? map_max_y : *in_cel_h) * 8;
    int16_t src_x = cel_x * 8;
    int16_t src_y = cel_y * 8;

    // Clamp to screen
    int16_t mx = std::max(-sx, 0);
    int16_t my = std::max(-sy, 0);
    src_x += mx;
    src_y += my;
    src_w -= mx + std::max(src_w + sx - 128, 0);
    src_h -= my + std::max(src_h + sy - 128, 0);
    sx += mx;
    sy += my;

    if (src_h <= 0 || src_w <= 0) return;

    int16_t max_map_x = map_size_x * 8;
    int16_t max_map_y = map_size_y * 8;

    u4mat2<128, 128>& gfx = m_ram.get_gfx();
    for (int16_t dy = 0; dy < src_h; ++dy)
    for (int16_t dx = 0; dx < src_w; ++dx)
    {
        int16_t cx = src_x + dx;
        int16_t cy = src_y + dy;
        if (cx < 0 || cx >= max_map_x || cy < 0 || cy >= max_map_y)
            continue;
        cx /= 8;
        cy /= 8;

        uint8_t sprite = m_ram.map[map_size_x * cy + cx];
        uint8_t bits = m_ram.gfx_flags[sprite];
        if (layer && !(bits & layer))
            continue;

        if (sprite || ds.misc_features.sprite_zero)
        {
            int col = gfx.get(sprite % 16 * 8 + (src_x + dx) % 8,
                                    sprite / 16 * 8 + (src_y + dy) % 8);
            if ((ds.draw_palette[col] & 0xf0) == 0)
            {
                uint32_t color_bits = (ds.draw_palette[col] & 0xf) << 16;
                set_pixel(sx + dx, sy + dy, color_bits);
            }
        }
    }
}

fix32 vm::api_mget(int16_t x, int16_t y)
{
    int16_t map_size_x = get_map_size_x();
    int16_t map_size_y = get_map_size_y(map_size_x);

    if (x < 0 || x >= map_size_x || y < 0 || y >= map_size_y)
        return 0;

    return m_ram.map[map_size_x * y + x];
}

void vm::api_mset(int16_t x, int16_t y, uint8_t n)
{
    int16_t map_size_x = get_map_size_x();
    int16_t map_size_y = get_map_size_y(map_size_x);

    if (x < 0 || x >= map_size_x || y < 0 || y >= map_size_y)
        return;

    m_ram.map[map_size_x * y + x] = n;
}

void vm::api_oval(int16_t x0, int16_t y0, int16_t x1, int16_t y1, opt<fix32> c)
{
    using std::max;

    auto &ds = m_ram.draw_state;

    x0 -= ds.camera.x;
    y0 -= ds.camera.y;
    x1 -= ds.camera.x;
    y1 -= ds.camera.y;

    if (x0 > x1)
        std::swap(x0, x1);

    if (y0 > y1)
        std::swap(y0, y1);

    if (x1 < 0 || x0 >= 128 || y1 < 0 || y0 >= 128) return;

    uint32_t color_bits = to_color_bits(c);

    // FIXME: not elegant at all
    float xc = float(x0 + x1) / 2;
    float yc = float(y0 + y1) / 2;

    auto plot = [&](int16_t x, int16_t y)
    {
        set_pixel(x, y, color_bits);
        set_pixel(int16_t(2 * xc) - x, y, color_bits);
        set_pixel(x, int16_t(2 * yc) - y, color_bits);
        set_pixel(int16_t(2 * xc) - x, int16_t(2 * yc) - y, color_bits);
    };

    // Cutoff for slope = 0.5 happens at x = a²/sqrt(a²+b²)
    float a = max(1.0f, float(x1 - x0) / 2);
    float b = max(1.0f, float(y1 - y0) / 2);
    float cutoff = a / sqrt(1 + b * b / (a * a));

    for (float dx = 0; dx <= cutoff; ++dx)
    {
        int16_t x = int16_t(ceil(xc + dx));
        int16_t y = int16_t(round(yc - b / a * sqrt(a * a - dx * dx)));
        plot(x, y);
    }
    cutoff = b / sqrt(1 + a * a / (b * b));
    for (float dy = 0; dy < cutoff; ++dy)
    {
        int16_t y = int16_t(ceil(yc + dy));
        int16_t x = int16_t(round(xc - a / b * sqrt(b * b - dy * dy)));
        plot(x, y);
    }
}

void vm::api_ovalfill(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                      opt<fix32> c)
{
    using std::max;

    auto &ds = m_ram.draw_state;

    x0 -= ds.camera.x;
    y0 -= ds.camera.y;
    x1 -= ds.camera.x;
    y1 -= ds.camera.y;

    if (x0 > x1)
        std::swap(x0, x1);

    if (y0 > y1)
        std::swap(y0, y1);

    if (x1 < 0 || x0 >= 128 || y1 < 0 || y0 >= 128) return;

    uint32_t color_bits = to_color_bits(c);

    // FIXME: not elegant at all
    float xc = float(x0 + x1) / 2;
    float yc = float(y0 + y1) / 2;

    float a = max(1.0f, float(x1 - x0) / 2);
    float b = max(1.0f, float(y1 - y0) / 2);

    float cutoff = a / sqrt(1 + b * b / (a * a));

    for (float dx = 0; dx <= cutoff; ++dx)
    {
        int16_t x = int16_t(ceil(xc + dx));
        int16_t y = int16_t(round(yc - b / a * sqrt(a * a - dx * dx)));
        vline(x, int16_t(2 * yc) - y, y, color_bits);
        vline(int16_t(2 * xc) - x, int16_t(2 * yc) - y, y, color_bits);
    }
    cutoff = b / sqrt(1 + a * a / (b * b));
    for (float dy = 0; dy <= cutoff; ++dy)
    {
        int16_t x = int16_t(round(xc - a / b * sqrt(b * b - dy * dy)));
        int16_t y = int16_t(ceil(yc + dy));
        hline(int16_t(2 * xc) - x, x, y, color_bits);
        hline(int16_t(2 * xc) - x, x, int16_t(2 * yc) - y, color_bits);
    }
}

opt<uint8_t> vm::api_private_pal(opt<uint8_t> c0, opt<uint8_t> c1, uint8_t p)
{
    auto &ds = m_ram.draw_state;
    auto &hw = m_ram.hw_state;

    if (!c0 || !c1)
    {
        // calling with one non-0 parameter don't do anything
        if (c0 && (*c0)!=0 && !c1) return std::nullopt;

        // PICO-8 documentation: “pal() to reset to system defaults (including
        // transparency values and fill pattern)”
        for (int i = 0; i < 16; ++i)
        {
            ds.draw_palette[i] = i | (i ? 0x00 : 0x10);
            ds.screen_palette[i] = i;
            hw.raster.palette[i] = 0;
        }
        ds.fillp[0] = 0;
        ds.fillp[1] = 0;
        ds.fillp_trans = 0;
        return std::nullopt;
    }
    else
    {
        uint8_t &data = (p == 2) ? hw.raster.palette[*c0 & 0xf]
                      : (p == 1) ? ds.screen_palette[*c0 & 0xf]
                                 : ds.draw_palette[*c0 & 0xf];
        uint8_t prev = data;

        if (p == 1 || p == 2)
            data = *c1 & 0xff;
        else
            data = (data & 0x10) | (*c1 & 0xf); // Transparency bit is preserved

        return prev%16;
    }
}

var<int16_t, bool> vm::api_palt(opt<int16_t> c, opt<bool> t)
{
    auto &ds = m_ram.draw_state;

    if (!t)
    {
        int16_t prev = 0;
        for (int i = 0; i < 16; ++i)
        {
            prev |= ((ds.draw_palette[i] & 0xf0) != 0 ? 1 : 0) << (15 - i);
            ds.draw_palette[i] &= 0xf;
            // If c is set, set transparency according to the (15 - i)th bit.
            // Otherwise, only color 0 is transparent.
            ds.draw_palette[i] |= (c ? *c & (1 << (15 - i)) : i == 0) ? 0x10 : 0x00;
        }
        return prev;
    }
    else
    {
        uint8_t &data = ds.draw_palette[*c & 0xf];
        bool prev = data & 0x10;
        data = (data & 0xf) | (*t ? 0x10 : 0x00);
        return prev;
    }
}

fix32 vm::api_pget(int16_t x, int16_t y)
{
    auto &ds = m_ram.draw_state;

    /* pget() is affected by camera() and by clip() */
    // FIXME: "and by clip()"? wut?
    x -= ds.camera.x;
    y -= ds.camera.y;
    return get_pixel(x, y);
}

void vm::api_pset(int16_t x, int16_t y, opt<fix32> c)
{
    auto &ds = m_ram.draw_state;

    x -= ds.camera.x;
    y -= ds.camera.y;
    uint32_t color_bits = to_color_bits(c);
    set_pixel(x, y, color_bits);
}

void vm::api_rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, opt<fix32> c)
{
    auto &ds = m_ram.draw_state;

    x0 -= ds.camera.x;
    y0 -= ds.camera.y;
    x1 -= ds.camera.x;
    y1 -= ds.camera.y;

    if (x0 > x1)
        std::swap(x0, x1);

    if (y0 > y1)
        std::swap(y0, y1);

    if (x1 < 0 || x0 >= 128 || y1 < 0 || y0 >= 128) return;

    uint32_t color_bits = to_color_bits(c);

    hline(x0, x1, y0, color_bits);
    hline(x0, x1, y1, color_bits);

    if (y0 + 1 < y1)
    {
        vline(x0, y0 + 1, y1 - 1, color_bits);
        vline(x1, y0 + 1, y1 - 1, color_bits);
    }
}

void vm::api_rectfill(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                      opt<fix32> c)
{
    auto &ds = m_ram.draw_state;

    x0 -= ds.camera.x;
    y0 -= ds.camera.y;
    x1 -= ds.camera.x;
    y1 -= ds.camera.y;

    if (y0 > y1)
        std::swap(y0, y1);

    if (x0 > x1)
    {
        if (x0 < 0 || x1 >= 128) return;
    }
    else
    {
        if (x1 < 0 || x0 >= 128) return;
    }

    // clamp to the screen

    if (y0 < -1) y0 = -1;
    if (y1 > 128) y1 = 128;

    uint32_t color_bits = to_color_bits(c);

    for (int16_t y = y0; y <= y1; ++y)
        hline(x0, x1, y, color_bits);
}

int16_t vm::api_sget(int16_t x, int16_t y)
{
    return m_ram.get_gfx().safe_get(x, y);
}

void vm::api_sset(int16_t x, int16_t y, opt<int16_t> c)
{
    auto &ds = m_ram.draw_state;

    uint8_t col = c ? (uint8_t)*c : ds.pen;
    m_ram.get_gfx().safe_set(x, y, ds.draw_palette[col & 0xf]);
}

void vm::api_spr(int16_t n, int16_t x, int16_t y, opt<fix32> w,
                 opt<fix32> h, bool flip_x, bool flip_y)
{
    auto &ds = m_ram.draw_state;

    x -= ds.camera.x;
    y -= ds.camera.y;

    int16_t w8 = w ? (int16_t)(*w * fix32(8.0)) : 8;
    int16_t h8 = h ? (int16_t)(*h * fix32(8.0)) : 8;

    if (x + w8 <= 0 || x >= 128 || y + h8 <= 0 || y >= 128) return;

    u4mat2<128, 128>& gfx = m_ram.get_gfx();
    for (int16_t j = 0; j < h8; ++j)
        for (int16_t i = 0; i < w8; ++i)
        {
            int16_t di = flip_x ? w8 - 1 - i : i;
            int16_t dj = flip_y ? h8 - 1 - j : j;
            uint8_t col = gfx.safe_get(n % 16 * 8 + di, n / 16 * 8 + dj);
            if ((ds.draw_palette[col] & 0xf0) == 0)
            {
                uint32_t color_bits = (ds.draw_palette[col] & 0xf) << 16;
                set_pixel(x + i, y + j, color_bits);
            }
        }
}

void vm::api_sspr(int16_t sx, int16_t sy, int16_t sw, int16_t sh,
                  int16_t dx, int16_t dy, opt<int16_t> in_dw,
                  opt<int16_t> in_dh, bool flip_x, bool flip_y)
{
    auto &ds = m_ram.draw_state;

    dx -= ds.camera.x;
    dy -= ds.camera.y;
    int16_t dw = in_dw ? *in_dw : sw;
    int16_t dh = in_dh ? *in_dh : sh;

    // Support negative dw and dh by flipping the target rectangle
    if (dw < 0) { dw = -dw; dx -= dw - 1; flip_x = !flip_x; }
    if (dh < 0) { dh = -dh; dy -= dh - 1; flip_y = !flip_y; }

    if (dx + dw <= 0 || dx >= 128 || dy + dh <= 0 || dy >= 128) return;

    // Iterate over destination pixels
    // FIXME: maybe clamp if target area is too big?
    u4mat2<128, 128>& gfx = m_ram.get_gfx();
    for (int16_t j = 0; j < dh; ++j)
    for (int16_t i = 0; i < dw; ++i)
    {
        int16_t di = flip_x ? dw - 1 - i : i;
        int16_t dj = flip_y ? dh - 1 - j : j;

        // Find source
        int16_t x = sx + sw * di / dw;
        int16_t y = sy + sh * dj / dh;

        uint8_t col = gfx.safe_get(x, y);
        if ((ds.draw_palette[col] & 0xf0) == 0)
        {
            uint32_t color_bits = (ds.draw_palette[col] & 0xf) << 16;
            set_pixel(dx + i, dy + j, color_bits);
        }
    }
}

} // namespace z8::pico8

