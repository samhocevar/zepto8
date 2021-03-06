
include $(top_srcdir)/src/3rdparty/lolengine/build/autotools/common.am

bin_PROGRAMS = ../zepto8 ../z8dev ../z8tool ../z8lua
libretro_LTLIBRARIES = libretro-zepto8.la

# Convenience libraries; need to be marked noinst
noinst_LTLIBRARIES = $(static_libs)

static_libs = libzepto8.la libz8lua.la libquickjs.la

___z8devdir = $(datarootdir)/zepto8
libzepto8_ladir = $(datarootdir)/zepto8
libretrodir = $(datarootdir)/libretro

#
# zepto8: the emulator
#

___zepto8_SOURCES = \
    zepto8.cpp \
    player.cpp player.h \
    $(NULL)
___zepto8_CPPFLAGS = -DLOL_CONFIG_SOLUTIONDIR=\"$(abs_top_srcdir)\" \
                     -DLOL_CONFIG_PROJECTDIR=\"$(abs_srcdir)\" \
                     $(AM_CPPFLAGS)
___zepto8_LDFLAGS = -ldl $(AM_LDFLAGS)
if LOL_USE_EMSCRIPTEN
___zepto8_LDFLAGS += --preload-file pico8/bios.p8 \
                     --shell-file template.html
endif
___zepto8_LDADD = libzepto8.la

EXTRA_DIST += zepto8.vcxproj

#
# libretro-zepto8: RetroArch core
#

libretro_zepto8_la_SOURCES = \
    libretro.cpp
libretro_zepto8_la_CPPFLAGS = -I3rdparty/libretro-common/include \
                              $(AM_CPPFLAGS)
libretro_zepto8_la_LIBADD = libzepto8.la

#
# z8dev: the IDE prototype
#

# FIXME: move player.cpp / player.h into a separate library?
___z8dev_SOURCES = \
    z8dev.cpp \
    ide/ide.cpp ide/ide.h \
    ide/text-editor.cpp ide/text-editor.h \
    ide/memory-editor.cpp ide/memory-editor.h \
    player.cpp player.h \
    $(3rdparty_sources) \
    $(NULL)
___z8dev_CPPFLAGS = -DLOL_CONFIG_SOLUTIONDIR=\"$(abs_top_srcdir)\" \
                    -DLOL_CONFIG_PROJECTDIR=\"$(abs_srcdir)\" \
                    -I3rdparty/zep/include \
                    $(AM_CPPFLAGS)
___z8dev_LDFLAGS = $(AM_LDFLAGS)
___z8dev_DATA = unz8.p8
if LOL_USE_EMSCRIPTEN
___z8dev_LDFLAGS += --preload-file pico8/bios.p8 --preload-file data/zepto8.ttf \
                    --shell-file template.html
endif
___z8dev_LDADD = libzepto8.la

3rdparty_sources = \
    3rdparty/imgui-club/imgui_memory_editor/imgui_memory_editor.h \
    3rdparty/zep/include/zep/buffer.h \
    3rdparty/zep/include/zep/commands.h \
    3rdparty/zep/include/zep/display.h \
    3rdparty/zep/include/zep/editor.h \
    3rdparty/zep/include/zep/filesystem.h \
    3rdparty/zep/include/zep/gap_buffer.h \
    3rdparty/zep/include/zep/imgui/console_imgui.h \
    3rdparty/zep/include/zep/imgui/display_imgui.h \
    3rdparty/zep/include/zep/imgui/editor_imgui.h \
    3rdparty/zep/include/zep/imgui/usb_hid_keys.h \
    3rdparty/zep/include/zep/line_widgets.h \
    3rdparty/zep/include/zep/mcommon/animation/timer.h \
    3rdparty/zep/include/zep/mcommon/file/cpptoml.h \
    3rdparty/zep/include/zep/mcommon/file/fnmatch.h \
    3rdparty/zep/include/zep/mcommon/file/path.h \
    3rdparty/zep/include/zep/mcommon/logger.h \
    3rdparty/zep/include/zep/mcommon/math/math.h \
    3rdparty/zep/include/zep/mcommon/string/murmur_hash.h \
    3rdparty/zep/include/zep/mcommon/string/stringutils.h \
    3rdparty/zep/include/zep/mcommon/threadpool.h \
    3rdparty/zep/include/zep/mcommon/threadutils.h \
    3rdparty/zep/include/zep/mode.h \
    3rdparty/zep/include/zep/mode_search.h \
    3rdparty/zep/include/zep/mode_standard.h \
    3rdparty/zep/include/zep/mode_vim.h \
    3rdparty/zep/include/zep/scroller.h \
    3rdparty/zep/include/zep/splits.h \
    3rdparty/zep/include/zep/syntax.h \
    3rdparty/zep/include/zep/syntax_providers.h \
    3rdparty/zep/include/zep/syntax_rainbow_brackets.h \
    3rdparty/zep/include/zep/tab_window.h \
    3rdparty/zep/include/zep/theme.h \
    3rdparty/zep/include/zep/window.h \
    3rdparty/zep/include/zep/zep_config.h \
    3rdparty/zep/include/zep.h \
    3rdparty/zep/src/buffer.cpp \
    3rdparty/zep/src/commands.cpp \
    3rdparty/zep/src/display.cpp \
    3rdparty/zep/src/editor.cpp \
    3rdparty/zep/src/indexer.cpp \
    3rdparty/zep/src/keymap.cpp \
    3rdparty/zep/src/line_widgets.cpp \
    3rdparty/zep/src/mcommon/animation/timer.cpp \
    3rdparty/zep/src/mcommon/file/path.cpp \
    3rdparty/zep/src/mcommon/string/stringutils.cpp \
    3rdparty/zep/src/mode.cpp \
    3rdparty/zep/src/mode_search.cpp \
    3rdparty/zep/src/mode_standard.cpp \
    3rdparty/zep/src/mode_tree.cpp \
    3rdparty/zep/src/mode_vim.cpp \
    3rdparty/zep/src/scroller.cpp \
    3rdparty/zep/src/splits.cpp \
    3rdparty/zep/src/syntax.cpp \
    3rdparty/zep/src/syntax_providers.cpp \
    3rdparty/zep/src/syntax_rainbow_brackets.cpp \
    3rdparty/zep/src/syntax_tree.cpp \
    3rdparty/zep/src/tab_window.cpp \
    3rdparty/zep/src/theme.cpp \
    3rdparty/zep/src/window.cpp \
    $(NULL)

EXTRA_DIST += $(___z8dev_DATA) z8dev.vcxproj

#
# z8tool: multi purpose tool
#

___z8tool_SOURCES = \
    z8tool.cpp \
    splore.cpp splore.h \
    dither.cpp dither.h \
    compress.cpp compress.h zlib/deflate.h zlib/gz8.h \
    zlib/trees.h zlib/zconf.h zlib/zlib.h zlib/zutil.h \
    minify.cpp minify.h \
    telnet.h \
    $(NULL)
___z8tool_CPPFLAGS = -DLOL_CONFIG_SOLUTIONDIR=\"$(abs_top_srcdir)\" \
                     -DLOL_CONFIG_PROJECTDIR=\"$(abs_srcdir)\" \
                     -Izlib -DGZ8 -DZ_SOLO -DNO_GZIP -DHAVE_MEMCPY -Dlocal= \
                     $(AM_CPPFLAGS)
___z8tool_LDFLAGS = -ldl $(AM_LDFLAGS)
___z8tool_LDADD = libzepto8.la

EXTRA_DIST += zlib/deflate.c zlib/trees.c
EXTRA_DIST += z8tool.vcxproj

#
# z8lua: our modified Lua interpreter
#

___z8lua_SOURCES = \
    3rdparty/z8lua/lua.c dummy.cpp \
    $(NULL)
___z8lua_CPPFLAGS = -xc++ -Iz8lua $(lua_cflags) $(AM_CPPFLAGS) $(AM_CXXFLAGS)
___z8lua_LDFLAGS = $(lua_ldflags)
___z8lua_LDADD = libz8lua.la

EXTRA_DIST += z8lua.vcxproj

#
# libzepto8: core library used by all programs
#

libzepto8_la_SOURCES = \
    zepto8.h \
    vm.cpp \
    bios.cpp bios.h \
    synth.cpp synth.h \
    \
    bindings/js.h bindings/lua.h \
    \
    pico8/vm.cpp pico8/vm.h \
    pico8/pico8.h pico8/memory.h pico8/grammar.h \
    pico8/cart.cpp pico8/cart.h \
    pico8/private.cpp pico8/gfx.cpp pico8/code.cpp pico8/ast.cpp \
    pico8/parser.cpp pico8/render.cpp pico8/sfx.cpp \
    pico8/api.cpp \
    \
    raccoon/vm.cpp raccoon/vm.h \
    raccoon/memory.h raccoon/font.h \
    raccoon/api.cpp \
    \
    3rdparty/lodepng/lodepng.cpp 3rdparty/lodepng/lodepng.h \
    $(NULL)
libzepto8_la_DATA = pico8/bios.p8
libzepto8_la_LIBADD = @LOL_DEPS@ libz8lua.la libquickjs.la
libzepto8_la_CPPFLAGS = $(AM_CPPFLAGS)

EXTRA_DIST += $(libzepto8_la_DATA) libzepto8.vcxproj

#
# libz8lua: z8lua library used by all programs
#

libz8lua_la_SOURCES = \
    3rdparty/z8lua/lapi.c 3rdparty/z8lua/lcode.c 3rdparty/z8lua/ldebug.c \
    3rdparty/z8lua/ldo.c 3rdparty/z8lua/ldump.c 3rdparty/z8lua/lfunc.c \
    3rdparty/z8lua/lgc.c 3rdparty/z8lua/llex.c 3rdparty/z8lua/lmem.c \
    3rdparty/z8lua/lobject.c 3rdparty/z8lua/lopcodes.c 3rdparty/z8lua/lparser.c \
    3rdparty/z8lua/lstate.c 3rdparty/z8lua/lstring.c 3rdparty/z8lua/ltable.c \
    3rdparty/z8lua/ltm.c 3rdparty/z8lua/lundump.c 3rdparty/z8lua/lvm.c \
    3rdparty/z8lua/lzio.c 3rdparty/z8lua/lctype.c 3rdparty/z8lua/lauxlib.c \
    3rdparty/z8lua/lbaselib.c 3rdparty/z8lua/lcorolib.c 3rdparty/z8lua/ltablib.c \
    3rdparty/z8lua/lstrlib.c 3rdparty/z8lua/lpico8lib.c 3rdparty/z8lua/linit.c \
    3rdparty/z8lua/ldblib.c 3rdparty/z8lua/eris.c \
    \
    3rdparty/z8lua/fix32.h 3rdparty/z8lua/lapi.h 3rdparty/z8lua/lauxlib.h \
    3rdparty/z8lua/lcode.h 3rdparty/z8lua/lctype.h 3rdparty/z8lua/ldebug.h \
    3rdparty/z8lua/ldo.h 3rdparty/z8lua/lfunc.h 3rdparty/z8lua/lgc.h \
    3rdparty/z8lua/llex.h 3rdparty/z8lua/llimits.h 3rdparty/z8lua/lmem.h \
    3rdparty/z8lua/lobject.h 3rdparty/z8lua/lopcodes.h 3rdparty/z8lua/lparser.h \
    3rdparty/z8lua/lstate.h 3rdparty/z8lua/lstring.h 3rdparty/z8lua/ltable.h \
    3rdparty/z8lua/ltests.h 3rdparty/z8lua/ltm.h 3rdparty/z8lua/luaconf.h \
    3rdparty/z8lua/lua.h 3rdparty/z8lua/lualib.h 3rdparty/z8lua/lundump.h \
    3rdparty/z8lua/lvm.h 3rdparty/z8lua/lzio.h 3rdparty/z8lua/eris.h
libz8lua_la_CPPFLAGS = $(lua_cflags) $(AM_CPPFLAGS) $(AM_CXXFLAGS)

lua_cflags = -xc++ -I3rdparty/z8lua -DLUA_USE_POSIX -DLUA_USE_STRTODHEX
lua_ldflags =
if HAVE_READLINE
lua_cflags += -DLUA_USE_READLINE
lua_ldflags += -lreadline
endif

EXTRA_DIST += libz8lua.vcxproj

#
# libquickjs: QuickJS library used by all programs
#

libquickjs_la_SOURCES = \
    3rdparty/quickjs/quickjs.c 3rdparty/quickjs/quickjs.h \
    3rdparty/quickjs/quickjs-atom.h 3rdparty/quickjs/quickjs-opcode.h \
    3rdparty/quickjs/list.h \
    3rdparty/quickjs/libbf.c 3rdparty/quickjs/libbf.h \
    3rdparty/quickjs/libregexp.c 3rdparty/quickjs/libregexp.h \
    3rdparty/quickjs/libregexp-opcode.h \
    3rdparty/quickjs/libunicode.c 3rdparty/quickjs/libunicode.h \
    3rdparty/quickjs/libunicode-table.h \
    3rdparty/quickjs/cutils.c 3rdparty/quickjs/cutils.h
libquickjs_la_CPPFLAGS = \
    -I3rdparty/quickjs -D_GNU_SOURCE -DCONFIG_VERSION=\"2019-08-18\" \
    $(AM_CPPFLAGS) $(AM_CFLAGS) \
    -Wno-unused-parameter -Wno-sign-compare

EXTRA_DIST += libquickjs.vcxproj

