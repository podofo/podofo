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

#include "PdfFont.h"

#include <sstream>
#include <iostream>
#include <stack>
#include <iomanip>
#include <string>

using namespace std;

namespace PoDoFo {

PdfIdentityEncoding::PdfIdentityEncoding( int nFirstChar, int nLastChar, bool bAutoDelete, PdfObject *pToUnicode )
    : PdfEncoding( nFirstChar, nLastChar, pToUnicode ), m_bAutoDelete( bAutoDelete )
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
    return ((nIndex & 0xFF00) >> 8) | ((nIndex & 0x00FF) << 8);
#else
    return static_cast<pdf_utf16be>(nIndex);
#endif // PODOFO_IS_LITTLE_ENDIAN
}

PdfString PdfIdentityEncoding::ConvertToUnicode( const PdfString & rEncodedString, const PdfFont* pFont ) const
{
    if(!m_toUnicode.empty())
    {
        return PdfEncoding::ConvertToUnicode(rEncodedString, pFont);
    }
    else {
        /* Identity-H means 1-1 mapping */	  
        //std::cout << "convertToUnicode(" << rEncodedString.IsUnicode() << ")" << std::endl;
        return ( rEncodedString.IsUnicode() ) ? PdfString(rEncodedString) : rEncodedString.ToUnicode();
    }
}

PdfRefCountedBuffer PdfIdentityEncoding::ConvertToEncoding( const PdfString & rString, const PdfFont* pFont ) const
{
    if(!m_toUnicode.empty())
    {
        return PdfEncoding::ConvertToEncoding(rString, pFont);
    }
    else if( pFont ) 
    {
        PdfString sStr = rString.ToUnicode();
        const pdf_utf16be* pStr = sStr.GetUnicode();
        PdfRefCountedBuffer buffer( sStr.GetLength() );
        char* outp = buffer.GetBuffer();
 
        // Get the string in UTF-16be format
        long  lGlyphId;
        while( *pStr ) 
        {
#ifdef PODOFO_IS_LITTLE_ENDIAN
            lGlyphId = pFont->GetFontMetrics()->GetGlyphId( (((*pStr << 8) & 0xFF00) | ((*pStr >> 8) & 0x00FF)) );
#else
            lGlyphId = pFont->GetFontMetrics()->GetGlyphId( *pStr );
#endif // PODOFO_IS_LITTLE_ENDIAN

            outp[0] = static_cast<char>(lGlyphId >> 8);
            outp[1] = static_cast<char>(lGlyphId);
            outp += 2;

            ++pStr;
        }
        return buffer;
    }
    else
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
        return PdfRefCountedBuffer();
    }
}
    
}; /* namespace PoDoFo */
