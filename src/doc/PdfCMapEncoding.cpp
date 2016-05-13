/***************************************************************************
 *   Copyright (C) 2011 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   Pdf CMAP encoding by kalyan                                           *
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

#include "PdfCMapEncoding.h"
#include "base/PdfDefinesPrivate.h"
#include "base/PdfEncodingFactory.h"
#include "base/PdfObject.h"
#include "base/PdfVariant.h"
#include "base/PdfLocale.h"
#include "base/PdfStream.h"
#include "base/PdfContentsTokenizer.h"


#include <iostream>
#include <stack>
#include <iomanip>
#include <string>

using namespace std;

namespace PoDoFo
{


PdfCMapEncoding::PdfCMapEncoding (PdfObject * pObject, PdfObject * pToUnicode) : PdfEncoding(0x0000, 0xffff, pToUnicode), PdfElement(NULL, pObject), m_baseEncoding( eBaseEncoding_Font )
{
    if (pObject && pObject->HasStream())
    {
        std::stack<std::string> stkToken;
        pdf_uint16 loop = 0;
        char *streamBuffer;
        const char *streamToken = NULL;
        EPdfTokenType *streamTokenType = NULL;
        pdf_long streamBufferLen;
        bool in_begincidrange = 0;
        bool in_begincidchar = 0;
        pdf_uint16 range_entries = 0;
        pdf_uint16 char_entries = 0;
        pdf_uint16 inside_hex_string = 0;
        pdf_uint16 inside_array = 0;
        pdf_uint16 range_start = 0;
        pdf_uint16 range_end = 0;
        pdf_uint16 i = 0;
        pdf_utf16be firstvalue = 0;
        const PdfStream *CIDStreamdata = pObject->GetStream ();
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
                
                i++;

            }

            if (strcmp (streamToken, "]") == 0)
            {
                if (inside_array == 0)
                    PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidStream, "CMap Error, got ] before [")
                    else
                        inside_array = 0;
                
                i++;

            }
            
            if (in_begincidrange == 1)
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
                            for (int k = range_start; k <= range_end; k++)
                            {
                                m_cMap[k] = num_value;
                                num_value++;
                            }

                            loop++;

                        }
                    }
                }

            }



            if (in_begincidchar == 1)
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
                            m_cMap[firstvalue] = num_value;
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





            
            
            if (strcmp (streamToken, "begincidrange") == 0)
            {
                i = loop = 0;
                in_begincidrange = 1;
                stkToken.pop ();
                std::stringstream ss;
                ss << std::hex << stkToken.top ();
                ss >> range_entries;

            }
            
            if (strcmp (streamToken, "endcidrange") == 0)
            {
                in_begincidrange = 0;
                i = 0;
            }
            
            if (strcmp (streamToken, "begincidchar") == 0)
            {
                i = loop = 0;
                in_begincidchar = 1;
                stkToken.pop ();
                std::stringstream ss;
                ss << std::hex << stkToken.top ();
                ss >> char_entries;

            }
            
            if (strcmp (streamToken, "endcidchar") == 0)
            {
                in_begincidchar = 0;
                i = 0;
            }



        }
        
        podofo_free(streamBuffer);
    }


}

void PdfCMapEncoding::AddToDictionary(PdfDictionary &) const
{

}

const PdfEncoding* PdfCMapEncoding::GetBaseEncoding() const
{
    const PdfEncoding* pEncoding = NULL;

    switch( m_baseEncoding ) 
    {
        case eBaseEncoding_WinAnsi:
            pEncoding = PdfEncodingFactory::GlobalWinAnsiEncodingInstance();
            break;

        case eBaseEncoding_MacRoman:
            pEncoding = PdfEncodingFactory::GlobalMacRomanEncodingInstance();
            break;

        case eBaseEncoding_MacExpert:
        case eBaseEncoding_Font:
        default:
            break;
    }

    if( !pEncoding ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    return pEncoding;
}

PdfString PdfCMapEncoding::ConvertToUnicode(const PdfString & rEncodedString, const PdfFont* pFont) const
{

    if(m_bToUnicodeIsLoaded)
    {
        return PdfEncoding::ConvertToUnicode(rEncodedString, pFont);
    }
    else
        PODOFO_RAISE_ERROR( ePdfError_NotImplemented );
}

PdfRefCountedBuffer PdfCMapEncoding::ConvertToEncoding( const PdfString & rString, const PdfFont* pFont ) const
{
    if(m_bToUnicodeIsLoaded)
    {
        return PdfEncoding::ConvertToEncoding(rString, pFont);
    }
    else
        PODOFO_RAISE_ERROR( ePdfError_NotImplemented );
}


bool PdfCMapEncoding::IsSingleByteEncoding() const
{
	return false;
}


bool PdfCMapEncoding::IsAutoDelete() const
{
    return true;
}


pdf_utf16be PdfCMapEncoding::GetCharCode( int nIndex ) const
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


const PdfName & PdfCMapEncoding::GetID() const
{
    PODOFO_RAISE_ERROR( ePdfError_NotImplemented );
}

};
