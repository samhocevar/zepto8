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
#include <lol/cli>  // lol::cli
#include <iostream> // std::cout

#include "zepto8.h"
#include "player.h"
#include "ide/ide.h"

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

    lol::ivec2 win_size(1280, 768);
    lol::Application app("zepto-8", win_size, 60.0f);

    auto ide = new z8::ide();

    if (has_cart)
    {
        ide->load(cart);
    }

    app.Run();

    return EXIT_SUCCESS;
}

