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

#include <lol/engine.h> // lol::net
#include <lol/msg>      // lol::msg
#include <lol/file>     // lol::file
#include <lol/utils> // lol::split

#include <algorithm>  // std::min
#include <filesystem>
#include <format>     // std::format
#include <chrono>
#include <ctime>
#include <cassert>
#include <sstream> // std::stringstream

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
    {
        size_t len;
        char const *s = lua_tolstring(l, n, &len);
        arg.assign(s, len);
    }
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
    load_config();

    m_bios = std::make_unique<bios>();

    m_lua = luaL_newstate();
    lua_atpanic(m_lua, &vm::panic_hook);
    lua_setpico8memory(m_lua, (uint8_t *)&m_ram);
    luaL_openlibs(m_lua);

    bindings::lua::init(m_lua, this);

    // Automatically yield every 1000 instructions
    lua_sethook(m_lua, &vm::instruction_hook, LUA_MASKCOUNT, 1000);

    // Clear memory
    private_init_ram();

    // Initialise VM state (TODO: check what else to init)
    ::memset(m_state.buttons, 0, sizeof(m_state.buttons));
    ::memset(&m_state.mouse, 0, sizeof(m_state.mouse));

    // Initialize Zepto8 runtime
    int status = luaL_dostring(m_lua, m_bios->get_code().c_str());
    if (status != LUA_OK)
    {
        char const *message = lua_tostring(m_lua, -1);
        lol::msg::error("error %d loading bios.p8: %s\n", status, message);
        lua_pop(m_lua, 1);
        assert(false);
    }
}

vm::~vm()
{
    save_cartdata(true);
    lua_close(m_lua);
}

std::string const &vm::get_code() const
{
    return m_cart.get_code();
}

u4mat2<128, 128> const &vm::get_front_screen() const
{
    return m_front_buffer;
}

u4mat2<128, 128> const& vm::get_current_screen() const
{
    if (m_in_pause) return m_front_buffer;
    if (m_ram.draw_state.misc_features.multi_screen)
    {
        if (m_multiscreen_current > 0 && m_multiscreen_current <= m_multiscreens.size())
        {
            return *m_multiscreens[m_multiscreen_current - 1];
        }
    }
    return m_ram.hw_state.mapping_screen == 0 ? m_ram.gfx : m_ram.screen;
}

u4mat2<128, 128>& vm::get_current_screen()
{
    return const_cast<u4mat2<128, 128> &>(std::as_const(*this).get_current_screen());
}

lol::ivec2 vm::get_screen_resolution() const
{
    return lol::ivec2(128 * m_multiscreens_x, 128 * m_multiscreens_y);
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
    assert(false);
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

    // FIXME: we should verify if we are in a coroutine or not before yielding
    // if cart is slow to render, this function can sometimes be triggered from bios

    // The value 135000 was found using trial and error, but it causes
    // side effects in lots of cases. Use 300000 instead.
    // FIXME: this is because we do not consider system costs, see
    // https://pico-8.fandom.com/wiki/CPU for more information
    that->m_instructions += 1000;
    if (that->m_instructions >= that->m_max_instructions)
    {
        lua_getglobal(l, "__z8_is_inside_main_loop");
        bool is_inside_loop = lua_toboolean(l, -1);

        lua_yield(l, 0);
        if (!is_inside_loop)
        {
            // should only handle buttons if we are outside _draw or _update
            that->private_buttons();
        }
    }
}

tup<bool, bool, std::string> vm::private_download(opt<std::string> str)
{
#if !__NX__ && !__SCE__
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
        if (lid.size() == 0 || mid.size() == 0)
            return std::make_tuple(true, false, std::string("no cart info found"));

        // Write cart info
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

        if (!lol::file::write(nfo_path, download_state.client.get_result()))
            return std::make_tuple(true, false, "can't save nfo file");

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
    case 4:
        // Step 4: bail out if errors, load cart otherwise
        if (download_state.client.get_status() != lol::net::http::status::success)
            return std::make_tuple(true, false, std::string("error downloading cart"));

        // Save cart
        if (!lol::file::write(download_state.cart_path, download_state.client.get_result()))
            return std::make_tuple(true, false, "can't save cart file");

        // Load cart
        load(download_state.cart_path);
        return std::make_tuple(true, true, std::string());
    default:
        break;
    }
#endif

    return std::make_tuple(false, false, std::string());
}

void vm::private_init_ram()
{
    ::memset(&m_ram, 0, sizeof(m_ram));

    // init mapping default values:
    m_ram.hw_state.mapping_screen = 0x60;
    m_ram.hw_state.mapping_map = 0x20;
    m_ram.hw_state.mapping_map_width = 0x80;

    // Initialise the PRNG with the current time
    auto now = std::chrono::high_resolution_clock::now();
    api_srand(fix32::frombits((int32_t)now.time_since_epoch().count()));

    // also reset timer, maybe should be done in a separate function?
    m_timer.reset();
}

bool vm::private_load(std::string name, opt<std::string> breadcrumb, opt<std::string> params)
{
    save_cartdata(true);

    std::string previous_cart = m_cart.get_filename();

    name = get_path_active_dir() + "/" + name;
    // tmp fix: if extension is not .p8 or .png, set it to .p8
    if (!lol::ends_with(lol::tolower(name), ".p8") && !lol::ends_with(lol::tolower(name), ".png"))
        name += ".p8";
    // Load cart from a file
    if (!load_cart(m_cart, name))
        return false;

    if (breadcrumb.has_value() && (*breadcrumb).length() > 1)
    {
        breadcrumb_path new_breadcrumb;
        new_breadcrumb.cart_path = previous_cart;
        new_breadcrumb.title = *breadcrumb;
        new_breadcrumb.params = params.has_value() ? *params : "";
        breadcrumbs.push_back(new_breadcrumb);
    }

    run();
    return true;
}

void vm::load(std::string const &name)
{
    save_cartdata(true);

    set_path_active_dir(name);
    load_cart(m_cart, name);
}

bool vm::load_cart(cart &target_cart, std::string const& filename)
{
    bool has_loaded = target_cart.load(filename);
    if (has_loaded)
    {
        // TODO: in pico 8, cstore is saved using file hash as a filename
        // goal is that if you make a new version of your cart, it will change hash and so have a separate save
        // also works if you have several cartridge with same file name
        std::string name_cstore = get_path_cstore(filename);
        bool save_exist = false;
        if (std::filesystem::exists(name_cstore))
        {
            save_exist = true;
        }
        if (save_exist)
        {
            // Load cart from the save file
            auto reload_cart = std::make_shared<cart>();
            reload_cart->load(name_cstore);
            // copy save cart rom to loaded cart rom
            target_cart.set_from_ram(reload_cart->get_rom(), 0, 0, offsetof(memory, code));
        }
    }
    return has_loaded;
}

bool vm::save_cart(cart& target_cart, std::string const& filename)
{
    // save in cstore
    std::string name_cstore = get_path_cstore(filename);
    bool success = target_cart.save(name_cstore);
    return success;
}

void vm::reset()
{
    load(m_cart.get_filename());
    run();
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

bool vm::step(float /* seconds */)
{
    if (m_exit_requested)
    {
        save_cartdata(true);
        m_is_running = false;
        return false;
    }

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

    save_cartdata(false);

    if (m_watch_file_change && m_cart.has_file_changed())
    {
        if (load_cart(m_cart, m_cart.get_filename())) {
            run();
        }
    }

    return ret;
}

void vm::button(int player, int index, int state)
{
    m_state.buttons[1][player * 8 + index] += state;
}

void vm::mouse(lol::ivec2 coords, lol::ivec2 relative, int buttons, int scroll)
{
    m_state.mouse.x = (double)coords.x;
    m_state.mouse.y = (double)coords.y;
    m_state.mouse.b = (double)buttons;
    m_state.mouse.s[0] = (double)scroll;

    bool has_button_pressed = buttons > 0 && m_state.mouse.lb != m_state.mouse.b;
    if (has_button_pressed)
        m_state.mouse.ac = 4;
    else
        m_state.mouse.ac = int(std::min(abs(double(m_state.mouse.rx) - relative.x) + abs(double(m_state.mouse.ry) - relative.y), 3.0));
    
    m_state.mouse.rx = (double)relative.x;
    m_state.mouse.ry = (double)relative.y;
    
    if (m_ram.draw_state.mouse_flags.buttons)
    {
        m_state.buttons[1][5] += (buttons & 0x1) ? 1 : 0;
        m_state.buttons[1][4] += (buttons & 0x2) ? 1 : 0;
        m_state.buttons[1][6] += (buttons & 0x4) ? 1 : 0;
    }
}

void vm::text(char ch)
{
    // Convert uppercase characters to special glyphs
    if (ch >= 'A' && ch <= 'Z')
        ch = '\x80' + (ch - 'A');
    m_state.kbd.chars[m_state.kbd.stop] = ch;
    m_state.kbd.stop = (m_state.kbd.stop + 1) % (int)sizeof(m_state.kbd.chars);
}

void vm::sixaxis(lol::vec3 angle)
{
    m_state.rotation.x = angle.x;
    m_state.rotation.y = angle.y;
    m_state.rotation.z = angle.z;
}

void vm::axis(int player, float valueX, float valueY)
{
    m_state.axes[player][0] = valueX;
    m_state.axes[player][1] = valueY;
}

//
// System
//

void vm::api_run()
{
    save_cartdata(true);

    // Initialise VM state (TODO: check what else to init)
    ::memset(m_state.buttons, 0, sizeof(m_state.buttons));
    ::memset(&m_state.mouse, 0, sizeof(m_state.mouse));

    // reset multiscreen
    m_ram.draw_state.misc_features.multi_screen = false;
    m_multiscreens_x = 1;
    m_multiscreens_y = 1;

    // Load cartridge code and call __z8_run_cart() on it
    lua_getglobal(m_sandbox_lua, "__z8_run_cart");
    lua_pushstring(m_sandbox_lua, m_cart.preprocess_code().c_str());
    lua_pcall(m_sandbox_lua, 1, 0, 0);
}

void vm::api_reload(int16_t in_dst, int16_t in_src, opt<int16_t> in_size, opt<std::string> filename)
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
    // FIXME: may not be true anymore with extended memory, as full 0xffff are accessible
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

    if (filename.has_value())
    {
        std::string name = get_path_active_dir() + "/" + filename.value();
        // tmp fix: if extension is not .p8 or .png, set it to .p8
        if (!lol::ends_with(lol::tolower(name), ".p8") && !lol::ends_with(lol::tolower(name), ".png"))
            name += ".p8";
        // Load cart from a file
        auto reload_cart = std::make_shared<cart>();
        load_cart(*reload_cart, name);
        // copy rom from loaded cart
        ::memcpy(&m_ram[dst], &reload_cart->get_rom()[src], amount);
    }
    else
    {
        // from same cart
        ::memcpy(&m_ram[dst], &m_cart.get_rom()[src], amount);
    }

    dst += amount;
    size -= amount;

    // If there is anything left to copy, it’s zeroes again
    ::memset(&m_ram[dst], 0, size);

    update_registers();
}

void vm::api_cstore(int16_t in_dst, int16_t in_src, opt<int16_t> in_size, opt<std::string> filename)
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

    if (filename.has_value())
    {
        std::string name = get_path_active_dir() + "/" + filename.value();
        // tmp fix: if extension is not .p8 or .png, set it to .p8
        if (!lol::ends_with(lol::tolower(name), ".p8") && !lol::ends_with(lol::tolower(name), ".png"))
            name += ".p8";
        // Load cart from a file
        auto reload_cart = std::make_shared<cart>();
        load_cart(*reload_cart, name);
        reload_cart->set_from_ram(m_ram, dst, src, size);
        save_cart(*reload_cart, reload_cart->get_filename());
    }
    else
    {
        // from same cart
        m_cart.set_from_ram(m_ram, dst, src, size);
        save_cart(m_cart, m_cart.get_filename());
    }

    update_registers();
}

fix32 vm::api_dget(int16_t n)
{
    // FIXME: cannot be used before cartdata()
    return n >= 0 && n < 64 ? api_peek4(0x5e00 + 4 * n, 1)[0] : fix32(0);
}

void vm::api_dset(int16_t n, fix32 x)
{
    // FIXME: cannot be used before cartdata()
    if (n >= 0 && n < 64)
        api_poke4(0x5e00 + 4 * n, std::vector<fix32> { x });
}

uint8_t vm::raw_peek(int16_t addr)
{
    addr = address_translate(addr);
    return m_ram[addr];
}

std::vector<int16_t> vm::api_peek(int16_t addr, opt<int16_t> count)
{
    std::vector<int16_t> ret;
    // Note: peek() is the same as peek(0)
    size_t n = count ? std::max(0, std::min(int(*count), 8192)) : 1;

    for ( ; ret.size() < n; ++addr)
    {
        int16_t bits = 0;
        bits = raw_peek(addr);
        ret.push_back(bits);
    }

    return ret;
}

std::vector<int16_t> vm::api_peek2(int16_t addr, opt<int16_t> count)
{
    std::vector<int16_t> ret;
    size_t n = count ? std::max(0, std::min(int(*count), 8192)) : 1;

    for ( ; ret.size() < n; addr += 2)
    {
        int16_t bits = 0;
        for (int i = 0; i < 2; ++i)
        {
            bits |= raw_peek(addr + i) << (8 * i);
        }
        ret.push_back(bits);
    }

    return ret;
}

std::vector<fix32> vm::api_peek4(int16_t addr, opt<int16_t> count)
{
    std::vector<fix32> ret;
    size_t n = count ? std::max(0, std::min(int(*count), 8192)) : 1;

    for ( ; ret.size() < n; addr += 4)
    {
        int32_t bits = 0;
        for (int i = 0; i < 4; ++i)
        {
            bits |= raw_peek(addr + i) << (8 * i);
        }
        ret.push_back(fix32::frombits(bits));
    }

    return ret;
}

int16_t vm::address_translate(int16_t addr)
{
    if (addr >= 0x0000 && addr < 0x2000 && m_ram.hw_state.mapping_spritesheet == 0x60) addr = addr + 0x6000;
    if (addr >= 0x6000 && m_ram.hw_state.mapping_screen == 0) addr = addr - 0x6000;
    return addr;
}

void vm::raw_poke(int16_t addr, uint8_t val)
{
    if (addr >= 0x5e00 && addr < 0x5f00) m_cartdata_dirty = true;
    addr = address_translate(addr);
    m_ram[addr] = (uint8_t)val;
}

void vm::api_poke(int16_t addr, std::vector<int16_t> args)
{
    // Note: poke() is the same as poke(0, 0)
    if (args.empty())
        args.push_back(0);

    for (auto val : args)
        raw_poke(addr++, (uint8_t)val);

    update_registers();
}

void vm::api_poke2(int16_t addr, std::vector<int16_t> args)
{
    // Note: poke2() is the same as poke2(0, 0)
    if (args.empty())
        args.push_back(0);

    for (auto val : args)
    {
        raw_poke(addr++, (uint8_t)val);
        raw_poke(addr++, (uint8_t)((uint16_t)val >> 8));
    }

    update_registers();
}

void vm::api_poke4(int16_t addr, std::vector<fix32> args)
{
    // Note: poke4() is the same as poke4(0, 0)
    if (args.empty())
        args.push_back(fix32(0));

    for (auto val : args)
    {
        uint32_t x = (uint32_t)val.bits();
        raw_poke(addr++, (uint8_t)x);
        raw_poke(addr++, (uint8_t)(x >> 8));
        raw_poke(addr++, (uint8_t)(x >> 16));
        raw_poke(addr++, (uint8_t)(x >> 24));
    }

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

    if (src < dst) // copy from the end
    {
        int16_t addr_dst = dst + size;
        int16_t addr_src = src + size;
        for (int16_t i = 0; i < size; ++i)
            raw_poke(--addr_dst, raw_peek(--addr_src));
    }
    else
    {
        int16_t addr_dst = dst;
        int16_t addr_src = src;
        for (int16_t i = 0; i < size; ++i)
            raw_poke(addr_dst++, raw_peek(addr_src++));
    }

    update_registers();
}

void vm::api_memset(int16_t dst, uint8_t val, int16_t size)
{
    if (size <= 0)
        return;

    for (int16_t i = 0; i < size; ++i)
        raw_poke(dst++, val);

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
    auto &prng = m_ram.hw_state.prng;
    prng.a = ((prng.a >> 16) | (prng.a << 16)) + prng.b;
    prng.b += prng.a;
}

bool vm::load_cartdata()
{
    // todo: verify cartdata is not empty
    std::string s;
    if (!lol::file::read(get_path_save(m_cartdata), s))
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
            m_ram.persistent[index] = x & 0xff;
        }
        j++;
    }

    return true;
}

bool vm::save_cartdata(bool force)
{
    if (m_cartdata.size() == 0) return false;

    m_cartdata_frames_since_last_save += 1;
    if (!force)
    {
        if (!m_cartdata_dirty) return true;
        if (m_cartdata_frames_since_last_save < m_cartdata_min_frames_between_saves) return true;
    }
    m_cartdata_frames_since_last_save = 0;
    m_cartdata_dirty = false;

    std::string content;
    for (int i = 0; i < 256; ++i)
    {
        char hex[3];
        // pico 8 store the numbers in reverse order from ram
        int index = (i & ~0x3) + 3 - i % 4;
        std::snprintf(hex, sizeof(hex), "%02x", m_ram.persistent[index]);
        content += hex;
        if (i % 32 == 31)
        {
            content += "\n";
        }
    }

    // todo: verify cartdata is not empty
    if (!lol::file::write(get_path_save(m_cartdata), content))
        return false;

    return true;
}

bool config_parse_256(std::string line, std::string name, float& value)
{
    if (!line.starts_with(name)) return false;
    std::string rest = line.substr(name.length());
    int rawvalue = std::stoi(rest);
    value = std::clamp(rawvalue / 256.0f, 0.0f, 1.0f);
    return true;
}

std::string config_make_256(std::string name, float value)
{
    int intvalue = std::clamp(int(value * 256.0f), 0, 256);
    return name + " " + std::to_string(intvalue) + "\n";
}

bool config_parse_int(std::string line, std::string name, int& value)
{
    if (!line.starts_with(name)) return false;
    std::string rest = line.substr(name.length());
    value = std::stoi(rest);
    return true;
}

std::string config_make_int(std::string name, int value)
{
    return name + " " + std::to_string(value) + "\n";
}

bool vm::load_config()
{
    std::string s;
    if (!lol::file::read(get_path_config(), s))
        return false;

    auto ss = std::stringstream(s);

    for (std::string line; std::getline(ss, line, '\n');)
    {
        config_parse_256(line, "sound_volume", m_state.music.volume_sfx);
        config_parse_256(line, "music_volume", m_state.music.volume_music);
        config_parse_int(line, "filter_index", m_filter_index);
        config_parse_int(line, "fullscreen_method", m_fullscreen);
    }

    return true;
}

bool vm::save_config()
{
    std::string content;
    content += "// :: Audio Settings\n";
    content += config_make_256("sound_volume", m_state.music.volume_sfx);
    content += config_make_256("music_volume", m_state.music.volume_music);
    content += config_make_int("filter_index", m_filter_index);
    content += config_make_int("fullscreen_method", m_fullscreen);

    if (!lol::file::write(get_path_config(), content))
        return false;

    return true;
}

fix32 vm::api_private_rnd(opt<fix32> in_range)
{
    if (in_range && in_range->bits() == 0) return fix32(0);
    update_prng();
    uint32_t a = m_ram.hw_state.prng.a;
    uint32_t range = in_range ? uint32_t(in_range->bits()) : 0x10000;
    return fix32::frombits(range > 0 ? a % range : 0);
}

void vm::api_srand(fix32 seed)
{
    // PICO-8 removes the seed’s MSB
    seed &= fix32::frombits(0x7fffffff);
    auto &prng = m_ram.hw_state.prng;
    prng.b = seed ? seed.bits() : 0xdeadbeef;
    prng.a = prng.b ^ 0xbead29ba;
    for (int i = 0; i < 32; ++i)
        update_prng();
}

template<class... T>
static auto any_to_variant(std::any a) -> var<T...>
{
    if (!a.has_value())
        return nullptr;

    opt<var<T...>> v = std::nullopt;

    if (!((a.type() == typeid(T) && (v = std::any_cast<T>(std::move(a)), true)) || ...))
        return nullptr;

    return std::move(*v);
}

var<bool, int16_t, fix32, std::string, std::nullptr_t> vm::api_stat(int16_t id)
{
    // Documented PICO-8 stat() arguments:
    //  0       Memory usage (0..2048)
    //  1       CPU used since last flip (1.0 == 100% CPU at 30fps)
    //  2       CPU used (system)
    //  3       Current Display
    //  4       Clipboard contents (after user has pressed CTRL-V)
    //  5       Pico-8 version
    //  6       Parameter string
    //  7       Current framerate
    //  8       Target framerate (30 or 60)
    //  9       Pico 8 framerate (system)
    // 
    //  11      Number of displays (1-4)
    //  12..15  Pause menu location
    //  16..19  Index of currently playing SFX on channels 0..3 (DEPRECATED, use 46)
    //  20..23  Note number (0..31) on channel 0..3             (DEPRECATED, use 46)
    //  24      Currently playing pattern index                 (DEPRECATED, use 46)
    //  25      Total patterns played                           (DEPRECATED, use 46)
    //  26      Ticks played on current pattern                 (DEPRECATED, use 46)
    //        
    //  28      Raw keyboard
    // 
    //  30..39  Mouse and keyboard
    //
    //  46..49  Index of currently playing SFX on channels 0..3
    //  50..53  Note number (0..31) on channel 0..3
    //  54      Currently playing pattern index
    //  55      Total patterns played
    //  56      Ticks played on current pattern
    //  57      is music playing
    //  80..85  UTC time: year, month, day, hour, minute, second
    //  90..95  Local time
    // 
    //  99      Garbage collection (raw)
    //
    //  100     Current breadcrumb label, or nil”
    //  101     current BBS ID
    //  102     website address
    //
    //  108..109 PCM Audio
    //  110      frame-by-frame mode
    //  120..121 Bytestream Availability
    //  124      Current path
    //
    //  130      ZEPTO metadata title
    //  131      ZEPTO metadata author
    
    //  140      ZEPTO Audio Volume Music
    //  141      ZEPTO Audio Volume Sfx
    //  142      ZEPTO Filter Name
    //  143      ZEPTO Fullscreen status
    //  144      ZEPTO Language
    //  145      ZEPTO Platform
    
    //  150..153 Gamepads axes values
    //  160..169 Physical sensors - gyro, magneto,etc.

    //  200..250 ZEPTO UI texts

    // Registered user functions have priority
    if (auto it = m_stats.find(id); it != m_stats.end())
    {
        auto ret = it->second();
        return any_to_variant<bool, int16_t, fix32, std::string, std::nullptr_t>(ret);
    }

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
        return fix32(m_instructions / float(m_max_instructions));

    if (id == 3)
        return int16_t(m_ram.draw_state.misc_features.multi_screen ? m_multiscreen_current : 0);

    if (id == 4)
        return std::string(); // TODO (clipboard)

    if (id == 5)
    {
        // Undocumented (see http://pico-8.wikia.com/wiki/Stat)
        return int16_t(PICO8_VERSION);
    }

    if (id == 6)
    {
        // TODO: get console command at launch param?
        if (breadcrumbs.size() > 0) return breadcrumbs.back().params;
        return "";
    }

    if (id == 7 || id == 8 || id == 9)
        return int16_t(0); // TODO (frame rate))

    if (id == 11)
        return int16_t(m_ram.draw_state.misc_features.multi_screen ? m_multiscreens_x * m_multiscreens_y : 1);

    if (id >= 12 && id <= 15)
        return int16_t(0); // TODO (pause menu)

    // two audio stats blocks
    if ((id >= 16 && id <= 26) || (id >= 46 && id <= 56))
    {
        int16_t audio_id = (id <= 26) ? id : id - 30;
        if (audio_id >= 16 && audio_id <= 19) 
            return m_state.channels[audio_id & 3].main_sfx.sfx;

        if (audio_id >= 20 && audio_id <= 23)
            return m_state.channels[audio_id & 3].main_sfx.sfx == -1 ? fix32(-1)
            : fix32((int)m_state.channels[audio_id & 3].main_sfx.offset);

        if (audio_id == 24)
            return int16_t(m_state.music.pattern);

        if (audio_id == 25)
            return int16_t(m_state.music.count);

        if (audio_id == 26)
            return int16_t(m_state.music.offset);
    }
    if (id == 57)
        return m_state.music.pattern != -1;

    if (id == 29)
    {
        // Undocumented, but PICO-8 calls codo_count_joysticks() here
        return int16_t(0);
    }

    if (id >= 30 && id <= 39)
    {
        bool devkit_mode = m_ram.draw_state.mouse_flags.enabled;
        bool devkit_pointer = devkit_mode && m_ram.draw_state.mouse_flags.locked;
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
            case 35: return (int16_t)0; break; // FIXME horizontal mouse wheel / two-finger-swipe
            case 36: return devkit_mode ? m_state.mouse.s[2] : fix32(0); break;
            case 37: return devkit_mode ? m_state.mouse.ac : fix32(0); break;
            case 38: return devkit_pointer ? m_state.mouse.rx : fix32(0); break;
            case 39: return devkit_pointer ? m_state.mouse.ry : fix32(0); break;
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

    if (id == 100)
    {
        if (breadcrumbs.size() > 0) return breadcrumbs.back().title;
        return nullptr;
    }

    if (id == 101)
        return nullptr;

    if (id == 124) return "/";

    if (id == 130) return m_metadata_title;
    if (id == 131) return m_metadata_author;
    
    if (id == 140) return fix32(m_state.music.volume_music);
    if (id == 141) return fix32(m_state.music.volume_sfx);
    if (id == 142) return "none";
    if (id == 143) return m_fullscreen;
    if (id == 149) return m_quit_confirmation;

    // Gamepads axes values
    if (id == 150) return fix32(m_state.axes[0][0]); // P1 X
    if (id == 151) return fix32(m_state.axes[0][1]); // P1 Y
    if (id == 152) return fix32(m_state.axes[1][0]); // P2 X
    if (id == 153) return fix32(m_state.axes[1][1]); // P2 Y

    if (id >= 160 && id < 170)
    {
        switch (id)
        {
        case 160: // get rotation
            return std::format("{} {} {}", (float)m_state.rotation.x, (float)m_state.rotation.y, (float)m_state.rotation.z); ;
        }
    }

    if (id >= 200 && id < 200 + m_ui_texts.size())
    {
        return m_ui_texts[id - 200];
    }

    return (int16_t)0;
}

void vm::add_stat(int16_t id, std::function<std::any()> fn)
{
    m_stats[id] = fn;
}

void vm::api_printh(rich_string str, opt<std::string> filename, opt<bool> overwrite)
{
    std::string decoded;
    for (uint8_t ch : str)
        decoded += charset::to_utf8[ch];

    // TODO: if filename is "@clip" the message should replaces the contents of the system clipboard instead of writing to a file
    if (filename.has_value())
    {
        FILE* file = fopen(filename.value().c_str(), overwrite.value_or(false) ? "w" : "a");
        if (file)
        {
            decoded += "\n"; // end of line
            fwrite(decoded.data(), sizeof(char), decoded.size(), file);
            fclose(file);
        }
        else
        {
            lol::msg::info("printh cannot open file %s to write to\n", filename.value().c_str());
        }
    }
    else
    {
        fprintf(stdout, "%s\n", decoded.c_str());
        fflush(stdout);
    }
}

void vm::fill_metadata(cart &metadata_cart)
{
    m_metadata_title = metadata_cart.get_title();
    m_metadata_author = metadata_cart.get_author();
    std::vector<uint8_t>& label = metadata_cart.get_label();
    if (label.size() >= LABEL_WIDTH * LABEL_HEIGHT)
    {
        // Direct copy to sprite memory
        for (int y = 0; y < LABEL_HEIGHT; ++y)
            for (int x = 0; x < LABEL_WIDTH; x+=2)
            {
                uint8_t col = label[y * LABEL_WIDTH + x] & 0x1f;
                col |= (label[y * LABEL_WIDTH + x + 1] & 0x1f) << 4;
                m_ram[(x/2) + y*64] = col;
            }
    }
}

void vm::api_extcmd(std::string cmdline)
{
    std::string cmd = cmdline.substr(0, cmdline.find(" "));
    std::string args = cmdline.substr(std::min(cmd.length() + 1, cmdline.length()));

    // Registered user functions have priority
    if (auto it = m_extcmds.find(cmd); it != m_extcmds.end())
    {
        it->second(args);
        return;
    }

    if (cmd == "z8_load_metadata")
    {
        if (args.length() > 0)
        {
            std::string name = get_path_active_dir() + "/" + args;
            // tmp fix: if extension is not .p8 or .png, set it to .p8
            if (!lol::ends_with(lol::tolower(name), ".p8") && !lol::ends_with(lol::tolower(name), ".png"))
                name += ".p8";
            // Load cart from a file
            auto metadata_cart = std::make_shared<cart>();
            load_cart(*metadata_cart, name);
            fill_metadata(*metadata_cart);
        }
        else
        {
            fill_metadata(m_cart);
        }
    }
    else if (cmd == "reset")
    {
        api_run();
    }
    else if (cmd == "pause")
    {
        luaL_dostring(m_lua, "__z8_enter_pause()");
    }
    else if (cmd == "z8_volume_music_up")
    {
        m_state.music.volume_music = std::clamp(m_state.music.volume_music + 0.125f, 0.0f, 1.0f);
        save_config();
    }
    else if (cmd == "z8_volume_music_down")
    {
        m_state.music.volume_music = std::clamp(m_state.music.volume_music - 0.125f, 0.0f, 1.0f);
        save_config();
    }
    else if (cmd == "z8_volume_sfx_up")
    {
        m_state.music.volume_sfx   = std::clamp(m_state.music.volume_sfx + 0.125f, 0.0f, 1.0f);
        save_config();
    }
    else if (cmd == "z8_volume_sfx_down")
    {
        m_state.music.volume_sfx   = std::clamp(m_state.music.volume_sfx - 0.125f, 0.0f, 1.0f);
        save_config();
    }
    else if (cmd == "z8_quit_confirmation")
    {
        if (args.length() > 0)
        {
            m_quit_confirmation = args == "true" || args == "1";
        }
        else
        {
            m_quit_confirmation = false;
        }
    }
    else if (cmd == "z8_app_requestexit")
    {
        request_exit();
    }
    else if (cmd == "breadcrumb" || cmd == "go_back")
    {
        if (breadcrumbs.size() == 0) return;
        breadcrumb_path breadcrumb = breadcrumbs.back();
        breadcrumbs.pop_back();
        save_cartdata(true);
        m_cart.load(breadcrumb.cart_path);
        run();
    }
    else if (cmd == "z8_setuitext")
    {
        auto param1 = args.substr(0, args.find(" "));
        auto param2 = args.substr(std::min(param1.length() + 1, args.length()));

        if (param1.length() > 0 && param2.length() > 0)
        {
            try
            {
                int index = std::stoi(param1);
                if (index >= 0 && index < m_ui_texts.size())
                {
                    m_ui_texts[index] = param2;
                }
            }
            catch (std::exception const& e)
            {
                lol::msg::info("ui text parse error: %s\n", e.what());
            }
        }
    }
    else if (cmd == "label" || cmd == "screen" || cmd == "rec" || cmd == "video")
    {
        // Command is unimplemented
        private_stub(std::format("extcmd({})\n", cmdline));
    }
    else
    {
        // Command is unknown
        lol::msg::error("unknown extcmd: %s\n", cmdline.c_str());
    }
}

void vm::add_extcmd(std::string const &name, std::function<void(std::string const&)> fn)
{
    m_extcmds[name] = fn;
}

void vm::api_map_display(int16_t id)
{
    if (!m_ram.draw_state.misc_features.multi_screen) return;
    m_multiscreen_current = id;

    if (m_multiscreen_current > 0)
    {
        // tmp: at least 2 horizontal screen
        m_multiscreens_x = std::max(m_multiscreens_x, 2);
        // tmp: if try to write to screen 2, we have at least 2 in height
        if (m_multiscreen_current >= 2)
        {
            m_multiscreens_y = std::max(m_multiscreens_y, 2);
        }
        // tmp: if try to write to screen 4, we have a super-wide cart 
        if (m_multiscreen_current >= 4)
        {
            m_multiscreens_x = std::max(m_multiscreens_x, 4);
        }
        int target_screen = m_multiscreen_current - 1;
        while (target_screen >= m_multiscreens.size())
        {
            m_multiscreens.push_back(std::make_shared< u4mat2<128, 128> >());
        }
    }
}

//
// I/O
//

void vm::private_buttons()
{
    uint8_t *btn_state = m_ram.hw_state.btn_state;

    // Update button state
    for (int i = 0; i < 64; ++i)
    {
        // masking system: each button is inactive after a load/reset until it has been fully released once
        if (m_state.buttons[1][i] == 0)
            m_state.buttons[2][i] = 1;
        if (m_state.buttons[2][i] == 0)
            m_state.buttons[1][i] = 0;

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
    m_state.mouse.s[2] = m_state.mouse.s[1] - m_state.mouse.s[0];
    m_state.mouse.s[1] = m_state.mouse.s[0];
    m_state.mouse.lb = m_state.mouse.b;

    // also handle possible pointer lock request here
    if (m_pointer_locked != m_ram.draw_state.mouse_flags.locked)
    {
        m_pointer_locked = m_ram.draw_state.mouse_flags.locked;
        //pointerLock_callback(m_ram.draw_state.mouse_flags.locked);
    }
}

void vm::private_mask_buttons()
{
    for (int i = 0; i < 64; ++i)
        m_state.buttons[2][i] = 0;
}

var<bool, int16_t> vm::api_btn(opt<int16_t> n, int16_t p)
{
    if (n)
    {
        if (n < 0 || n >= 8 || p < 0 || p >= 8) return false;
        return bool(m_state.buttons[0][(*n + 8 * p) & 0x3f]);
    }   

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
    {
        if (n < 0 || n >= 8 || p < 0 || p >= 8) return false;
        return was_pressed(m_state.buttons[0][(*n + 8 * p) & 0x3f]);
    }

    int16_t bits = 0;
    for (int i = 0; i < 16; ++i)
        bits |= was_pressed(m_state.buttons[0][i]) ? 1 << i : 0;
    return bits;
}

void vm::api_serial(int16_t chan, int16_t address, int16_t len)
{
    private_stub(std::format("serial(0x{:4x}, 0x{:4x}, 0x{:4x})", chan, address, len));
}

//
// Deprecated
//

fix32 vm::api_time()
{
    return fix32(double(m_timer.poll()));
}

} // namespace z8::pico8

