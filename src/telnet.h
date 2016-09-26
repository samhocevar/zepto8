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

#pragma once

#include <lol/engine.h>

#if HAVE_UNISTD_H
#   include <unistd.h>
#endif

#include "zepto8.h"
#include "vm.h"

namespace z8
{

struct telnet
{
    static void run(char const *cart)
    {
        disable_echo();

        z8::vm vm;
        vm.load(cart);
        vm.run();

        while (true)
        {
            lol::Timer t;

            for (int i = 0; i < 16; ++i)
                vm.button(i, 0);

            for (;;)
            {
                int key = get_key();
                if (key < 0)
                    break;

                switch (key)
                {
                    /* For now, Escape quits */
                    case 0x1b: return;

                    case 0x144: vm.button(0, 1); break; // left
                    case 0x143: vm.button(1, 1); break; // right
                    case 0x141: vm.button(2, 1); break; // up
                    case 0x142: vm.button(3, 1); break; // down
                    case 'z': case 'Z':
                    case 'c': case 'C':
                    case 'n': case 'N': vm.button(4, 1); break;
                    case 'x': case 'X':
                    case 'v': case 'V':
                    case 'm': case 'M': vm.button(5, 1); break;
                    case '\r': case '\n': vm.button(6, 1); break;
                    case 's': case 'S': vm.button(8, 1); break;
                    case 'f': case 'F': vm.button(9, 1); break;
                    case 'e': case 'E': vm.button(10, 1); break;
                    case 'd': case 'D': vm.button(11, 1); break;
                    case 'a': case 'A': vm.button(12, 1); break;
                    case '\t':
                    case 'q': case 'Q': vm.button(13, 1); break;
                    default:
                        lol::msg::info("Got unknown key %02x\n", key);
                        break;
                }
            }

            /* Do two full steps at 60fps */
            vm.step(1.f / 60.f);
            vm.step(1.f / 60.f);

            vm.print_ansi();

            /* Wait for 30fps */
            t.Wait(1.f / 30.f);
        }
    }

    static void disable_echo()
    {
#if HAVE_UNISTD_H
        uint8_t const message[] =
        {
            0xff, 0xfb, 0x03, // WILL suppress go ahead (no line buffering)
            0xff, 0xfe, 0x22, // DONT linemode (no idea what it does)

            // This will break rendering :/
            0xff, 0xfb, 0x01, // WILL echo (actually disables local echo)
        };

        write(STDOUT_FILENO, message, sizeof(message));
#endif
    }

    static int get_key()
    {
#if HAVE_UNISTD_H
        static int frames_since_esc = 0;

        /* Handle standalone Esc after 10 frames */
        if (frames_since_esc && ++frames_since_esc > 10)
        {
            frames_since_esc = 0;
            return 0x1b;
        }

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);

        struct timeval tv;
        tv.tv_sec = tv.tv_usec = 0;

        select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);

        if (!FD_ISSET(0, &fds))
            return -1;

        char ch;
        if (read(STDIN_FILENO, &ch, 1) <= 0)
            exit(EXIT_SUCCESS);

        if (frames_since_esc)
        {
            if (ch == 0x5b)
                return -1; // Skip this one

            frames_since_esc = 0;
            return 0x100 + ch;
        }

        if (ch == 0x1b)
        {
            ++frames_since_esc;
            return -1;
        }

        return ch;
#else
        return -1;
#endif
    }
};

} // namespace z8

