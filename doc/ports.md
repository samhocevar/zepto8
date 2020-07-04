# zepto8 ports to other platforms

## OpenDingux, GCW0, RG350, …

### Build toolchain

First you need to get a recent enough toolchain (GCC 8.x or clang) and its dependencies. I have had
success with [rg350-buildroot](https://github.com/tonyjih/RG350_buildroot):

```sh
sudo apt install gcc-multilib mercurial rsync
```

```sh
git clone https://github.com/tonyjih/RG350_buildroot
cd RG350_buildroot
make rg350_defconfig BR2_EXTERNAL=board/opendingux
make menuconfig
```

Make sure a recent version of GCC is selected: _Toolchain_ → _GCC Compiler Version_ → _gcc 9.x_
then exit the configuration utility.

Next, build the toolchain and the SDL libraries:

```sh
export BR2_JLEVEL=0
make toolchain
make sdl sdl_image sdl_mixer
```

### Build zepto8

Assuming `RG350_buildroot` is located in `${HOME}`:

```sh
./bootstrap

BUILDROOT="${HOME}/RG350_buildroot" \
    HOST="mipsel-gcw0-linux-uclibc" \
    TOOLCHAIN="${BUILDROOT}/output/host/usr/bin" \
    SYSROOT="${BUILDROOT}/output/host/usr/${HOST}/sysroot" \
    PKG_CONFIG_LIBDIR="${SYSROOT}/usr/lib/pkgconfig:${SYSROOT}/usr/share/pkgconfig" \
    PKG_CONFIG_SYSROOT_DIR="$SYSROOT" \
    ./configure --host="${HOST}" --with-sysroot="${SYSROOT}" \
        CC="${TOOLCHAIN}/${HOST}-cc" CXX="${TOOLCHAIN}/${HOST}-c++" \
        CPPFLAGS="-I${SYSROOT}/usr/include"

make -j
```

