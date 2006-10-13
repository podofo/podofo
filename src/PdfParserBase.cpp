/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#include "PdfParserBase.h"
#include "PdfInputDevice.h"

namespace {

const char * genDelMap();
const char * genWsMap();

}

namespace PoDoFo {

const char * const PdfParserBase::m_delimiterMap = genDelMap();
const char * const PdfParserBase::m_whitespaceMap = genWsMap();

#define PDF_BUFFER             4096

PdfParserBase::PdfParserBase()
    : m_buffer( PDF_BUFFER )
{
}

PdfParserBase::PdfParserBase( const PdfRefCountedInputDevice & rDevice, const PdfRefCountedBuffer & rBuffer )
    : m_device( rDevice ), m_buffer( rBuffer )
{
}

PdfParserBase::~PdfParserBase()
{
}

long PdfParserBase::GetNextNumberFromFile()
{
    long  l;
    int   c; 
    int   counter = 0;
    char* end;

    if( !m_device.Device() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    while( (c = m_device.Device()->GetChar()) != EOF && counter < m_buffer.GetSize() )
    {
        if( !counter && IsWhitespace( c ) )
            continue;
        else if( c >= '0' && c <= '9' )
        {
            m_buffer.GetBuffer()[counter] = c;
            ++counter;
        }
        else
            break;
    }

    m_buffer.GetBuffer()[counter] = '\0';

    if( c == EOF && !counter )
    {
        RAISE_ERROR( ePdfError_UnexpectedEOF );
    }

    l = strtol( m_buffer.GetBuffer(), &end, 10 );
    if( end == m_buffer.GetBuffer() )
    {
        RAISE_ERROR( ePdfError_NoNumber );
    }

    return l;
}

const char* PdfParserBase::GetNextStringFromFile()
{
    int c; 
    int counter = 0;

    if( !m_device.Device() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    while( (c = m_device.Device()->Look()) != EOF && counter < m_buffer.GetSize() )
    {
        if( !counter && IsWhitespace( c ) )
        {
            // retrieve c really from stream
            c = m_device.Device()->GetChar();
            continue;
        }
        else if( counter && (IsWhitespace( c ) || IsDelimiter( c )) )
        {
            // do nothing
            break;
        }
        else
        {
            // retrieve c really from stream
            c = m_device.Device()->GetChar();
            m_buffer.GetBuffer()[counter] = c;
            ++counter;
        }
    }

    m_buffer.GetBuffer()[counter] = '\0';

    if( c == EOF && !counter )
    {
        RAISE_ERROR( ePdfError_UnexpectedEOF );
    }

    return m_buffer.GetBuffer();
}

};

namespace {

// Generate the delimiter character map at runtime
// so that it can be derived from the more easily
// maintainable structures in PdfDefines.h
const char * genDelMap()
{
    char * map = static_cast<char*>(malloc(256));
    for (int i = 0; i < 256; i++)
        map[i] = '\0';
    for (int i = 0; i < PoDoFo::s_nNumDelimiters; ++i)
        map[PoDoFo::s_cDelimiters[i]] = 1;
    return map;
}

// Generate the whitespace character map at runtime
// so that it can be derived from the more easily
// maintainable structures in PdfDefines.h
const char * genWsMap()
{
    char * map = static_cast<char*>(malloc(256));
    for (int i = 0; i < 256; i++)
        map[i] = '\0';
    for (int i = 0; i < PoDoFo::s_nNumWhiteSpaces; ++i)
        map[PoDoFo::s_cWhiteSpaces[i]] = 1;
    return map;
}

};
