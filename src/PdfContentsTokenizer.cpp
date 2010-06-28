/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "PdfContentsTokenizer.h"

#include "PdfCanvas.h"
#include "PdfDocument.h"
#include "PdfInputDevice.h"
#include "PdfOutputStream.h"
#include "PdfStream.h"
#include "PdfVecObjects.h"
#include "PdfData.h"
#include "PdfDefinesPrivate.h"

#include <iostream>

namespace PoDoFo {

PdfContentsTokenizer::PdfContentsTokenizer( PdfCanvas* pCanvas )
    : PdfTokenizer(), m_readingInlineImgData(false)
{
    if( !pCanvas ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    PdfObject* pContents = pCanvas->GetContents();
    if( pContents && pContents->IsArray()  )
    {
        PdfArray& a = pContents->GetArray();
        for ( PdfArray::iterator it = a.begin(); it != a.end() ; ++it )
        {
            if ( !(*it).IsReference() )
            {
                PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "/Contents array contained non-references" );

            }

            m_lstContents.push_back( pContents->GetOwner()->GetObject( (*it).GetReference() ) );
        }
    }
    else if ( pContents && pContents->HasStream() )
    {
        m_lstContents.push_back( pContents );
    }
    else
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "Page /Contents not stream or array of streams" );
    }

    if( m_lstContents.size() )
    {
        SetCurrentContentsStream( m_lstContents.front() );
        m_lstContents.pop_front();
    }
}

void PdfContentsTokenizer::SetCurrentContentsStream( PdfObject* pObject )
{
    PODOFO_RAISE_LOGIC_IF( pObject == NULL, "Content stream object == NULL!" );

    PdfStream* pStream = pObject->GetStream();

    PdfBufferOutputStream stream( &m_curBuffer );
    pStream->GetFilteredCopy( &stream );

    m_device = PdfRefCountedInputDevice( m_curBuffer.GetBuffer(), m_curBuffer.GetSize() );
}

bool PdfContentsTokenizer::ReadNext( EPdfContentsType& reType, const char*& rpszKeyword, PdfVariant & rVariant )
{
    if (m_readingInlineImgData)
        return ReadInlineImgData(reType, rpszKeyword, rVariant);
    EPdfTokenType eTokenType;
    EPdfDataType  eDataType;
    const char*   pszToken;

    // While officially the keyword pointer is undefined if not needed, it
    // costs us practically nothing to zero it (in case someone fails to check
    // the return value and/or reType). Do so. We won't nullify the variant
    // since that has a real cost.
    //rpszKeyword = 0;

    // If we've run out of data in this stream and there's another one to read,
    // switch to reading the next stream.
    //if( m_device.Device() && m_device.Device()->Eof() && m_lstContents.size() )
    //{
    //    SetCurrentContentsStream( m_lstContents.front() );
    //    m_lstContents.pop_front();
    //}

    bool gotToken = this->GetNextToken( pszToken, &eTokenType );
    if ( !gotToken )
    {
        if ( m_lstContents.size() )
        {
        // We ran out of tokens in this stream. Switch to the next stream
        // and try again.
            SetCurrentContentsStream( m_lstContents.front() );
            m_lstContents.pop_front();
            return ReadNext( reType, rpszKeyword, rVariant );
        }
        else
        {
            // No more content stream tokens to read.
            return false;
        }
    }

    eDataType = this->DetermineDataType( pszToken, eTokenType, rVariant );

    // asume we read a variant unless we discover otherwise later.
    reType = ePdfContentsType_Variant;

    switch( eDataType )
    {
        case ePdfDataType_Null:
        case ePdfDataType_Bool:
        case ePdfDataType_Number:
        case ePdfDataType_Real:
            // the data was already read into rVariant by the DetermineDataType function
            break;

        case ePdfDataType_Reference:
        {
            // references are invalid in content streams
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "references are invalid in content streams" );
            break;
        }

        case ePdfDataType_Dictionary:
            this->ReadDictionary( rVariant, NULL );
            break;
        case ePdfDataType_Array:
            this->ReadArray( rVariant, NULL );
            break;
        case ePdfDataType_String:
            this->ReadString( rVariant, NULL );
            break;
        case ePdfDataType_HexString:
            this->ReadHexString( rVariant, NULL );
            break;
        case ePdfDataType_Name:
            this->ReadName( rVariant );
            break;

        case ePdfDataType_Unknown:
        case ePdfDataType_RawData:
        default:
            // Assume we have a keyword
            reType     = ePdfContentsType_Keyword;
            rpszKeyword = pszToken;
            break;
    }
    std::string idKW ("ID");
    if ((reType == ePdfContentsType_Keyword) && (idKW.compare(rpszKeyword) == 0) )
        m_readingInlineImgData = true;
    return true;
}

bool PdfContentsTokenizer::ReadInlineImgData( EPdfContentsType& reType, const char*& rpszKeyword, PdfVariant & rVariant )
{
    int  c;
    long long  counter  = 0;
    if( !m_device.Device() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // cosume the only whitespace between ID and data
    c = m_device.Device()->Look();
    if( PdfTokenizer::IsWhitespace( c ) )
    {
        c = m_device.Device()->GetChar();
    }

    while( (c = m_device.Device()->Look()) != EOF
           && counter < static_cast<long long>(m_buffer.GetSize()) )
    {
        if (PdfTokenizer::IsWhitespace(c))
        {
            // test if end-of-image-data is reached (hit EI keyword)
            c = m_device.Device()->GetChar(); // skip the white space
            char e = m_device.Device()->GetChar();
            char i = m_device.Device()->GetChar();
            m_device.Device()->Seek(-2, std::ios::cur);
            if (e == 'E' && i == 'I')
            {
                m_buffer.GetBuffer()[counter] = '\0';
                rVariant = PdfData(m_buffer.GetBuffer(), static_cast<size_t>(counter));
                reType = ePdfContentsType_ImageData;
                m_readingInlineImgData = false;
                return true;
            }
            m_buffer.GetBuffer()[counter] = c;
            ++counter;
        }
        else
        {
            c = m_device.Device()->GetChar();
            m_buffer.GetBuffer()[counter] = c;
            ++counter;
        }
    }
    return false;
}
};
