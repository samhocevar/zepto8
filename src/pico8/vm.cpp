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

#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <lol/engine.h>

#include "pico8/pico8.h"
#include "pico8/vm.h"
#include "bindings/lua.h"
#include "bios.h"

#include <httplib.h>

// FIXME: activate this one day, when we use Lua 5.3 maybe?
#define HAVE_LUA_GETEXTRASPACE 0

// Binding specialisations specific to PICO-8
template<> void z8::bindings::lua_get(lua_State *l, int n,
                                      z8::pico8::rich_string &arg)
{
    if (lua_isnone(l, n))
        arg.assign("[no value]");
    else if (lua_isnil(l, n))
        arg.assign("[nil]");
    else if (lua_type(l, n) == LUA_TSTRING)
        arg.assign(lua_tostring(l, n));
    else if (lua_isnumber(l, n))
    {
        char buffer[20];
        fix32 x = lua_tonumber(l, n);
        int i = sprintf(buffer, "%.4f", (double)x);
        // Remove trailing zeroes and comma
        while (i > 2 && buffer[i - 1] == '0' && ::isdigit(buffer[i - 2]))
            buffer[--i] = '\0';
        if (i > 2 && buffer[i - 1] == '0' && buffer[i - 2] == '.')
            buffer[i -= 2] = '\0';
        arg.assign(buffer);
    }
    else if (lua_istable(l, n))
        arg.assign("[table]");
    else if (lua_isthread(l, n))
        arg.assign("[thread]");
    else if (lua_isfunction(l, n))
        arg.assign("[function]");
    else
        arg.assign(lua_toboolean(l, n) ? "true" : "false");
}

namespace z8::pico8
{

using lol::msg;

vm::vm()
{
    m_bios = std::make_unique<bios>();

    m_lua = luaL_newstate();
    lua_atpanic(m_lua, &vm::panic_hook);
    luaL_openlibs(m_lua);

    bindings::lua::init(m_lua, this);

    // Automatically yield every 1000 instructions
    lua_sethook(m_lua, &vm::instruction_hook, LUA_MASKCOUNT, 1000);

    // Clear memory
    ::memset(&m_ram, 0, sizeof(m_ram));

    // Initialize Zepto8 runtime
    int status = luaL_dostring(m_lua, m_bios->get_code().c_str());
    if (status != LUA_OK)
    {
        char const *message = lua_tostring(m_lua, -1);
        msg::error("error %d loading bios.p8: %s\n", status, message);
        lua_pop(m_lua, 1);
        lol::abort();
    }
}

vm::~vm()
{
    lua_close(m_lua);
}

std::string const &vm::get_code() const
{
    return m_cart.get_code();
}

u4mat2<128, 128> const &vm::get_screen() const
{
    return m_ram.screen;
}

std::tuple<uint8_t *, size_t> vm::ram()
{
    return std::make_tuple(&m_ram[0], sizeof(m_ram));
}

std::tuple<uint8_t *, size_t> vm::rom()
{
    auto rom = m_cart.get_rom();
    return std::make_tuple(&rom[0], sizeof(rom));
}

void vm::runtime_error(std::string str)
{
    // This function never returns
    luaL_error(m_sandbox_lua, str.c_str());
}

int vm::panic_hook(lua_State* l)
{
    char const *message = lua_tostring(l, -1);
    msg::error("Lua panic: %s\n", message);
    lol::abort();
    return 0;
}

void vm::instruction_hook(lua_State *l, lua_Debug *)
{
#if HAVE_LUA_GETEXTRASPACE
    vm *that = *static_cast<vm**>(lua_getextraspace(l));
#else
    lua_getglobal(l, "\x01");
    vm *that = (vm *)lua_touserdata(l, -1);
    lua_remove(l, -1);
#endif

    // The value 135000 was found using trial and error, but it causes
    // side effects in lots of cases. Use 300000 instead.
    // FIXME: this is because we do not consider system costs, see
    // https://pico-8.fandom.com/wiki/CPU for more information
    that->m_instructions += 1000;
    if (that->m_instructions >= 300000)
        lua_yield(l, 0);
}

bool vm::private_download(std::string name)
{
    // Load cart from a URL
    std::string host_name = "www.lexaloffle.com";
    std::string url = "/bbs/cpost_lister3.php?nfo=1&version=000112bw&lid=" + name.substr(1);

    httplib::Client client(host_name);
    client.set_follow_location(true);

    auto res = client.Get(url.c_str());
    if (!res || res->status != 200)
        return false;

    std::map<std::string, std::string> data;
    for (auto& line : lol::split(res->body, '\n'))
    {
        size_t delim = line.find(':');
        if (delim != std::string::npos)
            data[line.substr(0, delim)] = line.substr(delim + 1);
    }

    if (data["lid"].size() == 0)
        return false;

    url = "/bbs/get_cart.php?cat=7&lid=" + data["lid"];
    res = client.Get(url.c_str());
    if (!res || res->status != 200)
        return false;

    return true;
}

bool vm::private_load(std::string name)
{
    // Load cart from a file
    private_stub(lol::format("load(%s)", name.c_str()));
    return true;
}

void vm::load(std::string const &name)
{
    m_cart.load(name);
}

void vm::run()
{
    // Start the cartridge!
    int status = luaL_dostring(m_lua, "run()");
    if (status != LUA_OK)
    {
        char const *message = lua_tostring(m_lua, -1);
        msg::error("error %d running cartridge: %s\n", status, message);
        lua_pop(m_lua, 1);
    }
}

bool vm::step(float seconds)
{
    UNUSED(seconds);

    lua_getglobal(m_lua, "_z8");
    lua_getfield(m_lua, -1, "tick");
    lua_pcall(m_lua, 0, 1, 0);

    bool ret = (int)lua_tonumber(m_lua, -1) >= 0;
    lua_pop(m_lua, 1);
    lua_remove(m_lua, -1);

    m_instructions = 0;
    return ret;
}

void vm::button(int index, int state)
{
    m_buttons[1][index] += state;
}

void vm::mouse(lol::ivec2 coords, int buttons)
{
    m_mouse.x = (double)coords.x;
    m_mouse.y = (double)coords.y;
    m_mouse.b = (double)buttons;
}

void vm::text(char ch)
{
    // Convert uppercase characters to special glyphs
    if (ch >= 'A' && ch <= 'Z')
        ch = '\x80' + (ch - 'A');
    m_keyboard.chars[m_keyboard.stop] = ch;
    m_keyboard.stop = (m_keyboard.stop + 1) % (int)sizeof(m_keyboard.chars);
}

//
// System
//

void vm::api_run()
{
    // Initialise VM state (TODO: check what else to init)
    ::memset(m_buttons, 0, sizeof(m_buttons));

    // Load cartridge code and call _z8.run_cart() on it
    lua_getglobal(m_sandbox_lua, "_z8");
    lua_getfield(m_sandbox_lua, -1, "run_cart");
    lua_pushstring(m_sandbox_lua, m_cart.get_lua().c_str());
    lua_pcall(m_sandbox_lua, 1, 0, 0);
}

void vm::api_menuitem()
{
    private_stub("menuitem");
}

void vm::api_reload(int16_t in_dst, int16_t in_src, opt<int16_t> in_size)
{
    int dst = 0, src = 0, size = offsetof(memory, code);

    if (in_size && *in_size <= 0)
        return;

    // PICO-8 fully reloads the cartridge if not all arguments are passed
    if (in_size)
    {
        dst = in_dst & 0xffff;
        src = in_src & 0xffff;
        size = *in_size & 0xffff;
    }

    // Attempting to write outside the memory area raises an error. Everything
    // else seems legal, especially reading from anywhere.
    if (dst + size > (int)sizeof(m_ram))
    {
        runtime_error("bad memory access");
        return;
    }

    // If reading from after the cart, fill that part with zeroes
    if (src > (int)offsetof(memory, code))
    {
        int amount = lol::min(size, (int)sizeof(m_ram) - src);
        ::memset(&m_ram[dst], 0, amount);
        dst += amount;
        src = (src + amount) & 0xffff;
        size -= amount;
    }

    // Now copy possibly legal data
    int amount = lol::min(size, (int)offsetof(memory, code) - src);
    ::memcpy(&m_ram[dst], &m_cart.get_rom()[src], amount);
    dst += amount;
    size -= amount;

    // If there is anything left to copy, it’s zeroes again
    ::memset(&m_ram[dst], 0, size);
}

int16_t vm::api_peek(int16_t addr)
{
    // Note: peek() is the same as peek(0)
    if (addr < 0 || (int)addr >= (int)sizeof(m_ram))
        return 0;
    return m_ram[addr];
}

int16_t vm::api_peek2(int16_t addr)
{
    int16_t bits = 0;
    for (int i = 0; i < 2; ++i)
    {
        /* This code handles partial reads by adding zeroes */
        if (addr + i < (int)sizeof(m_ram))
            bits |= m_ram[addr + i] << (8 * i);
        else if (addr + i >= (int)sizeof(m_ram))
            bits |= m_ram[addr + i - (int)sizeof(m_ram)] << (8 * i);
    }

    return bits;
}

fix32 vm::api_peek4(int16_t addr)
{
    int32_t bits = 0;
    for (int i = 0; i < 4; ++i)
    {
        /* This code handles partial reads by adding zeroes */
        if (addr + i < (int)sizeof(m_ram))
            bits |= m_ram[addr + i] << (8 * i);
        else if (addr + i >= (int)sizeof(m_ram))
            bits |= m_ram[addr + i - (int)sizeof(m_ram)] << (8 * i);
    }

    return fix32::frombits(bits);
}

void vm::api_poke(int16_t addr, int16_t val)
{
    // Note: poke() is the same as poke(0, 0)
    if (addr < 0 || addr > (int)sizeof(m_ram) - 1)
    {
        runtime_error("bad memory access");
        return;
    }
    m_ram[addr] = (uint8_t)val;
}

void vm::api_poke2(int16_t addr, int16_t val)
{
    // Note: poke2() is the same as poke2(0, 0)
    if (addr < 0 || addr > (int)sizeof(m_ram) - 2)
    {
        runtime_error("bad memory access");
        return;
    }

    m_ram[addr + 0] = (uint8_t)val;
    m_ram[addr + 1] = (uint8_t)((uint16_t)val >> 8);
}

void vm::api_poke4(int16_t addr, fix32 val)
{
    // Note: poke4() is the same as poke4(0, 0)
    if (addr < 0 || addr > (int)sizeof(m_ram) - 4)
    {
        runtime_error("bad memory access");
        return;
    }

    uint32_t x = (uint32_t)val.bits();
    m_ram[addr + 0] = (uint8_t)x;
    m_ram[addr + 1] = (uint8_t)(x >> 8);
    m_ram[addr + 2] = (uint8_t)(x >> 16);
    m_ram[addr + 3] = (uint8_t)(x >> 24);
}

void vm::api_memcpy(int16_t in_dst, int16_t in_src, int16_t in_size)
{
    if (in_size <= 0)
        return;

    // TODO: see if we can stick to int16_t instead of promoting these variables
    int src = in_src & 0xffff;
    int dst = in_dst & 0xffff;
    int size = in_size & 0xffff;

    // Attempting to write outside the memory area raises an error. Everything
    // else seems legal, especially reading from anywhere.
    if (dst < 0 || dst + size > (int)sizeof(m_ram))
    {
        msg::info("z8:segv:memcpy(0x%x,0x%x,0x%x)\n", src, dst, size);
        runtime_error("bad memory access");
        return;
    }

    // If source is outside main memory, part of the operation will be
    // memset(0). But we delay the operation in case the source and the
    // destination overlap.
    int delayed_dst = dst, delayed_size = 0;
    if (src > (int)sizeof(m_ram))
    {
        delayed_size = lol::min(size, (int)sizeof(m_ram) - src);
        dst += delayed_size;
        src = (src + delayed_size) & 0xffff;
        size -= delayed_size;
    }

    // Now copy possibly legal data
    int amount = lol::min(size, (int)sizeof(m_ram) - src);
    memmove(&m_ram[dst], &m_ram[src], amount);
    dst += amount;
    size -= amount;

    // Fill possible zeroes we saved before, and if there is still something
    // to copy, it’s zeroes again
    if (delayed_size)
        ::memset(&m_ram[delayed_dst], 0, delayed_size);
    if (size)
        ::memset(&m_ram[dst], 0, size);
}

void vm::api_memset(int16_t dst, uint8_t val, int16_t size)
{
    if (size <= 0)
        return;

    if (dst < 0 || dst + size > (int)sizeof(m_ram))
    {
        msg::info("z8:segv:memset(0x%x,0x%x,0x%x)\n", dst, val, size);
        runtime_error("bad memory access");
        return;
    }

    ::memset(&m_ram[dst], val, size);
}

var<bool, int16_t, fix32, std::string, std::nullptr_t> vm::api_stat(int16_t id)
{
    // Documented PICO-8 stat() arguments:
    // “0  Memory usage (0..2048)
    //  1  CPU used since last flip (1.0 == 100% CPU at 30fps)
    //  4  Clipboard contents (after user has pressed CTRL-V)
    //  6  Parameter string
    //  7  Current framerate
    //
    //  16..19  Index of currently playing SFX on channels 0..3
    //  20..23  Note number (0..31) on channel 0..3
    //  24      Currently playing pattern index
    //  25      Total patterns played
    //  26      Ticks played on current pattern
    //
    //  80..85  UTC time: year, month, day, hour, minute, second
    //  90..95  Local time
    //
    //  100     Current breadcrumb label, or nil”

    if (id == 0)
    {
        // Perform a GC to avoid accounting for short lifespan objects.
        // Not sure about the performance cost of this.
        lua_gc(m_sandbox_lua, LUA_GCCOLLECT, 0);

        // From the PICO-8 documentation:
        int32_t bits = ((int)lua_gc(m_sandbox_lua, LUA_GCCOUNT, 0) << 16)
                     + ((int)lua_gc(m_sandbox_lua, LUA_GCCOUNTB, 0) << 6);
        return fix32::frombits(bits);
    }

    if (id == 1)
        return (int16_t)0;

    if (id == 4)
        return std::string();

    if (id == 5)
    {
        // Undocumented (see http://pico-8.wikia.com/wiki/Stat)
        return (int16_t)PICO8_VERSION;
    }

    if (id == 6)
        return std::string();

    if (id >= 16 && id <= 19)
        return m_channels[id & 3].m_sfx;

    if (id >= 20 && id <= 23)
        return m_channels[id & 3].m_sfx == -1 ? fix32(-1)
                    : fix32((int)m_channels[id & 3].m_offset);

    if (id == 24)
        return int16_t(m_music.pattern);

    if (id == 25)
        return int16_t(m_music.count);

    if (id == 26)
        return int16_t(m_music.offset * m_music.speed);

    if (id >= 30 && id <= 36)
    {
        bool devkit_mode = m_ram.draw_state.mouse_flag == 1;
        bool has_text = devkit_mode && m_keyboard.start != m_keyboard.stop;

        // Undocumented (see http://pico-8.wikia.com/wiki/Stat)
        switch (id)
        {
            case 30: return has_text;
            case 31:
                if (!has_text)
                    return std::string();

                if (m_keyboard.stop > m_keyboard.start)
                {
                    std::string ret(&m_keyboard.chars[m_keyboard.start],
                                    m_keyboard.stop - m_keyboard.start);
                    m_keyboard.start = m_keyboard.stop = 0;
                    return ret;
                }

                /* if (m_keyboard.stop < m_keyboard.start) */
                {
                    std::string ret(&m_keyboard.chars[m_keyboard.start],
                                    (int)sizeof(m_keyboard.chars) - m_keyboard.start);
                    m_keyboard.start = 0;
                }
                return (int16_t)0;
            case 32: return devkit_mode ? m_mouse.x : fix32(0); break;
            case 33: return devkit_mode ? m_mouse.y : fix32(0); break;
            case 34: return devkit_mode ? m_mouse.b : fix32(0); break;
            case 35: return (int16_t)0; break; // FIXME
            case 36: return (int16_t)0; break; // FIXME
        }
    }

    if (id >= 48 && id < 72)
    {
        if (id == 49 || (id >= 58 && id <= 63))
            return std::string();
        return nullptr;
    }

    if (id >= 72 && id < 100)
        return (int16_t)0;

    if (id == 100 || id == 101)
        return nullptr;

    return (int16_t)0;
}

void vm::api_printh(rich_string str, opt<std::string> filename, opt<bool> overwrite)
{
    (void)filename;
    (void)overwrite;

    std::string decoded;
    for (uint8_t ch : str)
        decoded += charset::to_utf8[ch];
    fprintf(stdout, "%s\n", decoded.c_str());
    fflush(stdout);
}

void vm::api_extcmd(std::string cmd)
{
    if (cmd == "label" || cmd == "screen" || cmd == "rec" || cmd == "video")
        private_stub(lol::format("extcmd(%s)\n", cmd.c_str()));
}

//
// I/O
//

void vm::api_update_buttons()
{
    // Update button state
    for (int i = 0; i < 64; ++i)
    {
        if (m_buttons[1][i])
            ++m_buttons[0][i];
        else
            m_buttons[0][i] = 0;
        m_buttons[1][i] = 0;
    }
}

var<bool, int16_t> vm::api_btn(opt<int16_t> n, int16_t p)
{
    if (n)
        return (bool)m_buttons[0][(*n + 8 * p) & 0x3f];

    int16_t bits = 0;
    for (int i = 0; i < 16; ++i)
        bits |= m_buttons[0][i] ? 1 << i : 0;
    return bits;
}

var<bool, int16_t> vm::api_btnp(opt<int16_t> n, int16_t p)
{
    auto was_pressed = [](int i) -> bool
    {
        // “Same as btn() but only true when the button was not pressed the last frame”
        if (i == 1)
            return true;
        // “btnp() also returns true every 4 frames after the button is held for 15 frames.”
        if (i > 15 && i % 4 == 0)
            return true;
        return false;
    };

    if (n)
        return was_pressed(m_buttons[0][(*n + 8 * p) & 0x3f]);

    int16_t bits = 0;
    for (int i = 0; i < 16; ++i)
        bits |= was_pressed(m_buttons[0][i]) ? 1 << i : 0;
    return bits;
}

//
// Deprecated
//

fix32 vm::api_time()
{
    return (fix32)(double)m_timer.poll();
}

} // namespace z8::pico8

