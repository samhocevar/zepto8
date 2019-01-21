
include $(top_srcdir)/lol/build/autotools/common.am

bin_PROGRAMS = ../zepto8 ../z8player ../z8tool ../z8lua
noinst_LIBRARIES = libzepto8.a libz8lua.a

___zepto8dir = $(datarootdir)/zepto8

# FIXME: move player.cpp / player.h into a separate library?
___zepto8_SOURCES = \
    zepto8.cpp \
    ide/ide.cpp ide/ide.h \
    ide/editor.cpp ide/editor.h \
    player.cpp player.h \
    $(3rdparty_sources) \
    $(NULL)
___zepto8_CPPFLAGS = -DLOL_CONFIG_SOLUTIONDIR=\"$(abs_top_srcdir)\" \
                     -DLOL_CONFIG_PROJECTDIR=\"$(abs_srcdir)\" \
                     $(AM_CPPFLAGS)
___zepto8_LDFLAGS = libzepto8.a libz8lua.a $(AM_LDFLAGS)
___zepto8_DEPENDENCIES = libzepto8.a libz8lua.a @LOL_DEPS@
___zepto8_DATA = bios.p8 unz8.p8
if LOL_USE_EMSCRIPTEN
___zepto8_LDFLAGS += --preload-file bios.p8
endif

3rdparty_sources = \
    3rdparty/imgui-color-text-edit/TextEditor.cpp \
    3rdparty/imgui-color-text-edit/TextEditor.h \
    3rdparty/imgui-club/imgui_memory_editor/imgui_memory_editor.h \
    3rdparty/portable-file-dialogs/portable-file-dialogs.h \
    $(NULL)

EXTRA_DIST += zepto8.vcxproj

___z8player_SOURCES = \
    z8player.cpp \
    player.cpp player.h \
    $(NULL)
___z8player_CPPFLAGS = -DLOL_CONFIG_SOLUTIONDIR=\"$(abs_top_srcdir)\" \
                       -DLOL_CONFIG_PROJECTDIR=\"$(abs_srcdir)\" \
                       $(AM_CPPFLAGS)
___z8player_LDFLAGS = libzepto8.a libz8lua.a $(AM_LDFLAGS)
___z8player_DEPENDENCIES = libzepto8.a libz8lua.a @LOL_DEPS@
if LOL_USE_EMSCRIPTEN
___z8player_LDFLAGS += --preload-file bios.p8
endif

EXTRA_DIST += z8player.vcxproj

___z8tool_SOURCES = \
    z8tool.cpp \
    splore.cpp splore.h \
    dither.cpp dither.h \
    compress.cpp compress.h zlib/deflate.h \
    zlib/trees.h zlib/zconf.h zlib/zlib.h zlib/zutil.h \
    minify.cpp minify.h \
    $(NULL)
___z8tool_CPPFLAGS = -DLOL_CONFIG_SOLUTIONDIR=\"$(abs_top_srcdir)\" \
                     -DLOL_CONFIG_PROJECTDIR=\"$(abs_srcdir)\" \
                     -Izlib -DGZ8 -DZ_SOLO -DNO_GZIP -DHAVE_MEMCPY -Dlocal= \
                     $(AM_CPPFLAGS)
___z8tool_LDFLAGS = libzepto8.a libz8lua.a $(AM_LDFLAGS)
___z8tool_DEPENDENCIES = libzepto8.a libz8lua.a @LOL_DEPS@
if LOL_USE_EMSCRIPTEN
___z8tool_LDFLAGS += --preload-file bios.p8
endif

EXTRA_DIST += zlib/deflate.c zlib/trees.c
EXTRA_DIST += z8tool.vcxproj

___z8lua_SOURCES = \
    z8lua/lua.c dummy.cpp \
    $(NULL)
___z8lua_CPPFLAGS = -xc++ -Iz8lua $(lua_cflags) $(AM_CPPFLAGS) $(AM_CXXFLAGS)
___z8lua_LDFLAGS = libz8lua.a $(lua_ldflags)
___z8lua_DEPENDENCIES = libz8lua.a

#EXTRA_DIST += z8lua.vcxproj

libzepto8_a_SOURCES = \
    zepto8.h \
    bios.cpp bios.h cart.cpp cart.h \
    analyzer.cpp analyzer.h lua53-parse.h \
    vm/vm.cpp vm/vm.h \
    vm/z8lua.cpp vm/z8lua.h \
    vm/private.cpp vm/gfx.cpp vm/render.cpp vm/sfx.cpp \
    $(NULL)

EXTRA_DIST += libzepto8.vcxproj

libz8lua_a_SOURCES = \
    z8lua/lapi.c z8lua/lcode.c z8lua/ldebug.c z8lua/ldo.c z8lua/ldump.c \
    z8lua/lfunc.c z8lua/lgc.c z8lua/llex.c z8lua/lmem.c z8lua/lobject.c \
    z8lua/lopcodes.c z8lua/lparser.c z8lua/lstate.c z8lua/lstring.c \
    z8lua/ltable.c z8lua/ltm.c z8lua/lundump.c z8lua/lvm.c z8lua/lzio.c \
    z8lua/lctype.c z8lua/lauxlib.c z8lua/lbaselib.c z8lua/lcorolib.c \
    z8lua/ltablib.c z8lua/lstrlib.c z8lua/lpico8lib.c z8lua/linit.c \
    z8lua/ldblib.c \
    \
    z8lua/fix32.h z8lua/lapi.h z8lua/lauxlib.h z8lua/lcode.h z8lua/lctype.h \
    z8lua/ldebug.h z8lua/ldo.h z8lua/lfunc.h z8lua/lgc.h z8lua/llex.h \
    z8lua/llimits.h z8lua/lmem.h z8lua/lobject.h z8lua/lopcodes.h \
    z8lua/lparser.h z8lua/lstate.h z8lua/lstring.h z8lua/ltable.h \
    z8lua/ltests.h z8lua/ltm.h z8lua/luaconf.h z8lua/lua.h z8lua/lualib.h \
    z8lua/lundump.h z8lua/lvm.h z8lua/lzio.h
libz8lua_a_CPPFLAGS = -xc++ -Iz8lua $(lua_cflags) $(AM_CPPFLAGS) $(AM_CXXFLAGS)

lua_cflags = -xc++ -Iz8lua -DLUA_USE_POSIX -DLUA_USE_STRTODHEX
lua_ldflags =
if HAVE_READLINE
lua_cflags += -DLUA_USE_READLINE
lua_ldflags += -lreadline
endif

EXTRA_DIST += libz8lua.vcxproj

