# Porting zepto8 to other platforms

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
```

Make sure a recent version of GCC is selected: `make menuconfig` → _Toolchain_ → _GCC Compiler Version_ → _gcc 9.x_

```sh
export BR2_JLEVEL=0
make toolchain
make toolchain sdl sdl_image sdl_mixer
```

### Configure zepto8

Assuming `RG350_buildroot` is located in `${HOME}`:

```sh
./bootstrap

HOST="mipsel-gcw0-linux-uclibc"
XCHAIN="${HOME}/RG350_buildroot/output/host/usr/bin"
SYSROOT="${HOME}/RG350_buildroot/output/host/usr/${HOST}/sysroot"
PKG_CONFIG_LIBDIR="${SYSROOT}/usr/lib/pkgconfig:${SYSROOT}/usr/share/pkgconfig" \
    PKG_CONFIG_SYSROOT_DIR="$SYSROOT" \
    ./configure --host="${HOST}" --with-sysroot="${SYSROOT}" \
        CC="${XCHAIN}/${HOST}-cc" CXX="${XCHAIN}/${HOST}-c++" \
        CPPFLAGS="-I${SYSROOT}/usr/include"

make -j
```

