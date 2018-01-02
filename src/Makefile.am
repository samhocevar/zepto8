
include $(top_srcdir)/lol/build/autotools/common.am

bin_PROGRAMS = ../zepto8 ../z8tool ../z8lua
noinst_PROGRAMS = dither
noinst_LIBRARIES = libzepto8.a libz8lua.a

___zepto8dir = $(datarootdir)/zepto8

___zepto8_SOURCES = \
    zepto8.cpp \
    player.cpp player.h \
    $(NULL)
___zepto8_CPPFLAGS = $(AM_CPPFLAGS)
___zepto8_LDFLAGS = libzepto8.a libz8lua.a $(AM_LDFLAGS)
___zepto8_DEPENDENCIES = libzepto8.a libz8lua.a @LOL_DEPS@
___zepto8_DATA = data/zepto8.lua data/font.png

EXTRA_DIST += zepto8.vcxproj

___z8tool_SOURCES = \
    z8tool.cpp \
    splore.cpp splore.h \
    $(NULL)
___z8tool_CPPFLAGS = $(AM_CPPFLAGS)
___z8tool_LDFLAGS = libzepto8.a libz8lua.a $(AM_LDFLAGS)
___z8tool_DEPENDENCIES = libzepto8.a libz8lua.a @LOL_DEPS@

EXTRA_DIST += z8tool.vcxproj

___z8lua_SOURCES = \
    z8lua/lua.c dummy.cpp \
    $(NULL)
___z8lua_CPPFLAGS = -xc++ -Iz8lua $(lua_cflags)
___z8lua_LDFLAGS = libz8lua.a $(lua_ldflags)
___z8lua_DEPENDENCIES = libz8lua.a

#EXTRA_DIST += z8lua.vcxproj

libzepto8_a_SOURCES = \
    zepto8.h \
    cart.cpp cart.h \
    analyzer.cpp analyzer.h lua53-parse.h \
    vm/vm.cpp vm/vm.h \
    vm/private.cpp vm/maths.cpp vm/gfx.cpp vm/render.cpp vm/sfx.cpp \
    $(NULL)

EXTRA_DIST += libzepto8.vcxproj

libz8lua_a_SOURCES = \
    z8lua/lapi.c z8lua/lcode.c z8lua/ldebug.c z8lua/ldo.c z8lua/ldump.c \
    z8lua/lfunc.c z8lua/lgc.c z8lua/llex.c z8lua/lmem.c z8lua/lobject.c \
    z8lua/lopcodes.c z8lua/lparser.c z8lua/lstate.c z8lua/lstring.c \
    z8lua/ltable.c z8lua/ltm.c z8lua/lundump.c z8lua/lvm.c z8lua/lzio.c \
    z8lua/lctype.c z8lua/lauxlib.c z8lua/lbaselib.c z8lua/lcorolib.c \
    z8lua/ltablib.c z8lua/lstrlib.c z8lua/lpico8lib.c z8lua/linit.c \
    \
    z8lua/fix32.h z8lua/lapi.h z8lua/lauxlib.h z8lua/lcode.h z8lua/lctype.h \
    z8lua/ldebug.h z8lua/ldo.h z8lua/lfunc.h z8lua/lgc.h z8lua/llex.h \
    z8lua/llimits.h z8lua/lmem.h z8lua/lobject.h z8lua/lopcodes.h \
    z8lua/lparser.h z8lua/lstate.h z8lua/lstring.h z8lua/ltable.h \
    z8lua/ltests.h z8lua/ltm.h z8lua/luaconf.h z8lua/lua.h z8lua/lualib.h \
    z8lua/lundump.h z8lua/lvm.h z8lua/lzio.h
libz8lua_a_CPPFLAGS = -xc++ -Iz8lua $(lua_cflags) $(AM_CPPFLAGS)

lua_cflags = -xc++ -Iz8lua -DLUA_USE_POSIX -DLUA_USE_STRTODHEX
lua_ldflags =
if HAVE_READLINE
lua_cflags += -DLUA_USE_READLINE
lua_ldflags += -lreadline
endif

#EXTRA_DIST += libz8lua.vcxproj

dither_SOURCES = dither.cpp
dither_CPPFLAGS = $(AM_CPPFLAGS)
dither_LDFLAGS = $(AM_LDFLAGS)
dither_DEPENDENCIES = @LOL_DEPS@

#EXTRA_DIST += dither.vcxproj

