//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016 Sam Hocevar <sam@hocevar.net>
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

#include <fstream>
#include <sstream>

#include "zepto8.h"
#include "player.h"

int main(int argc, char **argv)
{
    lol::sys::init(argc, argv);

    lol::getopt opt(argc, argv);

    for (;;)
    {
        int c = opt.parse();
        if (c == -1)
            break;

        switch (c)
        {
        default:
            return EXIT_FAILURE;
        }
    }

    lol::ivec2 win_size(z8::WINDOW_WIDTH, z8::WINDOW_HEIGHT);
    lol::Application app("zepto-8", win_size, 60.0f);

    z8::player *player = new z8::player(win_size);

    if (argc >= 2)
    {
        player->load(argv[1]);
        player->run();
    }

    app.Run();

    return EXIT_SUCCESS;
}

