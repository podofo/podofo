/***************************************************************************
 *   Copyright (C) 2009 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _FONT_TEST_H_
#define _FONT_TEST_H_

#include <cppunit/extensions/HelperMacros.h>

#include <podofo.h>

#if defined(PODOFO_HAVE_FONTCONFIG)
#include <fontconfig/fontconfig.h>
#endif

/** This test tests the various PdfFont classes
 */
class FontTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( FontTest );
#if defined(PODOFO_HAVE_FONTCONFIG)
  CPPUNIT_TEST( testFonts );
  CPPUNIT_TEST( testCreateFontFtFace );
#endif
  CPPUNIT_TEST_SUITE_END();

 public:
  void setUp();
  void tearDown();

#if defined(PODOFO_HAVE_FONTCONFIG)
  void testFonts();
  void testCreateFontFtFace();
#endif

private:
#if defined(PODOFO_HAVE_FONTCONFIG)
    void testSingleFont(FcPattern* pFont, FcConfig* pConfig);

    bool GetFontInfo( FcPattern* pFont, std::string & rsFamily, std::string & rsPath, 
                      bool & rbBold, bool & rbItalic );
#endif

private:
    PoDoFo::PdfMemDocument* m_pDoc;
    PoDoFo::PdfVecObjects* m_pVecObjects;
    PoDoFo::PdfFontCache* m_pFontCache;

};

#endif // _FONT_TEST_H_


