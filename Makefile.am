
include $(top_srcdir)/src/3rdparty/lolengine/build/autotools/common.am

ACLOCAL_AMFLAGS = -I src/3rdparty/lolengine/build/autotools/m4
EXTRA_DIST += bootstrap

SUBDIRS = src/3rdparty/lolengine src
DIST_SUBDIRS = $(SUBDIRS) t utils carts

test: check

