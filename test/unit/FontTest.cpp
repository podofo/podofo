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

#include "FontTest.h"

#include <cppunit/Asserter.h>

#include <ft2build.h>
#include FT_FREETYPE_H

using namespace PoDoFo;

CPPUNIT_TEST_SUITE_REGISTRATION( FontTest );

void FontTest::setUp()
{
    m_pDoc = new PdfMemDocument();
    m_pVecObjects = new PdfVecObjects();
    m_pFontCache = new PdfFontCache( m_pVecObjects );
}

void FontTest::tearDown()
{
    delete m_pDoc;
    delete m_pFontCache;
    delete m_pVecObjects;
}

#if defined(PODOFO_HAVE_FONTCONFIG)
void FontTest::testFonts()
{
    FcObjectSet* objectSet = NULL;
    FcFontSet* fontSet = NULL;
    FcPattern* pattern = NULL;
    FcConfig* pConfig = NULL;

    // Initialize fontconfig
    CPPUNIT_ASSERT_EQUAL( !FcInit(), false );
    pConfig = FcInitLoadConfigAndFonts();
    CPPUNIT_ASSERT_EQUAL( !pConfig, false );

    // Get all installed fonts
    pattern = FcPatternCreate();
	objectSet = FcObjectSetBuild( FC_FAMILY, FC_STYLE, FC_FILE, FC_SLANT, FC_WEIGHT, NULL );
    fontSet = FcFontList( NULL, pattern, objectSet );

    FcObjectSetDestroy( objectSet );
	FcPatternDestroy( pattern );

    if( fontSet )
    {
        printf("Testing %i fonts\n", fontSet->nfont );
        int	j;
        for (j = 0; j < fontSet->nfont; j++)
        {
            testSingleFont( fontSet->fonts[j], pConfig );
        }

        FcFontSetDestroy( fontSet );
    }

    // Shut fontconfig down
    // Causes an assertion in fontconfig FcFini();
}

void FontTest::testSingleFont(FcPattern* pFont, FcConfig* pConfig) 
{
    std::string sFamily;
    std::string sPath;
    bool bBold;
    bool bItalic;

    if( GetFontInfo( pFont, sFamily, sPath, bBold, bItalic ) ) 
    {
        std::string sPodofoFontPath = 
            m_pFontCache->GetFontConfigFontPath( pConfig, sFamily.c_str(),
                                                 bBold, bItalic );
        
        std::string msg = "Font failed: " + sPodofoFontPath;
        EPdfFontType eFontType = PdfFontFactory::GetFontType( sPath.c_str() );
        if( eFontType == ePdfFontType_TrueType ) 
        {
            // Only TTF fonts can use identity encoding
            PdfFont* pFont = m_pDoc->CreateFont( sFamily.c_str(), bBold, bItalic,
                                                 new PdfIdentityEncoding() );
            CPPUNIT_ASSERT_EQUAL_MESSAGE( msg, pFont != NULL, true ); 
        }
        else if( eFontType != ePdfFontType_Unknown ) 
        {
            PdfFont* pFont = m_pDoc->CreateFont( sFamily.c_str(), bBold, bItalic );
            CPPUNIT_ASSERT_EQUAL_MESSAGE( msg, pFont != NULL, true ); 
        }
        else
        {
            printf("Ignoring font: %s\n", sPodofoFontPath.c_str());
        }
    }
} 

void FontTest::testCreateFontFtFace()
{
    FT_Face face;
    FT_Error error;
    
    // TODO: Find font file on disc!
    error = FT_New_Face( m_pDoc->GetFontLibrary(), "/usr/share/fonts/truetype/msttcorefonts/Arial.ttf", 0, &face );

    if( !error ) 
    {
        PdfFont* pFont = m_pDoc->CreateFont( face );

        CPPUNIT_ASSERT_MESSAGE( "Cannot create font from FT_Face.", pFont != NULL );
    }
}

bool FontTest::GetFontInfo( FcPattern* pFont, std::string & rsFamily, std::string & rsPath, 
                            bool & rbBold, bool & rbItalic )
{
    FcChar8* family = NULL;
    FcChar8* file = NULL;
    int slant;
    int weight;
    
    if( FcPatternGetString(pFont, FC_FAMILY, 0, &family) == FcResultMatch )
    {
        rsFamily = reinterpret_cast<char*>(family);
        if( FcPatternGetString(pFont, FC_FILE, 0, &file) == FcResultMatch )
        {
            rsPath = reinterpret_cast<char*>(file);
            
            if( FcPatternGetInteger(pFont, FC_SLANT, 0, &slant) == FcResultMatch )
            {
                if(slant == FC_SLANT_ROMAN) 
                    rbItalic = false;
                else if(slant == FC_SLANT_ITALIC)
                    rbItalic = true;
                else 
                    return false;

                if( FcPatternGetInteger(pFont, FC_WEIGHT, 0, &weight) == FcResultMatch )
                {
                    if(weight == FC_WEIGHT_MEDIUM)
                        rbBold = false;
                    else if(weight == FC_WEIGHT_BOLD)
                        rbBold = true;
                    else 
                        return false;

                    return true;
                }
            }
            //free( file );
        }
        //free( family );
    }

    return false;
}

#endif
