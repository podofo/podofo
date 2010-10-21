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
 ***************************************************************************/

#include "PdfIdentityEncoding.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfDictionary.h"
#include "base/PdfLocale.h"

#include "PdfFont.h"

#include <sstream>

namespace PoDoFo {

PdfIdentityEncoding::PdfIdentityEncoding( int nFirstChar, int nLastChar, bool bAutoDelete )
    : PdfEncoding( nFirstChar, nLastChar ), m_bAutoDelete( bAutoDelete )
{
    // create a unique ID
    std::ostringstream oss;
    oss << "/Identity-H" << nFirstChar << "_" << nLastChar;

    m_id = PdfName( oss.str() );
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
    // Get the string in UTF-16be format
    PdfString          sStr = rEncodedString.ToUnicode();
    const pdf_utf16be* pStr = sStr.GetUnicode();
    long               lUnicodeValue;

    std::ostringstream out;
    PdfLocaleImbue(out);

    while( *pStr ) 
    {
        lUnicodeValue = this->GetUnicodeValue( static_cast<long>(*(const_cast<pdf_utf16be*>(pStr))) );

#ifdef PODOFO_IS_LITTLE_ENDIAN
        out << static_cast<unsigned char>((lUnicodeValue & 0xff00) >> 8);
        out << static_cast<unsigned char>(lUnicodeValue & 0x00ff);
#else
        out << static_cast<unsigned char>(lUnicodeValue & 0x00ff);
        out << static_cast<unsigned char>((lUnicodeValue & 0xff00) >> 8);
#endif // PODOFO_IS_LITTLE_ENDIAN

        ++pStr;
    }

    return PdfString( out.str().c_str(), out.str().length() );;
}

PdfRefCountedBuffer PdfIdentityEncoding::ConvertToEncoding( const PdfString & rString, const PdfFont* pFont ) const
{
    if( !pFont ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // Get the string in UTF-16be format
    PdfString          sStr = rString.ToUnicode();
    const pdf_utf16be* pStr = sStr.GetUnicode();
    long               lGlyphId;

    std::ostringstream out;
    PdfLocaleImbue(out);

    while( *pStr ) 
    {
#ifdef PODOFO_IS_LITTLE_ENDIAN
        lGlyphId = pFont->GetFontMetrics()->GetGlyphId( (((*pStr & 0xff) << 8) | ((*pStr & 0xff00) >> 8)) );
#else
        lGlyphId = pFont->GetFontMetrics()->GetGlyphId( *pStr );
#endif // PODOFO_IS_LITTLE_ENDIAN

        out << static_cast<unsigned char>((lGlyphId & 0xff00) >> 8);
        out << static_cast<unsigned char>(lGlyphId & 0x00ff);

        ++pStr;
    }

    PdfRefCountedBuffer buffer( out.str().length() );
    memcpy( buffer.GetBuffer(), out.str().c_str(), out.str().length() );
    return buffer;
}

pdf_utf16be PdfIdentityEncoding::GetUnicodeValue( long lCharCode ) const
{
    return 0;
}

}; /* namespace PoDoFo */
