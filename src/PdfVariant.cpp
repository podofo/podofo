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

#include "PdfVariant.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfOutputDevice.h"
#include "PdfParserObject.h"

#include <sstream>

#define NULL_LENGTH  4
#define TRUE_LENGTH  4
#define FALSE_LENGTH 5

namespace PoDoFo {

using namespace std;

PdfVariant PdfVariant::NullValue;

PdfVariant::PdfVariant()
    : m_pData( NULL )
{
    Clear();

    m_eDataType = ePdfDataType_Null;
}

PdfVariant::PdfVariant( bool b )
    : m_pData( NULL )
{
    Clear();

    m_eDataType       = ePdfDataType_Bool;
    m_Data.bBoolValue = b;
}

PdfVariant::PdfVariant( long l )
    : m_pData( NULL )
{
    Clear();

    m_eDataType       = ePdfDataType_Number;
    m_Data.nNumber    = l;
}

PdfVariant::PdfVariant( double d )
    : m_pData( NULL )
{
    Clear();

    m_eDataType       = ePdfDataType_Real;
    m_Data.dNumber    = d;    
}

PdfVariant::PdfVariant( const PdfString & rsString )
    : m_pData( NULL )
{
    Clear();

    m_eDataType = rsString.IsHex() ? ePdfDataType_HexString : ePdfDataType_String;
    m_pData     = new PdfString( rsString );
}

PdfVariant::PdfVariant( const PdfName & rName )
    : m_pData( NULL )
{
    Clear();

    m_eDataType = ePdfDataType_Name;
    m_pData     = new PdfName( rName );
}

PdfVariant::PdfVariant( const PdfReference & rRef )
    : m_pData( NULL )
{
    Clear();

    m_eDataType = ePdfDataType_Reference;
    m_pData     = new PdfReference( rRef );
}

PdfVariant::PdfVariant( const PdfArray & rArray )
    : m_pData( NULL )
{
    Clear();

    m_eDataType = ePdfDataType_Array;
    m_pData     = new PdfArray( rArray );
}

PdfVariant::PdfVariant( const PdfDictionary & rObj )
    : m_pData( NULL )
{
    Clear();

    m_eDataType = ePdfDataType_Dictionary;
    m_pData     = new PdfDictionary( rObj );
}

PdfVariant::PdfVariant( const PdfVariant & rhs )
    : m_pData( NULL )
{
    this->operator=(rhs);
}

PdfVariant::~PdfVariant()
{
    Clear();
}

void PdfVariant::Parse( const char* pszData, int nLen, long* pLen )
{
    char*    pszBuf    = (char*)pszData;
    long     lLen      = 0;
    long     lArrayLen;

    PdfVariant vVar;

    if( !pszData )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    Clear();

    if( !nLen )
        nLen = strlen( pszData );

    if( !nLen )
        return;

    GetDataType( pszData, nLen, &m_eDataType, &lLen );

    if( m_eDataType == ePdfDataType_HexString )
    {
        while( pszBuf && *pszBuf != '>' && pszBuf - pszData < nLen )
            ++pszBuf;

        if( *pszBuf == '>' )
        {
            m_pData = new PdfString();
            static_cast<PdfString*>(m_pData)->SetHexData( pszData+1, pszBuf - pszData - 1 );
            ++pszBuf;
        }
        else
        {
            RAISE_ERROR( ePdfError_InvalidDataType );
        }
    }
    else if( m_eDataType == ePdfDataType_String )
    {
        bool bIgnore = false;

        while( pszBuf && pszBuf - pszData < nLen )
        {
            if( *pszBuf == ')' && !bIgnore )
                break;

            bIgnore = ((*pszBuf == '\\' ) && !bIgnore);
            ++pszBuf;
        }

        if( *pszBuf == ')' )
        {
            m_pData = new PdfString( pszData+1, pszBuf - pszData - 1 );
            ++pszBuf; // for correct calculation of the parsed length
        }
        else
        {
            RAISE_ERROR( ePdfError_InvalidDataType );
        }
    }
    else if( m_eDataType == ePdfDataType_Name )
    {
        ++pszBuf; // otherwise it will return already on the first '/'
        while( pszBuf && !PdfParserBase::IsDelimiter( *pszBuf ) && !PdfParserBase::IsWhitespace( *pszBuf ) )
            ++pszBuf;

        // if we read data from a PDF file we assume that it has been encoded
        // already correctly by the application that created the PDF.
        // For performance reasons PdfName won't encode the data again!
        m_pData = new PdfName( (pszData+1), pszBuf - (pszData+1), true );        
    }
    else if( m_eDataType == ePdfDataType_Number )
    {
        m_Data.nNumber = strtol( pszData, &pszBuf, 10 );
        if( pszBuf == pszData )
        {
            RAISE_ERROR( ePdfError_NoNumber );
        }
    }
    else if( m_eDataType == ePdfDataType_Real )
    {
        m_Data.dNumber = strtod( pszData, &pszBuf );
        if( pszBuf == pszData )
        {
            RAISE_ERROR( ePdfError_NoNumber );
        }
    }
    else if( m_eDataType == ePdfDataType_Array )
    {
        ++pszBuf; // skip '['

        m_pData = new PdfArray();

        while( *pszBuf != ']' && pszBuf - pszData < nLen )
        {
            while( PdfParserBase::IsWhitespace( *pszBuf ) && pszBuf - pszData < nLen )
                ++pszBuf;

            if( *pszBuf == ']' )
            {
                ++pszBuf;
                break;
            }

            vVar.Parse( pszBuf, (pszData+nLen)-pszBuf, &lArrayLen );

            pszBuf += lArrayLen;
            static_cast<PdfArray*>(m_pData)->push_back( vVar );
        }

        if( pszBuf - pszData < nLen && *pszBuf == ']' )
            ++pszBuf;
    }
    else if( m_eDataType == ePdfDataType_Dictionary )
    {
        // ParseDictionaryKeys does not need a buffer
        // so create an empty one
        PdfRefCountedBuffer buffer( 0 );
        PdfParserObject parser( buffer );
        parser.ParseDictionaryKeys( pszBuf, nLen - (pszBuf - pszData), &lArrayLen );
        pszBuf += lArrayLen;
        m_pData = new PdfDictionary( parser.GetDictionary() );

        if( pszBuf - pszData < nLen && *pszBuf == '>' )
            ++pszBuf;
    }

    if( pLen )
    {
        if( m_eDataType == ePdfDataType_Null )
            *pLen = NULL_LENGTH;
        else if( m_eDataType == ePdfDataType_Bool )
            *pLen = (m_Data.bBoolValue ? TRUE_LENGTH : FALSE_LENGTH );
        else if( m_eDataType == ePdfDataType_Reference )
            *pLen = lLen;
        else
            *pLen = pszBuf - pszData;
    }
}

void PdfVariant::GetDataType( const char* pszData, long nLen, EPdfDataType* eDataType, long* pLen )
{
    PdfReference ref;
    char         c     = pszData[0];
    char*        pszStart;
    char*        pszRefStart;
    long         lRef;

    if( !eDataType )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // check for the 2 special data types null and boolean first
    if( nLen >= NULL_LENGTH && strncmp( pszData, "null", NULL_LENGTH ) == 0 )
    {
        *eDataType = ePdfDataType_Null;
        return;
    }
    else if( nLen >= TRUE_LENGTH && strncmp( pszData, "true", TRUE_LENGTH ) == 0 )
    {
        *eDataType   = ePdfDataType_Bool;
        m_Data.bBoolValue = true;
        return;
    }
    else if( nLen >= FALSE_LENGTH && strncmp( pszData, "false", FALSE_LENGTH ) == 0 )
    {
        *eDataType   = ePdfDataType_Bool;
        m_Data.bBoolValue = false;
        return;
    }
    
    switch( c )
    {
        case '[':
            *eDataType = ePdfDataType_Array;
            break;
        case '(':
            *eDataType = ePdfDataType_String;
            break;
        case '<':
            if( nLen > 1 )
            {
                if( pszData[1] == '<' )
                    *eDataType = ePdfDataType_Dictionary;
                else
                    *eDataType = ePdfDataType_HexString;
            }
            else
                *eDataType = ePdfDataType_HexString;
            break;
        case '/':
            *eDataType = ePdfDataType_Name;
            break;
        default:
            *eDataType = ePdfDataType_Unknown;
            break;
    }

    if( *eDataType != ePdfDataType_Unknown )
        return;

    lRef = strtol( pszData, &pszStart, 10 );
    ref.SetObjectNumber( lRef );

    if( pszStart != pszData )
    {
        // skip whitespaces
        while( PdfParserBase::IsWhitespace( *pszStart ) && pszStart - pszData < nLen )
            ++pszStart;

        pszRefStart = pszStart;
        lRef = strtol( pszRefStart, &pszStart, 10 );
        ref.SetGenerationNumber( lRef );

        if( pszStart != pszRefStart )
        {
            while( PdfParserBase::IsWhitespace( *pszStart ) && pszStart - pszData < nLen )
                ++pszStart;
            
            if( *pszStart == 'R' )
            {
                *eDataType = ePdfDataType_Reference;
                m_pData    = new PdfReference( ref );
                if( pLen )
                    *pLen = pszStart - pszData + 1;
                return;
            }
        }
    }
        
    // check for numbers last
    if( (isdigit( c ) || c == '-' || c == '+' ) )
    {
        *eDataType   = ePdfDataType_Number;
        char* pszBuf = (char*)pszData;
        
        ++pszBuf;
        
        // check if it is an real
        while( pszBuf && *pszBuf )
        {
            if( *pszBuf == '.' )
                *eDataType = ePdfDataType_Real;
            
            if( !isdigit( *pszBuf ) )
                break;
            
            ++pszBuf;
        }
        return;
    }

    RAISE_ERROR( ePdfError_InvalidDataType );
}

void PdfVariant::Clear()
{
    if( m_pData )
        delete m_pData;

    m_nPadding   = 0;
    m_eDataType  = ePdfDataType_Null;
    m_pData      = NULL;

    memset( &m_Data, sizeof( UVariant ), 0 );
}

void PdfVariant::Write( PdfOutputDevice* pDevice ) const
{
    return this->Write( pDevice, PdfName::KeyNull );
}

void PdfVariant::Write( PdfOutputDevice* pDevice, const PdfName & keyStop ) const
{
    unsigned long lLen = pDevice->GetLength();
    int           nPad = 0;
 
    DelayedLoad(); 

    /* Check all handles first 
     */
    if( (m_eDataType == ePdfDataType_HexString ||
         m_eDataType == ePdfDataType_String ||
         m_eDataType == ePdfDataType_Array ||
         m_eDataType == ePdfDataType_Dictionary ||
         m_eDataType == ePdfDataType_Name) && !m_pData )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    switch( m_eDataType ) 
    {
        case ePdfDataType_Bool:
            pDevice->Print( m_Data.bBoolValue ? "true" : "false" );
            break;
        case ePdfDataType_Number:
            pDevice->Print( "%li", m_Data.nNumber );
            break;
        case ePdfDataType_Real:
            pDevice->Print( "%g", m_Data.dNumber );
            break;
        case ePdfDataType_HexString:
        case ePdfDataType_String:
        case ePdfDataType_Name:
        case ePdfDataType_Array:
        case ePdfDataType_Reference:
            m_pData->Write( pDevice );
            break;
        case ePdfDataType_Dictionary:
            static_cast<PdfDictionary*>(m_pData)->Write( pDevice, keyStop );
            break;
        case ePdfDataType_Null:
            pDevice->Print( "null" );
            break;
        case ePdfDataType_Unknown:
        default:
        {
            RAISE_ERROR( ePdfError_InvalidDataType );
            break;
        }
    };

    nPad = (int)(pDevice->GetLength() - lLen);
    if( m_nPadding && nPad < m_nPadding )
    {
        std::string str( m_nPadding - nPad, ' ' ); 
        pDevice->Print( str.c_str() );
    }
}

void PdfVariant::ToString( std::string & rsData ) const
{
    ostringstream   out;
    PdfOutputDevice device( &out );

    this->Write( &device );
    
    rsData = out.str();
}

const PdfVariant & PdfVariant::operator=( const PdfVariant & rhs )
{
    Clear();

    rhs.DelayedLoad();

    m_eDataType      = rhs.m_eDataType;
    m_Data           = rhs.m_Data;
    m_nPadding       = rhs.m_nPadding;
    
    if( rhs.m_pData ) 
    {
        switch( m_eDataType ) 
        {
            case ePdfDataType_Array:
                m_pData = new PdfArray( *(static_cast<PdfArray*>(rhs.m_pData)) );
                break;
            case ePdfDataType_Reference:
                m_pData = new PdfReference( *(static_cast<PdfReference*>(rhs.m_pData)) );
                break;
            case ePdfDataType_Dictionary:
                m_pData = new PdfDictionary( *(static_cast<PdfDictionary*>(rhs.m_pData)) );
                break;
            case ePdfDataType_Name:
                m_pData = new PdfName( *(static_cast<PdfName*>(rhs.m_pData)) );
                break;
            case ePdfDataType_String:
            case ePdfDataType_HexString:
                m_pData = new PdfString( *(static_cast<PdfString*>(rhs.m_pData)) );
                break;
            default:
                break;
        };
    }

    return (*this);
}

};

