
include $(top_srcdir)/lol/build/autotools/common.am

bin_PROGRAMS = zepto8 dither

zepto8dir = $(datarootdir)/zepto8

zepto8_SOURCES = \
    zepto8.cpp zepto8.h \
    cart.cpp cart.h \
    vm.cpp vm.h vm-maths.cpp vm-gfx.cpp \
    code-fixer.cpp code-fixer.h lua53_parse.h
zepto8_CPPFLAGS = $(AM_CPPFLAGS)
zepto8_LDFLAGS = $(AM_LDFLAGS)
zepto8_DEPENDENCIES = @LOL_DEPS@
zepto8_DATA = data/zepto8.lua data/font.png

EXTRA_DIST += zepto8.vcxproj

dither_SOURCES = \
    dither.cpp
dither_CPPFLAGS = $(AM_CPPFLAGS)
dither_LDFLAGS = $(AM_LDFLAGS)
dither_DEPENDENCIES = @LOL_DEPS@

