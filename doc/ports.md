# zepto8 ports to other platforms

## OpenDingux, GCW0, RG350, …

### Build toolchain

First you need to get a recent enough toolchain (GCC 8.x or clang) and its dependencies. I have had
success with [rg350-buildroot](https://github.com/tonyjih/RG350_buildroot):

```sh
sudo apt install python unzip bc gcc-multilib subversion mercurial rsync libncurses-dev
```

```sh
git clone https://github.com/tonyjih/RG350_buildroot ~/rg350
cd ~/rg350
make rg350_defconfig BR2_EXTERNAL=board/opendingux
make menuconfig
```

Make sure a recent version of GCC is selected: _Toolchain_ → _GCC Compiler Version_ → _gcc 9.x_
then exit the configuration utility.

Next, build the toolchain and the SDL libraries:

```sh
export BR2_JLEVEL=0
make toolchain
make sdl2 sdl2_image sdl2_mixer
```

For some reason the toolchain needs to be patched in the following way:

```sh
echo 'namespace std { using ::cbrt, ::round, ::exp2; }' \
    >> ~/rg350/output/host/usr/mipsel-gcw0-linux-uclibc/include/c++/9.2.0/cmath
```

### Build zepto8

Assuming `rg350` is located in `${HOME}`:

```sh
./bootstrap

BUILDROOT="${HOME}/rg350"
HOSTCONF="mipsel-gcw0-linux-uclibc"
TOOLCHAIN="${BUILDROOT}/output/host/usr/bin"
SYSROOT="${BUILDROOT}/output/host/usr/${HOSTCONF}/sysroot"
PKG_CONFIG_LIBDIR="${SYSROOT}/usr/lib/pkgconfig:${SYSROOT}/usr/share/pkgconfig"
PKG_CONFIG_SYSROOT_DIR="$SYSROOT"
./configure --host="${HOSTCONF}" --with-sysroot="${SYSROOT}" \
    CC="${TOOLCHAIN}/${HOSTCONF}-gcc" CXX="${TOOLCHAIN}/${HOSTCONF}-g++" \
    CPPFLAGS="-I${SYSROOT}/usr/include"

make -j
```

