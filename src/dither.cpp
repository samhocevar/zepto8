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

#include <string> // std::string
#include <vector> // std::vector
#include <array>  // std::array

#include <lol/engine.h> // lol::old_image
#include <lol/color>    // lol::color
#include <lol/msg>      // lol::msg

#include "zepto8.h"
#include "pico8/pico8.h"

#define DEPTH 512

namespace z8
{

struct ditherer
{
    ditherer(std::string const &palette)
    {
        // Generate list of available colors using palette argument
        if (palette == "classic")
            for (uint8_t i = 0; i < 16; ++i)
                colors.push_back(i);
        else if (palette == "best" || palette == "")
            for (uint8_t i = 0; i < 32; ++i)
                colors.push_back(i);
        else
        {
            for (auto const &c : lol::split(palette, ","))
            {
                uint8_t i = std::atoi(c.c_str()) & 0x8f;
                colors.push_back(i < 16 ? i : i - 128 + 32);
            }
        }

        // Add at least a few elements
        if (colors.size() < 2)
            colors.push_back(7);
        if (colors.size() < 2)
            colors.push_back(0);

        // Sort palette by luminance
        std::sort(colors.begin(), colors.end(), compare_colors);
    }

    // Number of colours
    size_t count() { return colors.size(); }

    // Return the nth colour (PICO-8 value 0…31)
    lol::vec3 get_color(uint8_t n)
    {
        return pico8::palette::get(colors[n]).rgb;
    }

    // Remove the least used color using a histogram
    void reduce_palette(std::vector<size_t> const &hist)
    {
        size_t best = 0;
        for (size_t i = 1; i < hist.size(); ++i)
            if (hist[i] < hist[best])
                best = i;

        //auto c = lol::dot(lol::ivec3(get_color(best) * 255.99f), lol::ivec3(0x1, 0x100, 0x10000));
        //printf("Remove color %d (#%06x), used for %d pixels\n", colors[best], c, int(hist[best]));
        lol::remove_at(colors, best);
    }

    // Return best color index from the list of available ones
    uint8_t best_color_index(lol::vec3 color)
    {
        float best_dist = FLT_MAX;
        int best = -1;
        for (size_t i = 0; i < colors.size(); ++i)
        {
            // FIXME: this works in sRGB space
            float dist = lol::distance(get_color(i), color);
            if (dist < best_dist)
            {
                best = i;
                best_dist = dist;
            }
        }

        return best;
    }

private:
    static int compare_colors(uint8_t a, uint8_t b)
    {
        // Convert sRGB to RGB, then to YUV, and use Y component
        auto ca = lol::color::srgb_to_rgb(pico8::palette::get(a).rgb);
        auto cb = lol::color::srgb_to_rgb(pico8::palette::get(b).rgb);
        return lol::color::rgb_to_yuv(ca)[0] > lol::color::rgb_to_yuv(cb)[0];
    }

    std::vector<uint8_t> colors;
};

void dither(std::string const &src, std::string const &out, std::string const &palette,
            bool hicolor, bool error_diffusion)
{
    struct ditherer d(palette);

#if 0
    std::vector<uint8_t> indices;
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
#endif

    // Load image
    lol::old_image im;
    im.load(src);
    im = im.Resize(lol::ivec2(128,128), lol::ResampleAlgorithm::Bicubic);

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
    im = im.Resize(size * 3, lol::ResampleAlgorithm::Bicubic)
           .Resize(size, lol::ResampleAlgorithm::Bresenham);
    //im = im.Contrast(0.1f);

    lol::msg::info("image size %d×%d\n", size.x, size.y);

    lol::old_image dst(size);
    std::vector<uint8_t> pixels;

    //auto kernel = lol::old_image::kernel::halftone(lol::ivec2(8));
    //auto kernel = lol::old_image::kernel::blue_noise(lol::ivec2(64));
    auto kernel = lol::old_image::kernel::bayer(lol::ivec2(32));
    auto original_image = im;

    for (;;)
    {
        im = original_image;
        pixels.clear();

        lol::array2d<lol::vec4> &curdata = im.lock2d<lol::PixelFormat::RGBA_F32>();
        lol::array2d<lol::vec4> &dstdata = dst.lock2d<lol::PixelFormat::RGBA_F32>();
        for (int j = 0; j < size.y; ++j)
        for (int i = 0; i < size.x; ++i)
        {
            lol::vec3 pixel = curdata[i][j].rgb;
            uint8_t nearest = 0;

            if (error_diffusion || d.count() > 16)
            {
                nearest = d.best_color_index(pixel);
                auto error = lol::vec4(pixel - d.get_color(nearest), 0.f) / 18.f;
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
                // Dither pixel DEPTH times with error diffusion and build a histogram
                std::vector<size_t> histogram(d.count(), 0);
                auto candidate = pixel;
                for (int n = 0; n < DEPTH; ++n)
                {
                    auto c = d.best_color_index(candidate);
                    ++histogram[c];
                    // FIXME: make the 0.5 here configurable
                    candidate = pixel + 0.9f * (candidate - d.get_color(c));
                }

                // If colors are sorted by luminance, we just accumulate the histogram
                // values and stop when the threshold is hit.
                size_t threshold = size_t(kernel[i % kernel.sizes().x][j % kernel.sizes().y] * DEPTH);
                for (size_t n = 0, total = 0; n < d.count(); ++n)
                {
                    total += histogram[n];
                    if (total > threshold)
                    {
                        nearest = n;
                        break;
                    }
                }
            }

            pixels.push_back(nearest);
            dstdata[i][j] = lol::vec4(d.get_color(nearest), 255.f);
        }

        im.unlock2d(curdata);
        dst.unlock2d(dstdata);

        // Trim unused colors
        if (d.count() <= 16)
            break;

        std::vector<size_t> histogram(d.count(), 0);
        for (uint8_t p : pixels)
            ++histogram[p];
        d.reduce_palette(histogram);
    }

#if 1
    /* Save image */
    dst = dst.Resize(size * 3, lol::ResampleAlgorithm::Bresenham);
    dst.save("test.png");
#else
    // TODO: write cart
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
    FILE *s = out.length() ? fopen(out.c_str(), "wb+") : stdout;
    fwrite(rawdata.data(), 1, rawdata.size(), s);
    if (out.length())
        fclose(s);
#endif
}

} // namespace z8

