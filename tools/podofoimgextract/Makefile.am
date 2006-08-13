
bin_PROGRAMS = podofoimgextract
podofoimgextract_LDADD = -L$(top_srcdir)/src/ -lpodofo
podofoimgextract_CXXFLAGS = -I$(top_srcdir)/src/ -I/usr/include/fontconfig/ $(FREETYPE_CFLAGS) 

noinst_HEADERS = ImageExtractor.h
podofoimgextract_SOURCES = podofoimgextract.cpp ImageExtractor.cpp


