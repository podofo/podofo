
SUBDIRS = FilterTest ObjectParserTest ParserTest VariantTest CreationTest WatermarkTest LargeTest

noinst_HEADERS = PdfTest.h

noinst_PROGRAMS = PdfString_Test PdfError_Test


PdfString_Test_LDADD = -L$(top_srcdir)/src/ -lpodofo
PdfString_Test_CXXFLAGS = -I$(top_srcdir)/src/ -I/usr/include/fontconfig/ -I/usr/include/freetype2/
PdfString_Test_SOURCES = PdfString_Test.cpp

PdfError_Test_LDADD = -L$(top_srcdir)/src/ -lpodofo
PdfError_Test_CXXFLAGS = -I$(top_srcdir)/src/ -I/usr/include/fontconfig/ -I/usr/include/freetype2/
PdfError_Test_SOURCES = PdfError_Test.cpp
