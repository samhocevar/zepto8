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

#include <lol/msg>    // lol::msg
#include <lol/narray> // lol::array2d
#include <array>      // std::array
#include <cstring>    // std::memset
#include <memory>     // std::shared_ptr
#include <vector>     // std::vector

#include "zepto8.h"
#include "pico8/vm.h"
#include "pico8/pico8.h"
#include "raccoon/vm.h"

#include "libretro.h"

#define EXPORT extern "C" RETRO_API

// Callbacks provided by retroarch
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t enviro_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

// Global core state
static bool is_raccoon;
static std::shared_ptr<z8::vm_base> vm;
static lol::array2d<lol::u8vec4> fb32;
static lol::array2d<uint16_t> fb16;

EXPORT void retro_set_environment(retro_environment_t cb)
{
    enviro_cb = cb;
    // We can run without a cartridge
    bool no_rom = true;
    enviro_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);
}

EXPORT void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
EXPORT void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
EXPORT void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
EXPORT void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
EXPORT void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

EXPORT void retro_init()
{
    // Allocate framebuffer and VM
    vm.reset((z8::vm_base *)new z8::pico8::vm());
    fb32.resize(lol::ivec2(128, 128));
    fb16.resize(lol::ivec2(128, 128));
}

EXPORT void retro_deinit()
{
    fb32.clear();
    fb16.clear();
}

EXPORT unsigned retro_api_version()
{
   return RETRO_API_VERSION;
}

EXPORT void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(*info));
    info->library_name = "zepto8";
    info->library_version = PACKAGE_VERSION;
    info->valid_extensions = "p8|p8.png|rcn.json";
    info->need_fullpath = true; // we load our own carts for now
}

EXPORT void retro_get_system_av_info(struct retro_system_av_info *info)
{
    memset(info, 0, sizeof(*info));
    info->geometry.base_width = 128;
    info->geometry.base_height = 128;
    info->geometry.max_width = 128;
    info->geometry.max_height = 128;
    info->geometry.aspect_ratio = 1.f;
    info->timing.fps = 60.f;
    info->timing.sample_rate = 44100.f;

    retro_pixel_format pf = RETRO_PIXEL_FORMAT_RGB565;
    enviro_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pf);
}

EXPORT void retro_set_controller_port_device(unsigned port, unsigned device)
{
}

EXPORT void retro_reset()
{
}

static std::array<int, 7> buttons
{
    RETRO_DEVICE_ID_JOYPAD_LEFT,
    RETRO_DEVICE_ID_JOYPAD_RIGHT,
    RETRO_DEVICE_ID_JOYPAD_UP,
    RETRO_DEVICE_ID_JOYPAD_DOWN,
    RETRO_DEVICE_ID_JOYPAD_A,
    RETRO_DEVICE_ID_JOYPAD_B,
    RETRO_DEVICE_ID_JOYPAD_START,
};

EXPORT void retro_run()
{
    // Update input
    input_poll_cb();
    for (int n = 0; n < 8; ++n)
        for (int k = 0; k < 7; ++k)
            vm->button(8 * n + k, input_state_cb(n, RETRO_DEVICE_JOYPAD, 0, buttons[k]));

    // Step VM
    vm->step(1.f / 60);

    // Render video, convert to RGB565, send back to frontend
    vm->render(fb32.data());
    for (int y = 0; y < 128; ++y)
    for (int x = 0; x < 128; ++x)
        fb16(x, y) = uint16_t(lol::dot(lol::ivec3(fb32(x, y).rgb) / lol::ivec3(8, 4, 8),
                                       lol::ivec3(2048, 32, 1)));
    video_cb(fb16.data(), 128, 128, 2 * 128);

    // Render audio
}

EXPORT size_t retro_serialize_size()
{
    return 0;
}

EXPORT bool retro_serialize(void *data, size_t size)
{
    return false;
}

EXPORT bool retro_unserialize(const void *data, size_t size)
{
    return false;
}

EXPORT void retro_cheat_reset()
{
}

EXPORT void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
}

EXPORT bool retro_load_game(struct retro_game_info const *info)
{
    is_raccoon = lol::ends_with(info->path, ".rcn.json");
    if (is_raccoon)
        vm.reset((z8::vm_base *)new z8::raccoon::vm());
    else
        vm.reset((z8::vm_base *)new z8::pico8::vm());
    vm->load(info->path);
    vm->run();
    return true;
}

EXPORT bool retro_load_game_special(unsigned game_type,
    const struct retro_game_info *info, size_t num_info)
{
    return false;
}

EXPORT void retro_unload_game()
{
}

EXPORT unsigned retro_get_region()
{
    return 0;
}

EXPORT void *retro_get_memory_data(unsigned id)
{
    return nullptr;
}

EXPORT size_t retro_get_memory_size(unsigned id)
{
    return 0;
}

