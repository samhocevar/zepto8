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

#include "zepto8.h"
#include "vm.h"

int main(int argc, char **argv)
{
    lol::System::Init(argc, argv);

    if (argc < 2)
        return EXIT_FAILURE;

    lol::Application app("ZEPTO-8", lol::ivec2(640, 640), 60.0f);

    z8::vm *vm = new z8::vm();
    vm->load(argv[1]);
    vm->run();

    app.Run();

    return EXIT_SUCCESS;
}

