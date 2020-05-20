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

#include <lol/engine.h> // lol::File
#include <lol/cli>   // lol::cli
#include <lol/msg>   // lol::msg
#include <lol/utils> // lol::ends_with
#include <fstream>
#include <sstream>
#include <iostream>
#include <streambuf>
#if _MSC_VER
#include <io.h>
#include <fcntl.h>
#endif

#include "zepto8.h"
#include "pico8/vm.h"
#include "pico8/pico8.h"
#include "raccoon/vm.h"
#include "telnet.h"
#include "splore.h"
#include "dither.h"
#include "minify.h"
#include "compress.h"

enum class mode
{
    none,

    tolua, topng, top8, tobin, todata,

    dither,
    minify,
    compress,
    run,
    inspect,
    headless,
    telnet,
    splore,
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

    mode run_mode = mode::none;
    std::string in, out, data;
    size_t raw = 0, skip = 0;
    bool hicolor = false;
    bool error_diffusion = false;

    auto help = lol::cli::required("-h", "--help").call([]()
                    { ::usage(); exit(EXIT_SUCCESS); });

    auto convert =
    (
        ( lol::cli::required("--tolua").set(run_mode, mode::tolua) |
          lol::cli::required("--topng").set(run_mode, mode::topng) |
          lol::cli::required("--top8").set(run_mode, mode::top8) |
          lol::cli::required("--tobin").set(run_mode, mode::tobin) |
          lol::cli::required("--todata").set(run_mode, mode::todata) ),
        lol::cli::option("--data") & lol::cli::value("file", data),
        lol::cli::value("cart", in),
        lol::cli::option("-o") & lol::cli::value("file", out)
    );

    auto dither =
    (
        lol::cli::required("--dither").set(run_mode, mode::dither),
        lol::cli::option("--hicolor").set(hicolor, true),
        lol::cli::option("--error-diffusion").set(error_diffusion, true),
        lol::cli::value("image").set(in),
        lol::cli::option("-o") & lol::cli::value("output", out)
    );

    auto minify = lol::cli::required("--minify").set(run_mode, mode::minify);

    auto compress =
    (
        lol::cli::required("--compress").set(run_mode, mode::compress),
        lol::cli::option("--raw") & lol::cli::value("num", raw),
        lol::cli::option("--skip") & lol::cli::value("num", skip)
    );

    auto other =
    (
        ( lol::cli::required("--run").set(run_mode, mode::run) |
          lol::cli::required("--inspect").set(run_mode, mode::inspect) |
          lol::cli::required("--headless").set(run_mode, mode::headless) |
#if HAVE_UNISTD_H
          lol::cli::required("--telnet").set(run_mode, mode::telnet) |
#endif
          lol::cli::required("--splore").set(run_mode, mode::splore) )
        & lol::cli::value("cart", in)
    );

    auto success = lol::cli::parse(argc, argv, help | convert | dither | minify | compress | other);

    if (!success)
        return EXIT_FAILURE;

    if (run_mode == mode::tolua || run_mode == mode::top8 ||
        run_mode == mode::tobin || run_mode == mode::topng ||
        run_mode == mode::todata || run_mode == mode::inspect)
    {
        z8::pico8::cart cart;
        cart.load(in);

        if (data.length())
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
            using std::min;
            memcpy(&cart.get_rom(), s.c_str(), min(s.length(), size_t(0x4300)));
        }

        if (run_mode == mode::tolua)
        {
            printf("%s", cart.get_code().c_str());
        }
        else if (run_mode == mode::top8)
        {
            printf("%s", cart.get_p8().c_str());
        }
        else if (run_mode == mode::tobin)
        {
            auto const &bin = cart.get_bin();
            fwrite(bin.data(), 1, bin.size(), stdout);
        }
        else if (run_mode == mode::topng)
        {
            if (!out.length())
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

            auto &code = cart.get_code();
            if (z8::pico8::code::parse(code))
                printf("Code is valid\n");
            else
                printf("Code has syntax errors\n");
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
#if _MSC_VER
        _setmode(_fileno(stdin), _O_BINARY);
#endif
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

