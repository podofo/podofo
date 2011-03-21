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

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfEncoding.h"
#include "base/PdfLocale.h"
#include "base/PdfName.h"
#include "base/PdfStream.h"

#include "PdfFontMetricsFreetype.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <sstream>

namespace PoDoFo {

struct TBFRange {
    int srcCode;
    std::vector<int> vecDest;
};

PdfFontCID::PdfFontCID( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, 
                        PdfVecObjects* pParent, bool bEmbed )
    : PdfFont( pMetrics, pEncoding, pParent )
{
    this->Init( bEmbed );
}

PdfFontCID::PdfFontCID( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfObject* pObject, bool bEmbed )
    : PdfFont( pMetrics, pEncoding, pObject )
{
    /* this->Init( bEmbed ); */
}

void PdfFontCID::Init( bool bEmbed )
{
    PdfObject* pDescriptor;
    PdfObject* pDescendantFonts;
    PdfObject* pCIDSystemInfo;
    PdfObject* pUnicode;

    PdfVariant var;
    PdfArray   array;

    // The descendant font is a CIDFont:
    pDescendantFonts = this->GetObject()->GetOwner()->CreateObject("Font");
    pCIDSystemInfo   = this->GetObject()->GetOwner()->CreateObject();
    pDescriptor      = this->GetObject()->GetOwner()->CreateObject("FontDescriptor");
    pUnicode         = this->GetObject()->GetOwner()->CreateObject(); // The ToUnicode CMap

    // Now setting each of the entries of the font
    this->GetObject()->GetDictionary().AddKey( PdfName::KeySubtype, PdfName("Type0") );
    this->GetObject()->GetDictionary().AddKey( "BaseFont", this->GetBaseFont() );
    this->GetObject()->GetDictionary().AddKey( "ToUnicode", pUnicode->Reference() );

    // The encoding is here usually a (Predefined) CMap from PdfIdentityEncoding:
    m_pEncoding->AddToDictionary( this->GetObject()->GetDictionary() );

    // The DecendantFonts, should be an indirect object:
    array.push_back( pDescendantFonts->Reference() );
    this->GetObject()->GetDictionary().AddKey( "DescendantFonts", array );

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

    // Create the ToUnicode CMap
    this->CreateCMap( pUnicode );

    // Setting the CIDSystemInfo paras:
    pCIDSystemInfo->GetDictionary().AddKey( "Registry", PdfString("Adobe") );
    pCIDSystemInfo->GetDictionary().AddKey( "Ordering", PdfString("Identity") );
    pCIDSystemInfo->GetDictionary().AddKey( "Supplement", PdfVariant(static_cast<pdf_int64>(0LL)) );


    // Setting the FontDescriptor paras:
    array.Clear();
    m_pMetrics->GetBoundingBox( array );

    pDescriptor->GetDictionary().AddKey( "FontName", this->GetBaseFont() );
    pDescriptor->GetDictionary().AddKey( PdfName::KeyFlags, PdfVariant( static_cast<pdf_int64>(32LL) ) ); // TODO: 0 ????
    pDescriptor->GetDictionary().AddKey( "FontBBox", array );
    pDescriptor->GetDictionary().AddKey( "ItalicAngle", PdfVariant( static_cast<pdf_int64>(m_pMetrics->GetItalicAngle()) ) );
    pDescriptor->GetDictionary().AddKey( "Ascent", m_pMetrics->GetPdfAscent() );
    pDescriptor->GetDictionary().AddKey( "Descent", m_pMetrics->GetPdfDescent() );
    pDescriptor->GetDictionary().AddKey( "CapHeight", m_pMetrics->GetPdfAscent() ); // m_pMetrics->CapHeight() );
    pDescriptor->GetDictionary().AddKey( "StemV", PdfVariant( static_cast<pdf_int64>(1LL) ) );               // m_pMetrics->StemV() );

    // Peter Petrov 24 September 2008
    m_pDescriptor = pDescriptor;
    
    if( bEmbed )
    {
        this->EmbedFont( pDescriptor );
        m_bWasEmbedded = true;
    }
}

void PdfFontCID::EmbedFont()
{
    if (!m_bWasEmbedded)
    {
        this->EmbedFont( m_pDescriptor );
        m_bWasEmbedded = true;
    }
}

void PdfFontCID::EmbedFont( PdfObject* pDescriptor )
{
    PdfObject* pContents;
    pdf_long       lSize = 0;
    
    m_bWasEmbedded = true;    
        
    pContents = this->GetObject()->GetOwner()->CreateObject();
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
        // Set Length1 before creating the stream
        // as PdfStreamedDocument does not allow 
        // adding keys to an object after a stream was written
        pContents->GetDictionary().AddKey( "Length1", PdfVariant( static_cast<pdf_int64>(lSize) ) );
            
        pContents->GetStream()->Set( pBuffer, lSize );
    } 
    else 
    {
        PdfFileInputStream stream( m_pMetrics->GetFilename() );
        lSize = stream.GetFileLength();

        // Set Length1 before creating the stream
        // as PdfStreamedDocument does not allow 
        // adding keys to an object after a stream was written
        pContents->GetDictionary().AddKey( "Length1", PdfVariant( static_cast<pdf_int64>(lSize) ) );
        pContents->GetStream()->Set( &stream );
    }
}

void PdfFontCID::CreateWidth( PdfObject* pFontDict ) const
{
    const int cAbsoluteMax = 0xffff;
    int nFirstChar = m_pEncoding->GetFirstChar();
    int nLastChar  = m_pEncoding->GetLastChar();

    int  i;

    // Allocate an initialize an array, large enough to 
    // hold a width value for every possible glyph index
    double* pdWidth = static_cast<double*>(malloc( sizeof(double) * cAbsoluteMax ) );
    if( !pdWidth )
    {
        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
    }

    for( i=0;i<cAbsoluteMax;i++ )
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
            nMin = PDF_MIN( static_cast<long>(nMin), lGlyph );
            nMax = PDF_MAX( static_cast<long>(nMax), lGlyph );
            nMax = PDF_MIN( nMax, cAbsoluteMax );

            if( lGlyph < cAbsoluteMax )
                pdWidth[lGlyph] = m_pMetrics->GetGlyphWidth( lGlyph );

        }
    }

	if (nMax >= nMin) {
        // Now compact the array
        std::ostringstream oss;
        PdfArray array;
        array.reserve( nMax - nMin + 1 );

        i = nMin;
        double    dCurWidth  = pdWidth[i];
        pdf_int64 lCurIndex  = i++;
        pdf_int64 lCurLength = 1L;
        
        for( ;i<=nMax;i++ )
        {
            if( static_cast<int>(pdWidth[i] - dCurWidth) == 0 )
                ++lCurLength;
            else
            {
                if( lCurLength > 1 ) 
                {
                    array.push_back( lCurIndex );
                    pdf_int64 temp = lCurIndex + lCurLength - 1;
                    array.push_back( temp ); 
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

        if (array.size() == 0) 
        {
            array.push_back( lCurIndex = nMin );
            array.push_back( lCurIndex = nMax );
            array.push_back( dCurWidth ); 
        }
        
        pFontDict->GetDictionary().AddKey( PdfName("W"), array ); 
    }

    free( pdWidth );
}

void PdfFontCID::CreateCMap( PdfObject* pUnicode ) const
{
    PdfFontMetricsFreetype* pFreetype = dynamic_cast<PdfFontMetricsFreetype*>(m_pMetrics);
	if (!pFreetype) return;

    int  nFirstChar = m_pEncoding->GetFirstChar();
    int  nLastChar  = m_pEncoding->GetLastChar();

    std::ostringstream oss;

    FT_Face   face = pFreetype->GetFace();
    FT_ULong  charcode;                                              
    FT_UInt   gindex;                                                

    TBFRange curRange;
    curRange.srcCode = -1;
    std::vector<TBFRange> vecRanges;
    // Only 255 sequent characters are allowed to be in one range!
    const unsigned int MAX_CHARS_IN_RANGE = 255;

    charcode = FT_Get_First_Char( face, &gindex );                   
    while ( gindex != 0 )                                            
    {        
        if( static_cast<int>(charcode) > nLastChar )
        {
            break;
        }
        else if( static_cast<int>(charcode) >= nFirstChar )
        {
            if( curRange.vecDest.size() == 0 ) 
            {
                curRange.srcCode  = gindex;
                curRange.vecDest.push_back( charcode );
            }
            else if( (curRange.srcCode + curRange.vecDest.size() == gindex) && 
                     ((gindex - curRange.srcCode + curRange.vecDest.size()) < MAX_CHARS_IN_RANGE) )
            {
                curRange.vecDest.push_back( charcode );
            } 
            else
            {
                // Create a new bfrange
                vecRanges.push_back( curRange );
                curRange.srcCode = gindex;
                curRange.vecDest.clear();
                curRange.vecDest.push_back( charcode );
            }
        }

        charcode = FT_Get_Next_Char( face, charcode, &gindex );        
    }   

    if( curRange.vecDest.size() ) 
        vecRanges.push_back( curRange );

    // Now transform it into a string
    // Make sure each bfrange section has a maximum of 
    // 100 entries. If there are more Acrobat Reader might crash
    std::ostringstream range;
    int numberOfEntries = 0;

    std::vector<TBFRange>::const_iterator it = vecRanges.begin();

    const int BUFFER_LEN = 5;
    char buffer[BUFFER_LEN]; // buffer of the format "XXXX\0"
    
    while( it != vecRanges.end() ) 
    {
        if( numberOfEntries == 99 ) 
        {
            oss << numberOfEntries << " beginbfrange" << std::endl;
            oss << range.str();
            oss << "endbfrange" << std::endl;

            numberOfEntries = 0;
            range.str("");
        }

        pdf_long iStart = (*it).srcCode;
        pdf_long iEnd   = (*it).srcCode + (*it).vecDest.size() - 1;

        snprintf( buffer, BUFFER_LEN, "%04X", static_cast<unsigned int>(iStart) );
        range << "<" << buffer << "> <";
        snprintf( buffer, BUFFER_LEN, "%04X", static_cast<unsigned int>(iEnd) );
        range << buffer << "> [ ";

        std::vector<int>::const_iterator it2 = (*it).vecDest.begin();
        while( it2 != (*it).vecDest.end() )
        {
            snprintf( buffer, BUFFER_LEN, "%04X", *it2 );
            range << "<" << buffer << "> ";

            ++it2;
        }

        range << "]" << std::endl;
        ++it;
        ++numberOfEntries;
    }

    if( numberOfEntries > 0 ) 
    {
        oss << numberOfEntries << " beginbfrange" << std::endl;
        oss << range.str();
        oss << "endbfrange" << std::endl;
    }

    /*
    // Create the beginbfchar section
    std::ostringstream oss;
    std::ostringstream tmp;
    const int BUFFER_LEN = 14;
    char buffer[BUFFER_LEN]; // buffer of the format "<XXXX> <XXXX>\0"
    
    bool bLittle = podofo_is_little_endian();
    int  nCount  = 1;

    while( nFirstChar <= nLastChar ) 
    {
        lGlyph = m_pMetrics->GetGlyphId( nFirstChar );
        if( lGlyph && lGlyph <= 0xffff)
        {
            if( bLittle ) 
                snprintf( buffer, BUFFER_LEN, "<%04X> <%04X>", static_cast<unsigned int>(lGlyph), 
                          ((nFirstChar & 0xff00) >> 8) | ((nFirstChar & 0x00ff) << 8) );
            else
                snprintf( buffer, BUFFER_LEN, "<%04X> <%04X>", static_cast<unsigned int>(lGlyph), nFirstChar );

            tmp << buffer << std::endl;

            if( nCount % 100 == 0 )
            {
                // There can be at maximum 100 lines in a bfchar section
                oss << nCount - 1 << " beginbfchar" << std::endl << tmp.str() << "endbfchar" << std::endl;
                tmp.str(""); // clear the stream
                nCount = 0;
            }

            ++nCount;
        }
        
        ++nFirstChar;
    }

    if( nCount > 1 )
        oss << nCount - 1 << " beginbfchar" << std::endl << tmp.str() << "endbfchar" << std::endl;
    */

    // Create the CMap
    pUnicode->GetStream()->BeginAppend();
    pUnicode->GetStream()->Append("/CIDInit /ProcSet findresource begin\n");
    pUnicode->GetStream()->Append("10000 dict begin\n");
    pUnicode->GetStream()->Append("begincmap\n");
    pUnicode->GetStream()->Append("/CIDSystemInfo\n");
    pUnicode->GetStream()->Append("<< /Registry (Adobe)\n");
    pUnicode->GetStream()->Append("/Ordering (UCS)\n");
    pUnicode->GetStream()->Append("/Supplement 0\n");
    pUnicode->GetStream()->Append(">> def\n");
    pUnicode->GetStream()->Append("/CMapName /Adobe-Identity-UCS def\n");
    pUnicode->GetStream()->Append("/CMapType 2 def\n");
    pUnicode->GetStream()->Append("1 begincodespacerange\n");
    pUnicode->GetStream()->Append("<0000> <FFFF>\n");
    pUnicode->GetStream()->Append("endcodespacerange\n");
    pUnicode->GetStream()->Append( oss.str().c_str() );
    pUnicode->GetStream()->Append("endcmap\n");
    pUnicode->GetStream()->Append("CMapName currentdict /CMap defineresource pop\n");
    pUnicode->GetStream()->Append("end\n");
    pUnicode->GetStream()->Append("end\n");
    pUnicode->GetStream()->EndAppend();
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

