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
#include <lol/cli>   // lol::cli
#include <lol/utils> // lol:ends_with
#include <iostream>  // std::cout

#include "zepto8.h"
#include "player.h"
#include "raccoon/vm.h"

int main(int argc, char **argv)
{
    lol::sys::init(argc, argv);

    std::string cart;
    bool has_cart = false;

    auto opts =
    (
        lol::cli::option("-V", "--version").call([]()
            { std::cout << PACKAGE_VERSION << '\n'; exit(EXIT_SUCCESS); })
            % "output version information and exit",
        lol::cli::opt_value("cart", cart).set(has_cart, true)
    );

    auto success = lol::cli::parse(argc, argv, opts);

    if (!success)
    {
        std::cout << lol::cli::usage_lines(opts, argv[0]) << '\n';
        return EXIT_FAILURE;
    }

    lol::ivec2 win_size(144 * 4, 144 * 4);
    lol::Application app("zepto-8", win_size, 60.0f);

    bool is_raccoon = has_cart && lol::ends_with(cart, ".rcn.json");

    z8::player *player = new z8::player(false, is_raccoon);

    if (has_cart)
    {
        player->load(cart);
        player->run();
    }

    app.Run();

    return EXIT_SUCCESS;
}

