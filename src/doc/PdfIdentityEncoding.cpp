/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
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

#include "PdfIdentityEncoding.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfDictionary.h"
#include "base/PdfLocale.h"
#include "base/PdfStream.h"
#include "base/PdfContentsTokenizer.h"

#include "PdfFont.h"

#include <sstream>
#include <iostream>
#include <stack>
#include <iomanip>
#include <string>

using namespace std;

namespace PoDoFo {

PdfIdentityEncoding::PdfIdentityEncoding( int nFirstChar, int nLastChar, bool bAutoDelete, PdfObject *pToUnicode )
    : PdfEncoding( nFirstChar, nLastChar ), m_bAutoDelete( bAutoDelete ), m_pToUnicode(pToUnicode), m_bToUnicodeIsLoaded(false)
{
    // create a unique ID
    std::ostringstream oss;
    oss << "/Identity-H" << nFirstChar << "_" << nLastChar;

    m_id = PdfName( oss.str() );
    
    ParseToUnicode();
}

void PdfIdentityEncoding::AddToDictionary( PdfDictionary & rDictionary ) const
{
    rDictionary.AddKey( "Encoding", PdfName("Identity-H") );
}

pdf_utf16be PdfIdentityEncoding::GetCharCode( int nIndex ) const
{
    if( nIndex < this->GetFirstChar() ||
	nIndex > this->GetLastChar() )
    {
	PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

#ifdef PODOFO_IS_LITTLE_ENDIAN
    return ((nIndex & 0xff00) >> 8) | ((nIndex & 0xff) << 8);
#else
    return static_cast<pdf_utf16be>(nIndex);
#endif // PODOFO_IS_LITTLE_ENDIAN
}

PdfString PdfIdentityEncoding::ConvertToUnicode( const PdfString & rEncodedString, const PdfFont* ) const
{
    if(m_pToUnicode && m_bToUnicodeIsLoaded)
    {
        
        const pdf_utf16be* pStr = reinterpret_cast<const pdf_utf16be*>(rEncodedString.GetString());
        const size_t lLen = rEncodedString.GetLength()/2;
        pdf_utf16be lCID, lUnicodeValue;
        
        pdf_utf16be* pszUtf16 = static_cast<pdf_utf16be*>(malloc(sizeof(pdf_utf16be)*lLen));
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
        free( pszUtf16 );
        
        return ret;
        
    }
    else
        return(PdfString("\0"));
}

PdfRefCountedBuffer PdfIdentityEncoding::ConvertToEncoding( const PdfString & rString, const PdfFont* pFont ) const
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

pdf_utf16be PdfIdentityEncoding::GetUnicodeValue( pdf_utf16be  value ) const
{
    if(m_bToUnicodeIsLoaded)
    {
        const map<pdf_utf16be, pdf_utf16be>::const_iterator found = m_cMapEncoding.find(value);
        return (found == m_cMapEncoding.end() ? 0 : found->second);
    }
    else
        return 0;
}
    
pdf_utf16be PdfIdentityEncoding::GetCIDValue( pdf_utf16be lUnicodeValue ) const
{
    if(m_bToUnicodeIsLoaded)
    {
        // TODO: optimize
        for(map<pdf_utf16be, pdf_utf16be>::const_iterator it = m_cMapEncoding.begin(); it != m_cMapEncoding.end(); ++it)
            if(it->second == lUnicodeValue)
                return it->first;
    }
    
    return 0;
}

void PdfIdentityEncoding::ParseToUnicode()
{
    if (m_pToUnicode && m_pToUnicode->HasStream())
    {
        stack<string> stkToken;
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
        pdf_uint16 range_start;
        pdf_uint16 range_end;
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
                    PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidStream, "Pdf Error, got > before <")
                else
                    inside_hex_string = 0;
                
                i++;
                
            }
            
            if (strcmp (streamToken, "]") == 0)
            {
                if (inside_array == 0)
                    PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidStream, "Pdf Error, got ] before [")
                else
                    inside_array = 0;
                
                i++;
                
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
                            for (int k = range_start; k < range_end; k++)
                            {
                                m_cMapEncoding[k] = num_value;
                                num_value++;
                            }
                            
                            loop++;
                            
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
                            m_cMapEncoding[firstvalue] = num_value;
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
        
        free(streamBuffer);
        
        m_bToUnicodeIsLoaded = true;
    }
}
    
}; /* namespace PoDoFo */
