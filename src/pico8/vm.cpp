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

#include <algorithm>  // std::min
#include <filesystem>
#include <chrono>
#include <ctime>

#include "pico8/pico8.h"
#include "pico8/vm.h"
#include "bindings/lua.h"
#include "bios.h"

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

    // Initialise the PRNG with the current time
    auto now = std::chrono::high_resolution_clock::now();
    api_srand(fix32::frombits((int32_t)now.time_since_epoch().count()));

    // Initialize Zepto8 runtime
    int status = luaL_dostring(m_lua, m_bios->get_code().c_str());
    if (status != LUA_OK)
    {
        char const *message = lua_tostring(m_lua, -1);
        lol::msg::error("error %d loading bios.p8: %s\n", status, message);
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
    lol::msg::error("Lua panic: %s\n", message);
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

tup<bool, bool, std::string> vm::private_download(opt<std::string> str)
{
    // Load cart info from a URL
    //std::string host = "https://www.lexaloffle.com";
    std::string host = "http://sam.hocevar.net/zepto8";

    if (str)
        download_state.step = 0;

    switch (download_state.step)
    {
    case 0:
        // Step 0: start downloading cart info
        download_state.client.get(host + "/bbs/cpost_lister3.php?nfo=1&version=000112bw&lid=" + str->substr(1));
        download_state.step = 1;
        break;
    case 1:
        // Step 1: wait while downloading
        if (download_state.client.get_status() != lol::net::http::status::pending)
            download_state.step = 2;
        break;
    case 2: {
        // Step 2: bail out if errors, save cart info, start downloading cart
        if (download_state.client.get_status() != lol::net::http::status::success)
            return std::make_tuple(true, false, std::string("error downloading info"));

        std::map<std::string, std::string> data;
        for (auto& line : lol::split(download_state.client.get_result(), '\n'))
        {
            size_t delim = line.find(':');
            if (delim != std::string::npos)
                data[line.substr(0, delim)] = line.substr(delim + 1);
        }

        auto const& lid = data["lid"];
        auto const& mid = data["mid"];
        if (lid.size() == 0 || lid.size() == 0)
            return std::make_tuple(true, false, std::string("no cart info found"));

        // Write cart info
        lol::File file;
#if _WIN32
        std::string cache_dir = lol::sys::getenv("APPDATA");
#else
        std::string cache_dir = lol::sys::getenv("HOME") + "/.lexaloffle";
#endif
        cache_dir += "/pico-8/bbs/";
        cache_dir += (mid[0] >= '1' && mid[0] <= '9') ? mid.substr(0, 1) : "carts";
        std::error_code code;
        if (!std::filesystem::create_directories(cache_dir, code))
            if (code.value() != 0) // XXX: workaround for Windows weird behaviour
                return std::make_tuple(true, false, "can't create cache dir");
        std::string nfo_path = cache_dir + "/" + mid + ".nfo";
        download_state.cart_path = cache_dir + "/" + lid + ".p8.png";

        file.Open(nfo_path, lol::FileAccess::Write);
        if (file.IsValid())
        {
            file.Write(download_state.client.get_result());
            file.Close();
        }

        // Download cart
        download_state.client.get(host + "/bbs/get_cart.php?cat=7&lid=" + lid);
        download_state.step = 3;
        break;
    }
    case 3:
        // Step 3: wait while downloading
        if (download_state.client.get_status() != lol::net::http::status::pending)
            download_state.step = 4;
        break;
    case 4: {
        // Step 4: bail out if errors, load cart otherwise
        if (download_state.client.get_status() != lol::net::http::status::success)
            return std::make_tuple(true, false, std::string("error downloading cart"));

        lol::File file;
        // Save cart
        file.Open(download_state.cart_path, lol::FileAccess::Write, true);
        if (file.IsValid())
        {
            file.Write(download_state.client.get_result());
            file.Close();
        }

        // Load cart
        m_cart.load(download_state.cart_path);
        return std::make_tuple(true, true, std::string());
    }
    default:
        break;
    }

    return std::make_tuple(false, false, std::string());
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
        lol::msg::error("error %d running cartridge: %s\n", status, message);
        lua_pop(m_lua, 1);
    }
}

bool vm::step(float seconds)
{
    UNUSED(seconds);

    bool ret = false;
    lua_getglobal(m_lua, "__z8_tick");
    int status = lua_pcall(m_lua, 0, 1, 0);
    if (status != LUA_OK)
    {
        char const *message = lua_tostring(m_lua, -1);
        lol::msg::error("error %d in main loop: %s\n", status, message);
    }
    else
    {
        ret = (int)lua_tonumber(m_lua, -1) >= 0;
    }
    lua_pop(m_lua, 1);

    m_instructions = 0;
    return ret;
}

void vm::button(int index, int state)
{
    m_state.buttons[1][index] += state;
}

void vm::mouse(lol::ivec2 coords, int buttons)
{
    m_state.mouse.x = (double)coords.x;
    m_state.mouse.y = (double)coords.y;
    m_state.mouse.b = (double)buttons;
}

void vm::text(char ch)
{
    // Convert uppercase characters to special glyphs
    if (ch >= 'A' && ch <= 'Z')
        ch = '\x80' + (ch - 'A');
    m_state.kbd.chars[m_state.kbd.stop] = ch;
    m_state.kbd.stop = (m_state.kbd.stop + 1) % (int)sizeof(m_state.kbd.chars);
}

//
// System
//

void vm::api_run()
{
    // Initialise VM state (TODO: check what else to init)
    ::memset(m_state.buttons, 0, sizeof(m_state.buttons));

    // Load cartridge code and call __z8_run_cart() on it
    lua_getglobal(m_sandbox_lua, "__z8_run_cart");
    lua_pushstring(m_sandbox_lua, m_cart.get_lua().c_str());
    lua_pcall(m_sandbox_lua, 1, 0, 0);
}

void vm::api_menuitem()
{
    private_stub("menuitem");
}

void vm::api_reload(int16_t in_dst, int16_t in_src, opt<int16_t> in_size)
{
    using std::min;

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
        int amount = min(size, (int)sizeof(m_ram) - src);
        ::memset(&m_ram[dst], 0, amount);
        dst += amount;
        src = (src + amount) & 0xffff;
        size -= amount;
    }

    // Now copy possibly legal data
    int amount = min(size, (int)offsetof(memory, code) - src);
    ::memcpy(&m_ram[dst], &m_cart.get_rom()[src], amount);
    dst += amount;
    size -= amount;

    // If there is anything left to copy, it’s zeroes again
    ::memset(&m_ram[dst], 0, size);

    update_registers();
}

fix32 vm::api_dget(int16_t n)
{
    // FIXME: cannot be used before cartdata()
    return n >= 0 && n < 64 ? api_peek4(0x5e00 + 4 * n) : fix32(0);
}

void vm::api_dset(int16_t n, fix32 x)
{
    // FIXME: cannot be used before cartdata()
    if (n >= 0 && n < 64)
        api_poke4(0x5e00 + 4 * n, x);
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

    update_registers();
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

    update_registers();
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

    update_registers();
}

void vm::api_memcpy(int16_t in_dst, int16_t in_src, int16_t in_size)
{
    using std::min;

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
        lol::msg::info("z8:segv:memcpy(0x%x,0x%x,0x%x)\n", src, dst, size);
        runtime_error("bad memory access");
        return;
    }

    // If source is outside main memory, part of the operation will be
    // memset(0). But we delay the operation in case the source and the
    // destination overlap.
    int delayed_dst = dst, delayed_size = 0;
    if (src > (int)sizeof(m_ram))
    {
        delayed_size = min(size, (int)sizeof(m_ram) - src);
        dst += delayed_size;
        src = (src + delayed_size) & 0xffff;
        size -= delayed_size;
    }

    // Now copy possibly legal data
    int amount = min(size, (int)sizeof(m_ram) - src);
    memmove(&m_ram[dst], &m_ram[src], amount);
    dst += amount;
    size -= amount;

    // Fill possible zeroes we saved before, and if there is still something
    // to copy, it’s zeroes again
    if (delayed_size)
        ::memset(&m_ram[delayed_dst], 0, delayed_size);
    if (size)
        ::memset(&m_ram[dst], 0, size);

    update_registers();
}

void vm::api_memset(int16_t dst, uint8_t val, int16_t size)
{
    if (size <= 0)
        return;

    if (dst < 0 || dst + size > (int)sizeof(m_ram))
    {
        lol::msg::info("z8:segv:memset(0x%x,0x%x,0x%x)\n", dst, val, size);
        runtime_error("bad memory access");
        return;
    }

    ::memset(&m_ram[dst], val, size);

    update_registers();
}

void vm::update_registers()
{
    // PICO-8 appears to update this after a poke(). No bits are set to 1 though.
    for (uint8_t &btn : m_ram.hw_state.btn_state)
        btn &= 0x3f;
}

void vm::update_prng()
{
    m_state.prng.b = m_state.prng.a + ((m_state.prng.b >> 16) | (m_state.prng.b << 16));
    m_state.prng.a += m_state.prng.b;
}

fix32 vm::api_rnd(opt<fix32> in_range)
{
    uint32_t range = in_range ? in_range->bits() : 0x10000;
    update_prng();
    return fix32::frombits(range ? m_state.prng.b % range : 0);
}

void vm::api_srand(fix32 seed)
{
    m_state.prng.a = seed ? seed.bits() : 0xdeadbeef;
    m_state.prng.b = m_state.prng.a ^ 0xbead29ba;
    for (int i = 0; i < 32; ++i)
        update_prng();
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

    if (id == 1 || id == 2)
        return int16_t(0); // TODO (CPU usage)

    if (id == 4)
        return std::string(); // TODO (clipboard)

    if (id == 5)
    {
        // Undocumented (see http://pico-8.wikia.com/wiki/Stat)
        return (int16_t)PICO8_VERSION;
    }

    if (id == 6)
        return std::string(); // TODO (call arguments)

    if (id == 7 || id == 8 || id == 9)
        return int16_t(0); // TODO (frame rate))

    if (id >= 12 && id <= 15)
        return int16_t(0); // TODO (pause menu)

    if (id >= 16 && id <= 19)
        return m_state.channels[id & 3].sfx;

    if (id >= 20 && id <= 23)
        return m_state.channels[id & 3].sfx == -1 ? fix32(-1)
                    : fix32((int)m_state.channels[id & 3].offset);

    if (id == 24)
        return int16_t(m_state.music.pattern);

    if (id == 25)
        return int16_t(m_state.music.count);

    if (id == 26)
        return int16_t(m_state.music.offset * m_state.music.speed);

    if (id >= 30 && id <= 36)
    {
        bool devkit_mode = m_ram.draw_state.mouse_flag == 1;
        bool has_text = devkit_mode && m_state.kbd.start != m_state.kbd.stop;

        // Undocumented (see http://pico-8.wikia.com/wiki/Stat)
        switch (id)
        {
            case 30: return has_text;
            case 31:
                if (!has_text)
                    return std::string();

                if (m_state.kbd.stop > m_state.kbd.start)
                {
                    std::string ret(&m_state.kbd.chars[m_state.kbd.start],
                                    m_state.kbd.stop - m_state.kbd.start);
                    m_state.kbd.start = m_state.kbd.stop = 0;
                    return ret;
                }

                /* if (m_state.kbd.stop < m_state.kbd.start) */
                {
                    std::string ret(&m_state.kbd.chars[m_state.kbd.start],
                                    (int)sizeof(m_state.kbd.chars) - m_state.kbd.start);
                    m_state.kbd.start = 0;
                }
                return (int16_t)0;
            case 32: return devkit_mode ? m_state.mouse.x : fix32(0); break;
            case 33: return devkit_mode ? m_state.mouse.y : fix32(0); break;
            case 34: return devkit_mode ? m_state.mouse.b : fix32(0); break;
            case 35: return (int16_t)0; break; // FIXME
            case 36: return (int16_t)0; break; // FIXME
        }
    }

    if ((id >= 80 && id <= 85) || (id >= 90 && id <= 95))
    {
        time_t t;
        time(&t);
        auto const *tm = (id <= 85 ? std::gmtime : std::localtime)(&t);
        switch (id % 10)
        {
            case 0: return int16_t(tm->tm_year + 1900);
            case 1: return int16_t(tm->tm_mon + 1);
            case 2: return int16_t(tm->tm_mday);
            case 3: return int16_t(tm->tm_hour);
            case 4: return int16_t(tm->tm_min);
            case 5: return int16_t(tm->tm_sec);
        }
    }

    // TODO: everything below this is unimplemented

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
    uint8_t *btn_state = m_ram.hw_state.btn_state;

    // Update button state
    for (int i = 0; i < 64; ++i)
    {
        if (m_state.buttons[1][i])
	{
            btn_state[i / 8] |= 1 << (i % 8);
            ++m_state.buttons[0][i];
	}
        else
	{
            btn_state[i / 8] &= ~(1 << (i % 8));
            m_state.buttons[0][i] = 0;
        }
        m_state.buttons[1][i] = 0;
    }
}

var<bool, int16_t> vm::api_btn(opt<int16_t> n, int16_t p)
{
    if (n)
        return (bool)m_state.buttons[0][(*n + 8 * p) & 0x3f];

    int16_t bits = 0;
    for (int i = 0; i < 16; ++i)
        bits |= m_state.buttons[0][i] ? 1 << i : 0;
    return bits;
}

var<bool, int16_t> vm::api_btnp(opt<int16_t> n, int16_t p)
{
    // Autorepeat behaviour is overridden by the hw state, and 255 disables it
    // (https://twitter.com/lexaloffle/status/1176688167719587841)
    int delay = m_ram.hw_state.btnp_delay ? m_ram.hw_state.btnp_delay : 15;
    int rate = m_ram.hw_state.btnp_rate ? m_ram.hw_state.btnp_rate : 4;

    auto was_pressed = [delay, rate](int i) -> bool
    {
        // “Same as btn() but only true when the button was not pressed the last frame”
        if (i == 1)
            return true;
        // “btnp() also returns true every 4 frames after the button is held for 15 frames.”
        if (delay != 255 && i > delay && (i - delay - 1) % rate == 0)
            return true;
        return false;
    };

    if (n)
        return was_pressed(m_state.buttons[0][(*n + 8 * p) & 0x3f]);

    int16_t bits = 0;
    for (int i = 0; i < 16; ++i)
        bits |= was_pressed(m_state.buttons[0][i]) ? 1 << i : 0;
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

