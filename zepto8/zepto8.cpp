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
#include "vm.h"

enum class mode
{
    cart,
    pico2lua,
};

int main(int argc, char **argv)
{
    lol::sys::init(argc, argv);

    lol::getopt opt(argc, argv);
    opt.add_opt(128, "pico2lua", true);

    mode run_mode = mode::cart;

    for (;;)
    {
        int c = opt.parse();
        if (c == -1)
            break;

        switch (c)
        {
        case 128: /* --pico2lua */
            run_mode = mode::pico2lua;
            break;
        default:
            return EXIT_FAILURE;
        }
    }

    if (run_mode == mode::pico2lua)
    {
        std::ifstream t(argv[2]);
        std::stringstream buffer;
        buffer << t.rdbuf();

        z8::code_fixer fixer(buffer.str().c_str());
        lol::String new_code = fixer.fix();

        printf("%s", new_code.C());
    }
    else if (run_mode == mode::cart)
    {
        if (argc < 2)
            return EXIT_FAILURE;

        lol::Application app("zepto-8", lol::ivec2(600, 600), 60.0f);

        z8::vm *vm = new z8::vm();
        vm->load(argv[1]);
        vm->run();

        app.Run();
    }

    return EXIT_SUCCESS;
}

