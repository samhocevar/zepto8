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

#include <lol/engine.h> // lol::Application
#include <lol/cli>   // lol::cli
#include <lol/utils> // lol:ends_with
#include <iostream>  // std::cout

#include "zepto8.h"
#include "player.h"
#include "raccoon/vm.h"

int main(int argc, char **argv)
{
    lol::sys::init(argc, argv);

    std::optional<std::string> cart;
    lol::ivec2 win_size(144 * 4, 144 * 4);

    lol::cli::app opts("zepto8");
    opts.set_version_flag("-V,--version", PACKAGE_VERSION);
    opts.add_option("cart", cart, "Load a cartridge")->type_name("<cart>");

    opts.add_option("-width", win_size.x, "Set the window width")->type_name("<int>");
    opts.add_option("-height", win_size.y, "Set the window height")->type_name("<int>");
    // -window n
    // -volume n
    // -joystick n
    // -pixel_perfect n
    // -preblit_scale n
    // -draw_rect x,y,w,h
    opts.add_option("-run", cart, "Load and run a cartridge")->type_name("<cart>");
    // -x filename
    // -export param_str
    // -p param_str
    // -splore
    // -home path
    // -root_path path
    // -desktop path
    // -screenshot_scale n
    // -gif_scale n
    // -gif_len n
    // -gui_theme n
    // -timeout n
    // -software_blit n
    // -foreground_sleep_ms n
    // -background_sleep_ms n
    // -accept_future n

    CLI11_PARSE(opts, argc, argv);

    lol::Application app("zepto8", win_size, 60.0f);

    bool is_raccoon = cart && lol::ends_with(*cart, ".rcn.json");

    z8::player *player = new z8::player(false, is_raccoon);

    if (cart)
    {
        player->load(*cart);
        player->run();
    }

    app.Run();

    return EXIT_SUCCESS;
}

