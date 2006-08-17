
bin_PROGRAMS = podofopdfinfo
podofopdfinfo_LDADD = -L$(top_srcdir)/src/ -lpodofo
podofopdfinfo_CXXFLAGS = -I$(top_srcdir)/src/ -I/usr/include/fontconfig/ $(FREETYPE_CFLAGS)

noinst_HEADERS = pdfinfo.h
podofopdfinfo_SOURCES = pdfinfo.cpp podofopdfinfo.cpp


