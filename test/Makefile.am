
SUBDIRS = AlgorithmTest ObjectParserTest ParserTest VariantTest CreationTest

EXTRA_DIST = runTests.sh

noinst_HEADERS = PdfTest.h

noinst_PROGRAMS = PdfString_Test 


PdfString_Test_LDADD = -L$(top_srcdir)/src/ -lpodofo
PdfString_Test_CXXFLAGS = -I$(top_srcdir)/src/ -I/usr/include/fontconfig/ -I/usr/include/freetype2/
PdfString_Test_SOURCES = PdfString_Test.cpp

test:
	./runTests.sh
