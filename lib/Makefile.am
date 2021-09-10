SUBDIRS = util

if USE_DOXYGEN
SUBDIRS += docs
endif

AM_CPPFLAGS = -Iinclude -Iutil
include_HEADERS = include/csv.h
lib_LTLIBRARIES = libcsv.la
libcsv_la_LDFLAGS = -version-info 1:0:0
libcsv_la_LIBADD = util/libutil.la
libcsv_la_SOURCES = misc.h misc.c \
					csverror.h csverror.c \
					csvsignal.h csvsignal.c \
					safegetline.h safegetline.c \
					internal.h reader.c writer.c csv.c
