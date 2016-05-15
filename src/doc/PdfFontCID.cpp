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
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#include "PdfFontCID.h"

#include "base/PdfDefines.h"
#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfEncoding.h"
#include "base/PdfLocale.h"
#include "base/PdfName.h"
#include "base/PdfStream.h"

#include "PdfFontMetricsFreetype.h"

#include "PdfFontTTFSubset.h"
#include "base/PdfInputDevice.h"
#include "base/PdfOutputDevice.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <sstream>

namespace PoDoFo {

struct TBFRange {
    FT_UInt srcCode;
    std::vector<FT_UInt> vecDest;
};

typedef std::map<long, double> GlyphWidths;
typedef std::map<FT_UInt, FT_ULong> GidToCodePoint;
static bool fillGidToCodePoint(GidToCodePoint& array, PdfFontMetrics* metrics);
typedef std::map<pdf_utf16be, int> UnicodeToIndex;
static UnicodeToIndex getUnicodeToIndexTable(const PdfEncoding* pEnconding);

static GlyphWidths getGlyphWidths(PdfFontMetrics* metrics, const std::set<pdf_utf16be>& setUsed, const UnicodeToIndex& unicodeToIndex);
static GlyphWidths getGlyphWidths(PdfFontMetrics* metrics, const std::set<pdf_utf16be>& setUsed);

static void createWidths(PdfObject* pFontDict, PdfFontMetrics* metrics, const std::set<pdf_utf16be>& setUsed, const UnicodeToIndex& unicodeToIndex);
static void createWidths(PdfObject* pFontDict, PdfFontMetrics* metrics, const std::set<pdf_utf16be>& setUsed);

static GidToCodePoint getGidToCodePoint(const PdfEncoding* pEncoding, PdfFontMetrics* pMetrics, const std::set<pdf_utf16be>& setUsed, const UnicodeToIndex& unicodeToIndex);
static GidToCodePoint getGidToCodePoint(const PdfEncoding* pEncoding, PdfFontMetrics* pMetrics, const std::set<pdf_utf16be>& setUsed);

static void fillUnicodeStream( PdfStream* pStream , const GidToCodePoint& gidToCodePoint, int nFirstChar, int nLastChar, bool bSingleByteEncoding);

#define SWAP_UTF16BE(x) static_cast<pdf_utf16be>(((x << 8) & 0xFF00) | ((x >> 8) & 0x00FF))

/** Build a reverse lookup table, determine a position/index of each unicode code 
 */
UnicodeToIndex getUnicodeToIndexTable(const PdfEncoding* pEncoding)
{
    UnicodeToIndex table;
    pdf_utf16be uc;
    int nLast  = pEncoding->GetLastChar();
    for (int nChar = pEncoding->GetFirstChar(); nChar <= nLast; ++nChar)
    {
        uc = pEncoding->GetCharCode(nChar);
        table[SWAP_UTF16BE(uc)] = nChar;
    }
    return table;
}

class WidthExporter {
    PdfArray& _output;
    PdfArray _widths;    /* array of consecutive different widths */

    long    _start;        /* glyphIndex of start range */
    double  _width;
    long    _count;        /* number of processed glyphIndex'es since start of range */
    /* methods */
    void reset(GlyphWidths::const_iterator& it);
    void emitSameWidth();
    void emitArrayWidths();
public:
    WidthExporter(PdfArray& output, GlyphWidths::const_iterator& it);
    void update(GlyphWidths::const_iterator& it);
    void finish();
    void updateSBE(GlyphWidths::const_iterator& it);
    void finishSBE();
};

PdfFontCID::PdfFontCID( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfObject* pObject, bool PODOFO_UNUSED_PARAM(bEmbed) )
    : PdfFont( pMetrics, pEncoding, pObject ), m_pDescendantFonts( NULL )
{
    /* this->Init( bEmbed, false ); No changes to dictionary */
    m_bWasEmbedded = true; /* embedding on this path is not allowed at all, so
                            pretend like it's already done */
}

PdfFontCID::PdfFontCID( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, 
                        PdfVecObjects* pParent, bool bEmbed, bool bSubset )
    : PdfFont( pMetrics, pEncoding, pParent ), m_pDescendantFonts( NULL )
{
    this->Init( bEmbed, bSubset );
}

void PdfFontCID::Init( bool bEmbed, bool bSubset )
{
    PdfObject* pDescriptor;

    PdfVariant var;
    PdfArray   array;

    if (m_pEncoding->IsSingleByteEncoding())
    {
    pDescriptor      = this->GetObject()->GetOwner()->CreateObject("FontDescriptor");

    // Now setting each of the entries of the font
        this->GetObject()->GetDictionary().AddKey( PdfName::KeySubtype, PdfName("TrueType"));
        this->GetObject()->GetDictionary().AddKey( "BaseFont", this->GetBaseFont() );
        this->GetObject()->GetDictionary().AddKey( "FontDescriptor", pDescriptor->Reference() );

        // The encoding is here usually a (Predefined) CMap from PdfIdentityEncoding:
        m_pEncoding->AddToDictionary( this->GetObject()->GetDictionary() );

    }
    else {
        pDescriptor = this->GetObject()->GetOwner()->CreateObject("FontDescriptor");

        // Now setting each of the entries of the font
    this->GetObject()->GetDictionary().AddKey( PdfName::KeySubtype, PdfName("Type0") );
    this->GetObject()->GetDictionary().AddKey( "BaseFont", this->GetBaseFont() );

    // The encoding is here usually a (Predefined) CMap from PdfIdentityEncoding:
    m_pEncoding->AddToDictionary( this->GetObject()->GetDictionary() );

        // The descendant font is a CIDFont:
        m_pDescendantFonts = this->GetObject()->GetOwner()->CreateObject("Font");

    // The DecendantFonts, should be an indirect object:
    array.push_back( m_pDescendantFonts->Reference() );
    this->GetObject()->GetDictionary().AddKey( "DescendantFonts", array );

    // Setting the DescendantFonts paras
    // This is a type2 CIDFont, which is also known as TrueType:
    m_pDescendantFonts->GetDictionary().AddKey( PdfName::KeySubtype, PdfName("CIDFontType2") );

    // Same base font as the owner font:
    m_pDescendantFonts->GetDictionary().AddKey( "BaseFont", this->GetBaseFont() );

    // The CIDSystemInfo, should be an indirect object:
        PdfObject* pCIDSystemInfo = this->GetObject()->GetOwner()->CreateObject();
    m_pDescendantFonts->GetDictionary().AddKey( "CIDSystemInfo", pCIDSystemInfo->Reference() );
    // Setting the CIDSystemInfo paras:
    pCIDSystemInfo->GetDictionary().AddKey( "Registry", PdfString("Adobe") );
    pCIDSystemInfo->GetDictionary().AddKey( "Ordering", PdfString("Identity") );
    pCIDSystemInfo->GetDictionary().AddKey( "Supplement", PdfVariant(static_cast<pdf_int64>(PODOFO_LL_LITERAL(0))) );

        // The FontDescriptor, should be an indirect object:
        m_pDescendantFonts->GetDictionary().AddKey( "FontDescriptor", pDescriptor->Reference() );
        m_pDescendantFonts->GetDictionary().AddKey( "CIDToGIDMap", PdfName("Identity") );
    }

    // Setting the FontDescriptor paras:
    array.Clear();
    m_pMetrics->GetBoundingBox( array );

    pDescriptor->GetDictionary().AddKey( "FontName", this->GetBaseFont() );
    pDescriptor->GetDictionary().AddKey( PdfName::KeyFlags, PdfVariant( static_cast<pdf_int64>(PODOFO_LL_LITERAL(32)) ) ); // TODO: 0 ????
    pDescriptor->GetDictionary().AddKey( "FontBBox", array );
    pDescriptor->GetDictionary().AddKey( "ItalicAngle", PdfVariant( static_cast<pdf_int64>(m_pMetrics->GetItalicAngle()) ) );
    pDescriptor->GetDictionary().AddKey( "Ascent", m_pMetrics->GetPdfAscent() );
    pDescriptor->GetDictionary().AddKey( "Descent", m_pMetrics->GetPdfDescent() );
    pDescriptor->GetDictionary().AddKey( "CapHeight", m_pMetrics->GetPdfAscent() ); // m_pMetrics->CapHeight() );
    pDescriptor->GetDictionary().AddKey( "StemV", PdfVariant( static_cast<pdf_int64>(PODOFO_LL_LITERAL(1)) ) );               // m_pMetrics->StemV() );

    // Peter Petrov 24 September 2008
    m_pDescriptor = pDescriptor;
    
	 m_bIsSubsetting = bSubset;
    if( bEmbed && !bSubset)
    {
        this->EmbedFont( pDescriptor );
        m_bWasEmbedded = true;
    } else if (!bEmbed && !bSubset) {
        // it's not asked to be embedded, thus mark as embedded already, to not do that at PdfFontCID::EmbedFont()
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

void PdfFontCID::EmbedSubsetFont()
{
	EmbedFont();
}

void PdfFontCID::AddUsedSubsettingGlyphs (const PdfString &sText, long lStringLen)
{
	if (IsSubsetting()) {
        PdfString uniText = sText.ToUnicode();
        const pdf_utf16be *uniChars = uniText.GetUnicode();
		for (long ii = 0; ii < lStringLen; ii++) {
            m_setUsed.insert(SWAP_UTF16BE(uniChars[ii]));
		}
	}
}

void PdfFontCID::EmbedFont( PdfObject* pDescriptor )
{
	bool fallback = true;
    
    if (IsSubsetting()) {
        if (m_setUsed.empty()) {
            /* Space at least should exist (as big endian) */
            m_setUsed.insert(0x20);
        }
        PdfFontMetrics *pMetrics = GetFontMetrics2();

        if (pMetrics && pMetrics->GetFontDataLen() && pMetrics->GetFontData()) {

            if (m_pEncoding->IsSingleByteEncoding()) {
                UnicodeToIndex unicodeToIndex = getUnicodeToIndexTable(m_pEncoding);
                createWidths(this->GetObject(), pMetrics, m_setUsed, unicodeToIndex);
        
                PdfObject* pUnicode = this->GetObject()->GetOwner()->CreateObject();
                GidToCodePoint gidToCodePoint = getGidToCodePoint( m_pEncoding, pMetrics, m_setUsed, unicodeToIndex);
                fillUnicodeStream( pUnicode->GetStream(), gidToCodePoint, *m_setUsed.begin(), *m_setUsed.rbegin(), true);
                this->GetObject()->GetDictionary().AddKey( "ToUnicode", pUnicode->Reference() );
            }
            else {
                createWidths(m_pDescendantFonts, pMetrics, m_setUsed);

                PdfObject* pUnicode = this->GetObject()->GetOwner()->CreateObject();
                GidToCodePoint gidToCodePoint = getGidToCodePoint( m_pEncoding, pMetrics, m_setUsed);
                fillUnicodeStream( pUnicode->GetStream(), gidToCodePoint, *m_setUsed.begin(), *m_setUsed.rbegin(), false);
                this->GetObject()->GetDictionary().AddKey( "ToUnicode", pUnicode->Reference() );
            }

            PdfInputDevice input(pMetrics->GetFontData(), pMetrics->GetFontDataLen());
			PdfRefCountedBuffer buffer;
			PdfOutputDevice output(&buffer);

            PdfFontTTFSubset subset(&input, pMetrics, PdfFontTTFSubset::eFontFileType_TTF);

            std::vector<unsigned char> array;
            subset.BuildFont(buffer, m_setUsed, array );

            if (!m_pEncoding->IsSingleByteEncoding())
            {
                if (!array.empty()) {
                    PdfObject* cidSet = pDescriptor->GetOwner()->CreateObject();
                    TVecFilters vecFlate;
                    vecFlate.push_back(ePdfFilter_FlateDecode);
#if (defined(_MSC_VER)  &&  _MSC_VER < 1700) || (defined(__BORLANDC__))	// MSC before VC11 has no data member, same as BorlandC
                    PdfMemoryInputStream stream(reinterpret_cast<const char*>(&array[0]), array.size());
#else
                    PdfMemoryInputStream stream(reinterpret_cast<const char*>(array.data()), array.size());
#endif
					cidSet->GetStream()->Set(&stream, vecFlate);
                    pDescriptor->GetDictionary().AddKey("CIDSet", cidSet->Reference());
			}
            }

            PdfObject *pContents = this->GetObject()->GetOwner()->CreateObject();
			pDescriptor->GetDictionary().AddKey( "FontFile2", pContents->Reference() );

			pdf_long lSize = buffer.GetSize();
			pContents->GetDictionary().AddKey("Length1", PdfVariant(static_cast<pdf_int64>(lSize)));
			pContents->GetStream()->Set(buffer.GetBuffer(), lSize);

			fallback = false;
		}
	}

	if (fallback) {
		PdfObject* pContents;
		pdf_long       lSize = 0;
    
    pContents = this->GetObject()->GetOwner()->CreateObject();
    if( !pContents || !m_pMetrics )
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
}

void PdfFontCID::CreateWidth( PdfObject* pFontDict ) const
{
    const int cAbsoluteMax = 0xffff;
    int nFirstChar = m_pEncoding->GetFirstChar();
    int nLastChar  = m_pEncoding->GetLastChar();

    int  i;

    // Allocate an initialize an array, large enough to 
    // hold a width value for every possible glyph index
    double* pdWidth = static_cast<double*>(podofo_calloc( cAbsoluteMax, sizeof(double) ) );
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

    podofo_free( pdWidth );
}

void PdfFontCID::CreateCMap( PdfObject* PODOFO_UNUSED_PARAM(pUnicode) ) const
{
    GidToCodePoint gidToCodePoint;
    if (fillGidToCodePoint(gidToCodePoint, m_pMetrics)) 
    {
        //createCMap(pUnicode, gidToCodePoint, m_pEncoding->GetFirstChar(), m_pEncoding->GetLastChar(), m_pEncoding->IsSingleByteEncoding() );
    }
}

static std::vector<TBFRange>
createUnicodeRanges(const GidToCodePoint& gidToCodePoint, int nFirstChar, int nLastChar)
{
    TBFRange curRange;
    curRange.srcCode = -1;
    std::vector<TBFRange> vecRanges;
    FT_UInt  gindex;
    FT_ULong charcode;

    // Only 255 sequent characters are allowed to be in one range!
    const unsigned int MAX_CHARS_IN_RANGE = 255;

    for(GidToCodePoint::const_iterator it = gidToCodePoint.begin(); it != gidToCodePoint.end(); ++it) {
        gindex   = it->first;
        charcode = it->second;

        if( static_cast<int>(charcode) > nLastChar )
        {
            break;
        }
        if( static_cast<int>(charcode) >= nFirstChar )
        {
            if( curRange.vecDest.size() == 0 ) 
            {
                curRange.srcCode  = gindex;
                curRange.vecDest.push_back( charcode );
            }
            else if( (curRange.srcCode + curRange.vecDest.size() == gindex) && 
                     ((gindex - curRange.srcCode + curRange.vecDest.size()) < MAX_CHARS_IN_RANGE) &&
                     ((gindex & 0xff00) == (curRange.srcCode & 0xff00))
                   )
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
    }   

    if( curRange.vecDest.size() ) 
        vecRanges.push_back( curRange );

    return vecRanges;
}

static void
fillUnicodeStream( PdfStream* pStream , const GidToCodePoint& gidToCodePoint, int nFirstChar, int nLastChar, bool bSingleByteEncoding)
{
    std::ostringstream oss;
    std::ostringstream range;

    std::vector<TBFRange> vecRanges = createUnicodeRanges( gidToCodePoint, nFirstChar, nLastChar );

    pStream->BeginAppend();
    pStream->Append("/CIDInit /ProcSet findresource begin\n"
        "12 dict begin\n"
        "begincmap\n"
        "/CIDSystemInfo\n"
        "<< /Registry (Adobe)\n"
        "/Ordering (UCS)\n"
        "/Supplement 0\n"
        ">> def\n"
        "/CMapName /Adobe-Identity-UCS def\n"
        "/CMapType 2 def\n"
        "1 begincodespacerange\n");

    if (bSingleByteEncoding)
        pStream->Append("<00> <FF>\n");
    else
        pStream->Append("<0000> <FFFF>\n");
    pStream->Append("endcodespacerange\n");

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

            pStream->Append(oss.str());

            oss.str("");
            range.str("");
            numberOfEntries = 0;
        }

        pdf_long iStart = (*it).srcCode;
        pdf_long iEnd   = (*it).srcCode + (*it).vecDest.size() - 1;

        if (bSingleByteEncoding)
        {
            snprintf( buffer, BUFFER_LEN, "%02X", static_cast<unsigned int>(iStart) );
            range << "<" << buffer << "> <";
            snprintf( buffer, BUFFER_LEN, "%02X", static_cast<unsigned int>(iEnd) );
        }
        else
        {
        snprintf( buffer, BUFFER_LEN, "%04X", static_cast<unsigned int>(iStart) );
        range << "<" << buffer << "> <";
        snprintf( buffer, BUFFER_LEN, "%04X", static_cast<unsigned int>(iEnd) );
        }
        range << buffer << "> [ ";

        std::vector<FT_UInt>::const_iterator it2 = (*it).vecDest.begin();
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
        pStream->Append( oss.str().c_str() );
    }

    pStream->Append("endcmap\n"
        "CMapName currentdict /CMap defineresource pop\n"
        "end\n"
        "end\n");
    pStream->EndAppend();
}

static GidToCodePoint
getGidToCodePoint(const PdfEncoding* PODOFO_UNUSED_PARAM(pEncoding), PdfFontMetrics* pMetrics, const std::set<pdf_utf16be>& setUsed, const UnicodeToIndex& unicodeToIndex)
{
    GidToCodePoint gidToCodePoint;
    pdf_utf16be codePoint;
    long lGlyph;
    long lRepl = pMetrics->GetGlyphId( 0xFFFD );

    UnicodeToIndex::const_iterator indexLookup;
    for (std::set<pdf_utf16be>::const_iterator it = setUsed.begin(); it != setUsed.end(); ++it)
    {
        codePoint = *it;
        indexLookup = unicodeToIndex.find( codePoint );
        if (indexLookup != unicodeToIndex.end()) {
            lGlyph = pMetrics->GetGlyphId( codePoint );
            if( lGlyph )
        {
                gidToCodePoint[indexLookup->second] = codePoint;
            }
            else if (lRepl)
            {
                gidToCodePoint[indexLookup->second] = 0xFFFD;
            }
        }
    }
    return gidToCodePoint;
}

static GidToCodePoint
getGidToCodePoint(const PdfEncoding* PODOFO_UNUSED_PARAM(pEncoding), PdfFontMetrics* pMetrics, const std::set<pdf_utf16be>& setUsed)
{
    GidToCodePoint gidToCodePoint;
    pdf_utf16be codePoint;
    long lGlyph;
    for (std::set<pdf_utf16be>::const_iterator it = setUsed.begin(); it != setUsed.end(); ++it)
    {
        codePoint = *it;
        lGlyph = pMetrics->GetGlyphId( codePoint );
        if( lGlyph )
        {
            gidToCodePoint[lGlyph] = codePoint;
    }
    }
    return gidToCodePoint;
}

void PdfFontCID::MaybeUpdateBaseFontKey(void)
{
   /* FIXME: I believe that this is not required on Win32 platforms, but... */ 
   if (!m_pDescendantFonts) {
      return;
   }
   const PdfFontMetricsFreetype *pFreetype = dynamic_cast<const PdfFontMetricsFreetype *>(this->GetFontMetrics());
   if (!pFreetype) {
      return;
   }

   std::string name = this->GetBaseFont().GetName();
   if (this->IsBold() && this->IsItalic()) {
      if (pFreetype->IsBold() && pFreetype->IsItalic()) {
         return;
      }
      if (pFreetype->IsBold() && !pFreetype->IsItalic()) {
         name += ",Italic";
      } else if (!pFreetype->IsBold() && pFreetype->IsItalic()) {
         name += ",Bold";
      } else {
         name += ",BoldItalic";
      }
   } else if (this->IsBold()) {
      if (pFreetype->IsBold()) {
         return;
      }
      name += ",Bold";
   } else if (this->IsItalic()) {
      if (pFreetype->IsItalic()) {
         return;
      }
      name += ",Italic";
   } else {
      return;
   }

   m_pDescendantFonts->GetDictionary().AddKey("BaseFont", PdfName( name ) );
}

void PdfFontCID::SetBold( bool bBold )
{
   PdfFont::SetBold(bBold);
   MaybeUpdateBaseFontKey();
}

void PdfFontCID::SetItalic( bool bItalic )
{
   PdfFont::SetItalic(bItalic);
   MaybeUpdateBaseFontKey();
}

WidthExporter::WidthExporter(PdfArray& output, GlyphWidths::const_iterator& it)
    : _output(output), _widths()
{
    reset(it);
}

void WidthExporter::reset(GlyphWidths::const_iterator& it)
{
    _start = it->first;
    _width = it->second;
    _count = 1;
}

void WidthExporter::update(GlyphWidths::const_iterator& it)
{
    if (it->first == (_start + _count)) {
        /* continous gid */
        if (static_cast<pdf_int64>(it->second - _width) != 0) {
            /* different width, so emit if previous range was with same width */
            if ((_count != 1) && _widths.empty()) {
                emitSameWidth();
                reset(it);
                return;
            }
            _widths.push_back(static_cast<pdf_int64>(_width + 0.5));
            _width = it->second;
            ++_count;
            return;
        }
        /* two or more gids with same width */
        if (!_widths.empty()) {
            emitArrayWidths();
            /* setup previous width as start position */
            _start += _count - 1;
            _count = 2;
            return;
        }
        /* consecutive range of same widths */
        ++_count;
        return;
    }
    /* gid gap (font subset) */
    finish();
    reset(it);
}

void WidthExporter::finish()
{
    /* if there is a single glyph remaining, emit it as array */
    if (!_widths.empty() || (_count == 1)) {
        _widths.push_back(static_cast<pdf_int64>(_width + 0.5));
        emitArrayWidths();
        return;
    }
    emitSameWidth();
}

void WidthExporter::emitSameWidth()
{
    _output.push_back(static_cast<pdf_int64>(_start));
    _output.push_back(static_cast<pdf_int64>(_start + _count - 1));
    _output.push_back(static_cast<pdf_int64>(_width + 0.5));
}

void WidthExporter::emitArrayWidths()
{
    _output.push_back(static_cast<pdf_int64>(_start));
    _output.push_back(_widths);
    _widths.Clear();
}

void WidthExporter::updateSBE(GlyphWidths::const_iterator& it)
{
    _output.push_back(static_cast<pdf_int64>(_width + 0.5));
    while(++_start < it->first) {
        _output.push_back(static_cast<pdf_int64>(0));
    }
    reset(it);
}
void WidthExporter::finishSBE()
{
    _output.push_back(static_cast<pdf_int64>(_width + 0.5));
}

static bool
fillGidToCodePoint(GidToCodePoint& array, PdfFontMetrics* metrics)
{
    PdfFontMetricsFreetype* pFreetype = dynamic_cast<PdfFontMetricsFreetype*>(metrics);
    if (!pFreetype) return false;

    FT_Face  face = pFreetype->GetFace();
    FT_UInt  gindex;
    FT_ULong charcode = FT_Get_First_Char( face, &gindex );

    while ( gindex != 0 )
    {
        array.insert(std::pair<FT_UInt, FT_ULong>(gindex, charcode));
        charcode = FT_Get_Next_Char( face, charcode, &gindex );
    }
    return true;
}

static GlyphWidths
getGlyphWidths(PdfFontMetrics* pMetrics, const std::set<pdf_utf16be>& setUsed)
{
    GlyphWidths glyphWidths;

    const long cAbsoluteMax = 0xffff;
    long nMin       = cAbsoluteMax;
    long nMax       = 0;

    long    lGlyph;
    double  dCurWidth = 1000;

    // Load the width of all requested glyph indeces
    for (std::set<pdf_utf16be>::const_iterator it = setUsed.begin(); it != setUsed.end(); ++it)
    {
        /* If font does not contain a character code, then .notdef */
        lGlyph = pMetrics->GetGlyphId( *it );
        if( lGlyph )
        {
            nMin = PDF_MIN( nMin, lGlyph );
            nMax = PDF_MAX( nMax, lGlyph );
            nMax = PDF_MIN( nMax, cAbsoluteMax );

            if( lGlyph < cAbsoluteMax )
            {
                dCurWidth = pMetrics->GetGlyphWidth( lGlyph );
                glyphWidths[lGlyph] = dCurWidth;
            }
        }
    }
    return glyphWidths;
}

static GlyphWidths
getGlyphWidths(PdfFontMetrics* metrics, const std::set<pdf_utf16be>& setUsed, const UnicodeToIndex& unicodeToIndex)
{
    GlyphWidths glyphWidths;

    // Load the width of all requested glyph indeces
    const long cAbsoluteMax = 0xffff;
    long nMin       = cAbsoluteMax;
    long nMax       = 0;

    long    lGlyph;
    double  dCurWidth = 1000;

    pdf_utf16be codePoint;
    UnicodeToIndex::const_iterator indexLookup;
    for (std::set<pdf_utf16be>::const_iterator it = setUsed.begin(); it != setUsed.end(); ++it)
    {
        codePoint = *it;
        indexLookup = unicodeToIndex.find( codePoint );
        if ( (indexLookup != unicodeToIndex.end()) && indexLookup->second ) {
            lGlyph = metrics->GetGlyphId( codePoint );
            /* XXX: If character code is not found in font, then do nothing */
            if( lGlyph )
            {
                nMin = PDF_MIN( nMin, lGlyph );
                nMax = PDF_MAX( nMax, lGlyph );
                nMax = PDF_MIN( nMax, cAbsoluteMax );

                if( lGlyph < cAbsoluteMax )
                {
                    dCurWidth = metrics->GetGlyphWidth( lGlyph );
                    glyphWidths[indexLookup->second] = dCurWidth;
                }
            }
        }
    }
    return glyphWidths;
}

static void
createWidths(PdfObject* pFontDict, PdfFontMetrics* metrics, const std::set<pdf_utf16be>& setUsed, const UnicodeToIndex& unicodeToIndex)
{
    PdfArray array;
    GlyphWidths glyphWidths = getGlyphWidths(metrics, setUsed, unicodeToIndex);
    if (!glyphWidths.empty()) {
        // Now compact the array
        array.reserve( glyphWidths.size() + 1 );

        GlyphWidths::const_iterator it = glyphWidths.begin();
        WidthExporter exporter(array, it);

        while(++it != glyphWidths.end()) {
            exporter.updateSBE(it);
        }
        exporter.finishSBE();
        if (!array.empty())
        {
#if USE_INDIRECT_WIDTHS         
            PdfObject* widthsObject = pFontDict->GetOwner()->CreateObject( array );
            if (widthsObject) {
                pFontDict->GetDictionary().AddKey( PdfName("Widths"), widthsObject->Reference() );
            }
#else
            pFontDict->GetDictionary().AddKey( PdfName("Widths"), array );
#endif          
        }
        pFontDict->GetDictionary().AddKey("FirstChar", PdfVariant( static_cast<pdf_int64>(glyphWidths.begin()->first) ) );
        pFontDict->GetDictionary().AddKey("LastChar",  PdfVariant( static_cast<pdf_int64>(glyphWidths.rbegin()->first) ) );
    }
}

static void
createWidths(PdfObject* pFontDict, PdfFontMetrics* metrics, const std::set<pdf_utf16be>& setUsed)
{
    PdfArray array;
    GlyphWidths glyphWidths = getGlyphWidths(metrics, setUsed);
    if (!glyphWidths.empty()) {
        // Now compact the array
        array.reserve( glyphWidths.size() + 1 );

        GlyphWidths::const_iterator it = glyphWidths.begin();
        WidthExporter exporter(array, it);

        while(++it != glyphWidths.end()) {
            exporter.update(it);
        }
        exporter.finish();
        if (!array.empty())
        {
#if USE_INDIRECT_WIDTHS         
            PdfObject* widthsObject = pFontDict->GetOwner()->CreateObject( array );
            if (widthsObject) {
                pFontDict->GetDictionary().AddKey( PdfName("W"), widthsObject->Reference() );
            }
#else
            pFontDict->GetDictionary().AddKey( PdfName("W"), array );
#endif          
        }
    }
}

};

