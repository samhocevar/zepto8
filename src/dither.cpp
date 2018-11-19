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

#include "zepto8.h"
#include "splore.h"

namespace z8
{

#define PRESERVATION 0.5f//0.9f

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

    /* Dither image for first destination */
    lol::array2d<lol::vec4> &curdata = im.lock2d<lol::PixelFormat::RGBA_F32>();
    lol::array2d<lol::vec4> &dstdata = dst.lock2d<lol::PixelFormat::RGBA_F32>();
    for (int j = 0; j < size.y; ++j)
    {
        for (int i = 0; i < size.x; ++i)
        {
            lol::vec4 pixel = curdata[i][j];

            // Find nearest colour
            int nearest = palette::best(pixel);

            // Append raw data
            if (i & 1)
                rawdata.last() |= (nearest << 4) & 0xf0;
            else
                rawdata.push(nearest & 0x0f);

            // Store colour
            lol::vec4 newpixel = lol::vec4(palette::get(nearest));
            dstdata[i][j] = newpixel;
            lol::vec4 error = PRESERVATION / 16.f * (pixel - newpixel);

            //float diff[] = { 7.0f, 1.0f, 5.0f, 3.0f }; // Floyd-Steinberg
            float diff[] = { 6.0f, 3.0f, 5.0f, 2.0f }; // Hocevar-Niger
            //float diff[] = { 0.0f, 5.0f, 7.0f, 4.0f }; // Horizontal stripes
            //float diff[] = { 9.0f, 3.0f, 0.0f, 4.0f }; // Vertical stripes

            // Propagate error to current image
            if (i + 1 < size.x)
                curdata[i + 1][j] += diff[0] * error;
            if (i - 1 >= 0 && j + 1 < size.y)
                curdata[i - 1][j + 1] += diff[1] * error;
            if (j + 1 < size.y)
                curdata[i][j + 1] += diff[2] * error;
            if (i + 1 < size.x && j + 1 < size.y)
                curdata[i + 1][j + 1] += diff[3] * error;
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

