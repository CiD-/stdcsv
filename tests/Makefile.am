TESTS = check_sgetline check_parse check_read check_mmap
check_PROGRAMS = check_sgetline check_parse check_read check_mmap

check_sgetline_SOURCES = check_sgetline.c $(top_builddir)/lib/util/util.h $(top_builddir)/lib/safegetline.h $(top_builddir)/lib/include/csv.h
check_sgetline_CFLAGS = $(CHECK_CFLAGS) -I$(top_builddir)/lib/ -I$(top_builddir)/lib/include
check_sgetline_LDADD = $(top_builddir)/lib/libcsv.la $(CHECK_LIBS)

check_parse_SOURCES = check_parse.c $(top_builddir)/lib/util/util.h $(top_builddir)/lib/include/csv.h
check_parse_CFLAGS = $(CHECK_CFLAGS) -I$(top_builddir)/lib/ -I$(top_builddir)/lib/include
check_parse_LDADD = $(top_builddir)/lib/libcsv.la $(CHECK_LIBS)

check_read_SOURCES = check_read.c $(top_builddir)/lib/util/util.h $(top_builddir)/lib/include/csv.h
check_read_CFLAGS = $(CHECK_CFLAGS) -I$(top_builddir)/lib/ -I$(top_builddir)/lib/include
check_read_LDADD = $(top_builddir)/lib/libcsv.la $(CHECK_LIBS)

check_mmap_SOURCES = check_mmap.c $(top_builddir)/lib/util/util.h $(top_builddir)/lib/include/csv.h
check_mmap_CFLAGS = $(CHECK_CFLAGS) -I$(top_builddir)/lib/ -I$(top_builddir)/lib/include
check_mmap_LDADD = $(top_builddir)/lib/libcsv.la $(CHECK_LIBS)

