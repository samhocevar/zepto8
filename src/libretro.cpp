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

#include <lol/engine.h>  // lol::input
#include <lol/transform> // lol::mat4
#include <lol/color>     // lol::color

#include "player.h"

#include "zepto8.h"
#include "pico8/vm.h"
#include "pico8/pico8.h"
#include "raccoon/vm.h"

extern "C"
{
#include "libretro.h"
}

namespace z8
{

extern "C" void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(*info));
    info->library_name = "zepto8";
    info->library_version = "0.0.0";
    info->valid_extensions = "p8|p8.png";
    info->need_fullpath = true; // we load our own carts for now
}

extern "C" void retro_get_system_av_info(struct retro_system_av_info *info)
{
    memset(info, 0, sizeof(*info));
    info->geometry.base_width  = info->geometry.max_width  = 128;
    info->geometry.base_height = info->geometry.max_height = 128;
    info->geometry.aspect_ratio = 1.f;
    info->timing.fps = 60.f;
    info->timing.sample_rate = 44100.f;
}

extern "C" void retro_set_environment(retro_environment_t cb) 
{
    // We can run without a cartridge
    bool no_rom = true;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);
}

extern "C" bool retro_load_game(const struct retro_game_info *info)
{
    // TODO: load cart
    (void)info;
    return true;
}

extern "C" void retro_unload_game()
{
}

extern "C" void retro_run(void)
{
    // TODO: vm->step(), audio_cb(), video_cb()
}


} // namespace z8

