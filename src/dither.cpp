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

#define DEPTH 512

namespace z8
{

void dither(char const *src, char const *out, bool hicolor, bool error_diffusion)
{

    std::vector<lol::vec3> colors;
    std::vector<uint8_t> indices;

    for (int i = 0; i < 16; ++i)
        colors.push_back(palette::get(i).rgb);

    // Fix gamma (kinda)
    for (auto &color : colors)
        color *= color;

    if (hicolor)
    {
        // Add colour combinations that aren’t too awful
        for (int i = 0; i < 16; ++i)
            for (int j = i + 1; j < 16; ++j)
                if (distance(colors[i], colors[j]) < 0.8f)
                {
                    colors.push_back(0.5f * (colors[i] + colors[j]));
                    indices.push_back(i * 16 + j);
                }
    }

#if 0
    for (int i = 0; i < 16; ++i)
    {
    printf("  d: %02x %02x %02x  f: %f %f %f\n",
       palette::get8(i).r,
       palette::get8(i).g,
       palette::get8(i).b,
       palette::get(i).r,
       palette::get(i).g,
       palette::get(i).b);
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
    std::vector<uint8_t> pixels;

    //auto kernel = lol::image::kernel::halftone(lol::ivec2(6));
    auto kernel = lol::image::kernel::blue_noise(lol::ivec2(64));
    //auto kernel = lol::image::kernel::bayer(lol::ivec2(32));

    auto closest = [& colors](lol::vec3 color) -> int
    {
        float best_dist = FLT_MAX;
        int best = -1;
        for (int i = 0; i < colors.size(); ++i)
        {
            float dist = lol::distance(colors[i], color);
            if (dist < best_dist)
            {
                best = i;
                best_dist = dist;
            }
        }

        return best;
    };

    /* Dither image for first destination */
    lol::array2d<lol::vec4> &curdata = im.lock2d<lol::PixelFormat::RGBA_F32>();
    lol::array2d<lol::vec4> &dstdata = dst.lock2d<lol::PixelFormat::RGBA_F32>();
    for (int j = 0; j < size.y; ++j)
    {
        for (int i = 0; i < size.x; ++i)
        {
            lol::vec3 pixel = curdata[i][j].rgb;
            uint8_t nearest = 0;

            if (error_diffusion)
            {
                nearest = closest(pixel);
                auto error = lol::vec4(pixel - colors[nearest], 0.f) / 18.f;
                if (i < size.x - 1)
                    curdata[i + 1][j] += 7.f * error;
                if (j < size.y - 1)
                {
                    if (i > 0)
                        curdata[i - 1][j + 1] += 1.f * error;
                    curdata[i][j + 1] += 5.f * error;
                    if (i < size.x - 1)
                        curdata[i + 1][j + 1] += 3.f * error;
                }
            }
            else
            {
                // Dither pixel DEPTH times with error diffusion
                int found[DEPTH];
                auto candidate = pixel;
                for (int n = 0; n < DEPTH; ++n)
                {
                    found[n] = closest(candidate);
                    candidate = pixel + 7.f / 16 * (candidate - colors[found[n]]);
                }

                // Sort results by luminance and pick the final color using a dithering kernel
                std::sort(found, found + DEPTH, [& colors](int a, int b)
                {
                    return lol::dot(colors[a] - colors[b], lol::vec3(1)) > 0;
                });
                nearest = found[(int)(kernel[i % kernel.size().x][j % kernel.size().y] * DEPTH)];
            }

            pixels.push_back(nearest);
        }
    }

    im.unlock2d(curdata);
    dst.unlock2d(dstdata);

#if 0
    /* Save image */
    //dst = dst.Resize(size * 4, ResampleAlgorithm::Bresenham);
    dst.save(argv[4]);
#endif

    std::vector<uint8_t> rawdata;
    rawdata.resize(hicolor ? pixels.size() : pixels.size() / 2);
    for (int j = 0; j < size.y; ++j)
        for (int i = 0; i < size.x; i += 2)
        {
            uint8_t c1 = pixels[j * 128 + i];
            uint8_t c2 = pixels[j * 128 + i + 1];
            if (hicolor)
            {
                int d = (j + i / 2) & 1;
                uint8_t a1 = c1 < 16 ? c1 : (indices[c1 - 16] << (4 * d) >> 4) & 0xf;
                uint8_t a2 = c2 < 16 ? c2 : (indices[c2 - 16] << (4 * d) >> 4) & 0xf;
                uint8_t b1 = c1 < 16 ? c1 : (indices[c1 - 16] >> (4 * d)) & 0xf;
                uint8_t b2 = c2 < 16 ? c2 : (indices[c2 - 16] >> (4 * d)) & 0xf;
                rawdata[j * 64 + i / 2] = a2 * 16 + a1;
                rawdata[(j + size.x) * 64 + i / 2] = b2 * 16 + b1;
            }
            else
            {
                rawdata[j * 64 + i / 2] = c2 * 16 + c1;
            }
        }

    /* Save data */
    FILE *s = out ? fopen(out, "wb+") : stdout;
    fwrite(rawdata.data(), 1, rawdata.size(), s);
    if (out)
        fclose(s);
}

} // namespace z8

