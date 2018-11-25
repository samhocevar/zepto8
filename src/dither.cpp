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

#include <lol/engine.h>

#include <string>
#include <algorithm>

#include "zepto8.h"
#include "splore.h"

namespace z8
{

void dither(char const *src, char const *out)
{
#if 0
    /* Add colour combinations that aren’t too awful */
    for (int i = 0; i < 16; ++i)
        for (int j = i + 1; j < 16; ++j)
        {
            if (distance(palette::get(i), palette::get(j)) < 0.8f)
                thing << mix(palette::get(i), palette::get(j), 0.5f);
        }
#endif

    /* Load images */
    lol::image im;
    im.load(src);

    lol::ivec2 size(im.size());

#if 0
    if (size.x != desired_width || size.y != desired_height)
    {
        size.x = desired_width;
        size.y = desired_height;
        im = im.Resize(size, lol::ResampleAlgorithm::Bicubic);
    }
#endif

    // Slight blur
//    im = im.Resize(size * 2, ResampleAlgorithm::Bicubic)
//           .Resize(size, ResampleAlgorithm::Bresenham);

    lol::msg::info("image size %d×%d\n", size.x, size.y);

    lol::image dst(size);
    lol::array<uint8_t> rawdata;

    auto kernel = lol::image::kernel::bayer(lol::ivec2(16));

    /* Dither image for first destination */
    lol::array2d<lol::vec4> &curdata = im.lock2d<lol::PixelFormat::RGBA_F32>();
    lol::array2d<lol::vec4> &dstdata = dst.lock2d<lol::PixelFormat::RGBA_F32>();
    for (int j = 0; j < size.y; ++j)
    {
        for (int i = 0; i < size.x; ++i)
        {
            lol::vec4 pixel = curdata[i][j];

            // Dither pixel 256 times with error diffusion
            int found[256];
            auto candidate = pixel;
            for (int n = 0; n < 256; ++n)
            {
                found[n] = palette::best(candidate);
                candidate = pixel + 7 / 16 * (candidate - palette::get(found[n]));
            }

            // Sort results and pick the final color using a dithering kernel
            std::sort(found, found + 256);
            int nearest = found[(int)(kernel[i % kernel.size().x][j % kernel.size().y] * 256)];

            // Append raw data
            if (i & 1)
                rawdata.last() |= (nearest << 4) & 0xf0;
            else
                rawdata.push(nearest & 0x0f);
        }
    }

    im.unlock2d(curdata);
    dst.unlock2d(dstdata);

#if 0
    /* Save image */
    //dst = dst.Resize(size * 4, ResampleAlgorithm::Bresenham);
    dst.save(argv[4]);
#endif

    /* Save data */
    FILE *s = out ? fopen(out, "wb+") : stdout;
    fwrite(rawdata.data(), 1, rawdata.count(), s);
    if (out)
        fclose(s);
}

} // namespace z8

