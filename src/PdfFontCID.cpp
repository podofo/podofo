/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#include "PdfFontCID.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfLocale.h"
#include "PdfName.h"
#include "PdfStream.h"

#include <sstream>

namespace PoDoFo {

PdfFontCID::PdfFontCID( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfVecObjects* pParent )
    : PdfFont( pMetrics, pEncoding, pParent )
{
    this->Init( true );
}

void PdfFontCID::Init( bool bEmbed )
{
    PdfObject* pDescriptor;
    PdfObject* pDescendantFonts;
    PdfObject* pCIDSystemInfo;

    PdfVariant var;
    PdfArray   array;

    // The descendant font is a CIDFont:
    pDescendantFonts = m_pObject->GetOwner()->CreateObject("Font");
    pCIDSystemInfo   = m_pObject->GetOwner()->CreateObject();
    pDescriptor      = m_pObject->GetOwner()->CreateObject("FontDescriptor");

    // Now setting each of the entries of the font
    m_pObject->GetDictionary().AddKey( PdfName::KeySubtype, PdfName("Type0") );
    m_pObject->GetDictionary().AddKey( "BaseFont", this->GetBaseFont() );

    // The encoding is here usually a (Predefined) CMap from PdfIdentityEncoding:
    m_pEncoding->AddToDictionary( m_pObject->GetDictionary() );

    // The DecendantFonts, should be an indirect object:
    array.push_back( pDescendantFonts->Reference() );
    m_pObject->GetDictionary().AddKey( "DescendantFonts", array );

    // Setting the DescendantFonts paras
    // This is a type2 CIDFont, which is also known as TrueType:
    pDescendantFonts->GetDictionary().AddKey( PdfName::KeySubtype, PdfName("CIDFontType2") );

    // Same base font as the owner font:
    pDescendantFonts->GetDictionary().AddKey( "BaseFont", this->GetBaseFont() );

    // The CIDSystemInfo, should be an indirect object:
    pDescendantFonts->GetDictionary().AddKey( "CIDSystemInfo", pCIDSystemInfo->Reference() );

    // The FontDescriptor, should be an indirect object:
    pDescendantFonts->GetDictionary().AddKey( "FontDescriptor", pDescriptor->Reference() );
    pDescendantFonts->GetDictionary().AddKey( "CIDToGIDMap", PdfName("Identity") );

    // Add the width keys
    this->CreateWidth( pDescendantFonts );

    // Setting the CIDSystemInfo paras:
    pCIDSystemInfo->GetDictionary().AddKey( "Registry", PdfString("Adobe") );
    pCIDSystemInfo->GetDictionary().AddKey( "Ordering", PdfString("Identity") );
    pCIDSystemInfo->GetDictionary().AddKey( "Supplement", PdfVariant(0L) );


    // Setting the FontDescriptor paras:
    array.clear();
    m_pMetrics->GetBoundingBox( array );

    pDescriptor->GetDictionary().AddKey( "FontName", this->GetBaseFont() );
    pDescriptor->GetDictionary().AddKey( PdfName::KeyFlags, PdfVariant( 32L ) ); // TODO: 0 ????
    pDescriptor->GetDictionary().AddKey( "FontBBox", array );
    pDescriptor->GetDictionary().AddKey( "ItalicAngle", PdfVariant( static_cast<long>(m_pMetrics->GetItalicAngle()) ) );
    pDescriptor->GetDictionary().AddKey( "Ascent", m_pMetrics->GetPdfAscent() );
    pDescriptor->GetDictionary().AddKey( "Descent", m_pMetrics->GetPdfDescent() );
    pDescriptor->GetDictionary().AddKey( "CapHeight", m_pMetrics->GetPdfAscent() ); // m_pMetrics->CapHeight() );
    pDescriptor->GetDictionary().AddKey( "StemV", PdfVariant( 1L ) );               // m_pMetrics->StemV() );

    
    if( bEmbed )
        this->EmbedFont( pDescriptor );
}

void PdfFontCID::EmbedFont( PdfObject* pDescriptor )
{
    PdfObject* pContents;
    long       lSize = 0;
        
    pContents = m_pObject->GetOwner()->CreateObject();
    if( !pContents )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
        
    pDescriptor->GetDictionary().AddKey( "FontFile2", pContents->Reference() );
        
    // if the data was loaded from memory - use it from there
    // otherwise, load from disk
    if ( m_pMetrics->GetFontDataLen() && m_pMetrics->GetFontData() ) 
    {
        // FIXME const_cast<char*> is dangerous if string literals may ever be passed
        char* pBuffer = const_cast<char*>( m_pMetrics->GetFontData() );
        lSize = m_pMetrics->GetFontDataLen();
            
        pContents->GetStream()->Set( pBuffer, lSize );
    } 
    else 
    {
        PdfFileInputStream stream( m_pMetrics->GetFilename() );
        pContents->GetStream()->Set( &stream );
            
        lSize = stream.GetFileLength();
    }
        
    pContents->GetDictionary().AddKey( "Length1", PdfVariant( lSize ) );
}

void PdfFontCID::CreateWidth( PdfObject* pFontDict ) const
{
    int nFirstChar = m_pEncoding->GetFirstChar();
    int nLastChar  = m_pEncoding->GetLastChar();

    int  i;

    // Allocate an initialize an array, large enough to 
    // hold a width value for every possible glyph index
    double* pdWidth = static_cast<double*>(malloc( sizeof(double) * 0xffff ) );
    if( !pdWidth ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
    }

    for( i=0;i<0xffff;i++ )
        pdWidth[i] = 0.0;

    // Load the width of all requested glyph indeces
    int nMin       = 0xffff;
    int nMax       = 0;

    long    lGlyph = 0;

    for( i=nFirstChar;i<=nLastChar;i++ )
    {
        lGlyph = m_pMetrics->GetGlyphId( i );
        if( lGlyph )
        {
            nMin = PDF_MIN( nMin, lGlyph );
            nMax = PDF_MAX( nMax, lGlyph );

            if( lGlyph < 0xffff )
                pdWidth[lGlyph] = m_pMetrics->GetGlyphWidth( lGlyph );
        }
    }

    // Now compact the array
    PdfArray array;
    array.reserve( nMax - nMin + 1 );

    i = nMin;
    double dCurWidth  = pdWidth[i];
    long   lCurIndex  = i++;
    long   lCurLength = 1L;

    for( ;i<=nMax;i++ )
    {
        if( static_cast<int>(pdWidth[i] - dCurWidth) == 0 )
            ++lCurLength;
        else
        {
            if( lCurLength > 1 ) 
            {
                array.push_back( lCurIndex ); 
                array.push_back( lCurIndex + lCurLength - 1 ); 
                array.push_back( dCurWidth ); 
            }
            else
            {
                if( array.size() && array.back().IsArray() ) 
                {
                    array.back().GetArray().push_back( dCurWidth );
                }
                else
                {
                    PdfArray tmp;
                    tmp.push_back( dCurWidth );
                        
                    array.push_back( lCurIndex );
                    array.push_back( tmp );
                }
            }

            lCurIndex  = i;
            lCurLength = 1L;
            dCurWidth  = pdWidth[i];
        }
    }

    pFontDict->GetDictionary().AddKey( PdfName("W"), array ); 
}

/*
    // Get the string in UTF-16be format
    PdfString    sStr = rsString.ToUnicode();
    const pdf_utf16be* pStr = sStr.GetUnicode();
    long         lGlyphId;

    const bool   bLittle = podofo_is_little_endian();

    std::ostringstream out;
    PdfLocaleImbue(out);

    out << "<";

    while( *pStr ) 
    {
        //printf("Current byte: %04x\n", *pStr );
        if( bLittle )
            lGlyphId = m_pMetrics->GetGlyphId( (((*pStr & 0xff) << 8) | ((*pStr & 0xff00) >> 8)) );
        else
            lGlyphId = m_pMetrics->GetGlyphId( *pStr );

        out << ToHex( (lGlyphId & 0xf000) >> 12 ) << ToHex( (lGlyphId & 0x0f00) >> 8 );
        out << ToHex( (lGlyphId & 0xf0) >> 4 ) << ToHex( lGlyphId & 0x0f );

        ++pStr;
    }

    out << ">";

    pStream->Append( out.str().c_str(), out.str().length() );
*/


};

