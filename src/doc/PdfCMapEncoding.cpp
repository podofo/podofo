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
#include "base/PdfStream.h"
#include "base/PdfContentsTokenizer.h"


#include <iostream>
#include <stack>
#include <iomanip>
#include <string>

using namespace std;

namespace PoDoFo
{


PdfCMapEncoding::PdfCMapEncoding (PdfObject * pObject, PdfObject * /*pToUnicode*/) : PdfEncoding(0x0000, 0xffff), PdfElement(NULL, pObject), m_baseEncoding( eBaseEncoding_Font )
{

    if(pObject->HasStream())
    {

        stack < string > stkToken;
        int loop = 0;
        char *streamBuffer;
        const char *streamToken = NULL;
        EPdfTokenType *streamTokenType = NULL;
        pdf_long streamBufferLen;
        bool in_codespacerange = 0;
        bool in_beginbfrange = 0;
        bool in_beginbfchar = 0;
        int code_space_entries = 0;
        int range_entries = 0;
        int char_entries = 0;
        int inside_hex_string = 0;
        int inside_array = 0;
        int range_start;
        int range_end;
        int i = 0;
        int firstvalue = 0;
        pair < long, long >encodingRange;
        const PdfStream *CIDStreamdata = pObject->GetStream ();
        CIDStreamdata->GetFilteredCopy (&streamBuffer, &streamBufferLen);

        PdfContentsTokenizer streamTokenizer (streamBuffer, streamBufferLen);
        while (streamTokenizer.GetNextToken (streamToken, streamTokenType))
        {
            stkToken.push (streamToken);

            if (strcmp (streamToken, ">") == 0)
            {
                if (inside_hex_string == 0)
                    cout << "\n Pdf Error, got > before <";
                else
                    inside_hex_string = 0;

                i++;

            }

            if (strcmp (streamToken, "]") == 0)
            {
                if (inside_array == 0)
                    cout << "\n Pdf Error, got ] before [";
                else
                    inside_array = 0;

                i++;

            }


            if (in_codespacerange == 1)
            {
                if (loop < code_space_entries)
                {
                    if (inside_hex_string == 1)
                    {
                        unsigned int num_value;
                        std::stringstream ss;
                        ss << std::hex << streamToken;
                        ss >> num_value;
                        if (i % 2 == 0)
                        {
                            encodingRange.first = num_value;
                        }
                        if (i % 2 == 1)
                        {
                            encodingRange.second = num_value;
                            loop++;
                        }



                    }

                }

            }



            if (in_beginbfrange == 1)
            {
                if (loop < range_entries)
                {
                    if (inside_hex_string == 1)
                    {
                        unsigned int num_value;
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
                                *(cMapEncoding + k) = num_value;
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
                        unsigned int num_value;
                        std::stringstream ss;
                        ss << std::hex << streamToken;
                        ss >> num_value;
                        if (i % 2 == 0)
                        {
                            firstvalue = num_value;
                        }
                        if (i % 2 == 1)
                        {
                            if (encodingRange.first <= static_cast<int>(num_value)
                                && static_cast<int>(num_value) <= encodingRange.second)
                                *(cMapEncoding + firstvalue-1) = num_value;
                            else
                                cout << "\n Error ... Value out of range" << endl;
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





            if (strcmp (streamToken, "begincodespacerange") == 0)
            {
                in_codespacerange = 1;
                stkToken.pop ();
                std::stringstream ss;
                ss << std::hex << stkToken.top ();
                ss >> code_space_entries;
            }

            if (strcmp (streamToken, "endcodespacerange") == 0)
            {
                if (in_codespacerange == 0)
                    cout <<
                        "\nError in pdf document got endcodespacerange before begincodespacerange";
                in_codespacerange = 0;
                cMapEncoding = static_cast <pdf_utf16be *>(calloc((encodingRange.second - encodingRange.first + 1), sizeof(pdf_utf16be)));
                i = 0;
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

        *(cMapEncoding + encodingRange.second-1) = 0;
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

PdfString PdfCMapEncoding::ConvertToUnicode(const PdfString & rEncodedString, const PdfFont*) const
{

	const PdfEncoding* const pEncoding = PdfEncodingFactory::GlobalPdfDocEncodingInstance();
	if(rEncodedString.IsHex())
	{
		PdfString PdfUnicodeString = rEncodedString.ToUnicode();
		char * ptrHexString = static_cast<char *>( malloc( sizeof(char) * (rEncodedString.GetLength() + 2 ) ) );
		pdf_utf16be* pszUtf16 = static_cast<pdf_utf16be*>(malloc(sizeof(pdf_utf16be)*rEncodedString.GetLength()));
		memcpy( ptrHexString, rEncodedString.GetString(), rEncodedString.GetLength() );
		for ( int strIndex = 0; strIndex < static_cast<int>(rEncodedString.GetLength()); strIndex++ )
        {
            *(pszUtf16 + strIndex) = pEncoding->GetCharCode(*(cMapEncoding + static_cast<int>(ptrHexString[strIndex]) -1));
        }
		PdfString ret(pszUtf16, rEncodedString.GetLength());
	   	free( ptrHexString ); 
		free( pszUtf16 );
		return ret;
	}
	else
	{

		return(PdfString("\0"));
	}
}

PdfRefCountedBuffer PdfCMapEncoding::ConvertToEncoding(const PdfString &, const PdfFont*) const
{
    PODOFO_RAISE_ERROR( ePdfError_NotImplemented );

    return PdfRefCountedBuffer();
}


bool PdfCMapEncoding::IsSingleByteEncoding() const
{
	return false;
}


bool PdfCMapEncoding::IsAutoDelete() const
{
    return true;
}


pdf_utf16be PdfCMapEncoding::GetCharCode(int) const
{
    PODOFO_RAISE_ERROR( ePdfError_NotImplemented );

    return static_cast<pdf_utf16be>(0);
}


const PdfName & PdfCMapEncoding::GetID() const
{
    PODOFO_RAISE_ERROR( ePdfError_NotImplemented );
}

};
