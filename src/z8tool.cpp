//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2019 Sam Hocevar <sam@hocevar.net>
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
#include <iostream>
#include <streambuf>

#include "zepto8.h"
#include "pico8/vm.h"
#include "raccoon/vm.h"
#include "telnet.h"
#include "splore.h"
#include "dither.h"
#include "minify.h"
#include "compress.h"

enum class mode
{
    none,
    run      = 130,
    inspect  = 131,
    headless = 132,
    telnet   = 133,
    splore   = 134,
    dither   = 135,
    minify   = 136,
    compress = 137,

    tolua  = 140,
    topng  = 141,
    top8   = 143,
    tobin  = 144,
    todata = 145,

    out     = 'o',
    data    = 150,
    hicolor = 151,
    error_diffusion = 152,
    raw     = 153,
    skip    = 154,
};

static void usage()
{
    printf("Usage: z8tool [--tolua|--topng|--top8|--tobin|--todata] [--data <file>] <cart> [-o <file>]\n");
    printf("       z8tool --dither [--hicolor] [--error-diffusion] <image> [-o <file>]\n");
    printf("       z8tool --minify\n");
    printf("       z8tool --compress [--raw <num>] [--skip <num>]\n");
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
    opt.add_opt(int(mode::run),      "run",      true);
    opt.add_opt(int(mode::dither),   "dither",   true);
    opt.add_opt(int(mode::minify),   "minify",   false);
    opt.add_opt(int(mode::compress), "compress", false);
    opt.add_opt(int(mode::inspect),  "inspect",  true);
    opt.add_opt(int(mode::headless), "headless", true);
    opt.add_opt(int(mode::tolua),    "tolua",    false);
    opt.add_opt(int(mode::topng),    "topng",    false);
    opt.add_opt(int(mode::top8),     "top8",     false);
    opt.add_opt(int(mode::tobin),    "tobin",    false);
    opt.add_opt(int(mode::todata),   "todata",   false);
    opt.add_opt(int(mode::out),      "out",      true);
    opt.add_opt(int(mode::data),     "data",     true);
    opt.add_opt(int(mode::hicolor),  "hicolor",  false);
    opt.add_opt(int(mode::raw),      "raw",      true);
    opt.add_opt(int(mode::skip),     "skip",     true);
    opt.add_opt(int(mode::error_diffusion), "error-diffusion", false);
#if HAVE_UNISTD_H
    opt.add_opt(int(mode::telnet),   "telnet",   true);
#endif
    opt.add_opt(int(mode::splore),   "splore",   true);

    mode run_mode = mode::none;
    char const *data = nullptr;
    char const *in = nullptr;
    char const *out = nullptr;
    size_t raw = 0, skip = 0;
    bool hicolor = false;
    bool error_diffusion = false;

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
        case (int)mode::run:
        case (int)mode::headless:
        case (int)mode::inspect:
        case (int)mode::dither:
        case (int)mode::telnet:
        case (int)mode::splore:
            run_mode = mode(c);
            in = opt.arg;
            break;
        case (int)mode::minify:
        case (int)mode::compress:
        case (int)mode::tolua:
        case (int)mode::topng:
        case (int)mode::top8:
        case (int)mode::tobin:
        case (int)mode::todata:
            run_mode = mode(c);
            break;
        case (int)mode::data:
            data = opt.arg;
            break;
        case (int)mode::out:
            out = opt.arg;
            break;
        case (int)mode::hicolor:
            hicolor = true;
            break;
        case (int)mode::raw:
            raw = atoi(opt.arg);
            break;
        case (int)mode::skip:
            skip = atoi(opt.arg);
            break;
        case (int)mode::error_diffusion:
            error_diffusion = true;
            break;
        default:
            return EXIT_FAILURE;
        }
    }

    if (!in)
        in = argv[opt.index];

    if (run_mode == mode::tolua || run_mode == mode::top8 ||
        run_mode == mode::tobin || run_mode == mode::topng ||
        run_mode == mode::todata || run_mode == mode::inspect)
    {
        z8::pico8::cart cart;
        cart.load(in);

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
            printf("Compressed code size: %d\n", (int)cart.get_compressed_code().size());
        }
    }
    else if (run_mode == mode::run || run_mode == mode::headless)
    {
        std::unique_ptr<z8::vm_base> vm;
        if (lol::ends_with(in, ".rcn.json"))
            vm.reset((z8::vm_base *)new z8::raccoon::vm());
        else
            vm.reset((z8::vm_base *)new z8::pico8::vm());
        vm->load(in);
        vm->run();
        for (bool running = true; running; )
        {
            lol::timer t;
            running = vm->step(1.f / 60.f);
            if (run_mode == mode::run)
            {
                vm->print_ansi(lol::ivec2(128, 64), nullptr);
                t.wait(1.f / 60.f);
            }
        }
    }
    else if (run_mode == mode::dither)
    {
        z8::dither(in, out, hicolor, error_diffusion);
    }
    else if (run_mode == mode::minify)
    {
        auto input = std::string{ std::istreambuf_iterator<char>(std::cin),
                                  std::istreambuf_iterator<char>() };
        std::cout << z8::minify(input) << '\n';
    }
    else if (run_mode == mode::compress)
    {
        std::vector<uint8_t> input;
        for (uint8_t ch : std::vector<char>{ std::istreambuf_iterator<char>(std::cin),
                                             std::istreambuf_iterator<char>() })
        input.push_back(ch);

        // Compress input buffer
        std::vector<uint8_t> output = z8::compress(input);

        // Output result, encoded according to user-provided flags
        if (raw > 0)
        {
            fwrite(output.data(), 1, std::min(raw, output.size()), stdout);
        }
        else
        {
            if (skip > 0)
                output.erase(output.begin(), output.begin() + std::min(skip, output.size()));
            std::cout << z8::encode59(output) << '\n';
        }
    }
    else if (run_mode == mode::splore)
    {
        z8::splore splore;
        splore.dump(in);
    }
#if HAVE_UNISTD_H
    else if (run_mode == mode::telnet)
    {
        z8::telnet telnet;
        telnet.run(in);
    }
#endif
    else
    {
        usage();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

