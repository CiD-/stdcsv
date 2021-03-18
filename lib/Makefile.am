SUBDIRS = docs util

AM_CPPFLAGS = -Iinclude -Iutil
include_HEADERS = include/csv.h
lib_LTLIBRARIES = libcsv.la
libcsv_la_LIBADD = util/libutil.la
libcsv_la_SOURCES = misc.h misc.c charnode.h csverror.h csvsignal.h safegetline.h reader.c writer.c safegetline.c csv.c csverror.c csvsignal.c charnode.c
