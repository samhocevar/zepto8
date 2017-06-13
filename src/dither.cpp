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

using namespace lol;

#define ITERATIONS 100
#define ERROR_FOR_CURRENT_IMAGE 0.7f//0.7f
#define PRESERVATION 0.7f//0.9f

struct bitstream
{
    bitstream()
      : m_remaining(0)
    {}

    inline void add(int count, uint8_t val)
    {
        while (count--)
            add((val >> count) & 1);
    }

    inline void add(uint8_t bit)
    {
        //printf(" + bit %d\n", bit);
        if (m_remaining == 0)
        {
            m_data.push(0);
            m_remaining = 8;
        }

        m_data.last() |= (bit & 1) << (8 - m_remaining);
        --m_remaining;
    }

    array<uint8_t> m_data;

private:
    int m_remaining = 0;
};

int main(int argc, char **argv)
{
    UNUSED(argc, argv);

    if (argc != 4)
        return EXIT_FAILURE;

    static u8vec4 const palette_u8[] =
    {
        u8vec4(0x00, 0x00, 0x00, 0xff), // black
        u8vec4(0x1d, 0x2b, 0x53, 0xff), // dark_blue
        u8vec4(0x7e, 0x25, 0x53, 0xff), // dark_purple
        u8vec4(0x00, 0x87, 0x51, 0xff), // dark_green
        u8vec4(0xab, 0x52, 0x36, 0xff), // brown
        u8vec4(0x5f, 0x57, 0x4f, 0xff), // dark_gray
        u8vec4(0xc2, 0xc3, 0xc7, 0xff), // light_gray
        u8vec4(0xff, 0xf1, 0xe8, 0xff), // white
        u8vec4(0xff, 0x00, 0x4d, 0xff), // red
        u8vec4(0xff, 0xa3, 0x00, 0xff), // orange
        u8vec4(0xff, 0xec, 0x27, 0xff), // yellow
        u8vec4(0x00, 0xe4, 0x36, 0xff), // green
        u8vec4(0x29, 0xad, 0xff, 0xff), // blue
        u8vec4(0x83, 0x76, 0x9c, 0xff), // indigo
        u8vec4(0xff, 0x77, 0xa8, 0xff), // pink
        u8vec4(0xff, 0xcc, 0xaa, 0xff), // peach
    };

    /* Fill palette with PICO-8 colours */
    array<vec4> palette;
    for (int i = 0; i < 16; ++i)
        palette << vec4(palette_u8[i]) * vec4(1.0f / 255);

#if 0
    /* Add colour combinations that aren’t too awful */
    for (int i = 0; i < 16; ++i)
        for (int j = i + 1; j < 16; ++j)
        {
            if (distance(palette[i], palette[j]) < 0.8f)
                palette << mix(palette[i], palette[j], 0.5f);
        }
#endif

    msg::info("palette has %d colours\n", palette.count());

    /* Load images */
    image im;
    im.load(argv[1]);

    ivec2 size(im.size());

    if (size.x != 128)
    {
        size.y = int(size.y * 128.f / size.x);
        size.x = 128;
        im = im.Resize(size, ResampleAlgorithm::Bicubic);
    }

    // Slight blur
    im = im.Resize(size * 2, ResampleAlgorithm::Bicubic)
           .Resize(size, ResampleAlgorithm::Bresenham);

    msg::info("image size %d×%d\n", size.x, size.y);

    image dst(size), otherdst(size);
    array2d<uint8_t> dstpixels(size), otherdstpixels(size);
    image cur = im;

    for (int iter = 0; iter < ITERATIONS; ++iter)
    {
        image next = im;

        /* Dither image for first destination */
        array2d<vec4> &curdata = cur.lock2d<PixelFormat::RGBA_F32>();
        array2d<vec4> &nextdata = next.lock2d<PixelFormat::RGBA_F32>();
        array2d<vec4> &dstdata = dst.lock2d<PixelFormat::RGBA_F32>();
        for (int j = 0; j < size.y; ++j)
        {
            for (int i = 0; i < size.x; ++i)
            {
                vec4 pixel = curdata[i][j];

                // Find nearest colour
                int nearest = -1;
                float best_dist = 1e8f;

                for (int n = 0; n < palette.count(); ++n)
                {
                    float new_dist = distance(palette[n].rgb, pixel.rgb);
                    if (new_dist < best_dist)
                    {
                        best_dist = new_dist;
                        nearest = n;
                    }
                }

                // Store colour
                dstpixels[i][j] = nearest;
                dstdata[i][j] = palette[nearest];
                vec4 error = PRESERVATION * (pixel - palette[nearest]);

                float k = ERROR_FOR_CURRENT_IMAGE;

                // Propagate error to current image
                vec4 e1 = k / 16 * error;
                if (i + 1 < size.x)
                    curdata[i + 1][j] += 7.f * e1;
                if (i - 1 >= 0 && j + 1 < size.y)
                    curdata[i - 1][j + 1] += 1.f * e1;
                if (j + 1 < size.y)
                    curdata[i][j + 1] += 5.f * e1;
                if (i + 1 < size.x && j + 1 < size.y)
                    curdata[i + 1][j + 1] += 3.f * e1;

                // Propagate error to next image
                vec4 e2 = (1.f - k) / 16 * error;
#if 1
                if (i - 1 >= 0 && j - 1 >= 0)
                    nextdata[i - 1][j - 1] += 5.f * e2;
                if (j - 1 >= 0)
                    nextdata[i][j - 1] += 3.f * e2;
                if (i - 1 >= 0)
                    nextdata[i - 1][j] += 1.f * e2;
                nextdata[i][j] += 7.f * e2;
#else
                nextdata[i][j] += 16.f * e2;
#endif
            }
        }

        cur.unlock2d(curdata);
        next.unlock2d(nextdata);
        dst.unlock2d(dstdata);

        /* Swap images */
        cur = next;

        auto tmp = dst; dst = otherdst; otherdst = tmp;
        auto tmppixels = dstpixels; dstpixels = otherdstpixels; otherdstpixels = tmppixels;
    }

    /* Move dark pixels to the first image and do stats */
    int stats[256];
    memset(stats, 0, sizeof(stats));
    for (int j = 0; j < size.y; ++j)
    {
        for (int i = 0; i < size.x; ++i)
        {
            uint8_t p = 16 * lol::min(dstpixels[i][j], otherdstpixels[i][j])
                           + lol::max(dstpixels[i][j], otherdstpixels[i][j]);
            dstpixels[i][j] = p;
            ++stats[p];
        }
    }

//    for (int bits = 1; bits < 7; ++bits)
    int const bits = 3; // empirical evidence shows this is good
    {
        /* Find the most frequent colours */
        array<uint8_t> best;
        for (int i = 0; i < (1 << bits); ++i)
        {
            int freq = -1;
            uint8_t candidate = 0;
            for (int j = 0; j < 256; ++j)
            {
                bool valid = true;
                for (int k = 0; k < i; ++k)
                    if (best[k] == j)
                        valid = false;

                if (valid && (freq < 0 || stats[j] >= freq))
                {
                    candidate = j;
                    freq = stats[j];
                }
            }

            best.push(candidate);
        }

        /* Print stats */
        int total = 0;
        for (int v : best)
            total += stats[v];
        msg::info("Most frequent %d colors: %d / %d\n",
                  1 << bits, total, size.x * size.y);

        int compressed = (bits + 1) * total + 8 * (size.x * size.y - total);
        msg::info("Would compress to %d bytes / %d (max %d)\n", (compressed + 7) / 8, 16384, 0x4300);

        /* Build dictionary */
        int dict_bits[256]; // values to push
        memset(dict_bits, 0, sizeof(dict_bits));
        int dict_counts[256]; // number of bits to push
        memset(dict_counts, 0, sizeof(dict_counts));

        printf("__lua__\n");

        printf("short = { ");
        for (int i = 0; i < (1 << bits); ++i)
        {
            dict_counts[best[i]] = 1 + bits;
            dict_bits[best[i]] = i;
            printf("%d, ", best[i]);
            msg::info("%x (%d bits) for %02x (%d occurrences)\n", i, 1 + bits, best[i], stats[best[i]]);
        }
        printf("}\n");

        printf("long = { ");
        int done = 0;
        for (int i = 0; i < 256; ++i)
        {
            if (stats[i] == 0)
                continue;

            bool valid = true;
            for (int j = 0; j < (1 << bits); ++j)
                if (best[j] == i)
                    valid = false;
            if (!valid)
                continue;

            dict_counts[i] = 8;
            dict_bits[i] = (1 << 7) | done;
            ++done;
            printf("%d, ", i);
            msg::info("%x (%d bits) for %02x (%d occurrences)\n", dict_bits[i], 8, i, stats[i]);
        }
        printf("}\n");

        /* Compress image */
        printf("__gfx__\n");
        bitstream stream;
        for (int j = 0; j < size.y; ++j)
        {
            for (int i = 0; i < size.x; ++i)
            {
                int p = dstpixels[i][j];
                //printf("+ pixel %x\n", p);
                stream.add(dict_counts[p], dict_bits[p]);
            }
        }
        for (int i = 0; i < 0x4300; ++i)
        {
            uint8_t nybble = i < stream.m_data.bytes() ? stream.m_data[i] : 0;

            // Must swap nybbles for PICO-8
            printf("%x%x", nybble & 0xf, nybble >> 4);
            if ((i + 1) % 64 == 0)
                printf("\n");
        }
    }

    /* Fixup images */
    array2d<vec4> &dstdata = dst.lock2d<PixelFormat::RGBA_F32>();
    array2d<vec4> &otherdstdata = otherdst.lock2d<PixelFormat::RGBA_F32>();
    for (int j = 0; j < size.y; ++j)
        for (int i = 0; i < size.x; ++i)
        {
            auto a = palette[dstpixels[i][j] >> 4];
            auto b = palette[dstpixels[i][j] & 0xf];
            if ((i ^ j) & 1)
                std::swap(a, b);
            dstdata[i][j] = mix(a, b, 0.25f);
            otherdstdata[i][j] = mix(a, b, 0.75f);
        }
    dst.unlock2d(dstdata);
    otherdst.unlock2d(otherdstdata);

    /* Save images */
    dst = dst.Resize(size * 4, ResampleAlgorithm::Bresenham);
    dst.save(argv[2]);
    otherdst = otherdst.Resize(size * 4, ResampleAlgorithm::Bresenham);
    otherdst.save(argv[3]);

    return 0;
}

