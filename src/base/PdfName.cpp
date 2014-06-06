/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#include "PdfName.h"

#include "PdfOutputDevice.h"
#include "PdfTokenizer.h"
#include "PdfDefinesPrivate.h"

#include <string.h>

using PoDoFo::ePdfError_InvalidName;

namespace {

/**
 * This function writes a hex encoded representation of the character
 * `ch' to `buf', advancing the iterator by two steps.
 *
 * \warning no buffer length checking is performed, so MAKE SURE
 *          you have enough room for the two characters that
 *          will be written to the buffer.
 *
 * \param ch The character to write a hex representation of
 * \param buf An iterator (eg a char* or std::string::iterator) to write the
 *            characters to.  Must support the postfix ++, operator=(char) and
 *            dereference operators.
 */
template<typename T>
inline void hexchr(const unsigned char ch, T & it)
{
    *(it++) = "0123456789ABCDEF"[ch / 16];
    *(it++) = "0123456789ABCDEF"[ch % 16];
}

/** Escape the input string according to the PDF name
 *  escaping rules and return the result.
 *
 *  \param it Iterator referring to the start of the input string
 *            ( eg a `const char *' or a `std::string::iterator' )
 *  \param length Length of input string
 *  \returns Escaped string
 */
template<typename T>
static std::string EscapeName(T it, size_t length)
{
    // Scan the input string once to find out how much memory we need
    // to reserve for the encoded result string. We could do this in one
    // pass using a std::ostringstream instead, but it's a LOT slower.
    T it2(it);
    unsigned int outchars = 0;
    for (size_t i = 0; i < length; ++i)
    {
        // Null chars are illegal in names, even escaped
        if (*it2 == '\0')
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidName, "Null byte in PDF name is illegal");
        }
        else 
        {
            // Leave room for either just the char, or a #xx escape of it.
            outchars += (::PoDoFo::PdfTokenizer::IsRegular(*it2) && 
                         ::PoDoFo::PdfTokenizer::IsPrintable(*it2) && (*it2 != '#')) ? 1 : 3;
        }
        ++it2;
    }
    // Reserve it. We can't use reserve() because the GNU STL doesn't seem to
    // do it correctly; the memory never seems to get allocated.
    std::string buf;
    buf.resize(outchars);
    // and generate the encoded string
    std::string::iterator bufIt(buf.begin());
    for (size_t z = 0; z < length; ++z)
    {
        if (::PoDoFo::PdfTokenizer::IsRegular(*it) && 
            ::PoDoFo::PdfTokenizer::IsPrintable(*it) && 
            (*it != '#') )
            *(bufIt++) = *it;
        else
        {
            *(bufIt++) = '#';
            hexchr(static_cast<unsigned char>(*it), bufIt);
        }
        ++it;
    }
    return buf;
}

/** Interpret the passed string as an escaped PDF name
 *  and return the unescaped form.
 *
 *  \param it Iterator referring to the start of the input string
 *            ( eg a `const char *' or a `std::string::iterator' )
 *  \param length Length of input string
 *  \returns Unescaped string
 */
template<typename T>
static std::string UnescapeName(T it, size_t length)
{
    // We know the decoded string can be AT MOST
    // the same length as the encoded one, so:
    std::string buf;
    buf.resize(length);
    unsigned int incount = 0, outcount = 0;
    while (incount++ < length)
    {
        if (*it == '#')
        {
            unsigned char hi = static_cast<unsigned char>(*(++it)); ++incount;
            unsigned char low = static_cast<unsigned char>(*(++it)); ++incount;
            hi  -= ( hi  < 'A' ? '0' : 'A'-10 );
            low -= ( low < 'A' ? '0' : 'A'-10 );
            buf[outcount++] = (hi << 4) | (low & 0x0F);
        }
        else
            buf[outcount++] = *it;
        ++it;
    }
    // Chop buffer off at number of decoded bytes
    buf.resize(outcount);
    return buf;
}

}; // End anonymous namespace

namespace PoDoFo {

const PdfName PdfName::KeyContents  = PdfName( "Contents" );
const PdfName PdfName::KeyFlags     = PdfName( "Flags" );
const PdfName PdfName::KeyLength    = PdfName( "Length" );
const PdfName PdfName::KeyNull      = PdfName();
const PdfName PdfName::KeyRect      = PdfName( "Rect" );
const PdfName PdfName::KeySize      = PdfName( "Size" );
const PdfName PdfName::KeySubtype   = PdfName( "Subtype" );
const PdfName PdfName::KeyType      = PdfName( "Type" );
const PdfName PdfName::KeyFilter    = PdfName( "Filter" );

PdfName::~PdfName()
{
}

PdfName PdfName::FromEscaped( const std::string & sName )
{
    return PdfName(UnescapeName(sName.begin(), sName.length()));
}

PdfName PdfName::FromEscaped( const char * pszName, pdf_long ilen )
{
    if( !ilen && pszName )
        ilen = strlen( pszName );

    return PdfName(UnescapeName(pszName, ilen));
}

void PdfName::Write( PdfOutputDevice* pDevice, EPdfWriteMode, const PdfEncrypt* ) const
{
    // Allow empty names, which are legal according to the PDF specification
    pDevice->Print( "/" );
    if( m_Data.length() )
    {
        std::string escaped( EscapeName(m_Data.begin(), m_Data.length()) );
        pDevice->Write( escaped.c_str(), escaped.length() );
    }
}

std::string PdfName::GetEscapedName() const
{
    return EscapeName(m_Data.begin(), m_Data.length());
}

bool PdfName::operator==( const char* rhs ) const
{
    /*
      If the string is empty and you pass NULL - that's equivalent
      If the string is NOT empty and you pass NULL - that's not equal
      Otherwise, compare them
    */
    if( m_Data.empty() && !rhs )
        return true;
    else if( !m_Data.empty() && !rhs )
        return false;
    else
        return ( m_Data == std::string( rhs ) );
}

};

