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
#include <lol/cli>    // lol::cli
#include <lol/msg>    // lol::msg
#include <lol/utils>  // lol::ends_with
#include <lol/thread> // lol::timer
#include <fstream>    // std::ofstream
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

    test,
    stats,
    luamin,
    listlua,
    printast,
    convert,
    run, headless, telnet,

    dither,
    compress,
    splore,
};

void test()
{
#if 1
    int const foo[] = { 2, 4, 8, 10, 16, 26, 41, 48, 64, 85, 112, 128, 224, 256 };
    for (int as : foo)
    {
        int const steps = 4;
        size_t chars = 65535;
        //size_t chars = 127;
        float time[2] = { 0 };
        size_t size[2] = { 0 };
        for (int k = 0; k < steps; ++k)
        {
            std::string data;
            for (size_t i = 0; i < chars; ++i)
                data += char((as <= 10 ? '0' : as <= 128 ? 'A' : '\0') + lol::rand(as));
            lol::timer t;
            auto c0 = z8::pico8::code::compress(data, z8::pico8::code::format::pxa);
            time[0] += t.get();
            auto c1 = z8::pico8::code::compress(data, z8::pico8::code::format::pxa_fast);
            time[1] += t.get();
            size[0] += c0.size();
            size[1] += c1.size();
        }
        float time0 = time[0] * 1000.f / steps;
        float time1 = time[1] * 1000.f / steps;
        float ratio0 = float(size[0]) / steps / chars * 100.f;
        float ratio1 = float(size[1]) / steps / chars * 100.f;
        printf("n %d\tbest %.2f%% %.2fms\tfast %.2f%% %.2fms\n", as, ratio0, time0, ratio1, time1);
    }
#elif 0
    int const foo[] = { 2, 4, 8, 10, 16, 26, 41, 48, 64, 85, 112, 128, 224, 256 };
    //std::vector<int> foo(50, 10);
    for (int as = 2; as < 240; ++as)
    //for (int as : foo)
    {
        int const steps = 4;
        int bytes = 8192;
        int chars = int(std::ceil(bytes * 8.f / std::log2(float(as))));
        int comp[steps];
        float mean = 0.f;
        lol::timer t;
        printf("%d %d %d ", as, bytes, chars);
        for (int k = 0; k < steps; ++k)
        {
            std::string data;
            for (int i = 0; i < chars; ++i)
                data += char((as <= 10 ? '0' : as <= 128 ? 'A' : '\0') + lol::rand(as));
            comp[k] = int(z8::pico8::code::compress(data, z8::pico8::code::format::pxa).size()) - 8;
            printf("%d ", comp[k]);
            mean += float(comp[k]) / steps;
        }
        float eff = mean / bytes;
        float time = t.get() * 1000.f / steps;
        printf("%f %f %f\n", mean, eff, time);
    }
#else
    // alphabet size
    //for (int a = 20; a <= 256; ++a)
    for (int a = 200; a <= 256; ++a)
    {
        float bits = std::log2(float(a));

        // char group size
        for (int n = 1; n < 32; ++n)
        {
            // total bits that a group can encode
            float gbits = n * bits;
            // effective bits it will encode
            int ebits = int(std::floor(gbits));

            bool ok = true;
            for (int k = 2; k < n; ++k)
                if ((n / k * k == n) && (ebits / k * k == ebits))
                    ok = false;

            if (!ok || ebits <= 0 || ebits > 64)
                continue;

            // amount of data we want to encode
            int bytes = 2048;
            // number of chars this represents
            int nchars = int(std::ceil(bytes * 8.f / ebits * n));

            size_t source = 0;
            size_t compressed = 0;

            for (int step = 0; step < 100; ++step)
            {
                std::string data;
                for (int i = 0; i < nchars; ++i)
                {
                    auto ch = char(255 - lol::rand(a));
                    switch (ch)
                    {
                        case '\\': data += "\\\\"; break;
                        case '"': data += "\\\""; break;
                        case '\r': data += "\\r"; break;
                        case '\n': data += "\\n"; break;
                        case '\0': data += "\\0"; break;
                        default: data += ch; break;
                    }
                }

                source += bytes;
                compressed += z8::pico8::code::compress(data, z8::pico8::code::format::pxa).size();
            }

            float eff = float(source) / compressed;
            printf("% 3u % 2u % 3.4f % 2u % 5u %f\n", a, n, gbits, ebits, nchars, eff);
            fflush(stdout);
        }
    }
#endif
}

int main(int argc, char **argv)
{
    lol::sys::init(argc, argv);

    mode run_mode = mode::none, override_mode = mode::none;
    std::string in, out, data, palette;
    size_t raw = 0, skip = 0;
    bool hicolor = false;
    bool error_diffusion = false;

    lol::cli::app app("z8tool");

    // Compatibility with p8tool
    app.add_subcommand("stats", "Print statistics about a cart")
        ->callback([&]() { run_mode = mode::stats; })
        ->add_option("cart", in, "Cartridge to load")->required();

    app.add_subcommand("listlua", "Extract Lua code from a cart")
        ->callback([&]() { run_mode = mode::listlua; })
        ->add_option("cart", in, "Cartridge to load")->required();

    app.add_subcommand("luamin", "Minify the Lua code of a cart")
        ->callback([&]() { run_mode = mode::luamin; })
        ->add_option("cart", in, "Cartridge to load")->required();

    app.add_subcommand("printast", "Print an abstract syntax tree of the code of a cart")
        ->callback([&]() { run_mode = mode::printast; })
        ->add_option("cart", in, "Cartridge to load")->required();

    // Exists as writep8 in p8tool
    auto convert = app.add_subcommand("convert", "Convert a cart to a different format")
                       ->callback([&]() { run_mode = mode::convert; });
    convert->add_option("--data", data, "Binary file to store in the data section");
    convert->add_option("cart", in, "Source cartridge")->required();
    convert->add_option("output", out, "Destination cartridge")->required();

    // Not in p8tool
    auto run = app.add_subcommand("run", "Run a cart in the terminal")
                   ->callback([&]() { run_mode = mode::run; });
#if HAVE_UNISTD_H
    run->add_flag_function("--telnet", [&](int64_t) { override_mode = mode::telnet; },
                            "Act as telnet server");
#endif
    run->add_flag_function("--headless", [&](int64_t) { override_mode = mode::headless; },
                            "Run without any output");
    run->add_option("cart", in, "Cartridge to load")->required();;

#if 0
    // TODO: splore
    auto splore = app.add_subcommand("splore", "XXXXX")
                      ->callback([&]() { run_mode = mode::splore; });
    splore->add_option("-o", out, "file")->required();
#endif

    // Dither an image
    auto dither = app.add_subcommand("dither", "Convert image to a PICO-8 friendly format")
                      ->callback([&]() { run_mode = mode::dither; });
    dither->add_option("-p,--palette", palette, "List of palette indices, or 'best', or 'classic'")
          ->type_name("<string>");
    dither->add_flag("--hicolor", hicolor, "High color mode");
    dither->add_flag("--error-diffusion", error_diffusion, "Use Floyd-Steinberg error diffusion");
    dither->add_option("image", in, "Image to load")->required();

    // Compress a file
    auto compress = app.add_subcommand("compress", "Compress a stream of data")
                      ->callback([&]() { run_mode = mode::compress; });
    compress->add_option("--skip", skip, "Number of source bytes to skip");
    compress->add_option("--raw", raw, "Number of raw bytes to store");

    // Internal test suite
    app.add_subcommand("test", "Run the test suite")
        ->callback([&]() { run_mode = mode::test; });

    CLI11_PARSE(app, argc, argv);

    if (override_mode != mode::none)
        run_mode = override_mode;

    // Most commands manipulate a cart, so get it right now
    z8::pico8::cart cart;

    switch (run_mode)
    {
    case mode::test:
        test();
        break;

    case mode::stats: {
        printf("file_name: %s\n", in.c_str());
        cart.load(in);
        auto &code = cart.get_code();
        int tokens = z8::pico8::code::count_tokens(code);

        printf("token_count: %d [8192]\n", tokens);
        printf("code_size: %d [65535]\n", int(code.size()));
        auto const &original_code = cart.get_rom().code;
        if (original_code[0] == '\0' && original_code[1] == 'p'
             && original_code[2] == 'x'  && original_code[3] == 'a')
        {
            printf("stored_code_size: %d [%d]\n",
               int(original_code[6] * 256 + original_code[7]), int(sizeof(z8::pico8::memory::code)));
        }
        printf("compressed_code_size: %d [%d]\n",
               int(cart.get_compressed_code().size()), int(sizeof(z8::pico8::memory::code)));

        printf("\n");
        break;
    }
    case mode::listlua:
        cart.load(in);
        printf("%s", cart.get_code().c_str());
        break;

    case mode::luamin:
        cart.load(in);
        std::cout << z8::minify(cart.get_code()) << '\n';
        break;

    case mode::printast: {
        cart.load(in);
        auto &code = cart.get_code();
        printf("%s", z8::pico8::code::ast(code).c_str());
        break;
    }
    case mode::convert:
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

        if (lol::ends_with(out, ".bin"))
        {
            std::ofstream f(out, std::ios::binary);
            auto const &bin = cart.get_bin();
            f.write((char const *)bin.data(), bin.size());
        }
        else if (lol::ends_with(out, ".png"))
            cart.get_png().save(out);
        else
            std::ofstream(out) << cart.get_p8();

        break;

    case mode::headless:
    case mode::run: {
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
            if (run_mode != mode::headless)
            {
                vm->print_ansi(lol::ivec2(128, 64), nullptr);
                t.wait(1.f / 60.f);
            }
        }
        break;
    }

    case mode::dither:
        z8::dither(in, out, palette, hicolor, error_diffusion);
        break;

    case mode::compress: {
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
            std::cout << z8::encode49(output) << '\n';
        }
        break;
    }
    case mode::splore: {
        z8::splore splore;
        splore.dump(in);
        break;
    }
#if HAVE_UNISTD_H
    case mode::telnet: {
        z8::telnet telnet;
        telnet.run(in);
        break;
    }
#endif
    default:
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

