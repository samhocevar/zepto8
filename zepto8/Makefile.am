
include $(top_srcdir)/build/autotools/common.am

bin_PROGRAMS = zepto8

zepto8dir = $(datarootdir)/zepto8

zepto8_SOURCES = \
    zepto8.cpp zepto8.h \
    cart.cpp cart.h \
    vm.cpp vm.h \
    code-fixer.cpp code-fixer.h lua53_parse.h
zepto8_CPPFLAGS = $(AM_CPPFLAGS)
zepto8_LDFLAGS = $(AM_LDFLAGS)
zepto8_DEPENDENCIES = @LOL_DEPS@
zepto8_DATA = zepto8.lua

