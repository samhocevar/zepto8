//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2018 Sam Hocevar <sam@hocevar.net>
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
#include "vm/vm.h"
#include "telnet.h"
#include "splore.h"

enum class mode
{
    none,
    run      = 130,
    inspect  = 131,
    headless = 132,
    telnet   = 133,
    splore   = 134,

    tolua  = 140,
    topng  = 141,
    top8   = 143,
    tobin  = 144,
    todata = 145,

    out    = 'o',
    data   = 150,
};

static void usage()
{
    printf("Usage: z8tool [--tolua|--topng|--top8|--tobin|--todata] [--data <file>] <cart> [-o <file>]\n");
    printf("       z8tool --run <cart>\n");
    printf("       z8tool --inspect <cart>\n");
    printf("       z8tool --headless <cart>\n");
#if HAVE_UNISTD_H
    printf("       z8tool --telnet <cart>\n");
#endif
    printf("       z8tool --splore <image>\n");
}

int main(int argc, char **argv)
{
    lol::sys::init(argc, argv);

    lol::getopt opt(argc, argv);
    opt.add_opt('h',                 "help",     false);
    opt.add_opt(int(mode::run),      "run",      false);
    opt.add_opt(int(mode::inspect),  "inspect",  false);
    opt.add_opt(int(mode::headless), "headless", false);
    opt.add_opt(int(mode::tolua),    "tolua",    false);
    opt.add_opt(int(mode::topng),    "topng",    false);
    opt.add_opt(int(mode::top8),     "top8",     false);
    opt.add_opt(int(mode::tobin),    "tobin",    false);
    opt.add_opt(int(mode::todata),   "todata",   false);
    opt.add_opt(int(mode::out),      "out",      true);
    opt.add_opt(int(mode::data),     "data",     true);
#if HAVE_UNISTD_H
    opt.add_opt(int(mode::telnet),   "telnet",   false);
#endif
    opt.add_opt(int(mode::splore),   "splore",   false);

    mode run_mode = mode::none;
    char const *data = nullptr;
    char const *out = nullptr;

    for (;;)
    {
        int c = opt.parse();
        if (c == -1)
            break;

        switch (c)
        {
        case 'h':
            usage();
            return EXIT_SUCCESS;
        case (int)mode::tolua:
        case (int)mode::topng:
        case (int)mode::top8:
        case (int)mode::tobin:
        case (int)mode::todata:
        case (int)mode::run:
        case (int)mode::inspect:
        case (int)mode::headless:
        case (int)mode::telnet:
        case (int)mode::splore:
            run_mode = mode(c);
            break;
        case (int)mode::data:
            data = opt.arg;
            break;
        case (int)mode::out:
            out = opt.arg;
            break;
        default:
            return EXIT_FAILURE;
        }
    }

    char const *cart_name = argv[opt.index];

    if (run_mode == mode::tolua || run_mode == mode::top8 ||
        run_mode == mode::tobin || run_mode == mode::topng ||
        run_mode == mode::todata || run_mode == mode::inspect)
    {
        z8::cart cart;
        cart.load(cart_name);

        if (data)
        {
            std::string s;
            lol::File f;
            for (auto const &candidate : lol::sys::get_path_list(data))
            {
                f.Open(candidate, lol::FileAccess::Read);
                if (f.IsValid())
                {
                    s = f.ReadString();
                    f.Close();

                    lol::msg::debug("loaded file %s (%d bytes, max %d)\n",
                                    candidate.c_str(), int(s.length()), 0x4300);
                    break;
                }
            }
            memcpy(&cart.get_rom(), s.c_str(), lol::min(int(s.length()), 0x4300));
        }

        if (run_mode == mode::tolua)
        {
            printf("%s", cart.get_lua().c_str());
        }
        else if (run_mode == mode::top8)
        {
            printf("%s", cart.get_p8().c_str());
        }
        else if (run_mode == mode::tobin)
        {
            auto const &bin = cart.get_bin();
            fwrite(&bin, 1, sizeof(bin), stdout);
        }
        else if (run_mode == mode::topng)
        {
            if (!out)
                return EXIT_FAILURE;
            cart.get_png().save(out);
        }
        else if (run_mode == mode::todata)
        {
            fwrite(&cart.get_rom(), 1, 0x4300, stdout);
        }
        else if (run_mode == mode::inspect)
        {
            printf("Code size: %d\n", (int)cart.get_p8().size());
            printf("Compressed code size: %d\n", (int)cart.get_compressed_code().count());
        }
    }
    else if (run_mode == mode::run || run_mode == mode::headless)
    {
        z8::vm vm;
        vm.load(cart_name);
        vm.run();
        while (true)
        {
            lol::timer t;
            vm.step(1.f / 60.f);
            if (run_mode == mode::run)
            {
                vm.print_ansi();
                t.wait(1.f / 60.f);
            }
        }
    }
    else if (run_mode == mode::splore)
    {
        z8::splore splore;
        splore.dump(cart_name);
    }
#if HAVE_UNISTD_H
    else if (run_mode == mode::telnet)
    {
        z8::telnet telnet;
        telnet.run(cart_name);
    }
#endif
    else
    {
        usage();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

