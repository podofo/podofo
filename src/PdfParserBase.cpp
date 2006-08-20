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

namespace PoDoFo {

#define PDF_BUFFER             4096

PdfParserBase::PdfParserBase()
    : m_buffer( PDF_BUFFER )
{
}

PdfParserBase::PdfParserBase( const PdfRefCountedFile & rFile, const PdfRefCountedBuffer & rBuffer )
    : m_file( rFile ), m_buffer( rBuffer )
{
}

PdfParserBase::~PdfParserBase()
{
}

bool PdfParserBase::IsDelimiter( const char c )
{
/*
    char* p = s_cDelimiters;

    while( p )
    {
        if( c == *p )
            return true;
        ++p;
    }
*/

    int i;
    
    for( i=0; i<s_nNumDelimiters; i++ )
        if( c == s_cDelimiters[i] )
            return true;

    return false;
}

bool PdfParserBase::IsWhitespace( const char c )
{
    int i;
    
    for( i=0; i<s_nNumWhiteSpaces; i++ )
        if( c == s_cWhiteSpaces[i] )
            return true;

    return false;
}

long PdfParserBase::GetNextNumberFromFile()
{
    long  l;
    int   c; 
    int   counter = 0;
    char* end;

    if( !m_file.Handle() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    while( (c = fgetc( m_file.Handle() )) != EOF && counter < m_buffer.Size() )
    {
        if( !counter && IsWhitespace( c ) )
            continue;
        else if( c >= '0' && c <= '9' )
        {
            m_buffer.Buffer()[counter] = c;
            ++counter;
        }
        else
            break;
    }

    m_buffer.Buffer()[counter] = '\0';

    if( c == EOF && !counter )
    {
        RAISE_ERROR( ePdfError_UnexpectedEOF );
    }

    l = strtol( m_buffer.Buffer(), &end, 10 );
    if( end == m_buffer.Buffer() )
    {
        RAISE_ERROR( ePdfError_NoNumber );
    }

    return l;
}

const char* PdfParserBase::GetNextStringFromFile()
{
    int c; 
    int counter = 0;

    if( !m_file.Handle() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    while( (c = fgetc( m_file.Handle() )) != EOF && counter < m_buffer.Size() )
    {
        if( !counter && IsWhitespace( c ) )
            continue;
        else if( counter && (IsWhitespace( c ) || IsDelimiter( c )) )
        {
            // push c back onto the stream
            ungetc( c, m_file.Handle() );
            break;
        }
        else
        {
            m_buffer.Buffer()[counter] = c;
            ++counter;
        }
    }

    m_buffer.Buffer()[counter] = '\0';

    if( c == EOF && !counter )
    {
        RAISE_ERROR( ePdfError_UnexpectedEOF );
    }

    return m_buffer.Buffer();
}

};


