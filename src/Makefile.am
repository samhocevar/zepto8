
include $(top_srcdir)/lol/build/autotools/common.am

bin_PROGRAMS = zepto8 zeptool dither
noinst_LIBRARIES = libzepto8.a

zepto8dir = $(datarootdir)/zepto8

zepto8_SOURCES = \
    zepto8.cpp \
    player.cpp player.h \
    $(NULL)
zepto8_CPPFLAGS = $(AM_CPPFLAGS)
zepto8_LDFLAGS = libzepto8.a $(AM_LDFLAGS)
zepto8_DEPENDENCIES = libzepto8.a @LOL_DEPS@
zepto8_DATA = data/zepto8.lua data/font.png

EXTRA_DIST += zepto8.vcxproj

zeptool_SOURCES = zeptool.cpp
zeptool_CPPFLAGS = $(AM_CPPFLAGS)
zeptool_LDFLAGS = libzepto8.a $(AM_LDFLAGS)
zeptool_DEPENDENCIES = libzepto8.a @LOL_DEPS@

libzepto8_a_SOURCES = \
    zepto8.h \
    vm.cpp vm.h vm-maths.cpp vm-gfx.cpp vm-render.cpp vm-sfx.cpp \
    cart.cpp cart.h \
    code-fixer.cpp code-fixer.h lua53-parse.h \
    $(NULL)

dither_SOURCES = dither.cpp
dither_CPPFLAGS = $(AM_CPPFLAGS)
dither_LDFLAGS = $(AM_LDFLAGS)
dither_DEPENDENCIES = @LOL_DEPS@

