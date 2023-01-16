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
#include <lol/cli>  // lol::cli
#include <iostream> // std::cout
#include <optional> // std::optional

#include "zepto8.h"
#include "player.h"
#include "ide/ide.h"

int main(int argc, char **argv)
{
    lol::sys::init(argc, argv);

    std::optional<std::string> cart;

    lol::cli::app opts("zepto8");
    opts.add_option("cart", cart, "cartridge");
    opts.set_version_flag("-V,--version", PACKAGE_VERSION);

    CLI11_PARSE(opts, argc, argv);

    lol::ivec2 win_size(1280, 768);
    auto app = lol::app::init("zepto8 IDE", win_size, 60.0f);

    auto ide = new z8::ide();

    if (cart)
    {
        ide->load(*cart);
    }

    app->run();

    return EXIT_SUCCESS;
}

