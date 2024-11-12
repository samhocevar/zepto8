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

#pragma once

#include <lol/engine.h> // lol::net

#include <optional>
#include <variant>
#include <functional> // std::function
#include <unordered_map> // std::unordered_map

#include "zepto8.h"
#include "bios.h"
#include "pico8/cart.h"
#include "pico8/memory.h"
#include "3rdparty/z8lua/lua.h"
#include "filter.h"

namespace z8 { class player; }

namespace z8::pico8
{

template<typename T> using opt = std::optional<T>;
template<typename... T> using var = std::variant<T...>;
template<typename... T> using tup = std::tuple<T...>;

struct rich_string : public std::string {};

// The VM state: everything that does not reside in PICO-8 memory or in Lua
// memory but that we want to serialise for snapshots.
struct state
{
    // Input
    int buttons[3][64];
    float axes[8][2];
    struct { fix32 x, y, b, lb, rx, ry, ac, s[3]; } mouse;
    struct { int start = 0, stop = 0; char chars[256]; } kbd;
    struct { fix32 x, y, z; } rotation;

    // Audio states: one music and four audio channels
    struct music
    {
        int16_t count = -1;
        int16_t pattern = -1;
        uint8_t mask = 0;
        float volume_music = 0.5f;
        float volume_sfx = 0.5f;
        float fade_volume = 0.f;
        float fade_volume_step = 0.f;
        double offset = -1;
        float length = 0;
    }
    music;

    struct synth_param
    {
        uint8_t instrument = 0;
        bool custom = false;
        uint8_t filters = 0;
        uint8_t key = 0;
        float freq = 0;
        float volume = 0;
        float phi = 0;
        float last_advance = 0;
        float last_sample = 0;
        bool is_music = false;
    };

    struct sfx_state
    {
        int16_t sfx = -1;
        double offset = 0;
        double time = 0;
        int8_t prev_key = 0;
        float prev_vol = 0;
    };

    struct channel
    {
        sfx_state main_sfx;
        sfx_state custom_sfx;

        int16_t sfx_music = -1;
        float length = 0;
        bool can_loop = true;
        bool is_music = false;

        synth_param last_synth;
        synth_param fade_synth;
        float fade = 0.0f;
        uint8_t last_main_instrument = 0;
        uint8_t last_main_key = 0;

        int reverb_index = 0;
        float reverb_2[366] = {};
        float reverb_4[732] = {};

        filter damp1 = filter(filter::type::highshelf, 2400.0f, 1.0f, -6.0f);
        filter damp2 = filter(filter::type::highshelf, 1000.0f, 1.0f, -12.0f);
    }
    channels[4];
};

struct breadcrumb_path
{
    std::string cart_path;
    std::string title;
    std::string params;
};

class vm : public z8::vm_base
{
    friend class z8::player;

public:
    vm();
    virtual ~vm();

    virtual void load(std::string const &name) override;
    virtual void run() override;
    virtual void reset() override;
    virtual bool step(float seconds) override;
    virtual float getTime() override {
        return api_time();
    };

    virtual std::string const &get_code() const override;
    virtual u4mat2<128, 128> const &get_front_screen() const override;
    u4mat2<128, 128> const& get_current_screen() const;
    u4mat2<128, 128>& get_current_screen();
    virtual lol::ivec2 get_screen_resolution() const override;

    virtual int get_ansi_color(uint8_t c) const override;

    virtual void render(lol::u8vec4 *screen) const override;

    virtual void get_audio(void* inbuffer, size_t in_bytes) override;

    virtual void button(int player, int index, int state) override;
    virtual void mouse(lol::ivec2 coords, lol::ivec2 relative, int buttons, int scroll) override;
    virtual void text(char ch) override;
    virtual void sixaxis(lol::vec3 angle) override;
    virtual void axis(int player, float valueX, float valueY) override;

    virtual std::tuple<uint8_t *, size_t> ram() override;
    virtual std::tuple<uint8_t *, size_t> rom() override;

    virtual void request_exit() override { m_exit_requested = true; };
    virtual bool is_running() override { return m_is_running; };
    virtual int get_filter_index() override { return m_filter_index; }
    virtual int get_fullscreen() override { return m_fullscreen; }
    virtual void set_fullscreen(int value, bool save = true, bool runCallback = false) override
    {
        m_fullscreen = value;
        if (save)
            save_config();
    };

    virtual void set_config_dir(std::string new_path_config_dir) override
    {
        m_path_config_dir = new_path_config_dir;
        load_config();
    };

    virtual void add_extcmd(std::string const &, std::function<void(std::string const &)>) override;
    virtual void add_stat(int16_t, std::function<std::any()>) override;

private:
    void runtime_error(std::string str);
    static int panic_hook(struct lua_State *l);
    static void instruction_hook(struct lua_State *l, struct lua_Debug *ar);

    // Private methods (hidden from the user)
    opt<bool> private_cartdata(opt<std::string> str);
    bool private_is_api(std::string str);
    void private_init_ram();
    bool private_load(std::string name, opt<std::string> breadcrumb, opt<std::string> params);
    std::vector<std::string> private_dir(opt<std::string> target_dir);
    void private_stub(std::string str);

    // Asynchronous download system (WIP)
    tup<bool, bool, std::string> private_download(opt<std::string> str);
    struct
    {
        int step;
#if !__NX__
        lol::net::http::client client;
#endif
        std::string cart_path;
    }
    download_state;

    // System
    void api_run();
    void api_reload(int16_t in_dst, int16_t in_src, opt<int16_t> in_size, opt<std::string> filename);
    void api_cstore(int16_t in_dst, int16_t in_src, opt<int16_t> in_size, opt<std::string> filename);
    fix32 api_dget(int16_t addr);
    void api_dset(int16_t addr, fix32 val);
    int16_t address_translate(int16_t addr);
    uint8_t raw_peek(int16_t addr);
    std::vector<int16_t> api_peek(int16_t addr, opt<int16_t> count);
    std::vector<int16_t> api_peek2(int16_t addr, opt<int16_t> count);
    std::vector<fix32> api_peek4(int16_t addr, opt<int16_t> count);
    void raw_poke(int16_t addr, uint8_t val);
    void api_poke(int16_t addr, std::vector<int16_t> args);
    void api_poke2(int16_t addr, std::vector<int16_t> args);
    void api_poke4(int16_t addr, std::vector<fix32> args);
    void api_memcpy(int16_t dst, int16_t src, int16_t size);
    void api_memset(int16_t dst, uint8_t val, int16_t size);
    fix32 api_private_rnd(opt<fix32>);
    void api_srand(fix32);
    var<bool, int16_t, fix32, std::string, std::nullptr_t> api_stat(int16_t id);
    void api_printh(rich_string str, opt<std::string> filename, opt<bool> overwrite);
    void api_extcmd(std::string cmdline);
    void api_map_display(int16_t id);

    // I/O
    void private_buttons();
    void private_mask_buttons();
    var<bool, int16_t> api_btn(opt<int16_t> n, int16_t p);
    var<bool, int16_t> api_btnp(opt<int16_t> n, int16_t p);
    void api_serial(int16_t chan, int16_t address, int16_t len);

    // Text
    tup<uint8_t, uint8_t, uint8_t> api_cursor(uint8_t x, uint8_t y, opt<uint8_t> c);
    void api_print(opt<rich_string> str, opt<fix32> x, opt<fix32> y, opt<fix32> c);

    // Graphics
    tup<int16_t, int16_t> api_camera(int16_t x, int16_t y);
    void api_circ(int16_t x, int16_t y, int16_t r, opt<fix32> c);
    void api_circfill(int16_t x, int16_t y, int16_t r, opt<fix32> c);
    tup<uint8_t, uint8_t, uint8_t, uint8_t> api_clip(int16_t x, int16_t y,
                                                     int16_t w, opt<int16_t> h, bool intersect);
    void api_cls(uint8_t c);
    uint8_t api_color(opt<uint8_t> c);
    fix32 api_fillp(fix32 fillp);
    opt<var<int16_t, bool>> api_fget(opt<int16_t> n, opt<int16_t> f);
    void api_fset(opt<int16_t> n, opt<int16_t> f, opt<bool> b);
    void api_line(opt<fix32> arg0, opt<fix32> arg1, opt<fix32> arg2,
                  opt<fix32> arg3, opt<fix32> arg4);
    void api_map(int16_t cel_x, int16_t cel_y, int16_t sx, int16_t sy,
                 opt<int16_t> in_cel_w, opt<int16_t> in_cel_h, int16_t layer);
    fix32 api_mget(int16_t x, int16_t y);
    void api_mset(int16_t x, int16_t y, uint8_t n);
    void api_oval(int16_t x0, int16_t y0, int16_t x1, int16_t y1, opt<fix32> c);
    void api_ovalfill(int16_t x0, int16_t y0, int16_t x1, int16_t y1, opt<fix32> c);
    opt<uint8_t> api_private_pal(opt<uint8_t> c0, opt<uint8_t> c1, uint8_t p);
    var<int16_t, bool> api_palt(opt<int16_t> c, opt<bool> t);
    fix32 api_pget(int16_t x, int16_t y);
    void api_pset(int16_t x, int16_t y, opt<fix32> c);
    void api_rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, opt<fix32> c);
    void api_rectfill(int16_t x0, int16_t y0, int16_t x1, int16_t y1, opt<fix32> c);
    int16_t api_sget(int16_t x, int16_t y);
    void api_sset(int16_t x, int16_t y, opt<int16_t> c);
    void api_spr(int16_t n, int16_t x, int16_t y, opt<fix32> w,
                 opt<fix32> h, bool flip_x, bool flip_y);
    void api_sspr(int16_t sx, int16_t sy, int16_t sw, int16_t sh,
                  int16_t dx, int16_t dy, opt<int16_t> in_dw,
                  opt<int16_t> in_dh, bool flip_x, bool flip_y);
    void api_tline(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                   fix32 mx, fix32 my, opt<fix32> mdx, opt<fix32> mdy, int16_t layers);

    // Sound
    void api_music(int16_t pattern, int16_t fade_len, int16_t mask);
    void api_sfx(int16_t sfx, opt<int16_t> in_chan, int16_t offset, int16_t length);

    // Deprecated
    fix32 api_time();

public:
    // The API we export to extension languages
    template<typename T> struct exported_api
    {
        template<auto U> using bind = typename T::template bind<U>;

        std::vector<typename T::bind_desc> data =
        {
            { "run",      bind<&vm::api_run>() },
            { "reload",   bind<&vm::api_reload>() },
            { "cstore",   bind<&vm::api_cstore>() },
            { "dget",     bind<&vm::api_dget>() },
            { "dset",     bind<&vm::api_dset>() },
            { "peek",     bind<&vm::api_peek>() },
            { "peek2",    bind<&vm::api_peek2>() },
            { "peek4",    bind<&vm::api_peek4>() },
            { "poke",     bind<&vm::api_poke>() },
            { "poke2",    bind<&vm::api_poke2>() },
            { "poke4",    bind<&vm::api_poke4>() },
            { "memcpy",   bind<&vm::api_memcpy>() },
            { "memset",   bind<&vm::api_memset>() },
            { "__rnd",    bind<&vm::api_private_rnd>() },
            { "srand",    bind<&vm::api_srand>() },
            { "stat",     bind<&vm::api_stat>() },
            { "printh",   bind<&vm::api_printh>() },
            { "extcmd",   bind<&vm::api_extcmd>() },
            { "_map_display",   bind<&vm::api_map_display>() },

            { "btn",  bind<&vm::api_btn>() },
            { "btnp", bind<&vm::api_btnp>() },

            { "cursor", bind<&vm::api_cursor>() },
            { "print",  bind<&vm::api_print>() },

            { "camera",   bind<&vm::api_camera>() },
            { "circ",     bind<&vm::api_circ>() },
            { "circfill", bind<&vm::api_circfill>() },
            { "clip",     bind<&vm::api_clip>() },
            { "cls",      bind<&vm::api_cls>() },
            { "color",    bind<&vm::api_color>() },
            { "fillp",    bind<&vm::api_fillp>() },
            { "fget",     bind<&vm::api_fget>() },
            { "fset",     bind<&vm::api_fset>() },
            { "line",     bind<&vm::api_line>() },
            { "map",      bind<&vm::api_map>() },
            { "mget",     bind<&vm::api_mget>() },
            { "mset",     bind<&vm::api_mset>() },
            { "oval",     bind<&vm::api_oval>() },
            { "ovalfill", bind<&vm::api_ovalfill>() },
            { "__pal",    bind<&vm::api_private_pal>() },
            { "palt",     bind<&vm::api_palt>() },
            { "pget",     bind<&vm::api_pget>() },
            { "pset",     bind<&vm::api_pset>() },
            { "rect",     bind<&vm::api_rect>() },
            { "rectfill", bind<&vm::api_rectfill>() },
            { "sget",     bind<&vm::api_sget>() },
            { "sset",     bind<&vm::api_sset>() },
            { "spr",      bind<&vm::api_spr>() },
            { "sspr",     bind<&vm::api_sspr>() },
            { "tline",    bind<&vm::api_tline>() },

            { "music", bind<&vm::api_music>() },
            { "sfx",   bind<&vm::api_sfx>() },

            { "time", bind<&vm::api_time>() },

            { "__buttons",  bind<&vm::private_buttons>() },
            { "__mask_buttons",  bind<&vm::private_mask_buttons>() },
            { "__set_pause",  bind<&vm::private_set_pause>() },
            { "__end_render",  bind<&vm::private_end_render>() },
            { "__cartdata", bind<&vm::private_cartdata>() },
            { "__download", bind<&vm::private_download>() },
            { "__is_api",   bind<&vm::private_is_api>() },
            { "__init_ram", bind<&vm::private_init_ram>() },
            { "__load",     bind<&vm::private_load>() },
            { "__dir",      bind<&vm::private_dir>() },
            { "__stub",     bind<&vm::private_stub>() },
        };
    };

private:
    uint8_t get_pixel(int16_t x, int16_t y) const;
    uint8_t pixel(int x, int y, u4mat2<128, 128> const& screen) const;
    void private_set_pause(bool pause);
    void private_end_render();

    uint32_t to_color_bits(opt<fix32> c);
    uint32_t raw_to_bits(uint8_t c) const;

    void scrool_screen(int16_t scroll_amount);

    void set_pixel(int16_t x, int16_t y, uint32_t color_bits);

    void hline(int16_t x1, int16_t x2, int16_t y, uint32_t color_bits);
    void vline(int16_t x, int16_t y1, int16_t y2, uint32_t color_bits);

    float get_synth_sample(state::synth_param &params);
    void update_sfx_state(state::sfx_state& cur_sfx, state::synth_param& new_synth, float freq_factor, float length, bool is_music, bool can_loop, bool half_rate, double inv_frames_per_second);
    void update_registers();
    void update_prng();
    void set_music_pattern(int pattern);
    void launch_sfx(int16_t sfx, int16_t chan, float offset, float length, bool is_music);

    bool load_cartdata();
    bool save_cartdata(bool force);

    bool load_cart(cart& target_cart, std::string const& filename);
    bool save_cart(cart& target_cart, std::string const& filename);

    bool load_config();
    bool save_config();

    std::string get_path_config();
    std::string get_path_cstore(std::string cart_name);
    std::string get_path_save(std::string cart_name);
    void set_path_active_dir(std::string filename);
    std::string get_path_active_dir();
    std::string get_default_carts_dir();

    void fill_metadata(cart& metadata_cart);

public:
    // TODO: try to get rid of this
    struct lua_State *m_sandbox_lua;

private:
    struct lua_State *m_lua;
    cart m_cart;
    memory m_ram;
    state m_state;
    u4mat2<128, 128> m_front_buffer;
    draw_state_t m_front_draw_state;
    hw_state_t m_front_hw_state;

    int m_filter_index = 0;
    int m_fullscreen = 1;
    bool m_pointer_locked = false;

    // multiscreen
    int m_multiscreen_current = 0;
    int m_multiscreens_x = 1;
    int m_multiscreens_y = 1;
    std::vector<std::shared_ptr<u4mat2<128, 128>>> m_multiscreens;

    bool m_in_pause = false;

    // Files
    std::string m_cartdata;
    int m_cartdata_min_frames_between_saves = 360;
    int m_cartdata_frames_since_last_save = 0;
    bool m_cartdata_dirty = false;

    // Label & cart title/author
    std::string m_metadata_title;
    std::string m_metadata_author;

    // breadcrumb and params
    std::vector<breadcrumb_path> breadcrumbs;

    lol::timer m_timer;
    int m_instructions = 0;
    int m_max_instructions = 300000;

    std::string m_path_active_dir;
    std::string m_path_config_dir = "zepto-8";

    std::vector<std::string> m_ui_texts = {  "pause"
                                            ,"options"
                                            ,"music"
                                            ,"sfx"
                                            ,"screen"
                                            ,"filter"
                                            ,"back"
                                            ,"continue"
                                            ,"reset cart"
                                            ,"quit"
                                            ,"windowed"
                                            ,"full" };

    std::unordered_map<std::string, std::function<void(std::string const&)>> m_extcmds;
    std::unordered_map<int16_t, std::function<std::any()>> m_stats;

    bool m_watch_file_change = false;
    bool m_exit_requested = false;
    bool m_is_running = true;
};

} // namespace z8::pico8
