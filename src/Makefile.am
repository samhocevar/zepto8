
include $(top_srcdir)/lol/build/autotools/common.am

bin_PROGRAMS = zepto8 z8tool
noinst_PROGRAMS = dither
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

z8tool_SOURCES = \
    z8tool.cpp \
    splore.cpp splore.h \
    $(NULL)
z8tool_CPPFLAGS = $(AM_CPPFLAGS)
z8tool_LDFLAGS = libzepto8.a $(AM_LDFLAGS)
z8tool_DEPENDENCIES = libzepto8.a @LOL_DEPS@

EXTRA_DIST += z8tool.vcxproj

libzepto8_a_SOURCES = \
    zepto8.h \
    vm.cpp vm.h vm-maths.cpp vm-gfx.cpp vm-render.cpp vm-sfx.cpp \
    fix32.h \
    cart.cpp cart.h \
    analyzer.cpp analyzer.h lua53-parse.h \
    $(NULL)

dither_SOURCES = dither.cpp
dither_CPPFLAGS = $(AM_CPPFLAGS)
dither_LDFLAGS = $(AM_LDFLAGS)
dither_DEPENDENCIES = @LOL_DEPS@

