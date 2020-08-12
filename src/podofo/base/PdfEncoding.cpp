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

#include "PdfEncoding.h"

#include "PdfDictionary.h"
#include "PdfLocale.h"
#include "util/PdfMutexWrapper.h"
#include "PdfDefinesPrivate.h"
#include "base/PdfStream.h"
#include "base/PdfContentsTokenizer.h"

#include "doc/PdfFont.h"

#include <stack>
#include <stdlib.h>
#include <string.h>
#include <limits>
#include <sstream>
#include "PdfArray.h"
#include "doc/PdfDifferenceEncoding.h"

namespace PoDoFo {

PdfEncoding::PdfEncoding( int nFirstChar, int nLastChar, PdfObject* pToUnicode )
    : m_bToUnicodeIsLoaded(false), m_nFirstChar( nFirstChar ), m_nLastChar( nLastChar ), m_pToUnicode(pToUnicode)
{
    if( !(m_nFirstChar < m_nLastChar) )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_ValueOutOfRange, "PdfEncoding: nFirstChar must be smaller than nLastChar" ); 
    }
    
    ParseToUnicode();
}

PdfEncoding::~PdfEncoding()
{

}
    
PdfString PdfEncoding::ConvertToUnicode(const PdfString & rEncodedString, const PdfFont*) const
{
    
    if(!m_toUnicode.empty())
    {
        
        const pdf_utf16be* pStr = reinterpret_cast<const pdf_utf16be*>(rEncodedString.GetString());
        const size_t lLen = rEncodedString.GetLength()/2;
        pdf_utf16be lCID, lUnicodeValue;
        
        pdf_utf16be* pszUtf16 = static_cast<pdf_utf16be*>(podofo_calloc(lLen, sizeof(pdf_utf16be)));
        if( !pszUtf16 )
        {
            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
        }
        
        for(size_t i = 0 ; i<lLen ; i++)
        {
            
#ifdef PODOFO_IS_LITTLE_ENDIAN
            lCID = (pStr[i] << 8) | (pStr[i] >> 8 );
#else
            lCID = pStr[i];
#endif // PODOFO_IS_LITTLE_ENDIAN
            
            lUnicodeValue = this->GetUnicodeValue(lCID);
            
#ifdef PODOFO_IS_LITTLE_ENDIAN
            pszUtf16[i] = (lUnicodeValue << 8) | (lUnicodeValue >> 8 );
#else
            pszUtf16[i] = lUnicodeValue;
#endif // PODOFO_IS_LITTLE_ENDIAN
        }
        
        PdfString ret( pszUtf16, lLen );
        podofo_free( pszUtf16 );
        
        return ret;
        
    }
    else
        return(PdfString("\0"));
}

PdfRefCountedBuffer PdfEncoding::ConvertToEncoding( const PdfString & rString, const PdfFont* pFont ) const
{
    if(!m_toUnicode.empty())
    {
        // Get the string in UTF-16be format
        PdfString sStr = rString.ToUnicode();
        const pdf_utf16be* pStr = sStr.GetUnicode();
        pdf_utf16be lUnicodeValue, lCID;
        
        std::ostringstream out;
        PdfLocaleImbue(out);
        
        while( *pStr )
        {
            
#ifdef PODOFO_IS_LITTLE_ENDIAN
            lUnicodeValue = (*pStr << 8) | (*pStr >> 8);
#else
            lUnicodeValue = *pStr;
#endif // PODOFO_IS_LITTLE_ENDIAN
            
            lCID = this->GetCIDValue(lUnicodeValue);
            if (lCID == 0 && pFont) {
#ifdef PODOFO_IS_LITTLE_ENDIAN
                lCID = static_cast<pdf_utf16be>(pFont->GetFontMetrics()->GetGlyphId( (((*pStr & 0xff) << 8) | ((*pStr & 0xff00) >> 8)) ));
#else
                lCID = static_cast<pdf_utf16be>(pFont->GetFontMetrics()->GetGlyphId( *pStr ));
#endif // PODOFO_IS_LITTLE_ENDIAN
            }
            
            out << static_cast<unsigned char>((lCID & 0xff00) >> 8);
            out << static_cast<unsigned char>(lCID & 0x00ff);
            
            ++pStr;
        }
        
        PdfRefCountedBuffer buffer( out.str().length() );
        memcpy( buffer.GetBuffer(), out.str().c_str(), out.str().length() );
        return buffer;
    }
    else
        return PdfRefCountedBuffer();
}

void PdfEncoding::ParseToUnicode()
{
    if (m_pToUnicode && m_pToUnicode->HasStream())
    {
        std::stack<std::string> stkToken;
        pdf_uint16 loop = 0;
        char *streamBuffer;
        const char *streamToken = NULL;
        EPdfTokenType *streamTokenType = NULL;
        pdf_long streamBufferLen;
        bool in_beginbfrange = 0;
        bool in_beginbfchar = 0;
        pdf_uint16 range_entries = 0;
        pdf_uint16 char_entries = 0;
        pdf_uint16 inside_hex_string = 0;
        pdf_uint16 inside_array = 0;
        pdf_uint16 range_start = 0;
        pdf_uint16 range_end = 0;
        pdf_uint16 i = 0;
        pdf_utf16be firstvalue = 0;
        const PdfStream *CIDStreamdata = m_pToUnicode->GetStream ();
        CIDStreamdata->GetFilteredCopy (&streamBuffer, &streamBufferLen);
        
        PdfContentsTokenizer streamTokenizer (streamBuffer, streamBufferLen);
        while (streamTokenizer.GetNextToken (streamToken, streamTokenType))
        {
            stkToken.push (streamToken);
            
            if (strcmp (streamToken, ">") == 0)
            {
                if (inside_hex_string == 0)
                    PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidStream, "CMap Error, got > before <")
                    else
                        inside_hex_string = 0;
                
                if (inside_array == 0)
                {
                    i++;
                }
            }
            
            if (strcmp (streamToken, "]") == 0)
            {
                if (inside_array == 0)
                    PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidStream, "CMap Error, got ] before [")
                else
                    inside_array = 0;
                
                i++;
                loop++;
            }
            
            if (in_beginbfrange == 1)
            {
                if (loop < range_entries)
                {
                    if (inside_hex_string == 1)
                    {
                        pdf_utf16be num_value;
                        std::stringstream ss;
                        ss << std::hex << streamToken;
                        ss >> num_value;
                        if (i % 3 == 0)
                            range_start = num_value;
                        if (i % 3 == 1)
                        {
                            range_end = num_value;
                        }
                        if (i % 3 == 2)
                        {
                            if (inside_array == 0)
                            {
                                for (int k = range_start; k <= range_end; k++)
                                {
                                    m_toUnicode[k] = num_value;
                                    num_value++;
                                }
							
                                loop++;
                            }
                            else
                            {
                                m_toUnicode[range_start] = num_value;
                            }

                            range_start++;
                        }
                    }
                }
            }
            
            if (in_beginbfchar == 1)
            {
                if (loop < char_entries)
                {
                    if (inside_hex_string == 1)
                    {
                        pdf_utf16be num_value;
                        std::stringstream ss;
                        ss << std::hex << streamToken;
                        ss >> num_value;
                        if (i % 2 == 0)
                        {
                            firstvalue = num_value;
                        }
                        if (i % 2 == 1)
                        {
                            m_toUnicode[firstvalue] = num_value;
                        }
                    }
                }
            }
            
            
            if (strcmp (streamToken, "<") == 0)
            {
                inside_hex_string = 1;
            }
            
            
            
            if (strcmp (streamToken, "[") == 0)
            {
                inside_array = 1;
            }
            
            
            if (strcmp (streamToken, "beginbfrange") == 0)
            {
                // need 2 entries - one to pop() and one for top()
                if ( stkToken.size() < 2 )
                {
                    PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidStream, "CMap missing object number before beginbfrange");
                }
                
                i = loop = 0;
                in_beginbfrange = 1;
                stkToken.pop ();
                std::stringstream ss;
                ss << std::hex << stkToken.top ();
                ss >> range_entries;
            }
            
            if (strcmp (streamToken, "endbfrange") == 0)
            {
                in_beginbfrange = 0;
                i = 0;
            }
            
            if (strcmp (streamToken, "beginbfchar") == 0)
            {
                // need 2 entries - one to pop() and one for top()
                if ( stkToken.size() < 2 )
                {
                    PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidStream, "CMap missing object number before beginbfchar");
                }
                
                i = loop = 0;
                in_beginbfchar = 1;
                stkToken.pop ();
                std::stringstream ss;
                ss << std::hex << stkToken.top ();
                ss >> char_entries;
            }
            
            if (strcmp (streamToken, "endbfchar") == 0)
            {
                in_beginbfchar = 0;
                i = 0;
            }
        }
        
        podofo_free(streamBuffer);
        
        m_bToUnicodeIsLoaded = true;
    }
}

pdf_utf16be PdfEncoding::GetUnicodeValue( pdf_utf16be  value ) const
{
    if(!m_toUnicode.empty())
    {
        const std::map<pdf_utf16be, pdf_utf16be>::const_iterator found = m_toUnicode.find(value);
        return (found == m_toUnicode.end() ? 0 : found->second);
    }
    
    return 0;
}

pdf_utf16be PdfEncoding::GetCIDValue( pdf_utf16be lUnicodeValue ) const
{
    if(!m_toUnicode.empty())
    {
        // TODO: optimize
        for(std::map<pdf_utf16be, pdf_utf16be>::const_iterator it = m_toUnicode.begin(); it != m_toUnicode.end(); ++it)
            if(it->second == lUnicodeValue)
                return it->first;
    }
    
    return 0;
}
    
// -----------------------------------------------------
// PdfSimpleEncoding
// -----------------------------------------------------
PdfSimpleEncoding::PdfSimpleEncoding( const PdfName & rName )
    : PdfEncoding( 0, 255 ), m_mutex( new PoDoFo::Util::PdfMutex() ), m_name( rName ), m_pEncodingTable( NULL )
{
}

PdfSimpleEncoding::~PdfSimpleEncoding() 
{
    podofo_free( m_pEncodingTable );
    delete m_mutex;
}

void PdfSimpleEncoding::InitEncodingTable() 
{
    Util::PdfMutexWrapper wrapper( *m_mutex );
	// CVE-2017-7379 - previously lTableLength was 0xffff, but pdf_utf16be characters can be in range 0..0xffff so this
	// caused out-by-one heap overflow when character 0xffff was encoded
    const long         lTableLength     = std::numeric_limits<pdf_utf16be>::max() + 1;
    const pdf_utf16be* cpUnicodeTable   = this->GetToUnicodeTable();

    if( !m_pEncodingTable ) // double check
    {
        m_pEncodingTable = static_cast<char*>(podofo_calloc(lTableLength, sizeof(char)));
		if (!m_pEncodingTable)
		{
			PODOFO_RAISE_ERROR(ePdfError_OutOfMemory);
		}

        // fill the table with data
        for( size_t i=0; i<256; i++ )
        {
            m_pEncodingTable[ static_cast<size_t>(cpUnicodeTable[i]) ] = 
                static_cast<unsigned char>(i);
        }
    }
}

void PdfSimpleEncoding::AddToDictionary( PdfDictionary & rDictionary ) const
{
    rDictionary.AddKey( PdfName("Encoding"), m_name );
}

pdf_utf16be PdfSimpleEncoding::GetCharCode( int nIndex ) const
{
    if( nIndex < this->GetFirstChar() ||
        nIndex > this->GetLastChar() )
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    const pdf_utf16be* cpUnicodeTable   = this->GetToUnicodeTable();

#ifdef PODOFO_IS_LITTLE_ENDIAN
    return ((cpUnicodeTable[nIndex] & 0xff00) >> 8) | ((cpUnicodeTable[nIndex] & 0xff) << 8);
#else
    return cpUnicodeTable[nIndex];
#endif // PODOFO_IS_LITTLE_ENDIAN

}

PdfString PdfSimpleEncoding::ConvertToUnicode( const PdfString & rEncodedString, const PdfFont* pFont) const
{
    if(m_bToUnicodeIsLoaded)
    {
        return PdfEncoding::ConvertToUnicode(rEncodedString, pFont);
    }
    else
    {
        const pdf_utf16be* cpUnicodeTable = this->GetToUnicodeTable();
        pdf_long           lLen           = rEncodedString.GetLength();
        
        if( lLen  <= 0 )
            return PdfString(L"");
        
        pdf_utf16be* pszStringUtf16 = static_cast<pdf_utf16be*>(podofo_calloc( (lLen + 1), sizeof(pdf_utf16be)));
        if( !pszStringUtf16 )
        {
            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
        }
        
        const char* pszString = rEncodedString.GetString();
        for( int i=0;i<lLen;i++ )
        {
#ifdef PODOFO_IS_BIG_ENDIAN
            pszStringUtf16[i] = cpUnicodeTable[ static_cast<unsigned char>(*pszString) ];
#else
            pszStringUtf16[i] =
            ((( cpUnicodeTable[ static_cast<unsigned char>(*pszString) ] << 8 ) & 0xff00) |
             (( cpUnicodeTable[ static_cast<unsigned char>(*pszString) ] >> 8 ) & 0x00ff));
#endif // PODOFO_IS_BIG_ENDIAN
            ++pszString;
        }
        
        pszStringUtf16[lLen] = 0;
        
        PdfString sStr( pszStringUtf16 );
        podofo_free( pszStringUtf16 );
        
        return sStr;
    }
}

PdfRefCountedBuffer PdfSimpleEncoding::ConvertToEncoding( const PdfString & rString, const PdfFont* pFont) const
{
    if(m_bToUnicodeIsLoaded)
    {
        return PdfEncoding::ConvertToEncoding(rString, pFont);
    }
    else
    {
        if( !m_pEncodingTable )
            const_cast<PdfSimpleEncoding*>(this)->InitEncodingTable();
        
        PdfString sSrc = rString.ToUnicode(); // make sure the string is unicode and not PdfDocEncoding!
        pdf_long  lLen = sSrc.GetCharacterLength();
        
        if( !lLen )
            return PdfRefCountedBuffer();
        
        char* pDest = static_cast<char*>(podofo_calloc( (lLen + 1), sizeof(char) ));
        if( !pDest )
        {
            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
        }
        
        const pdf_utf16be* pszUtf16 = sSrc.GetUnicode();
        char*              pCur     = pDest;
        long               lNewLen  = 0L;
        
        for( int i=0;i<lLen;i++ )
        {
            pdf_utf16be val = pszUtf16[i];
#ifdef PODOFO_IS_LITTLE_ENDIAN
            val = ((val & 0xff00) >> 8) | ((val & 0xff) << 8);
#endif // PODOFO_IS_LITTLE_ENDIAN
            
            *pCur = m_pEncodingTable[val];
            if( *pCur ) // ignore 0 characters, as they cannot be converted to the current encoding
            {
                ++pCur;
                ++lNewLen;
            }
        }
        
        *pCur = '\0';
        
        
        PdfRefCountedBuffer cDest( lNewLen );
        memcpy( cDest.GetBuffer(), pDest, lNewLen );
        podofo_free( pDest );
        
        return cDest;
    }
}

char PdfSimpleEncoding::GetUnicodeCharCode(pdf_utf16be unicodeValue) const
{
    if( !m_pEncodingTable )
        const_cast<PdfSimpleEncoding*>(this)->InitEncodingTable();

#ifdef PODOFO_IS_LITTLE_ENDIAN
    unicodeValue = ((unicodeValue & 0xff00) >> 8) | ((unicodeValue & 0xff) << 8);
#endif // PODOFO_IS_LITTLE_ENDIAN

    return m_pEncodingTable[unicodeValue];
}

// -----------------------------------------------------
// PdfDocEncoding
// -----------------------------------------------------

// -----------------------------------------------------
// 
// -----------------------------------------------------
const pdf_utf16be* PdfDocEncoding::GetToUnicodeTable() const
{
    return PdfDocEncoding::s_cEncoding;
}

const pdf_utf16be PdfDocEncoding::s_cEncoding[256] = {
    0x0000,
    0x0001,
    0x0002,
    0x0003,
    0x0004,
    0x0005,
    0x0006,
    0x0007,
    0x0008,
    0x0009,
    0x000A,
    0x000B,
    0x000C,
    0x000D,
    0x000E,
    0x000F,
    0x0010,
    0x0011,
    0x0012,
    0x0013,
    0x0014,
    0x0015,
    0x0017,
    0x0017,
    0x02D8,
    0x02C7, // dec 25
    0x02C6,
    0x02D9,
    0x02DD,
    0x02DB,
    0x02DA,
    0x02DC,
    0x0020,
    0x0021,
    0x0022,
    0x0023,
    0x0024,
    0x0025,
    0x0026,
    0x0027,
    0x0028,
    0x0029,
    0x002A,
    0x002B,
    0x002C,
    0x002D,
    0x002E,
    0x002F,
    0x0030,
    0x0031,
    0x0032,
    0x0033,
    0x0034,
    0x0035,
    0x0036,
    0x0037,
    0x0038,
    0x0039, // dec 57 
    0x003A,
    0x003B,
    0x003C,
    0x003D,
    0x003E,
    0x003F,
    0x0040,
    0x0041,
    0x0042,
    0x0043,
    0x0044,
    0x0045,
    0x0046,
    0x0047,
    0x0048,
    0x0049,
    0x004A,
    0x004B,
    0x004C,
    0x004D,
    0x004E,
    0x004F,
    0x0050,
    0x0051,
    0x0052,
    0x0053,
    0x0054,
    0x0055,
    0x0056,
    0x0057,
    0x0058,
    0x0059, // 89
    0x005A,
    0x005B,
    0x005C,
    0x005D,
    0x005E,
    0x005F,
    0x0060,
    0x0061,
    0x0062,
    0x0063,
    0x0064,
    0x0065,
    0x0066,
    0x0067,
    0x0068,
    0x0069,
    0x006A,
    0x006B,
    0x006C,
    0x006D,
    0x006E,
    0x006F,
    0x0070,
    0x0071,
    0x0072,
    0x0073,
    0x0074,
    0x0075,
    0x0076,
    0x0077,
    0x0078,
    0x0079, //121 
    0x007A,
    0x007B,
    0x007C,
    0x007D,
    0x007E,
    0x0000, // Undefined
    0x2022,
    0x2020,
    0x2021,
    0x2026,
    0x2014,
    0x2013,
    0x0192,
    0x2044,
    0x2039,
    0x203A,
    0x2212,
    0x2030,
    0x201E,
    0x201C,
    0x201D,
    0x2018,
    0x2019,
    0x201A,
    0x2122,
    0xFB01, // dec147 
    0xFB02,
    0x0141,
    0x0152,
    0x0160,
    0x0178,
    0x017D,
    0x0131,
    0x0142,
    0x0153,
    0x0161,
    0x017E,
    0x0000, // Undefined
    0x20AC, // Euro
    0x00A1,
    0x00A2,
    0x00A3,
    0x00A4,
    0x00A5,
    0x00A6,
    0x00A7,
    0x00A8,
    0x00A9,
    0x00AA,
    0x00AB,
    0x00AC,
    0x0000, // Undefined
    0x00AE,
    0x00AF,
    0x00B0,
    0x00B1,
    0x00B2,
    0x00B3,
    0x00B4,
    0x00B5,
    0x00B6,
    0x00B7,
    0x00B8,
    0x00B9,
    0x00BA,
    0x00BB,
    0x00BC,
    0x00BD,
    0x00BE,
    0x00BF,
    0x00C0,
    0x00C1,
    0x00C2,
    0x00C3,
    0x00C4,
    0x00C5,
    0x00C6,
    0x00C7,
    0x00C8,
    0x00C9,
    0x00CA,
    0x00CB,
    0x00CC,
    0x00CD,
    0x00CE,
    0x00CF,
    0x00D0,
    0x00D1,
    0x00D2,
    0x00D3,
    0x00D4,
    0x00D5,
    0x00D6,
    0x00D7,
    0x00D8,
    0x00D9,
    0x00DA,
    0x00DB,
    0x00DC,
    0x00DD,
    0x00DE,
    0x00DF,
    0x00E0,
    0x00E1,
    0x00E2,
    0x00E3,
    0x00E4,
    0x00E5,
    0x00E6,
    0x00E7,
    0x00E8,
    0x00E9,
    0x00EA,
    0x00EB,
    0x00EC,
    0x00ED,
    0x00EE,
    0x00EF,
    0x00F0,
    0x00F1,
    0x00F2,
    0x00F3,
    0x00F4,
    0x00F5,
    0x00F6,
    0x00F7,
    0x00F8,
    0x00F9,
    0x00FA,
    0x00FB,
    0x00FC,
    0x00FD,
    0x00FE,
    0x00FF
};

// -----------------------------------------------------
// PdfWinAnsiEncoding
// See: http://www.microsoft.com/globaldev/reference/sbcs/1252.mspx
// -----------------------------------------------------

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfWinAnsiEncoding::AddToDictionary( PdfDictionary & rDictionary ) const
{
    PdfArray arDifferences;

    for (int i = 0; i < 256; i++)
    {
        if (PdfWinAnsiEncoding::GetToUnicodeTable()[i] != this->GetToUnicodeTable()[i])
        {
            arDifferences.push_back(PdfObject((pdf_int64)i));
            unsigned short shCode = this->GetToUnicodeTable()[i];
#ifdef PODOFO_IS_LITTLE_ENDIAN
            shCode = ((shCode & 0x00FF) << 8) | ((shCode & 0xFF00) >> 8);
#endif
            arDifferences.push_back( PdfDifferenceEncoding::UnicodeIDToName(shCode) );
        }
    }

    if (!arDifferences.empty())
    {
        PdfDictionary dictEncoding;
        dictEncoding.AddKey(PdfName("BaseEncoding"), PdfWinAnsiEncoding::GetName());
        dictEncoding.AddKey(PdfName("Differences"), arDifferences);
        rDictionary.AddKey(PdfName("Encoding"), dictEncoding);
    }
    else
    {
        PdfSimpleEncoding::AddToDictionary(rDictionary);
    }
}

const pdf_utf16be* PdfWinAnsiEncoding::GetToUnicodeTable() const
{
    return PdfWinAnsiEncoding::s_cEncoding;
}

const pdf_utf16be PdfWinAnsiEncoding::s_cEncoding[256] = {
    0x0000, // NULL
    0x0001, // START OF HEADING
    0x0002, // START OF TEXT
    0x0003, // END OF TEXT
    0x0004, // END OF TRANSMISSION
    0x0005, // ENQUIRY
    0x0006, // ACKNOWLEDGE
    0x0007, // BELL
    0x0008, // BACKSPACE
    0x0009, // HORIZONTAL TABULATION
    0x000A, // LINE FEED
    0x000B, // VERTICAL TABULATION
    0x000C, // FORM FEED
    0x000D, // CARRIAGE RETURN
    0x000E, // SHIFT OUT
    0x000F, // SHIFT IN
    0x0010, // DATA LINK ESCAPE
    0x0011, // DEVICE CONTROL ONE
    0x0012, // DEVICE CONTROL TWO
    0x0013, // DEVICE CONTROL THREE
    0x0014, // DEVICE CONTROL FOUR
    0x0015, // NEGATIVE ACKNOWLEDGE
    0x0016, // SYNCHRONOUS IDLE
    0x0017, // END OF TRANSMISSION BLOCK
    0x0018, // CANCEL
    0x0019, // END OF MEDIUM
    0x001A, // SUBSTITUTE
    0x001B, // ESCAPE
    0x001C, // FILE SEPARATOR
    0x001D, // GROUP SEPARATOR
    0x001E, // RECORD SEPARATOR
    0x001F, // UNIT SEPARATOR
    0x0020, // SPACE
    0x0021, // EXCLAMATION MARK
    0x0022, // QUOTATION MARK
    0x0023, // NUMBER SIGN
    0x0024, // DOLLAR SIGN
    0x0025, // PERCENT SIGN
    0x0026, // AMPERSAND
    0x0027, // APOSTROPHE
    0x0028, // LEFT PARENTHESIS
    0x0029, // RIGHT PARENTHESIS
    0x002A, // ASTERISK
    0x002B, // PLUS SIGN
    0x002C, // COMMA
    0x002D, // HYPHEN-MINUS
    0x002E, // FULL STOP
    0x002F, // SOLIDUS
    0x0030, // DIGIT ZERO
    0x0031, // DIGIT ONE
    0x0032, // DIGIT TWO
    0x0033, // DIGIT THREE
    0x0034, // DIGIT FOUR
    0x0035, // DIGIT FIVE
    0x0036, // DIGIT SIX
    0x0037, // DIGIT SEVEN
    0x0038, // DIGIT EIGHT
    0x0039, // DIGIT NINE
    0x003A, // COLON
    0x003B, // SEMICOLON
    0x003C, // LESS-THAN SIGN
    0x003D, // EQUALS SIGN
    0x003E, // GREATER-THAN SIGN
    0x003F, // QUESTION MARK
    0x0040, // COMMERCIAL AT
    0x0041, // LATIN CAPITAL LETTER A
    0x0042, // LATIN CAPITAL LETTER B
    0x0043, // LATIN CAPITAL LETTER C
    0x0044, // LATIN CAPITAL LETTER D
    0x0045, // LATIN CAPITAL LETTER E
    0x0046, // LATIN CAPITAL LETTER F
    0x0047, // LATIN CAPITAL LETTER G
    0x0048, // LATIN CAPITAL LETTER H
    0x0049, // LATIN CAPITAL LETTER I
    0x004A, // LATIN CAPITAL LETTER J
    0x004B, // LATIN CAPITAL LETTER K
    0x004C, // LATIN CAPITAL LETTER L
    0x004D, // LATIN CAPITAL LETTER M
    0x004E, // LATIN CAPITAL LETTER N
    0x004F, // LATIN CAPITAL LETTER O
    0x0050, // LATIN CAPITAL LETTER P
    0x0051, // LATIN CAPITAL LETTER Q
    0x0052, // LATIN CAPITAL LETTER R
    0x0053, // LATIN CAPITAL LETTER S
    0x0054, // LATIN CAPITAL LETTER T
    0x0055, // LATIN CAPITAL LETTER U
    0x0056, // LATIN CAPITAL LETTER V
    0x0057, // LATIN CAPITAL LETTER W
    0x0058, // LATIN CAPITAL LETTER X
    0x0059, // LATIN CAPITAL LETTER Y
    0x005A, // LATIN CAPITAL LETTER Z
    0x005B, // LEFT SQUARE BRACKET
    0x005C, // REVERSE SOLIDUS
    0x005D, // RIGHT SQUARE BRACKET
    0x005E, // CIRCUMFLEX ACCENT
    0x005F, // LOW LINE
    0x0060, // GRAVE ACCENT
    0x0061, // LATIN SMALL LETTER A
    0x0062, // LATIN SMALL LETTER B
    0x0063, // LATIN SMALL LETTER C
    0x0064, // LATIN SMALL LETTER D
    0x0065, // LATIN SMALL LETTER E
    0x0066, // LATIN SMALL LETTER F
    0x0067, // LATIN SMALL LETTER G
    0x0068, // LATIN SMALL LETTER H
    0x0069, // LATIN SMALL LETTER I
    0x006A, // LATIN SMALL LETTER J
    0x006B, // LATIN SMALL LETTER K
    0x006C, // LATIN SMALL LETTER L
    0x006D, // LATIN SMALL LETTER M
    0x006E, // LATIN SMALL LETTER N
    0x006F, // LATIN SMALL LETTER O
    0x0070, // LATIN SMALL LETTER P
    0x0071, // LATIN SMALL LETTER Q
    0x0072, // LATIN SMALL LETTER R
    0x0073, // LATIN SMALL LETTER S
    0x0074, // LATIN SMALL LETTER T
    0x0075, // LATIN SMALL LETTER U
    0x0076, // LATIN SMALL LETTER V
    0x0077, // LATIN SMALL LETTER W
    0x0078, // LATIN SMALL LETTER X
    0x0079, // LATIN SMALL LETTER Y
    0x007A, // LATIN SMALL LETTER Z
    0x007B, // LEFT CURLY BRACKET
    0x007C, // VERTICAL LINE
    0x007D, // RIGHT CURLY BRACKET
    0x007E, // TILDE
    0x007F, // DELETE
    0x20AC, // EURO SIGN
    0x0000,
    0x201A, // SINGLE LOW-9 QUOTATION MARK
    0x0192, // LATIN SMALL LETTER F WITH HOOK
    0x201E, // DOUBLE LOW-9 QUOTATION MARK
    0x2026, // HORIZONTAL ELLIPSIS
    0x2020, // DAGGER
    0x2021, // DOUBLE DAGGER
    0x02C6, // MODIFIER LETTER CIRCUMFLEX ACCENT
    0x2030, // PER MILLE SIGN
    0x0160, // LATIN CAPITAL LETTER S WITH CARON
    0x2039, // SINGLE LEFT-POINTING ANGLE QUOTATION MARK
    0x0152, // LATIN CAPITAL LIGATURE OE
    0x0000,
    0x017D, // LATIN CAPITAL LETTER Z WITH CARON
    0x0000, 
    0x0000,
    0x2018, // LEFT SINGLE QUOTATION MARK
    0x2019, // RIGHT SINGLE QUOTATION MARK
    0x201C, // LEFT DOUBLE QUOTATION MARK
    0x201D, // RIGHT DOUBLE QUOTATION MARK
    0x2022, // BULLET
    0x2013, // EN DASH
    0x2014, // EM DASH
    0x02DC, // SMALL TILDE
    0x2122, // TRADE MARK SIGN
    0x0161, // LATIN SMALL LETTER S WITH CARON
    0x203A, // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
    0x0153, // LATIN SMALL LIGATURE OE
    0x0000,
    0x017E, // LATIN SMALL LETTER Z WITH CARON
    0x0178, // LATIN CAPITAL LETTER Y WITH DIAERESIS
    0x00A0, // NO-BREAK SPACE
    0x00A1, // INVERTED EXCLAMATION MARK
    0x00A2, // CENT SIGN
    0x00A3, // POUND SIGN
    0x00A4, // CURRENCY SIGN
    0x00A5, // YEN SIGN
    0x00A6, // BROKEN BAR
    0x00A7, // SECTION SIGN
    0x00A8, // DIAERESIS
    0x00A9, // COPYRIGHT SIGN
    0x00AA, // FEMININE ORDINAL INDICATOR
    0x00AB, // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00AC, // NOT SIGN
    0x00AD, // SOFT HYPHEN
    0x00AE, // REGISTERED SIGN
    0x00AF, // MACRON
    0x00B0, // DEGREE SIGN
    0x00B1, // PLUS-MINUS SIGN
    0x00B2, // SUPERSCRIPT TWO
    0x00B3, // SUPERSCRIPT THREE
    0x00B4, // ACUTE ACCENT
    0x00B5, // MICRO SIGN
    0x00B6, // PILCROW SIGN
    0x00B7, // MIDDLE DOT
    0x00B8, // CEDILLA
    0x00B9, // SUPERSCRIPT ONE
    0x00BA, // MASCULINE ORDINAL INDICATOR
    0x00BB, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00BC, // VULGAR FRACTION ONE QUARTER
    0x00BD, // VULGAR FRACTION ONE HALF
    0x00BE, // VULGAR FRACTION THREE QUARTERS
    0x00BF, // INVERTED QUESTION MARK
    0x00C0, // LATIN CAPITAL LETTER A WITH GRAVE
    0x00C1, // LATIN CAPITAL LETTER A WITH ACUTE
    0x00C2, // LATIN CAPITAL LETTER A WITH CIRCUMFLEX
    0x00C3, // LATIN CAPITAL LETTER A WITH TILDE
    0x00C4, // LATIN CAPITAL LETTER A WITH DIAERESIS
    0x00C5, // LATIN CAPITAL LETTER A WITH RING ABOVE
    0x00C6, // LATIN CAPITAL LETTER AE
    0x00C7, // LATIN CAPITAL LETTER C WITH CEDILLA
    0x00C8, // LATIN CAPITAL LETTER E WITH GRAVE
    0x00C9, // LATIN CAPITAL LETTER E WITH ACUTE
    0x00CA, // LATIN CAPITAL LETTER E WITH CIRCUMFLEX
    0x00CB, // LATIN CAPITAL LETTER E WITH DIAERESIS
    0x00CC, // LATIN CAPITAL LETTER I WITH GRAVE
    0x00CD, // LATIN CAPITAL LETTER I WITH ACUTE
    0x00CE, // LATIN CAPITAL LETTER I WITH CIRCUMFLEX
    0x00CF, // LATIN CAPITAL LETTER I WITH DIAERESIS
    0x00D0, // LATIN CAPITAL LETTER ETH
    0x00D1, // LATIN CAPITAL LETTER N WITH TILDE
    0x00D2, // LATIN CAPITAL LETTER O WITH GRAVE
    0x00D3, // LATIN CAPITAL LETTER O WITH ACUTE
    0x00D4, // LATIN CAPITAL LETTER O WITH CIRCUMFLEX
    0x00D5, // LATIN CAPITAL LETTER O WITH TILDE
    0x00D6, // LATIN CAPITAL LETTER O WITH DIAERESIS
    0x00D7, // MULTIPLICATION SIGN
    0x00D8, // LATIN CAPITAL LETTER O WITH STROKE
    0x00D9, // LATIN CAPITAL LETTER U WITH GRAVE
    0x00DA, // LATIN CAPITAL LETTER U WITH ACUTE
    0x00DB, // LATIN CAPITAL LETTER U WITH CIRCUMFLEX
    0x00DC, // LATIN CAPITAL LETTER U WITH DIAERESIS
    0x00DD, // LATIN CAPITAL LETTER Y WITH ACUTE
    0x00DE, // LATIN CAPITAL LETTER THORN
    0x00DF, // LATIN SMALL LETTER SHARP S
    0x00E0, // LATIN SMALL LETTER A WITH GRAVE
    0x00E1, // LATIN SMALL LETTER A WITH ACUTE
    0x00E2, // LATIN SMALL LETTER A WITH CIRCUMFLEX
    0x00E3, // LATIN SMALL LETTER A WITH TILDE
    0x00E4, // LATIN SMALL LETTER A WITH DIAERESIS
    0x00E5, // LATIN SMALL LETTER A WITH RING ABOVE
    0x00E6, // LATIN SMALL LETTER AE
    0x00E7, // LATIN SMALL LETTER C WITH CEDILLA
    0x00E8, // LATIN SMALL LETTER E WITH GRAVE
    0x00E9, // LATIN SMALL LETTER E WITH ACUTE
    0x00EA, // LATIN SMALL LETTER E WITH CIRCUMFLEX
    0x00EB, // LATIN SMALL LETTER E WITH DIAERESIS
    0x00EC, // LATIN SMALL LETTER I WITH GRAVE
    0x00ED, // LATIN SMALL LETTER I WITH ACUTE
    0x00EE, // LATIN SMALL LETTER I WITH CIRCUMFLEX
    0x00EF, // LATIN SMALL LETTER I WITH DIAERESIS
    0x00F0, // LATIN SMALL LETTER ETH
    0x00F1, // LATIN SMALL LETTER N WITH TILDE
    0x00F2, // LATIN SMALL LETTER O WITH GRAVE
    0x00F3, // LATIN SMALL LETTER O WITH ACUTE
    0x00F4, // LATIN SMALL LETTER O WITH CIRCUMFLEX
    0x00F5, // LATIN SMALL LETTER O WITH TILDE
    0x00F6, // LATIN SMALL LETTER O WITH DIAERESIS
    0x00F7, // DIVISION SIGN
    0x00F8, // LATIN SMALL LETTER O WITH STROKE
    0x00F9, // LATIN SMALL LETTER U WITH GRAVE
    0x00FA, // LATIN SMALL LETTER U WITH ACUTE
    0x00FB, // LATIN SMALL LETTER U WITH CIRCUMFLEX
    0x00FC, // LATIN SMALL LETTER U WITH DIAERESIS
    0x00FD, // LATIN SMALL LETTER Y WITH ACUTE
    0x00FE, // LATIN SMALL LETTER THORN
    0x00FF, // LATIN SMALL LETTER Y WITH DIAERESIS
};

// -----------------------------------------------------
// PdfMacRomanEncoding
// -----------------------------------------------------

// -----------------------------------------------------
// 
// -----------------------------------------------------
const pdf_utf16be* PdfMacRomanEncoding::GetToUnicodeTable() const
{
    return PdfMacRomanEncoding::s_cEncoding;
}

const pdf_utf16be PdfMacRomanEncoding::s_cEncoding[256] = {
    0x0000, // NULL
    0x0001, // START OF HEADING
    0x0002, // START OF TEXT
    0x0003, // END OF TEXT
    0x0004, // END OF TRANSMISSION
    0x0005, // ENQUIRY
    0x0006, // ACKNOWLEDGE
    0x0007, // BELL
    0x0008, // BACKSPACE
    0x0009, // HORIZONTAL TABULATION
    0x000A, // LINE FEED
    0x000B, // VERTICAL TABULATION
    0x000C, // FORM FEED
    0x000D, // CARRIAGE RETURN
    0x000E, // SHIFT OUT
    0x000F, // SHIFT IN
    0x0010, // DATA LINK ESCAPE
    0x0011, // DEVICE CONTROL ONE
    0x0012, // DEVICE CONTROL TWO
    0x0013, // DEVICE CONTROL THREE
    0x0014, // DEVICE CONTROL FOUR
    0x0015, // NEGATIVE ACKNOWLEDGE
    0x0016, // SYNCHRONOUS IDLE
    0x0017, // END OF TRANSMISSION BLOCK
    0x0018, // CANCEL
    0x0019, // END OF MEDIUM
    0x001A, // SUBSTITUTE
    0x001B, // ESCAPE
    0x001C, // FILE SEPARATOR
    0x001D, // GROUP SEPARATOR
    0x001E, // RECORD SEPARATOR
    0x001F, // UNIT SEPARATOR
    0x0020, // SPACE
    0x0021, // EXCLAMATION MARK
    0x0022, // QUOTATION MARK
    0x0023, // NUMBER SIGN
    0x0024, // DOLLAR SIGN
    0x0025, // PERCENT SIGN
    0x0026, // AMPERSAND
    0x0027, // APOSTROPHE
    0x0028, // LEFT PARENTHESIS
    0x0029, // RIGHT PARENTHESIS
    0x002A, // ASTERISK
    0x002B, // PLUS SIGN
    0x002C, // COMMA
    0x002D, // HYPHEN-MINUS
    0x002E, // FULL STOP
    0x002F, // SOLIDUS
    0x0030, // DIGIT ZERO
    0x0031, // DIGIT ONE
    0x0032, // DIGIT TWO
    0x0033, // DIGIT THREE
    0x0034, // DIGIT FOUR
    0x0035, // DIGIT FIVE
    0x0036, // DIGIT SIX
    0x0037, // DIGIT SEVEN
    0x0038, // DIGIT EIGHT
    0x0039, // DIGIT NINE
    0x003A, // COLON
    0x003B, // SEMICOLON
    0x003C, // LESS-THAN SIGN
    0x003D, // EQUALS SIGN
    0x003E, // GREATER-THAN SIGN
    0x003F, // QUESTION MARK
    0x0040, // COMMERCIAL AT
    0x0041, // LATIN CAPITAL LETTER A
    0x0042, // LATIN CAPITAL LETTER B
    0x0043, // LATIN CAPITAL LETTER C
    0x0044, // LATIN CAPITAL LETTER D
    0x0045, // LATIN CAPITAL LETTER E
    0x0046, // LATIN CAPITAL LETTER F
    0x0047, // LATIN CAPITAL LETTER G
    0x0048, // LATIN CAPITAL LETTER H
    0x0049, // LATIN CAPITAL LETTER I
    0x004A, // LATIN CAPITAL LETTER J
    0x004B, // LATIN CAPITAL LETTER K
    0x004C, // LATIN CAPITAL LETTER L
    0x004D, // LATIN CAPITAL LETTER M
    0x004E, // LATIN CAPITAL LETTER N
    0x004F, // LATIN CAPITAL LETTER O
    0x0050, // LATIN CAPITAL LETTER P
    0x0051, // LATIN CAPITAL LETTER Q
    0x0052, // LATIN CAPITAL LETTER R
    0x0053, // LATIN CAPITAL LETTER S
    0x0054, // LATIN CAPITAL LETTER T
    0x0055, // LATIN CAPITAL LETTER U
    0x0056, // LATIN CAPITAL LETTER V
    0x0057, // LATIN CAPITAL LETTER W
    0x0058, // LATIN CAPITAL LETTER X
    0x0059, // LATIN CAPITAL LETTER Y
    0x005A, // LATIN CAPITAL LETTER Z
    0x005B, // LEFT SQUARE BRACKET
    0x005C, // REVERSE SOLIDUS
    0x005D, // RIGHT SQUARE BRACKET
    0x005E, // CIRCUMFLEX ACCENT
    0x005F, // LOW LINE
    0x0060, // GRAVE ACCENT
    0x0061, // LATIN SMALL LETTER A
    0x0062, // LATIN SMALL LETTER B
    0x0063, // LATIN SMALL LETTER C
    0x0064, // LATIN SMALL LETTER D
    0x0065, // LATIN SMALL LETTER E
    0x0066, // LATIN SMALL LETTER F
    0x0067, // LATIN SMALL LETTER G
    0x0068, // LATIN SMALL LETTER H
    0x0069, // LATIN SMALL LETTER I
    0x006A, // LATIN SMALL LETTER J
    0x006B, // LATIN SMALL LETTER K
    0x006C, // LATIN SMALL LETTER L
    0x006D, // LATIN SMALL LETTER M
    0x006E, // LATIN SMALL LETTER N
    0x006F, // LATIN SMALL LETTER O
    0x0070, // LATIN SMALL LETTER P
    0x0071, // LATIN SMALL LETTER Q
    0x0072, // LATIN SMALL LETTER R
    0x0073, // LATIN SMALL LETTER S
    0x0074, // LATIN SMALL LETTER T
    0x0075, // LATIN SMALL LETTER U
    0x0076, // LATIN SMALL LETTER V
    0x0077, // LATIN SMALL LETTER W
    0x0078, // LATIN SMALL LETTER X
    0x0079, // LATIN SMALL LETTER Y
    0x007A, // LATIN SMALL LETTER Z
    0x007B, // LEFT CURLY BRACKET
    0x007C, // VERTICAL LINE
    0x007D, // RIGHT CURLY BRACKET
    0x007E, // TILDE
    0x007F, // DEL
    0x00C4, // LATIN CAPITAL LETTER A WITH DIAERESIS
    0x00C5, // LATIN CAPITAL LETTER A WITH RING ABOVE
    0x00C7, // LATIN CAPITAL LETTER C WITH CEDILLA
    0x00C9, // LATIN CAPITAL LETTER E WITH ACUTE
    0x00D1, // LATIN CAPITAL LETTER N WITH TILDE
    0x00D6, // LATIN CAPITAL LETTER O WITH DIAERESIS
    0x00DC, // LATIN CAPITAL LETTER U WITH DIAERESIS
    0x00E1, // LATIN SMALL LETTER A WITH ACUTE
    0x00E0, // LATIN SMALL LETTER A WITH GRAVE
    0x00E2, // LATIN SMALL LETTER A WITH CIRCUMFLEX
    0x00E4, // LATIN SMALL LETTER A WITH DIAERESIS
    0x00E3, // LATIN SMALL LETTER A WITH TILDE
    0x00E5, // LATIN SMALL LETTER A WITH RING ABOVE
    0x00E7, // LATIN SMALL LETTER C WITH CEDILLA
    0x00E9, // LATIN SMALL LETTER E WITH ACUTE
    0x00E8, // LATIN SMALL LETTER E WITH GRAVE
    0x00EA, // LATIN SMALL LETTER E WITH CIRCUMFLEX
    0x00EB, // LATIN SMALL LETTER E WITH DIAERESIS
    0x00ED, // LATIN SMALL LETTER I WITH ACUTE
    0x00EC, // LATIN SMALL LETTER I WITH GRAVE
    0x00EE, // LATIN SMALL LETTER I WITH CIRCUMFLEX
    0x00EF, // LATIN SMALL LETTER I WITH DIAERESIS
    0x00F1, // LATIN SMALL LETTER N WITH TILDE
    0x00F3, // LATIN SMALL LETTER O WITH ACUTE
    0x00F2, // LATIN SMALL LETTER O WITH GRAVE
    0x00F4, // LATIN SMALL LETTER O WITH CIRCUMFLEX
    0x00F6, // LATIN SMALL LETTER O WITH DIAERESIS
    0x00F5, // LATIN SMALL LETTER O WITH TILDE
    0x00FA, // LATIN SMALL LETTER U WITH ACUTE
    0x00F9, // LATIN SMALL LETTER U WITH GRAVE
    0x00FB, // LATIN SMALL LETTER U WITH CIRCUMFLEX
    0x00FC, // LATIN SMALL LETTER U WITH DIAERESIS
    0x2020, // DAGGER
    0x00B0, // DEGREE SIGN
    0x00A2, // CENT SIGN
    0x00A3, // POUND SIGN
    0x00A7, // SECTION SIGN
    0x2022, // BULLET
    0x00B6, // PILCROW SIGN
    0x00DF, // LATIN SMALL LETTER SHARP S
    0x00AE, // REGISTERED SIGN
    0x00A9, // COPYRIGHT SIGN
    0x2122, // TRADE MARK SIGN
    0x00B4, // ACUTE ACCENT
    0x00A8, // DIAERESIS
    0x2260, // NOT EQUAL TO
    0x00C6, // LATIN CAPITAL LETTER AE
    0x00D8, // LATIN CAPITAL LETTER O WITH STROKE
    0x221E, // INFINITY
    0x00B1, // PLUS-MINUS SIGN
    0x2264, // LESS-THAN OR EQUAL TO
    0x2265, // GREATER-THAN OR EQUAL TO
    0x00A5, // YEN SIGN
    0x00B5, // MICRO SIGN
    0x2202, // PARTIAL DIFFERENTIAL
    0x2211, // N-ARY SUMMATION
    0x220F, // N-ARY PRODUCT
    0x03C0, // GREEK SMALL LETTER PI
    0x222B, // INTEGRAL
    0x00AA, // FEMININE ORDINAL INDICATOR
    0x00BA, // MASCULINE ORDINAL INDICATOR
    0x03A9, // GREEK CAPITAL LETTER OMEGA
    0x00E6, // LATIN SMALL LETTER AE
    0x00F8, // LATIN SMALL LETTER O WITH STROKE
    0x00BF, // INVERTED QUESTION MARK
    0x00A1, // INVERTED EXCLAMATION MARK
    0x00AC, // NOT SIGN
    0x221A, // SQUARE ROOT
    0x0192, // LATIN SMALL LETTER F WITH HOOK
    0x2248, // ALMOST EQUAL TO
    0x2206, // INCREMENT
    0x00AB, // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00BB, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x2026, // HORIZONTAL ELLIPSIS
    0x00A0, // NO-BREAK SPACE
    0x00C0, // LATIN CAPITAL LETTER A WITH GRAVE
    0x00C3, // LATIN CAPITAL LETTER A WITH TILDE
    0x00D5, // LATIN CAPITAL LETTER O WITH TILDE
    0x0152, // LATIN CAPITAL LIGATURE OE
    0x0153, // LATIN SMALL LIGATURE OE
    0x2013, // EN DASH
    0x2014, // EM DASH
    0x201C, // LEFT DOUBLE QUOTATION MARK
    0x201D, // RIGHT DOUBLE QUOTATION MARK
    0x2018, // LEFT SINGLE QUOTATION MARK
    0x2019, // RIGHT SINGLE QUOTATION MARK
    0x00F7, // DIVISION SIGN
    0x25CA, // LOZENGE
    0x00FF, // LATIN SMALL LETTER Y WITH DIAERESIS
    0x0178, // LATIN CAPITAL LETTER Y WITH DIAERESIS
    0x2044, // FRACTION SLASH
    0x20AC, // EURO SIGN
    0x2039, // SINGLE LEFT-POINTING ANGLE QUOTATION MARK
    0x203A, // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
    0xFB01, // LATIN SMALL LIGATURE FI
    0xFB02, // LATIN SMALL LIGATURE FL
    0x2021, // DOUBLE DAGGER
    0x00B7, // MIDDLE DOT
    0x201A, // SINGLE LOW-9 QUOTATION MARK
    0x201E, // DOUBLE LOW-9 QUOTATION MARK
    0x2030, // PER MILLE SIGN
    0x00C2, // LATIN CAPITAL LETTER A WITH CIRCUMFLEX
    0x00CA, // LATIN CAPITAL LETTER E WITH CIRCUMFLEX
    0x00C1, // LATIN CAPITAL LETTER A WITH ACUTE
    0x00CB, // LATIN CAPITAL LETTER E WITH DIAERESIS
    0x00C8, // LATIN CAPITAL LETTER E WITH GRAVE
    0x00CD, // LATIN CAPITAL LETTER I WITH ACUTE
    0x00CE, // LATIN CAPITAL LETTER I WITH CIRCUMFLEX
    0x00CF, // LATIN CAPITAL LETTER I WITH DIAERESIS
    0x00CC, // LATIN CAPITAL LETTER I WITH GRAVE
    0x00D3, // LATIN CAPITAL LETTER O WITH ACUTE
    0x00D4, // LATIN CAPITAL LETTER O WITH CIRCUMFLEX
    0xF8FF, // Apple logo
    0x00D2, // LATIN CAPITAL LETTER O WITH GRAVE
    0x00DA, // LATIN CAPITAL LETTER U WITH ACUTE
    0x00DB, // LATIN CAPITAL LETTER U WITH CIRCUMFLEX
    0x00D9, // LATIN CAPITAL LETTER U WITH GRAVE
    0x0131, // LATIN SMALL LETTER DOTLESS I
    0x02C6, // MODIFIER LETTER CIRCUMFLEX ACCENT
    0x02DC, // SMALL TILDE
    0x00AF, // MACRON
    0x02D8, // BREVE
    0x02D9, // DOT ABOVE
    0x02DA, // RING ABOVE
    0x00B8, // CEDILLA
    0x02DD, // DOUBLE ACUTE ACCENT
    0x02DB, // OGONEK
    0x02C7, // CARON
};

// OC 13.08.2010 New: PdfMacExpertEncoding
// -----------------------------------------------------
// PdfMacExpertEncoding
// See: ghostscript-8.71/Resource/Init/gs_mex_e.ps
//      --> array of 256 glyphs for MacExpertEncoding
// See: http://www.adobe.com/devnet/opentype/archives/glyphlist.txt
//      --> glyphs to unicodes
// -----------------------------------------------------

// -----------------------------------------------------
// 
// -----------------------------------------------------
const pdf_utf16be* PdfMacExpertEncoding::GetToUnicodeTable() const
{
    return PdfMacExpertEncoding::s_cEncoding;
}

const pdf_utf16be PdfMacExpertEncoding::s_cEncoding[256] = {
// \00x
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
// \04x
    0x0020, 0xF721, 0xF6F8, 0xF7A2, 0xF724, 0xF6E4, 0xF726, 0xF7B4,
    0x207D, 0x207E, 0x2025, 0x2024, 0x002C, 0x002D, 0x002E, 0x2044,
    0xF730, 0xF731, 0xF732, 0xF733, 0xF734, 0xF735, 0xF736, 0xF737,
    0xF738, 0xF739, 0x003A, 0x003B, 0x0000, 0xF6DE, 0x0000, 0xF73F,
// \10x
    0x0000, 0x0000, 0x0000, 0x0000, 0xF7F0, 0x0000, 0x0000, 0x00BC,
    0x00BD, 0x00BE, 0x215B, 0x215C, 0x215D, 0x215E, 0x2153, 0x2154,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFB00, 0xFB01,
    0xFB02, 0xFB03, 0xFB04, 0x208D, 0x0000, 0x208E, 0xF6F6, 0xF6E5,
// \14x
    0xF760, 0xF761, 0xF762, 0xF763, 0xF764, 0xF765, 0xF766, 0xF767,
    0xF768, 0xF769, 0xF76A, 0xF76B, 0xF76C, 0xF76D, 0xF76E, 0xF76F,
    0xF770, 0xF771, 0xF772, 0xF773, 0xF774, 0xF775, 0xF776, 0xF777,
    0xF778, 0xF779, 0xF77A, 0x20A1, 0xF6DC, 0xF6DD, 0xF6FE, 0x0000,
// \20x
    0x0000, 0xF6E9, 0xF6E0, 0x0000, 0x0000, 0x0000, 0x0000, 0xF7E1,
    0xF7E0, 0xF7E2, 0xF7E4, 0xF7E3, 0xF7E5, 0xF7E7, 0xF7E9, 0xF7E8,
    0xF7EA, 0xF7EB, 0xF7ED, 0xF7EC, 0xF7EE, 0xF7EF, 0xF7F1, 0xF7F3,
    0xF7F2, 0xF7F4, 0xF7F6, 0xF7F5, 0xF7FA, 0xF7F9, 0xF7FB, 0xF7FC,
// \24x
    0x0000, 0x2078, 0x2084, 0x2083, 0x2086, 0x2088, 0x2087, 0xF6FD,
    0x0000, 0xF6DF, 0x2082, 0x0000, 0xF7A8, 0x0000, 0xF6F5, 0xF6F0,
    0x2085, 0x0000, 0xF6E1, 0xF6E7, 0xF7FD, 0x0000, 0xF6E3, 0x0000,
    0x0000, 0xF7FE, 0x0000, 0x2089, 0x2080, 0xF6FF, 0xF7E6, 0xF7F8,
// \30x
    0xF7BF, 0x2081, 0xF6F9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0xF7B8, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xF6FA,
    0x2012, 0xF6E6, 0x0000, 0x0000, 0x0000, 0x0000, 0xF7A1, 0x0000,
    0xF7FF, 0x0000, 0x00B9, 0x00B2, 0x00B3, 0x2074, 0x2075, 0x2076,
// \34x
    0x2077, 0x2079, 0x2070, 0x0000, 0xF6EC, 0xF6F1, 0xF6F3, 0x0000,
    0x0000, 0xF6ED, 0xF6F2, 0xF6EB, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0xF6EE, 0xF6FB, 0xF6F4, 0xF7AF, 0xF6EA, 0x207F, 0xF6EF,
    0xF6E2, 0xF6E8, 0xF6F7, 0xF6FC, 0x0000, 0x0000, 0x0000, 0x0000
};

// OC 13.08.2010 New: PdfStandardEncoding
// -----------------------------------------------------
// PdfStandardEncoding
// See: http://unicode.org/Public/MAPPINGS/VENDORS/ADOBE/stdenc.txt
// -----------------------------------------------------

// -----------------------------------------------------
// 
// -----------------------------------------------------
const pdf_utf16be* PdfStandardEncoding::GetToUnicodeTable() const
{
    return PdfStandardEncoding::s_cEncoding;
}

const pdf_utf16be PdfStandardEncoding::s_cEncoding[256] = {
//0, // uncomment to check compiler error cause of 257 members
 // 0x00..0x1f undefined:
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0020,  // 20   # SPACE   # space
    // Duplicated char, commented out
    // 0x00A0,  // 20   # NO-BREAK SPACE   # space
    0x0021,  // 21   # EXCLAMATION MARK   # exclam
    0x0022,  // 22   # QUOTATION MARK   # quotedbl
    0x0023,  // 23   # NUMBER SIGN   # numbersign
    0x0024,  // 24   # DOLLAR SIGN   # dollar
    0x0025,  // 25   # PERCENT SIGN   # percent
    0x0026,  // 26   # AMPERSAND   # ampersand
    0x2019,  // 27   # RIGHT SINGLE QUOTATION MARK   # quoteright
    0x0028,  // 28   # LEFT PARENTHESIS   # parenleft
    0x0029,  // 29   # RIGHT PARENTHESIS   # parenright
    0x002A,  // 2A   # ASTERISK   # asterisk
    0x002B,  // 2B   # PLUS SIGN   # plus
    0x002C,  // 2C   # COMMA   # comma
    0x002D,  // 2D   # HYPHEN-MINUS   # hyphen
    // Duplicated char, commented out
    // 0x00AD,  // 2D   # SOFT HYPHEN   # hyphen
    0x002E,  // 2E   # FULL STOP   # period
    0x002F,  // 2F   # SOLIDUS   # slash
    0x0030,  // 30   # DIGIT ZERO   # zero
    0x0031,  // 31   # DIGIT ONE   # one
    0x0032,  // 32   # DIGIT TWO   # two
    0x0033,  // 33   # DIGIT THREE   # three
    0x0034,  // 34   # DIGIT FOUR   # four
    0x0035,  // 35   # DIGIT FIVE   # five
    0x0036,  // 36   # DIGIT SIX   # six
    0x0037,  // 37   # DIGIT SEVEN   # seven
    0x0038,  // 38   # DIGIT EIGHT   # eight
    0x0039,  // 39   # DIGIT NINE   # nine
    0x003A,  // 3A   # COLON   # colon
    0x003B,  // 3B   # SEMICOLON   # semicolon
    0x003C,  // 3C   # LESS-THAN SIGN   # less
    0x003D,  // 3D   # EQUALS SIGN   # equal
    0x003E,  // 3E   # GREATER-THAN SIGN   # greater
    0x003F,  // 3F   # QUESTION MARK   # question
    0x0040,  // 40   # COMMERCIAL AT   # at
    0x0041,  // 41   # LATIN CAPITAL LETTER A   # A
    0x0042,  // 42   # LATIN CAPITAL LETTER B   # B
    0x0043,  // 43   # LATIN CAPITAL LETTER C   # C
    0x0044,  // 44   # LATIN CAPITAL LETTER D   # D
    0x0045,  // 45   # LATIN CAPITAL LETTER E   # E
    0x0046,  // 46   # LATIN CAPITAL LETTER F   # F
    0x0047,  // 47   # LATIN CAPITAL LETTER G   # G
    0x0048,  // 48   # LATIN CAPITAL LETTER H   # H
    0x0049,  // 49   # LATIN CAPITAL LETTER I   # I
    0x004A,  // 4A   # LATIN CAPITAL LETTER J   # J
    0x004B,  // 4B   # LATIN CAPITAL LETTER K   # K
    0x004C,  // 4C   # LATIN CAPITAL LETTER L   # L
    0x004D,  // 4D   # LATIN CAPITAL LETTER M   # M
    0x004E,  // 4E   # LATIN CAPITAL LETTER N   # N
    0x004F,  // 4F   # LATIN CAPITAL LETTER O   # O
    0x0050,  // 50   # LATIN CAPITAL LETTER P   # P
    0x0051,  // 51   # LATIN CAPITAL LETTER Q   # Q
    0x0052,  // 52   # LATIN CAPITAL LETTER R   # R
    0x0053,  // 53   # LATIN CAPITAL LETTER S   # S
    0x0054,  // 54   # LATIN CAPITAL LETTER T   # T
    0x0055,  // 55   # LATIN CAPITAL LETTER U   # U
    0x0056,  // 56   # LATIN CAPITAL LETTER V   # V
    0x0057,  // 57   # LATIN CAPITAL LETTER W   # W
    0x0058,  // 58   # LATIN CAPITAL LETTER X   # X
    0x0059,  // 59   # LATIN CAPITAL LETTER Y   # Y
    0x005A,  // 5A   # LATIN CAPITAL LETTER Z   # Z
    0x005B,  // 5B   # LEFT SQUARE BRACKET   # bracketleft
    0x005C,  // 5C   # REVERSE SOLIDUS   # backslash
    0x005D,  // 5D   # RIGHT SQUARE BRACKET   # bracketright
    0x005E,  // 5E   # CIRCUMFLEX ACCENT   # asciicircum
    0x005F,  // 5F   # LOW LINE   # underscore
#if 1
    0x0060,  // 60   # GRAVE ACCENT
#else
    // OC 13.08.2010: See http://unicode.org/Public/MAPPINGS/VENDORS/ADOBE/stdenc.txt
    // The following unicode char should be encoded here.
    // But i keep the identity ascii 7bit sign.
    0x2018,  // 60   # LEFT SINGLE QUOTATION MARK   # quoteleft
#endif
    0x0061,  // 61   # LATIN SMALL LETTER A   # a
    0x0062,  // 62   # LATIN SMALL LETTER B   # b
    0x0063,  // 63   # LATIN SMALL LETTER C   # c
    0x0064,  // 64   # LATIN SMALL LETTER D   # d
    0x0065,  // 65   # LATIN SMALL LETTER E   # e
    0x0066,  // 66   # LATIN SMALL LETTER F   # f
    0x0067,  // 67   # LATIN SMALL LETTER G   # g
    0x0068,  // 68   # LATIN SMALL LETTER H   # h
    0x0069,  // 69   # LATIN SMALL LETTER I   # i
    0x006A,  // 6A   # LATIN SMALL LETTER J   # j
    0x006B,  // 6B   # LATIN SMALL LETTER K   # k
    0x006C,  // 6C   # LATIN SMALL LETTER L   # l
    0x006D,  // 6D   # LATIN SMALL LETTER M   # m
    0x006E,  // 6E   # LATIN SMALL LETTER N   # n
    0x006F,  // 6F   # LATIN SMALL LETTER O   # o
    0x0070,  // 70   # LATIN SMALL LETTER P   # p
    0x0071,  // 71   # LATIN SMALL LETTER Q   # q
    0x0072,  // 72   # LATIN SMALL LETTER R   # r
    0x0073,  // 73   # LATIN SMALL LETTER S   # s
    0x0074,  // 74   # LATIN SMALL LETTER T   # t
    0x0075,  // 75   # LATIN SMALL LETTER U   # u
    0x0076,  // 76   # LATIN SMALL LETTER V   # v
    0x0077,  // 77   # LATIN SMALL LETTER W   # w
    0x0078,  // 78   # LATIN SMALL LETTER X   # x
    0x0079,  // 79   # LATIN SMALL LETTER Y   # y
    0x007A,  // 7A   # LATIN SMALL LETTER Z   # z
    0x007B,  // 7B   # LEFT CURLY BRACKET   # braceleft
    0x007C,  // 7C   # VERTICAL LINE   # bar
    0x007D,  // 7D   # RIGHT CURLY BRACKET   # braceright
    0x007E,  // 7E   # TILDE   # asciitilde

 // 0x7f..0xA0 undefined:
    0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000,

    0x00A1,  // A1   # INVERTED EXCLAMATION MARK   # exclamdown
    0x00A2,  // A2   # CENT SIGN   # cent
    0x00A3,  // A3   # POUND SIGN   # sterling
    0x2044,  // A4   # FRACTION SLASH	# fraction
    // Duplicated char, commented out
    // 0x2215,  // A4   # DIVISION SLASH	# fraction
    0x00A5,  // A5   # YEN SIGN   # yen
    0x0192,  // A6   # LATIN SMALL LETTER F WITH HOOK   # florin
    0x00A7,  // A7   # SECTION SIGN   # section
    0x00A4,  // A8   # CURRENCY SIGN   # currency
    0x0027,  // A9   # APOSTROPHE   # quotesingle
    0x201C,  // AA   # LEFT DOUBLE QUOTATION MARK   # quotedblleft
    0x00AB,  // AB   # LEFT-POINTING DOUBLE ANGLE QUOTATION MARK   # guillemotleft
    0x2039,  // AC   # SINGLE LEFT-POINTING ANGLE QUOTATION MARK   # guilsinglleft
    0x203A,  // AD   # SINGLE RIGHT-POINTING ANGLE QUOTATION MARK   # guilsinglright
    0xFB01,  // AE   # LATIN SMALL LIGATURE FI   # fi
    0xFB02,  // AF   # LATIN SMALL LIGATURE FL   # fl
    0x0000,  // B0 undefined
    0x2013,  // B1   # EN DASH   # endash
    0x2020,  // B2   # DAGGER   # dagger
    0x2021,  // B3   # DOUBLE DAGGER   # daggerdbl
    0x00B7,  // B4   # MIDDLE DOT   # periodcentered
 // 0x2219,  // B4   # BULLET OPERATOR   # periodcentered
    0x0000,  // B5 undefined
    0x00B6,  // B6   # PILCROW SIGN   # paragraph
    0x2022,  // B7   # BULLET   # bullet
    0x201A,  // B8   # SINGLE LOW-9 QUOTATION MARK   # quotesinglbase
    0x201E,  // B9   # DOUBLE LOW-9 QUOTATION MARK   # quotedblbase
    0x201D,  // BA   # RIGHT DOUBLE QUOTATION MARK   # quotedblright
    0x00BB,  // BB   # RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK   # guillemotright
    0x2026,  // BC   # HORIZONTAL ELLIPSIS   # ellipsis
    0x2030,  // BD   # PER MILLE SIGN   # perthousand
    0x0000,  // BE undefined
    0x00BF,  // BF   # INVERTED QUESTION MARK   # questiondown
    0x0000,  // C0 undefined
    0x0060,  // C1   # GRAVE ACCENT   # grave
    0x00B4,  // C2   # ACUTE ACCENT   # acute
    0x02C6,  // C3   # MODIFIER LETTER CIRCUMFLEX ACCENT   # circumflex
    0x02DC,  // C4   # SMALL TILDE   # tilde
    0x00AF,  // C5   # MACRON   # macron
    // Duplicated char, commented out
    //0x02C9,  // C5   # MODIFIER LETTER MACRON   # macron
    0x02D8,  // C6   # BREVE   # breve
    0x02D9,  // C7   # DOT ABOVE   # dotaccent
    0x00A8,  // C8   # DIAERESIS   # dieresis
    0x0000,  // C9 undefined
    0x02DA,  // CA   # RING ABOVE   # ring
    0x00B8,  // CB   # CEDILLA   # cedilla
    0x0000,  // CC undefined
    0x02DD,  // CD   # DOUBLE ACUTE ACCENT   # hungarumlaut
    0x02DB,  // CE   # OGONEK   # ogonek
    0x02C7,  // CF   # CARON   # caron
    0x2014,  // D0   # EM DASH   # emdash
    0x0000,  // D1 undefined
    0x0000,  // D2 undefined
    0x0000,  // D3 undefined
    0x0000,  // D4 undefined
    0x0000,  // D5 undefined
    0x0000,  // D6 undefined
    0x0000,  // D7 undefined
    0x0000,  // D8 undefined
    0x0000,  // D9 undefined
    0x0000,  // DA undefined
    0x0000,  // DB undefined
    0x0000,  // DC undefined
    0x0000,  // DD undefined
    0x0000,  // DE undefined
    0x0000,  // DF undefined
    0x0000,  // E0 undefined
    0x00C6,  // E1   # LATIN CAPITAL LETTER AE   # AE
    0x0000,  // E2 undefined
    0x00AA,  // E3   # FEMININE ORDINAL INDICATOR   # ordfeminine
    0x0000,  // E4 undefined
    0x0000,  // E5 undefined
    0x0000,  // E6 undefined
    0x0000,  // E7 undefined
    0x0141,  // E8   # LATIN CAPITAL LETTER L WITH STROKE   # Lslash
    0x00D8,  // E9   # LATIN CAPITAL LETTER O WITH STROKE   # Oslash
    0x0152,  // EA   # LATIN CAPITAL LIGATURE OE   # OE
    0x00BA,  // EB   # MASCULINE ORDINAL INDICATOR   # ordmasculine
    0x0000,  // EC undefined
    0x0000,  // ED undefined
    0x0000,  // EE undefined
    0x0000,  // EF undefined
    0x0000,  // F0 undefined
    0x00E6,  // F1   # LATIN SMALL LETTER AE   # ae
    0x0000,  // F2 undefined
    0x0000,  // F3 undefined
    0x0000,  // F4 undefined
    0x0131,  // F5   # LATIN SMALL LETTER DOTLESS I   # dotlessi
    0x0000,  // F6 undefined
    0x0000,  // F7 undefined
    0x0142,  // F8   # LATIN SMALL LETTER L WITH STROKE   # lslash
    0x00F8,  // F9   # LATIN SMALL LETTER O WITH STROKE   # oslash
    0x0153,  // FA   # LATIN SMALL LIGATURE OE   # oe
    0x00DF,  // FB   # LATIN SMALL LETTER SHARP S   # germandbls
    0x0000,  // FC undefined
    0x0000,  // FD undefined
    0x0000,  // FE undefined
    0x0000   // FF undefined
};

// OC 13.08.2010 New: PdfSymbolEncoding
// -----------------------------------------------------
// PdfSymbolEncoding
// See: http://unicode.org/Public/MAPPINGS/VENDORS/ADOBE/symbol.txt
// -----------------------------------------------------

// -----------------------------------------------------
// 
// -----------------------------------------------------
const pdf_utf16be* PdfSymbolEncoding::GetToUnicodeTable() const
{
    return PdfSymbolEncoding::s_cEncoding;
}

const pdf_utf16be PdfSymbolEncoding::s_cEncoding[256] = {
//0, // uncomment to check compiler error cause of 257 members
 // 0x00..0x1f undefined:
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0020,  // 20   # SPACE   # space
 // 0x00A0,  // 20   # NO-BREAK SPACE   # space
    0x0021,  // 21   # EXCLAMATION MARK   # exclam
    0x2200,  // 22   # FOR ALL   # universal
    0x0023,  // 23   # NUMBER SIGN   # numbersign
    0x2203,  // 24   # THERE EXISTS   # existential
    0x0025,  // 25   # PERCENT SIGN   # percent
    0x0026,  // 26   # AMPERSAND   # ampersand
    0x220B,  // 27   # CONTAINS AS MEMBER   # suchthat
    0x0028,  // 28   # LEFT PARENTHESIS   # parenleft
    0x0029,  // 29   # RIGHT PARENTHESIS   # parenright
    0x2217,  // 2A   # ASTERISK OPERATOR   # asteriskmath
    0x002B,  // 2B   # PLUS SIGN   # plus
    0x002C,  // 2C   # COMMA   # comma
    0x2212,  // 2D   # MINUS SIGN   # minus
    0x002E,  // 2E   # FULL STOP   # period
    0x002F,  // 2F   # SOLIDUS   # slash
    0x0030,  // 30   # DIGIT ZERO   # zero
    0x0031,  // 31   # DIGIT ONE   # one
    0x0032,  // 32   # DIGIT TWO   # two
    0x0033,  // 33   # DIGIT THREE   # three
    0x0034,  // 34   # DIGIT FOUR   # four
    0x0035,  // 35   # DIGIT FIVE   # five
    0x0036,  // 36   # DIGIT SIX   # six
    0x0037,  // 37   # DIGIT SEVEN   # seven
    0x0038,  // 38   # DIGIT EIGHT   # eight
    0x0039,  // 39   # DIGIT NINE   # nine
    0x003A,  // 3A   # COLON   # colon
    0x003B,  // 3B   # SEMICOLON   # semicolon
    0x003C,  // 3C   # LESS-THAN SIGN   # less
    0x003D,  // 3D   # EQUALS SIGN   # equal
    0x003E,  // 3E   # GREATER-THAN SIGN   # greater
    0x003F,  // 3F   # QUESTION MARK   # question
    0x2245,  // 40   # APPROXIMATELY EQUAL TO   # congruent
    0x0391,  // 41   # GREEK CAPITAL LETTER ALPHA   # Alpha
    0x0392,  // 42   # GREEK CAPITAL LETTER BETA   # Beta
    0x03A7,  // 43   # GREEK CAPITAL LETTER CHI   # Chi
    0x0394,  // 44   # GREEK CAPITAL LETTER DELTA   # Delta
 // 0x2206,  // 44   # INCREMENT   # Delta
    0x0395,  // 45   # GREEK CAPITAL LETTER EPSILON   # Epsilon
    0x03A6,  // 46   # GREEK CAPITAL LETTER PHI   # Phi
    0x0393,  // 47   # GREEK CAPITAL LETTER GAMMA   # Gamma
    0x0397,  // 48   # GREEK CAPITAL LETTER ETA   # Eta
    0x0399,  // 49   # GREEK CAPITAL LETTER IOTA   # Iota
    0x03D1,  // 4A   # GREEK THETA SYMBOL   # theta1
    0x039A,  // 4B   # GREEK CAPITAL LETTER KAPPA   # Kappa
    0x039B,  // 4C   # GREEK CAPITAL LETTER LAMDA   # Lambda
    0x039C,  // 4D   # GREEK CAPITAL LETTER MU   # Mu
    0x039D,  // 4E   # GREEK CAPITAL LETTER NU   # Nu
    0x039F,  // 4F   # GREEK CAPITAL LETTER OMICRON   # Omicron
    0x03A0,  // 50   # GREEK CAPITAL LETTER PI   # Pi
    0x0398,  // 51   # GREEK CAPITAL LETTER THETA   # Theta
    0x03A1,  // 52   # GREEK CAPITAL LETTER RHO   # Rho
    0x03A3,  // 53   # GREEK CAPITAL LETTER SIGMA   # Sigma
    0x03A4,  // 54   # GREEK CAPITAL LETTER TAU   # Tau
    0x03A5,  // 55   # GREEK CAPITAL LETTER UPSILON   # Upsilon
    0x03C2,  // 56   # GREEK SMALL LETTER FINAL SIGMA   # sigma1
    0x03A9,  // 57   # GREEK CAPITAL LETTER OMEGA   # Omega
 // 0x2126,  // 57   # OHM SIGN   # Omega
    0x039E,  // 58   # GREEK CAPITAL LETTER XI   # Xi
    0x03A8,  // 59   # GREEK CAPITAL LETTER PSI   # Psi
    0x0396,  // 5A   # GREEK CAPITAL LETTER ZETA   # Zeta
    0x005B,  // 5B   # LEFT SQUARE BRACKET   # bracketleft
    0x2234,  // 5C   # THEREFORE   # therefore
    0x005D,  // 5D   # RIGHT SQUARE BRACKET   # bracketright
    0x22A5,  // 5E   # UP TACK   # perpendicular
    0x005F,  // 5F   # LOW LINE   # underscore
    0xF8E5,  // 60   # RADICAL EXTENDER   # radicalex (CUS)
    0x03B1,  // 61   # GREEK SMALL LETTER ALPHA   # alpha
    0x03B2,  // 62   # GREEK SMALL LETTER BETA   # beta
    0x03C7,  // 63   # GREEK SMALL LETTER CHI   # chi
    0x03B4,  // 64   # GREEK SMALL LETTER DELTA   # delta
    0x03B5,  // 65   # GREEK SMALL LETTER EPSILON   # epsilon
    0x03C6,  // 66   # GREEK SMALL LETTER PHI   # phi
    0x03B3,  // 67   # GREEK SMALL LETTER GAMMA   # gamma
    0x03B7,  // 68   # GREEK SMALL LETTER ETA   # eta
    0x03B9,  // 69   # GREEK SMALL LETTER IOTA   # iota
    0x03D5,  // 6A   # GREEK PHI SYMBOL   # phi1
    0x03BA,  // 6B   # GREEK SMALL LETTER KAPPA   # kappa
    0x03BB,  // 6C   # GREEK SMALL LETTER LAMDA   # lambda
 // 0x00B5,  // 6D   # MICRO SIGN   # mu
    0x03BC,  // 6D   # GREEK SMALL LETTER MU   # mu
    0x03BD,  // 6E   # GREEK SMALL LETTER NU   # nu
    0x03BF,  // 6F   # GREEK SMALL LETTER OMICRON   # omicron
    0x03C0,  // 70   # GREEK SMALL LETTER PI   # pi
    0x03B8,  // 71   # GREEK SMALL LETTER THETA   # theta
    0x03C1,  // 72   # GREEK SMALL LETTER RHO   # rho
    0x03C3,  // 73   # GREEK SMALL LETTER SIGMA   # sigma
    0x03C4,  // 74   # GREEK SMALL LETTER TAU   # tau
    0x03C5,  // 75   # GREEK SMALL LETTER UPSILON   # upsilon
    0x03D6,  // 76   # GREEK PI SYMBOL   # omega1
    0x03C9,  // 77   # GREEK SMALL LETTER OMEGA   # omega
    0x03BE,  // 78   # GREEK SMALL LETTER XI   # xi
    0x03C8,  // 79   # GREEK SMALL LETTER PSI   # psi
    0x03B6,  // 7A   # GREEK SMALL LETTER ZETA   # zeta
    0x007B,  // 7B   # LEFT CURLY BRACKET   # braceleft
    0x007C,  // 7C   # VERTICAL LINE   # bar
    0x007D,  // 7D   # RIGHT CURLY BRACKET   # braceright
    0x223C,  // 7E   # TILDE OPERATOR   # similar

 // 0x7f..0x9F undefined:
    0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

    0x20AC,  // A0   # EURO SIGN   # Euro
    0x03D2,  // A1   # GREEK UPSILON WITH HOOK SYMBOL   # Upsilon1
    0x2032,  // A2   # PRIME   # minute
    0x2264,  // A3   # LESS-THAN OR EQUAL TO   # lessequal
    0x2044,  // A4   # FRACTION SLASH   # fraction
 // 0x2215,  // A4   # DIVISION SLASH   # fraction
    0x221E,  // A5   # INFINITY   # infinity
    0x0192,  // A6   # LATIN SMALL LETTER F WITH HOOK   # florin
    0x2663,  // A7   # BLACK CLUB SUIT   # club
    0x2666,  // A8   # BLACK DIAMOND SUIT   # diamond
    0x2665,  // A9   # BLACK HEART SUIT   # heart
    0x2660,  // AA   # BLACK SPADE SUIT   # spade
    0x2194,  // AB   # LEFT RIGHT ARROW   # arrowboth
    0x2190,  // AC   # LEFTWARDS ARROW   # arrowleft
    0x2191,  // AD   # UPWARDS ARROW   # arrowup
    0x2192,  // AE   # RIGHTWARDS ARROW   # arrowright
    0x2193,  // AF   # DOWNWARDS ARROW   # arrowdown
    0x00B0,  // B0   # DEGREE SIGN   # degree
    0x00B1,  // B1   # PLUS-MINUS SIGN   # plusminus
    0x2033,  // B2   # DOUBLE PRIME   # second
    0x2265,  // B3   # GREATER-THAN OR EQUAL TO   # greaterequal
    0x00D7,  // B4   # MULTIPLICATION SIGN   # multiply
    0x221D,  // B5   # PROPORTIONAL TO   # proportional
    0x2202,  // B6   # PARTIAL DIFFERENTIAL   # partialdiff
    0x2022,  // B7   # BULLET   # bullet
    0x00F7,  // B8   # DIVISION SIGN   # divide
    0x2260,  // B9   # NOT EQUAL TO   # notequal
    0x2261,  // BA   # IDENTICAL TO   # equivalence
    0x2248,  // BB   # ALMOST EQUAL TO   # approxequal
    0x2026,  // BC   # HORIZONTAL ELLIPSIS   # ellipsis
    0xF8E6,  // BD   # VERTICAL ARROW EXTENDER   # arrowvertex (CUS)
    0xF8E7,  // BE   # HORIZONTAL ARROW EXTENDER   # arrowhorizex (CUS)
    0x21B5,  // BF   # DOWNWARDS ARROW WITH CORNER LEFTWARDS   # carriagereturn
    0x2135,  // C0   # ALEF SYMBOL   # aleph
    0x2111,  // C1   # BLACK-LETTER CAPITAL I   # Ifraktur
    0x211C,  // C2   # BLACK-LETTER CAPITAL R   # Rfraktur
    0x2118,  // C3   # SCRIPT CAPITAL P   # weierstrass
    0x2297,  // C4   # CIRCLED TIMES   # circlemultiply
    0x2295,  // C5   # CIRCLED PLUS   # circleplus
    0x2205,  // C6   # EMPTY SET   # emptyset
    0x2229,  // C7   # INTERSECTION   # intersection
    0x222A,  // C8   # UNION   # union
    0x2283,  // C9   # SUPERSET OF   # propersuperset
    0x2287,  // CA   # SUPERSET OF OR EQUAL TO   # reflexsuperset
    0x2284,  // CB   # NOT A SUBSET OF   # notsubset
    0x2282,  // CC   # SUBSET OF   # propersubset
    0x2286,  // CD   # SUBSET OF OR EQUAL TO   # reflexsubset
    0x2208,  // CE   # ELEMENT OF   # element
    0x2209,  // CF   # NOT AN ELEMENT OF   # notelement
    0x2220,  // D0   # ANGLE   # angle
    0x2207,  // D1   # NABLA   # gradient
    0xF6DA,  // D2   # REGISTERED SIGN SERIF   # registerserif (CUS)
    0xF6D9,  // D3   # COPYRIGHT SIGN SERIF   # copyrightserif (CUS)
    0xF6DB,  // D4   # TRADE MARK SIGN SERIF   # trademarkserif (CUS)
    0x220F,  // D5   # N-ARY PRODUCT   # product
    0x221A,  // D6   # SQUARE ROOT   # radical
    0x22C5,  // D7   # DOT OPERATOR   # dotmath
    0x00AC,  // D8   # NOT SIGN   # logicalnot
    0x2227,  // D9   # LOGICAL AND   # logicaland
    0x2228,  // DA   # LOGICAL OR   # logicalor
    0x21D4,  // DB   # LEFT RIGHT DOUBLE ARROW   # arrowdblboth
    0x21D0,  // DC   # LEFTWARDS DOUBLE ARROW   # arrowdblleft
    0x21D1,  // DD   # UPWARDS DOUBLE ARROW   # arrowdblup
    0x21D2,  // DE   # RIGHTWARDS DOUBLE ARROW   # arrowdblright
    0x21D3,  // DF   # DOWNWARDS DOUBLE ARROW   # arrowdbldown
    0x25CA,  // E0   # LOZENGE   # lozenge
    0x2329,  // E1   # LEFT-POINTING ANGLE BRACKET   # angleleft
    0xF8E8,  // E2   # REGISTERED SIGN SANS SERIF   # registersans (CUS)
    0xF8E9,  // E3   # COPYRIGHT SIGN SANS SERIF   # copyrightsans (CUS)
    0xF8EA,  // E4   # TRADE MARK SIGN SANS SERIF   # trademarksans (CUS)
    0x2211,  // E5   # N-ARY SUMMATION   # summation
    0xF8EB,  // E6   # LEFT PAREN TOP   # parenlefttp (CUS)
    0xF8EC,  // E7   # LEFT PAREN EXTENDER   # parenleftex (CUS)
    0xF8ED,  // E8   # LEFT PAREN BOTTOM   # parenleftbt (CUS)
    0xF8EE,  // E9   # LEFT SQUARE BRACKET TOP   # bracketlefttp (CUS)
    0xF8EF,  // EA   # LEFT SQUARE BRACKET EXTENDER   # bracketleftex (CUS)
    0xF8F0,  // EB   # LEFT SQUARE BRACKET BOTTOM   # bracketleftbt (CUS)
    0xF8F1,  // EC   # LEFT CURLY BRACKET TOP   # bracelefttp (CUS)
    0xF8F2,  // ED   # LEFT CURLY BRACKET MID   # braceleftmid (CUS)
    0xF8F3,  // EE   # LEFT CURLY BRACKET BOTTOM   # braceleftbt (CUS)
    0xF8F4,  // EF   # CURLY BRACKET EXTENDER   # braceex (CUS)
    0x0000,  // F0 undefined
    0x232A,  // F1   # RIGHT-POINTING ANGLE BRACKET   # angleright
    0x222B,  // F2   # INTEGRAL   # integral
    0x2320,  // F3   # TOP HALF INTEGRAL   # integraltp
    0xF8F5,  // F4   # INTEGRAL EXTENDER   # integralex (CUS)
    0x2321,  // F5   # BOTTOM HALF INTEGRAL   # integralbt
    0xF8F6,  // F6   # RIGHT PAREN TOP   # parenrighttp (CUS)
    0xF8F7,  // F7   # RIGHT PAREN EXTENDER   # parenrightex (CUS)
    0xF8F8,  // F8   # RIGHT PAREN BOTTOM   # parenrightbt (CUS)
    0xF8F9,  // F9   # RIGHT SQUARE BRACKET TOP   # bracketrighttp (CUS)
    0xF8FA,  // FA   # RIGHT SQUARE BRACKET EXTENDER   # bracketrightex (CUS)
    0xF8FB,  // FB   # RIGHT SQUARE BRACKET BOTTOM   # bracketrightbt (CUS)
    0xF8FC,  // FC   # RIGHT CURLY BRACKET TOP   # bracerighttp (CUS)
    0xF8FD,  // FD   # RIGHT CURLY BRACKET MID   # bracerightmid (CUS)
    0xF8FE,  // FE   # RIGHT CURLY BRACKET BOTTOM   # bracerightbt (CUS)
    0x0000   // FF undefined
};

// OC 13.08.2010 New: PdfZapfDingbatsEncoding
// -----------------------------------------------------
// PdfZapfDingbatsEncoding
// See: http://unicode.org/Public/MAPPINGS/VENDORS/ADOBE/zdingbat.txt
// -----------------------------------------------------

// -----------------------------------------------------
// 
// -----------------------------------------------------
const pdf_utf16be* PdfZapfDingbatsEncoding::GetToUnicodeTable() const
{
    return PdfZapfDingbatsEncoding::s_cEncoding;
}

const pdf_utf16be PdfZapfDingbatsEncoding::s_cEncoding[256] = {
//0, // uncomment to check compiler error cause of 257 members
 // 0x00..0x1f undefined:
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0020,  // 20   # SPACE   # space
 // 0x00A0,  // 20   # NO-BREAK SPACE   # space
    0x2701,  // 21   # UPPER BLADE SCISSORS   # a1
    0x2702,  // 22   # BLACK SCISSORS   # a2
    0x2703,  // 23   # LOWER BLADE SCISSORS   # a202
    0x2704,  // 24   # WHITE SCISSORS   # a3
    0x260E,  // 25   # BLACK TELEPHONE   # a4
    0x2706,  // 26   # TELEPHONE LOCATION SIGN   # a5
    0x2707,  // 27   # TAPE DRIVE   # a119
    0x2708,  // 28   # AIRPLANE   # a118
    0x2709,  // 29   # ENVELOPE   # a117
    0x261B,  // 2A   # BLACK RIGHT POINTING INDEX   # a11
    0x261E,  // 2B   # WHITE RIGHT POINTING INDEX   # a12
    0x270C,  // 2C   # VICTORY HAND   # a13
    0x270D,  // 2D   # WRITING HAND   # a14
    0x270E,  // 2E   # LOWER RIGHT PENCIL   # a15
    0x270F,  // 2F   # PENCIL   # a16
    0x2710,  // 30   # UPPER RIGHT PENCIL   # a105
    0x2711,  // 31   # WHITE NIB   # a17
    0x2712,  // 32   # BLACK NIB   # a18
    0x2713,  // 33   # CHECK MARK   # a19
    0x2714,  // 34   # HEAVY CHECK MARK   # a20
    0x2715,  // 35   # MULTIPLICATION X   # a21
    0x2716,  // 36   # HEAVY MULTIPLICATION X   # a22
    0x2717,  // 37   # BALLOT X   # a23
    0x2718,  // 38   # HEAVY BALLOT X   # a24
    0x2719,  // 39   # OUTLINED GREEK CROSS   # a25
    0x271A,  // 3A   # HEAVY GREEK CROSS   # a26
    0x271B,  // 3B   # OPEN CENTRE CROSS   # a27
    0x271C,  // 3C   # HEAVY OPEN CENTRE CROSS   # a28
    0x271D,  // 3D   # LATIN CROSS   # a6
    0x271E,  // 3E   # SHADOWED WHITE LATIN CROSS   # a7
    0x271F,  // 3F   # OUTLINED LATIN CROSS   # a8
    0x2720,  // 40   # MALTESE CROSS   # a9
    0x2721,  // 41   # STAR OF DAVID   # a10
    0x2722,  // 42   # FOUR TEARDROP-SPOKED ASTERISK   # a29
    0x2723,  // 43   # FOUR BALLOON-SPOKED ASTERISK   # a30
    0x2724,  // 44   # HEAVY FOUR BALLOON-SPOKED ASTERISK   # a31
    0x2725,  // 45   # FOUR CLUB-SPOKED ASTERISK   # a32
    0x2726,  // 46   # BLACK FOUR POINTED STAR   # a33
    0x2727,  // 47   # WHITE FOUR POINTED STAR   # a34
    0x2605,  // 48   # BLACK STAR   # a35
    0x2729,  // 49   # STRESS OUTLINED WHITE STAR   # a36
    0x272A,  // 4A   # CIRCLED WHITE STAR   # a37
    0x272B,  // 4B   # OPEN CENTRE BLACK STAR   # a38
    0x272C,  // 4C   # BLACK CENTRE WHITE STAR   # a39
    0x272D,  // 4D   # OUTLINED BLACK STAR   # a40
    0x272E,  // 4E   # HEAVY OUTLINED BLACK STAR   # a41
    0x272F,  // 4F   # PINWHEEL STAR   # a42
    0x2730,  // 50   # SHADOWED WHITE STAR   # a43
    0x2731,  // 51   # HEAVY ASTERISK   # a44
    0x2732,  // 52   # OPEN CENTRE ASTERISK   # a45
    0x2733,  // 53   # EIGHT SPOKED ASTERISK   # a46
    0x2734,  // 54   # EIGHT POINTED BLACK STAR   # a47
    0x2735,  // 55   # EIGHT POINTED PINWHEEL STAR   # a48
    0x2736,  // 56   # SIX POINTED BLACK STAR   # a49
    0x2737,  // 57   # EIGHT POINTED RECTILINEAR BLACK STAR   # a50
    0x2738,  // 58   # HEAVY EIGHT POINTED RECTILINEAR BLACK STAR   # a51
    0x2739,  // 59   # TWELVE POINTED BLACK STAR   # a52
    0x273A,  // 5A   # SIXTEEN POINTED ASTERISK   # a53
    0x273B,  // 5B   # TEARDROP-SPOKED ASTERISK   # a54
    0x273C,  // 5C   # OPEN CENTRE TEARDROP-SPOKED ASTERISK   # a55
    0x273D,  // 5D   # HEAVY TEARDROP-SPOKED ASTERISK   # a56
    0x273E,  // 5E   # SIX PETALLED BLACK AND WHITE FLORETTE   # a57
    0x273F,  // 5F   # BLACK FLORETTE   # a58
    0x2740,  // 60   # WHITE FLORETTE   # a59
    0x2741,  // 61   # EIGHT PETALLED OUTLINED BLACK FLORETTE   # a60
    0x2742,  // 62   # CIRCLED OPEN CENTRE EIGHT POINTED STAR   # a61
    0x2743,  // 63   # HEAVY TEARDROP-SPOKED PINWHEEL ASTERISK   # a62
    0x2744,  // 64   # SNOWFLAKE   # a63
    0x2745,  // 65   # TIGHT TRIFOLIATE SNOWFLAKE   # a64
    0x2746,  // 66   # HEAVY CHEVRON SNOWFLAKE   # a65
    0x2747,  // 67   # SPARKLE   # a66
    0x2748,  // 68   # HEAVY SPARKLE   # a67
    0x2749,  // 69   # BALLOON-SPOKED ASTERISK   # a68
    0x274A,  // 6A   # EIGHT TEARDROP-SPOKED PROPELLER ASTERISK   # a69
    0x274B,  // 6B   # HEAVY EIGHT TEARDROP-SPOKED PROPELLER ASTERISK   # a70
    0x25CF,  // 6C   # BLACK CIRCLE   # a71
    0x274D,  // 6D   # SHADOWED WHITE CIRCLE   # a72
    0x25A0,  // 6E   # BLACK SQUARE   # a73
    0x274F,  // 6F   # LOWER RIGHT DROP-SHADOWED WHITE SQUARE   # a74
    0x2750,  // 70   # UPPER RIGHT DROP-SHADOWED WHITE SQUARE   # a203
    0x2751,  // 71   # LOWER RIGHT SHADOWED WHITE SQUARE   # a75
    0x2752,  // 72   # UPPER RIGHT SHADOWED WHITE SQUARE   # a204
    0x25B2,  // 73   # BLACK UP-POINTING TRIANGLE   # a76
    0x25BC,  // 74   # BLACK DOWN-POINTING TRIANGLE   # a77
    0x25C6,  // 75   # BLACK DIAMOND   # a78
    0x2756,  // 76   # BLACK DIAMOND MINUS WHITE X   # a79
    0x25D7,  // 77   # RIGHT HALF BLACK CIRCLE   # a81
    0x2758,  // 78   # LIGHT VERTICAL BAR   # a82
    0x2759,  // 79   # MEDIUM VERTICAL BAR   # a83
    0x275A,  // 7A   # HEAVY VERTICAL BAR   # a84
    0x275B,  // 7B   # HEAVY SINGLE TURNED COMMA QUOTATION MARK ORNAMENT   # a97
    0x275C,  // 7C   # HEAVY SINGLE COMMA QUOTATION MARK ORNAMENT   # a98
    0x275D,  // 7D   # HEAVY DOUBLE TURNED COMMA QUOTATION MARK ORNAMENT   # a99
    0x275E,  // 7E   # HEAVY DOUBLE COMMA QUOTATION MARK ORNAMENT   # a100
    0x0000,  // 7F undefined
    0xF8D7,  // 80   # MEDIUM LEFT PARENTHESIS ORNAMENT   # a89 (CUS)
    0xF8D8,  // 81   # MEDIUM RIGHT PARENTHESIS ORNAMENT   # a90 (CUS)
    0xF8D9,  // 82   # MEDIUM FLATTENED LEFT PARENTHESIS ORNAMENT   # a93 (CUS)
    0xF8DA,  // 83   # MEDIUM FLATTENED RIGHT PARENTHESIS ORNAMENT   # a94 (CUS)
    0xF8DB,  // 84   # MEDIUM LEFT-POINTING ANGLE BRACKET ORNAMENT   # a91 (CUS)
    0xF8DC,  // 85   # MEDIUM RIGHT-POINTING ANGLE BRACKET ORNAMENT   # a92 (CUS)
    0xF8DD,  // 86   # HEAVY LEFT-POINTING ANGLE QUOTATION MARK ORNAMENT   # a205 (CUS)
    0xF8DE,  // 87   # HEAVY RIGHT-POINTING ANGLE QUOTATION MARK ORNAMENT   # a85 (CUS)
    0xF8DF,  // 88   # HEAVY LEFT-POINTING ANGLE BRACKET ORNAMENT   # a206 (CUS)
    0xF8E0,  // 89   # HEAVY RIGHT-POINTING ANGLE BRACKET ORNAMENT   # a86 (CUS)
    0xF8E1,  // 8A   # LIGHT LEFT TORTOISE SHELL BRACKET ORNAMENT   # a87 (CUS)
    0xF8E2,  // 8B   # LIGHT RIGHT TORTOISE SHELL BRACKET ORNAMENT   # a88 (CUS)
    0xF8E3,  // 8C   # MEDIUM LEFT CURLY BRACKET ORNAMENT   # a95 (CUS)
    0xF8E4,  // 8D   # MEDIUM RIGHT CURLY BRACKET ORNAMENT   # a96 (CUS)

 // 0x8E..0xA0 undefined:
    0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000,

    0x2761,  // A1   # CURVED STEM PARAGRAPH SIGN ORNAMENT   # a101
    0x2762,  // A2   # HEAVY EXCLAMATION MARK ORNAMENT   # a102
    0x2763,  // A3   # HEAVY HEART EXCLAMATION MARK ORNAMENT   # a103
    0x2764,  // A4   # HEAVY BLACK HEART   # a104
    0x2765,  // A5   # ROTATED HEAVY BLACK HEART BULLET   # a106
    0x2766,  // A6   # FLORAL HEART   # a107
    0x2767,  // A7   # ROTATED FLORAL HEART BULLET   # a108
    0x2663,  // A8   # BLACK CLUB SUIT   # a112
    0x2666,  // A9   # BLACK DIAMOND SUIT   # a111
    0x2665,  // AA   # BLACK HEART SUIT   # a110
    0x2660,  // AB   # BLACK SPADE SUIT   # a109
    0x2460,  // AC   # CIRCLED DIGIT ONE   # a120
    0x2461,  // AD   # CIRCLED DIGIT TWO   # a121
    0x2462,  // AE   # CIRCLED DIGIT THREE   # a122
    0x2463,  // AF   # CIRCLED DIGIT FOUR   # a123
    0x2464,  // B0   # CIRCLED DIGIT FIVE   # a124
    0x2465,  // B1   # CIRCLED DIGIT SIX   # a125
    0x2466,  // B2   # CIRCLED DIGIT SEVEN   # a126
    0x2467,  // B3   # CIRCLED DIGIT EIGHT   # a127
    0x2468,  // B4   # CIRCLED DIGIT NINE   # a128
    0x2469,  // B5   # CIRCLED NUMBER TEN   # a129
    0x2776,  // B6   # DINGBAT NEGATIVE CIRCLED DIGIT ONE   # a130
    0x2777,  // B7   # DINGBAT NEGATIVE CIRCLED DIGIT TWO   # a131
    0x2778,  // B8   # DINGBAT NEGATIVE CIRCLED DIGIT THREE   # a132
    0x2779,  // B9   # DINGBAT NEGATIVE CIRCLED DIGIT FOUR   # a133
    0x277A,  // BA   # DINGBAT NEGATIVE CIRCLED DIGIT FIVE   # a134
    0x277B,  // BB   # DINGBAT NEGATIVE CIRCLED DIGIT SIX   # a135
    0x277C,  // BC   # DINGBAT NEGATIVE CIRCLED DIGIT SEVEN   # a136
    0x277D,  // BD   # DINGBAT NEGATIVE CIRCLED DIGIT EIGHT   # a137
    0x277E,  // BE   # DINGBAT NEGATIVE CIRCLED DIGIT NINE   # a138
    0x277F,  // BF   # DINGBAT NEGATIVE CIRCLED NUMBER TEN   # a139
    0x2780,  // C0   # DINGBAT CIRCLED SANS-SERIF DIGIT ONE   # a140
    0x2781,  // C1   # DINGBAT CIRCLED SANS-SERIF DIGIT TWO   # a141
    0x2782,  // C2   # DINGBAT CIRCLED SANS-SERIF DIGIT THREE   # a142
    0x2783,  // C3   # DINGBAT CIRCLED SANS-SERIF DIGIT FOUR   # a143
    0x2784,  // C4   # DINGBAT CIRCLED SANS-SERIF DIGIT FIVE   # a144
    0x2785,  // C5   # DINGBAT CIRCLED SANS-SERIF DIGIT SIX   # a145
    0x2786,  // C6   # DINGBAT CIRCLED SANS-SERIF DIGIT SEVEN   # a146
    0x2787,  // C7   # DINGBAT CIRCLED SANS-SERIF DIGIT EIGHT   # a147
    0x2788,  // C8   # DINGBAT CIRCLED SANS-SERIF DIGIT NINE   # a148
    0x2789,  // C9   # DINGBAT CIRCLED SANS-SERIF NUMBER TEN   # a149
    0x278A,  // CA   # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT ONE   # a150
    0x278B,  // CB   # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT TWO   # a151
    0x278C,  // CC   # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT THREE   # a152
    0x278D,  // CD   # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT FOUR   # a153
    0x278E,  // CE   # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT FIVE   # a154
    0x278F,  // CF   # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT SIX   # a155
    0x2790,  // D0   # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT SEVEN   # a156
    0x2791,  // D1   # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT EIGHT   # a157
    0x2792,  // D2   # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT NINE   # a158
    0x2793,  // D3   # DINGBAT NEGATIVE CIRCLED SANS-SERIF NUMBER TEN   # a159
    0x2794,  // D4   # HEAVY WIDE-HEADED RIGHTWARDS ARROW   # a160
    0x2192,  // D5   # RIGHTWARDS ARROW   # a161
    0x2194,  // D6   # LEFT RIGHT ARROW   # a163
    0x2195,  // D7   # UP DOWN ARROW   # a164
    0x2798,  // D8   # HEAVY SOUTH EAST ARROW   # a196
    0x2799,  // D9   # HEAVY RIGHTWARDS ARROW   # a165
    0x279A,  // DA   # HEAVY NORTH EAST ARROW   # a192
    0x279B,  // DB   # DRAFTING POINT RIGHTWARDS ARROW   # a166
    0x279C,  // DC   # HEAVY ROUND-TIPPED RIGHTWARDS ARROW   # a167
    0x279D,  // DD   # TRIANGLE-HEADED RIGHTWARDS ARROW   # a168
    0x279E,  // DE   # HEAVY TRIANGLE-HEADED RIGHTWARDS ARROW   # a169
    0x279F,  // DF   # DASHED TRIANGLE-HEADED RIGHTWARDS ARROW   # a170
    0x27A0,  // E0   # HEAVY DASHED TRIANGLE-HEADED RIGHTWARDS ARROW   # a171
    0x27A1,  // E1   # BLACK RIGHTWARDS ARROW   # a172
    0x27A2,  // E2   # THREE-D TOP-LIGHTED RIGHTWARDS ARROWHEAD   # a173
    0x27A3,  // E3   # THREE-D BOTTOM-LIGHTED RIGHTWARDS ARROWHEAD   # a162
    0x27A4,  // E4   # BLACK RIGHTWARDS ARROWHEAD   # a174
    0x27A5,  // E5   # HEAVY BLACK CURVED DOWNWARDS AND RIGHTWARDS ARROW   # a175
    0x27A6,  // E6   # HEAVY BLACK CURVED UPWARDS AND RIGHTWARDS ARROW   # a176
    0x27A7,  // E7   # SQUAT BLACK RIGHTWARDS ARROW   # a177
    0x27A8,  // E8   # HEAVY CONCAVE-POINTED BLACK RIGHTWARDS ARROW   # a178
    0x27A9,  // E9   # RIGHT-SHADED WHITE RIGHTWARDS ARROW   # a179
    0x27AA,  // EA   # LEFT-SHADED WHITE RIGHTWARDS ARROW   # a193
    0x27AB,  // EB   # BACK-TILTED SHADOWED WHITE RIGHTWARDS ARROW   # a180
    0x27AC,  // EC   # FRONT-TILTED SHADOWED WHITE RIGHTWARDS ARROW   # a199
    0x27AD,  // ED   # HEAVY LOWER RIGHT-SHADOWED WHITE RIGHTWARDS ARROW   # a181
    0x27AE,  // EE   # HEAVY UPPER RIGHT-SHADOWED WHITE RIGHTWARDS ARROW   # a200
    0x27AF,  // EF   # NOTCHED LOWER RIGHT-SHADOWED WHITE RIGHTWARDS ARROW   # a182
    0x0000,  // F0 undefined
    0x27B1,  // F1   # NOTCHED UPPER RIGHT-SHADOWED WHITE RIGHTWARDS ARROW   # a201
    0x27B2,  // F2   # CIRCLED HEAVY WHITE RIGHTWARDS ARROW   # a183
    0x27B3,  // F3   # WHITE-FEATHERED RIGHTWARDS ARROW   # a184
    0x27B4,  // F4   # BLACK-FEATHERED SOUTH EAST ARROW   # a197
    0x27B5,  // F5   # BLACK-FEATHERED RIGHTWARDS ARROW   # a185
    0x27B6,  // F6   # BLACK-FEATHERED NORTH EAST ARROW   # a194
    0x27B7,  // F7   # HEAVY BLACK-FEATHERED SOUTH EAST ARROW   # a198
    0x27B8,  // F8   # HEAVY BLACK-FEATHERED RIGHTWARDS ARROW   # a186
    0x27B9,  // F9   # HEAVY BLACK-FEATHERED NORTH EAST ARROW   # a195
    0x27BA,  // FA   # TEARDROP-BARBED RIGHTWARDS ARROW   # a187
    0x27BB,  // FB   # HEAVY TEARDROP-SHANKED RIGHTWARDS ARROW   # a188
    0x27BC,  // FC   # WEDGE-TAILED RIGHTWARDS ARROW   # a189
    0x27BD,  // FD   # HEAVY WEDGE-TAILED RIGHTWARDS ARROW   # a190
    0x27BE,  // FE   # OPEN-OUTLINED RIGHTWARDS ARROW   # a191
    0x0000   // FF undefined
};

// -----------------------------------------------------
// PdfWin1250Encoding
// See: http://www.microsoft.com/globaldev/reference/sbcs/1250.mspx
// -----------------------------------------------------

// -----------------------------------------------------
// 
// -----------------------------------------------------
const pdf_utf16be PdfWin1250Encoding::s_cEncoding[256] = {
    0x0000,  // NULL
    0x0001,  // START OF HEADING
    0x0002,  // START OF TEXT
    0x0003,  // END OF TEXT
    0x0004,  // END OF TRANSMISSION
    0x0005,  // ENQUIRY
    0x0006,  // ACKNOWLEDGE
    0x0007,  // BELL
    0x0008,  // BACKSPACE
    0x0009,  // HORIZONTAL TABULATION
    0x000A,  // LINE FEED
    0x000B,  // VERTICAL TABULATION
    0x000C,  // FORM FEED
    0x000D,  // CARRIAGE RETURN
    0x000E,  // SHIFT OUT
    0x000F,  // SHIFT IN
    0x0010,  // DATA LINK ESCAPE
    0x0011,  // DEVICE CONTROL ONE
    0x0012,  // DEVICE CONTROL TWO
    0x0013,  // DEVICE CONTROL THREE
    0x0014,  // DEVICE CONTROL FOUR
    0x0015,  // NEGATIVE ACKNOWLEDGE
    0x0016,  // SYNCHRONOUS IDLE
    0x0017,  // END OF TRANSMISSION BLOCK
    0x0018,  // CANCEL
    0x0019,  // END OF MEDIUM
    0x001A,  // SUBSTITUTE
    0x001B,  // ESCAPE
    0x001C,  // FILE SEPARATOR
    0x001D,  // GROUP SEPARATOR
    0x001E,  // RECORD SEPARATOR
    0x001F,  // UNIT SEPARATOR
    0x0020,  // SPACE
    0x0021,  // EXCLAMATION MARK
    0x0022,  // QUOTATION MARK
    0x0023,  // NUMBER SIGN
    0x0024,  // DOLLAR SIGN
    0x0025,  // PERCENT SIGN
    0x0026,  // AMPERSAND
    0x0027,  // APOSTROPHE
    0x0028,  // LEFT PARENTHESIS
    0x0029,  // RIGHT PARENTHESIS
    0x002A,  // ASTERISK
    0x002B,  // PLUS SIGN
    0x002C,  // COMMA
    0x002D,  // HYPHEN-MINUS
    0x002E,  // FULL STOP
    0x002F,  // SOLIDUS
    0x0030,  // DIGIT ZERO
    0x0031,  // DIGIT ONE
    0x0032,  // DIGIT TWO
    0x0033,  // DIGIT THREE
    0x0034,  // DIGIT FOUR
    0x0035,  // DIGIT FIVE
    0x0036,  // DIGIT SIX
    0x0037,  // DIGIT SEVEN
    0x0038,  // DIGIT EIGHT
    0x0039,  // DIGIT NINE
    0x003A,  // COLON
    0x003B,  // SEMICOLON
    0x003C,  // LESS-THAN SIGN
    0x003D,  // EQUALS SIGN
    0x003E,  // GREATER-THAN SIGN
    0x003F,  // QUESTION MARK
    0x0040,  // COMMERCIAL AT
    0x0041,  // LATIN CAPITAL LETTER A
    0x0042,  // LATIN CAPITAL LETTER B
    0x0043,  // LATIN CAPITAL LETTER C
    0x0044,  // LATIN CAPITAL LETTER D
    0x0045,  // LATIN CAPITAL LETTER E
    0x0046,  // LATIN CAPITAL LETTER F
    0x0047,  // LATIN CAPITAL LETTER G
    0x0048,  // LATIN CAPITAL LETTER H
    0x0049,  // LATIN CAPITAL LETTER I
    0x004A,  // LATIN CAPITAL LETTER J
    0x004B,  // LATIN CAPITAL LETTER K
    0x004C,  // LATIN CAPITAL LETTER L
    0x004D,  // LATIN CAPITAL LETTER M
    0x004E,  // LATIN CAPITAL LETTER N
    0x004F,  // LATIN CAPITAL LETTER O
    0x0050,  // LATIN CAPITAL LETTER P
    0x0051,  // LATIN CAPITAL LETTER Q
    0x0052,  // LATIN CAPITAL LETTER R
    0x0053,  // LATIN CAPITAL LETTER S
    0x0054,  // LATIN CAPITAL LETTER T
    0x0055,  // LATIN CAPITAL LETTER U
    0x0056,  // LATIN CAPITAL LETTER V
    0x0057,  // LATIN CAPITAL LETTER W
    0x0058,  // LATIN CAPITAL LETTER X
    0x0059,  // LATIN CAPITAL LETTER Y
    0x005A,  // LATIN CAPITAL LETTER Z
    0x005B,  // LEFT SQUARE BRACKET
    0x005C,  // REVERSE SOLIDUS
    0x005D,  // RIGHT SQUARE BRACKET
    0x005E,  // CIRCUMFLEX ACCENT
    0x005F,  // LOW LINE
    0x0060,  // GRAVE ACCENT
    0x0061,  // LATIN SMALL LETTER A
    0x0062,  // LATIN SMALL LETTER B
    0x0063,  // LATIN SMALL LETTER C
    0x0064,  // LATIN SMALL LETTER D
    0x0065,  // LATIN SMALL LETTER E
    0x0066,  // LATIN SMALL LETTER F
    0x0067,  // LATIN SMALL LETTER G
    0x0068,  // LATIN SMALL LETTER H
    0x0069,  // LATIN SMALL LETTER I
    0x006A,  // LATIN SMALL LETTER J
    0x006B,  // LATIN SMALL LETTER K
    0x006C,  // LATIN SMALL LETTER L
    0x006D,  // LATIN SMALL LETTER M
    0x006E,  // LATIN SMALL LETTER N
    0x006F,  // LATIN SMALL LETTER O
    0x0070,  // LATIN SMALL LETTER P
    0x0071,  // LATIN SMALL LETTER Q
    0x0072,  // LATIN SMALL LETTER R
    0x0073,  // LATIN SMALL LETTER S
    0x0074,  // LATIN SMALL LETTER T
    0x0075,  // LATIN SMALL LETTER U
    0x0076,  // LATIN SMALL LETTER V
    0x0077,  // LATIN SMALL LETTER W
    0x0078,  // LATIN SMALL LETTER X
    0x0079,  // LATIN SMALL LETTER Y
    0x007A,  // LATIN SMALL LETTER Z
    0x007B,  // LEFT CURLY BRACKET
    0x007C,  // VERTICAL LINE
    0x007D,  // RIGHT CURLY BRACKET
    0x007E,  // TILDE
    0x007F,  // DELETE
    0x20AC,  // EURO SIGN
    0x0000,
    0x201A,  // SINGLE LOW-9 QUOTATION MARK
    0x0000,
    0x201E,  // DOUBLE LOW-9 QUOTATION MARK
    0x2026,  // HORIZONTAL ELLIPSIS
    0x2020,  // DAGGER
    0x2021,  // DOUBLE DAGGER
    0x0000,
    0x2030,  // PER MILLE SIGN
    0x0160,  // LATIN CAPITAL LETTER S WITH CARON
    0x2039,  // SINGLE LEFT-POINTING ANGLE QUOTATION MARK
    0x015A,  // LATIN CAPITAL LETTER S WITH ACUTE
    0x0164,  // LATIN CAPITAL LETTER T WITH CARON
    0x017D,  // LATIN CAPITAL LETTER Z WITH CARON
    0x0179,  // LATIN CAPITAL LETTER Z WITH ACUTE
    0x0000,
    0x2018,  // LEFT SINGLE QUOTATION MARK
    0x2019,  // RIGHT SINGLE QUOTATION MARK
    0x201C,  // LEFT DOUBLE QUOTATION MARK
    0x201D,  // RIGHT DOUBLE QUOTATION MARK
    0x2022,  // BULLET
    0x2013,  // EN DASH
    0x2014,  // EM DASH
    0x0000,
    0x2122,  // TRADE MARK SIGN
    0x0161,  // LATIN SMALL LETTER S WITH CARON
    0x203A,  // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
    0x015B,  // LATIN SMALL LETTER S WITH ACUTE
    0x0165,  // LATIN SMALL LETTER T WITH CARON
    0x017E,  // LATIN SMALL LETTER Z WITH CARON
    0x017A,  // LATIN SMALL LETTER Z WITH ACUTE
    0x00A0,  // NO-BREAK SPACE
    0x02C7,  // CARON
    0x02D8,  // BREVE
    0x0141,  // LATIN CAPITAL LETTER L WITH STROKE
    0x00A4,  // CURRENCY SIGN
    0x0104,  // LATIN CAPITAL LETTER A WITH OGONEK
    0x00A6,  // BROKEN BAR
    0x00A7,  // SECTION SIGN
    0x00A8,  // DIAERESIS
    0x00A9,  // COPYRIGHT SIGN
    0x015E,  // LATIN CAPITAL LETTER S WITH CEDILLA
    0x00AB,  // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00AC,  // NOT SIGN
    0x00AD,  // SOFT HYPHEN
    0x00AE,  // REGISTERED SIGN
    0x017B,  // LATIN CAPITAL LETTER Z WITH DOT ABOVE
    0x00B0,  // DEGREE SIGN
    0x00B1,  // PLUS-MINUS SIGN
    0x02DB,  // OGONEK
    0x0142,  // LATIN SMALL LETTER L WITH STROKE
    0x00B4,  // ACUTE ACCENT
    0x00B5,  // MICRO SIGN
    0x00B6,  // PILCROW SIGN
    0x00B7,  // MIDDLE DOT
    0x00B8,  // CEDILLA
    0x0105,  // LATIN SMALL LETTER A WITH OGONEK
    0x015F,  // LATIN SMALL LETTER S WITH CEDILLA
    0x00BB,  // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x013D,  // LATIN CAPITAL LETTER L WITH CARON
    0x02DD,  // DOUBLE ACUTE ACCENT
    0x013E,  // LATIN SMALL LETTER L WITH CARON
    0x017C,  // LATIN SMALL LETTER Z WITH DOT ABOVE
    0x0154,  // LATIN CAPITAL LETTER R WITH ACUTE
    0x00C1,  // LATIN CAPITAL LETTER A WITH ACUTE
    0x00C2,  // LATIN CAPITAL LETTER A WITH CIRCUMFLEX
    0x0102,  // LATIN CAPITAL LETTER A WITH BREVE
    0x00C4,  // LATIN CAPITAL LETTER A WITH DIAERESIS
    0x0139,  // LATIN CAPITAL LETTER L WITH ACUTE
    0x0106,  // LATIN CAPITAL LETTER C WITH ACUTE
    0x00C7,  // LATIN CAPITAL LETTER C WITH CEDILLA
    0x010C,  // LATIN CAPITAL LETTER C WITH CARON
    0x00C9,  // LATIN CAPITAL LETTER E WITH ACUTE
    0x0118,  // LATIN CAPITAL LETTER E WITH OGONEK
    0x00CB,  // LATIN CAPITAL LETTER E WITH DIAERESIS
    0x011A,  // LATIN CAPITAL LETTER E WITH CARON
    0x00CD,  // LATIN CAPITAL LETTER I WITH ACUTE
    0x00CE,  // LATIN CAPITAL LETTER I WITH CIRCUMFLEX
    0x010E,  // LATIN CAPITAL LETTER D WITH CARON
    0x0110,  // LATIN CAPITAL LETTER D WITH STROKE
    0x0143,  // LATIN CAPITAL LETTER N WITH ACUTE
    0x0147,  // LATIN CAPITAL LETTER N WITH CARON
    0x00D3,  // LATIN CAPITAL LETTER O WITH ACUTE
    0x00D4,  // LATIN CAPITAL LETTER O WITH CIRCUMFLEX
    0x0150,  // LATIN CAPITAL LETTER O WITH DOUBLE ACUTE
    0x00D6,  // LATIN CAPITAL LETTER O WITH DIAERESIS
    0x00D7,  // MULTIPLICATION SIGN
    0x0158,  // LATIN CAPITAL LETTER R WITH CARON
    0x016E,  // LATIN CAPITAL LETTER U WITH RING ABOVE
    0x00DA,  // LATIN CAPITAL LETTER U WITH ACUTE
    0x0170,  // LATIN CAPITAL LETTER U WITH DOUBLE ACUTE
    0x00DC,  // LATIN CAPITAL LETTER U WITH DIAERESIS
    0x00DD,  // LATIN CAPITAL LETTER Y WITH ACUTE
    0x0162,  // LATIN CAPITAL LETTER T WITH CEDILLA
    0x00DF,  // LATIN SMALL LETTER SHARP S
    0x0155,  // LATIN SMALL LETTER R WITH ACUTE
    0x00E1,  // LATIN SMALL LETTER A WITH ACUTE
    0x00E2,  // LATIN SMALL LETTER A WITH CIRCUMFLEX
    0x0103,  // LATIN SMALL LETTER A WITH BREVE
    0x00E4,  // LATIN SMALL LETTER A WITH DIAERESIS
    0x013A,  // LATIN SMALL LETTER L WITH ACUTE
    0x0107,  // LATIN SMALL LETTER C WITH ACUTE
    0x00E7,  // LATIN SMALL LETTER C WITH CEDILLA
    0x010D,  // LATIN SMALL LETTER C WITH CARON
    0x00E9,  // LATIN SMALL LETTER E WITH ACUTE
    0x0119,  // LATIN SMALL LETTER E WITH OGONEK
    0x00EB,  // LATIN SMALL LETTER E WITH DIAERESIS
    0x011B,  // LATIN SMALL LETTER E WITH CARON
    0x00ED,  // LATIN SMALL LETTER I WITH ACUTE
    0x00EE,  // LATIN SMALL LETTER I WITH CIRCUMFLEX
    0x010F,  // LATIN SMALL LETTER D WITH CARON
    0x0111,  // LATIN SMALL LETTER D WITH STROKE
    0x0144,  // LATIN SMALL LETTER N WITH ACUTE
    0x0148,  // LATIN SMALL LETTER N WITH CARON
    0x00F3,  // LATIN SMALL LETTER O WITH ACUTE
    0x00F4,  // LATIN SMALL LETTER O WITH CIRCUMFLEX
    0x0151,  // LATIN SMALL LETTER O WITH DOUBLE ACUTE
    0x00F6,  // LATIN SMALL LETTER O WITH DIAERESIS
    0x00F7,  // DIVISION SIGN
    0x0159,  // LATIN SMALL LETTER R WITH CARON
    0x016F,  // LATIN SMALL LETTER U WITH RING ABOVE
    0x00FA,  // LATIN SMALL LETTER U WITH ACUTE
    0x0171,  // LATIN SMALL LETTER U WITH DOUBLE ACUTE
    0x00FC,  // LATIN SMALL LETTER U WITH DIAERESIS
    0x00FD,  // LATIN SMALL LETTER Y WITH ACUTE
    0x0163,  // LATIN SMALL LETTER T WITH CEDILLA
    0x02D9   // DOT ABOVE
};

const pdf_utf16be* PdfWin1250Encoding::GetToUnicodeTable() const
{
    return PdfWin1250Encoding::s_cEncoding;
}

// -----------------------------------------------------
// PdfIso88592Encoding
// See: http://unicode.org/Public/MAPPINGS/ISO8859/8859-2.TXT
// -----------------------------------------------------

// -----------------------------------------------------
// 
// -----------------------------------------------------

const pdf_utf16be PdfIso88592Encoding::s_cEncoding[256] = {
    0x0000,  // NULL
    0x0001,  // START OF HEADING
    0x0002,  // START OF TEXT
    0x0003,  // END OF TEXT
    0x0004,  // END OF TRANSMISSION
    0x0005,  // ENQUIRY
    0x0006,  // ACKNOWLEDGE
    0x0007,  // BELL
    0x0008,  // BACKSPACE
    0x0009,  // HORIZONTAL TABULATION
    0x000A,  // LINE FEED
    0x000B,  // VERTICAL TABULATION
    0x000C,  // FORM FEED
    0x000D,  // CARRIAGE RETURN
    0x000E,  // SHIFT OUT
    0x000F,  // SHIFT IN
    0x0010,  // DATA LINK ESCAPE
    0x0011,  // DEVICE CONTROL ONE
    0x0012,  // DEVICE CONTROL TWO
    0x0013,  // DEVICE CONTROL THREE
    0x0014,  // DEVICE CONTROL FOUR
    0x0015,  // NEGATIVE ACKNOWLEDGE
    0x0016,  // SYNCHRONOUS IDLE
    0x0017,  // END OF TRANSMISSION BLOCK
    0x0018,  // CANCEL
    0x0019,  // END OF MEDIUM
    0x001A,  // SUBSTITUTE
    0x001B,  // ESCAPE
    0x001C,  // FILE SEPARATOR
    0x001D,  // GROUP SEPARATOR
    0x001E,  // RECORD SEPARATOR
    0x001F,  // UNIT SEPARATOR
    0x0020,  // SPACE
    0x0021,  // EXCLAMATION MARK
    0x0022,  // QUOTATION MARK
    0x0023,  // NUMBER SIGN
    0x0024,  // DOLLAR SIGN
    0x0025,  // PERCENT SIGN
    0x0026,  // AMPERSAND
    0x0027,  // APOSTROPHE
    0x0028,  // LEFT PARENTHESIS
    0x0029,  // RIGHT PARENTHESIS
    0x002A,  // ASTERISK
    0x002B,  // PLUS SIGN
    0x002C,  // COMMA
    0x002D,  // HYPHEN-MINUS
    0x002E,  // FULL STOP
    0x002F,  // SOLIDUS
    0x0030,  // DIGIT ZERO
    0x0031,  // DIGIT ONE
    0x0032,  // DIGIT TWO
    0x0033,  // DIGIT THREE
    0x0034,  // DIGIT FOUR
    0x0035,  // DIGIT FIVE
    0x0036,  // DIGIT SIX
    0x0037,  // DIGIT SEVEN
    0x0038,  // DIGIT EIGHT
    0x0039,  // DIGIT NINE
    0x003A,  // COLON
    0x003B,  // SEMICOLON
    0x003C,  // LESS-THAN SIGN
    0x003D,  // EQUALS SIGN
    0x003E,  // GREATER-THAN SIGN
    0x003F,  // QUESTION MARK
    0x0040,  // COMMERCIAL AT
    0x0041,  // LATIN CAPITAL LETTER A
    0x0042,  // LATIN CAPITAL LETTER B
    0x0043,  // LATIN CAPITAL LETTER C
    0x0044,  // LATIN CAPITAL LETTER D
    0x0045,  // LATIN CAPITAL LETTER E
    0x0046,  // LATIN CAPITAL LETTER F
    0x0047,  // LATIN CAPITAL LETTER G
    0x0048,  // LATIN CAPITAL LETTER H
    0x0049,  // LATIN CAPITAL LETTER I
    0x004A,  // LATIN CAPITAL LETTER J
    0x004B,  // LATIN CAPITAL LETTER K
    0x004C,  // LATIN CAPITAL LETTER L
    0x004D,  // LATIN CAPITAL LETTER M
    0x004E,  // LATIN CAPITAL LETTER N
    0x004F,  // LATIN CAPITAL LETTER O
    0x0050,  // LATIN CAPITAL LETTER P
    0x0051,  // LATIN CAPITAL LETTER Q
    0x0052,  // LATIN CAPITAL LETTER R
    0x0053,  // LATIN CAPITAL LETTER S
    0x0054,  // LATIN CAPITAL LETTER T
    0x0055,  // LATIN CAPITAL LETTER U
    0x0056,  // LATIN CAPITAL LETTER V
    0x0057,  // LATIN CAPITAL LETTER W
    0x0058,  // LATIN CAPITAL LETTER X
    0x0059,  // LATIN CAPITAL LETTER Y
    0x005A,  // LATIN CAPITAL LETTER Z
    0x005B,  // LEFT SQUARE BRACKET
    0x005C,  // REVERSE SOLIDUS
    0x005D,  // RIGHT SQUARE BRACKET
    0x005E,  // CIRCUMFLEX ACCENT
    0x005F,  // LOW LINE
    0x0060,  // GRAVE ACCENT
    0x0061,  // LATIN SMALL LETTER A
    0x0062,  // LATIN SMALL LETTER B
    0x0063,  // LATIN SMALL LETTER C
    0x0064,  // LATIN SMALL LETTER D
    0x0065,  // LATIN SMALL LETTER E
    0x0066,  // LATIN SMALL LETTER F
    0x0067,  // LATIN SMALL LETTER G
    0x0068,  // LATIN SMALL LETTER H
    0x0069,  // LATIN SMALL LETTER I
    0x006A,  // LATIN SMALL LETTER J
    0x006B,  // LATIN SMALL LETTER K
    0x006C,  // LATIN SMALL LETTER L
    0x006D,  // LATIN SMALL LETTER M
    0x006E,  // LATIN SMALL LETTER N
    0x006F,  // LATIN SMALL LETTER O
    0x0070,  // LATIN SMALL LETTER P
    0x0071,  // LATIN SMALL LETTER Q
    0x0072,  // LATIN SMALL LETTER R
    0x0073,  // LATIN SMALL LETTER S
    0x0074,  // LATIN SMALL LETTER T
    0x0075,  // LATIN SMALL LETTER U
    0x0076,  // LATIN SMALL LETTER V
    0x0077,  // LATIN SMALL LETTER W
    0x0078,  // LATIN SMALL LETTER X
    0x0079,  // LATIN SMALL LETTER Y
    0x007A,  // LATIN SMALL LETTER Z
    0x007B,  // LEFT CURLY BRACKET
    0x007C,  // VERTICAL LINE
    0x007D,  // RIGHT CURLY BRACKET
    0x007E,  // TILDE
    0x007F,  // DELETE
    0x0080,  // <control>
    0x0081,  // <control>
    0x0082,  // <control>
    0x0083,  // <control>
    0x0084,  // <control>
    0x0085,  // <control>
    0x0086,  // <control>
    0x0087,  // <control>
    0x0088,  // <control>
    0x0089,  // <control>
    0x008A,  // <control>
    0x008B,  // <control>
    0x008C,  // <control>
    0x008D,  // <control>
    0x008E,  // <control>
    0x008F,  // <control>
    0x0090,  // <control>
    0x0091,  // <control>
    0x0092,  // <control>
    0x0093,  // <control>
    0x0094,  // <control>
    0x0095,  // <control>
    0x0096,  // <control>
    0x0097,  // <control>
    0x0098,  // <control>
    0x0099,  // <control>
    0x009A,  // <control>
    0x009B,  // <control>
    0x009C,  // <control>
    0x009D,  // <control>
    0x009E,  // <control>
    0x009F,  // <control>
    0x00A0,  // NO-BREAK SPACE
    0x0104,  // LATIN CAPITAL LETTER A WITH OGONEK
    0x02D8,  // BREVE
    0x0141,  // LATIN CAPITAL LETTER L WITH STROKE
    0x00A4,  // CURRENCY SIGN
    0x013D,  // LATIN CAPITAL LETTER L WITH CARON
    0x015A,  // LATIN CAPITAL LETTER S WITH ACUTE
    0x00A7,  // SECTION SIGN
    0x00A8,  // DIAERESIS
    0x0160,  // LATIN CAPITAL LETTER S WITH CARON
    0x015E,  // LATIN CAPITAL LETTER S WITH CEDILLA
    0x0164,  // LATIN CAPITAL LETTER T WITH CARON
    0x0179,  // LATIN CAPITAL LETTER Z WITH ACUTE
    0x00AD,  // SOFT HYPHEN
    0x017D,  // LATIN CAPITAL LETTER Z WITH CARON
    0x017B,  // LATIN CAPITAL LETTER Z WITH DOT ABOVE
    0x00B0,  // DEGREE SIGN
    0x0105,  // LATIN SMALL LETTER A WITH OGONEK
    0x02DB,  // OGONEK
    0x0142,  // LATIN SMALL LETTER L WITH STROKE
    0x00B4,  // ACUTE ACCENT
    0x013E,  // LATIN SMALL LETTER L WITH CARON
    0x015B,  // LATIN SMALL LETTER S WITH ACUTE
    0x02C7,  // CARON
    0x00B8,  // CEDILLA
    0x0161,  // LATIN SMALL LETTER S WITH CARON
    0x015F,  // LATIN SMALL LETTER S WITH CEDILLA
    0x0165,  // LATIN SMALL LETTER T WITH CARON
    0x017A,  // LATIN SMALL LETTER Z WITH ACUTE
    0x02DD,  // DOUBLE ACUTE ACCENT
    0x017E,  // LATIN SMALL LETTER Z WITH CARON
    0x017C,  // LATIN SMALL LETTER Z WITH DOT ABOVE
    0x0154,  // LATIN CAPITAL LETTER R WITH ACUTE
    0x00C1,  // LATIN CAPITAL LETTER A WITH ACUTE
    0x00C2,  // LATIN CAPITAL LETTER A WITH CIRCUMFLEX
    0x0102,  // LATIN CAPITAL LETTER A WITH BREVE
    0x00C4,  // LATIN CAPITAL LETTER A WITH DIAERESIS
    0x0139,  // LATIN CAPITAL LETTER L WITH ACUTE
    0x0106,  // LATIN CAPITAL LETTER C WITH ACUTE
    0x00C7,  // LATIN CAPITAL LETTER C WITH CEDILLA
    0x010C,  // LATIN CAPITAL LETTER C WITH CARON
    0x00C9,  // LATIN CAPITAL LETTER E WITH ACUTE
    0x0118,  // LATIN CAPITAL LETTER E WITH OGONEK
    0x00CB,  // LATIN CAPITAL LETTER E WITH DIAERESIS
    0x011A,  // LATIN CAPITAL LETTER E WITH CARON
    0x00CD,  // LATIN CAPITAL LETTER I WITH ACUTE
    0x00CE,  // LATIN CAPITAL LETTER I WITH CIRCUMFLEX
    0x010E,  // LATIN CAPITAL LETTER D WITH CARON
    0x0110,  // LATIN CAPITAL LETTER D WITH STROKE
    0x0143,  // LATIN CAPITAL LETTER N WITH ACUTE
    0x0147,  // LATIN CAPITAL LETTER N WITH CARON
    0x00D3,  // LATIN CAPITAL LETTER O WITH ACUTE
    0x00D4,  // LATIN CAPITAL LETTER O WITH CIRCUMFLEX
    0x0150,  // LATIN CAPITAL LETTER O WITH DOUBLE ACUTE
    0x00D6,  // LATIN CAPITAL LETTER O WITH DIAERESIS
    0x00D7,  // MULTIPLICATION SIGN
    0x0158,  // LATIN CAPITAL LETTER R WITH CARON
    0x016E,  // LATIN CAPITAL LETTER U WITH RING ABOVE
    0x00DA,  // LATIN CAPITAL LETTER U WITH ACUTE
    0x0170,  // LATIN CAPITAL LETTER U WITH DOUBLE ACUTE
    0x00DC,  // LATIN CAPITAL LETTER U WITH DIAERESIS
    0x00DD,  // LATIN CAPITAL LETTER Y WITH ACUTE
    0x0162,  // LATIN CAPITAL LETTER T WITH CEDILLA
    0x00DF,  // LATIN SMALL LETTER SHARP S
    0x0155,  // LATIN SMALL LETTER R WITH ACUTE
    0x00E1,  // LATIN SMALL LETTER A WITH ACUTE
    0x00E2,  // LATIN SMALL LETTER A WITH CIRCUMFLEX
    0x0103,  // LATIN SMALL LETTER A WITH BREVE
    0x00E4,  // LATIN SMALL LETTER A WITH DIAERESIS
    0x013A,  // LATIN SMALL LETTER L WITH ACUTE
    0x0107,  // LATIN SMALL LETTER C WITH ACUTE
    0x00E7,  // LATIN SMALL LETTER C WITH CEDILLA
    0x010D,  // LATIN SMALL LETTER C WITH CARON
    0x00E9,  // LATIN SMALL LETTER E WITH ACUTE
    0x0119,  // LATIN SMALL LETTER E WITH OGONEK
    0x00EB,  // LATIN SMALL LETTER E WITH DIAERESIS
    0x011B,  // LATIN SMALL LETTER E WITH CARON
    0x00ED,  // LATIN SMALL LETTER I WITH ACUTE
    0x00EE,  // LATIN SMALL LETTER I WITH CIRCUMFLEX
    0x010F,  // LATIN SMALL LETTER D WITH CARON
    0x0111,  // LATIN SMALL LETTER D WITH STROKE
    0x0144,  // LATIN SMALL LETTER N WITH ACUTE
    0x0148,  // LATIN SMALL LETTER N WITH CARON
    0x00F3,  // LATIN SMALL LETTER O WITH ACUTE
    0x00F4,  // LATIN SMALL LETTER O WITH CIRCUMFLEX
    0x0151,  // LATIN SMALL LETTER O WITH DOUBLE ACUTE
    0x00F6,  // LATIN SMALL LETTER O WITH DIAERESIS
    0x00F7,  // DIVISION SIGN
    0x0159,  // LATIN SMALL LETTER R WITH CARON
    0x016F,  // LATIN SMALL LETTER U WITH RING ABOVE
    0x00FA,  // LATIN SMALL LETTER U WITH ACUTE
    0x0171,  // LATIN SMALL LETTER U WITH DOUBLE ACUTE
    0x00FC,  // LATIN SMALL LETTER U WITH DIAERESIS
    0x00FD,  // LATIN SMALL LETTER Y WITH ACUTE
    0x0163,  // LATIN SMALL LETTER T WITH CEDILLA
    0x02D9  // DOT ABOVE
};

const pdf_utf16be* PdfIso88592Encoding::GetToUnicodeTable() const
{
    return PdfIso88592Encoding::s_cEncoding;
}

}; /* namespace PoDoFo */
