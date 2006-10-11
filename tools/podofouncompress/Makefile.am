
bin_PROGRAMS = podofouncompress
podofouncompress_LDADD = -L$(top_srcdir)/src/ -lpodofo
AM_CPPFLAGS = -I$(top_srcdir)/src/ -I/usr/include/fontconfig/ $(FREETYPE_CFLAGS)

noinst_HEADERS = Uncompress.h
podofouncompress_SOURCES = podofouncompress.cpp Uncompress.cpp


