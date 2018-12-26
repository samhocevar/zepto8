# zepto8
Fantasy fantasy console emulator emulator

This is all experimental.

### z8player

    # z8player cart.p8

Plays a PICO-8 cartridge.

### z8tool

This tool does a lot of things.

## Z8 compression

Compress any file:

    # cat file | z8tool --compress

This will output a base59 string that can be decompressed by
[unz8](https://github.com/samhocevar/zepto8/blob/master/src/unz8). The
decompressed data is an 1-indexed array of 32-bit numbers.

## Image dithering

Dither a 128×128 image to the PICO-8 palette:

    # z8tool --dither image.png > image.data

Dither a 128×128 image and compress it using the Z8 algorithm:

    # z8tool --dither image.png | z8tool --compress

The resulting data can be decompressed using `unz8()` and displayed in PICO-8:

    local image = unz8(data)
    for i=1,#image do poke4(24572+4*i,image[i]) end

