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
    : m_pString( NULL ), m_pName( NULL ), m_pDictionary( NULL ), m_pArray( NULL )
{
    Clear();

    m_eDataType = ePdfDataType_Null;
}

PdfVariant::PdfVariant( bool b )
    : m_pString( NULL ), m_pName( NULL ), m_pDictionary( NULL ), m_pArray( NULL )
{
    Clear();

    m_eDataType       = ePdfDataType_Bool;
    m_Data.bBoolValue = b;
}

PdfVariant::PdfVariant( long l )
    : m_pString( NULL ), m_pName( NULL ), m_pDictionary( NULL ), m_pArray( NULL )
{
    Clear();

    m_eDataType       = ePdfDataType_Number;
    m_Data.nNumber    = l;
}

PdfVariant::PdfVariant( double d )
    : m_pString( NULL ), m_pName( NULL ), m_pDictionary( NULL ), m_pArray( NULL )
{
    Clear();

    m_eDataType       = ePdfDataType_Real;
    m_Data.dNumber    = d;    
}

PdfVariant::PdfVariant( const PdfString & rsString )
    : m_pString( NULL ), m_pName( NULL ), m_pDictionary( NULL ), m_pArray( NULL )
{
    Clear();

    m_eDataType = rsString.IsHex() ? ePdfDataType_HexString : ePdfDataType_String;
    m_pString   = new PdfString( rsString );
}

PdfVariant::PdfVariant( const PdfName & rName )
    : m_pString( NULL ), m_pName( NULL ), m_pDictionary( NULL ), m_pArray( NULL )
{
    Clear();

    m_eDataType = ePdfDataType_Name;
    m_pName     = new PdfName( rName );
}

PdfVariant::PdfVariant( const PdfReference & rRef )
    : m_pString( NULL ), m_pName( NULL ), m_pDictionary( NULL ), m_pArray( NULL )
{
    Clear();

    m_eDataType = ePdfDataType_Reference;
    m_reference = rRef;
}

PdfVariant::PdfVariant( const PdfArray & rArray )
    : m_pString( NULL ), m_pName( NULL ), m_pDictionary( NULL ), m_pArray( NULL )
{
    Clear();

    m_eDataType = ePdfDataType_Array;
    m_pArray = new PdfArray( rArray );
}

PdfVariant::PdfVariant( const PdfDictionary & rObj )
    : m_pString( NULL ), m_pName( NULL ), m_pDictionary( NULL ), m_pArray( NULL )
{
    Clear();

    m_eDataType   = ePdfDataType_Dictionary;
    m_pDictionary = new PdfDictionary( rObj );
}

PdfVariant::PdfVariant( const PdfVariant & rhs )
    : m_pString( NULL ), m_pName( NULL ), m_pDictionary( NULL ), m_pArray( NULL )
{
    this->operator=(rhs);
}

PdfVariant::~PdfVariant()
{
    Clear();
}

PdfError PdfVariant::Parse( const char* pszData, int nLen, long* pLen )
{
    PdfError eCode;
    char*    pszBuf    = (char*)pszData;
    long     lLen      = 0;
    long     lArrayLen;

    PdfVariant       vVar;

    if( !pszData )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    Clear();

    if( !nLen )
        nLen = strlen( pszData );

    SAFE_OP( GetDataType( pszData, nLen, &m_eDataType, &lLen ) );

    if( m_eDataType == ePdfDataType_HexString )
    {
        while( pszBuf && *pszBuf != '>' && pszBuf - pszData < nLen )
            ++pszBuf;

        if( *pszBuf == '>' )
        {
            m_pString = new PdfString();
            SAFE_OP( m_pString->SetHexData( pszData+1, pszBuf - pszData - 1 ) );
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
            m_pString = new PdfString( pszData+1, pszBuf - pszData - 1 );
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

        m_pName = new PdfName( (pszData+1), pszBuf - (pszData+1) );        
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

        while( *pszBuf != ']' && pszBuf - pszData < nLen )
        {
            while( PdfParserBase::IsWhitespace( *pszBuf ) && pszBuf - pszData < nLen )
                ++pszBuf;

            if( *pszBuf == ']' )
            {
                ++pszBuf;
                break;
            }

            SAFE_OP( vVar.Parse( pszBuf, (pszData+nLen)-pszBuf, &lArrayLen ) );

            pszBuf += lArrayLen;

            if( !m_pArray )
                m_pArray = new PdfArray();

            m_pArray->push_back( vVar );
        }

        if( pszBuf - pszData < nLen && *pszBuf == ']' )
            ++pszBuf;
    }
    else if( m_eDataType == ePdfDataType_Dictionary )
    {
        PdfParserObject parser( NULL, 0 );
        SAFE_OP( parser.ParseDictionaryKeys( pszBuf, nLen - (pszBuf - pszData), &lArrayLen ) );
        pszBuf += lArrayLen;
        m_pDictionary = new PdfDictionary( parser.GetDictionary() );

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

    return eCode;
}

#if 0
PdfError PdfVariant::Init( const char* pszData, EPdfDataType eDataType )
{
    PdfError eCode;

    if( !pszData )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    Clear();

    m_eDataType = eDataType;

    switch( m_eDataType )
    {
        case ePdfDataType_HexString:
            m_pString = new PdfString( pszData, true );
            break;
        case ePdfDataType_String:
            m_pString = new PdfString( pszData );
            break;
        case ePdfDataType_Name:
            m_pName = new PdfName( pszData );
            break;
        case ePdfDataType_Number:
            m_Data.nNumber = strtol( pszData, NULL, 10 );
            break;
        case ePdfDataType_Real:
            m_Data.dNumber = strtod( pszData, NULL );
            break;
        case ePdfDataType_Array:
        case ePdfDataType_Reference:
            SAFE_OP( this->Init( pszData ) );
            break;
        case ePdfDataType_Null:
            break;
        case ePdfDataType_Bool:
            m_Data.bBoolValue = (strncmp( pszData, "true", TRUE_LENGTH ) == 0 );            
            break;
        default:
        {
            RAISE_ERROR( ePdfError_InvalidDataType );
        }
    }

    return eCode;
}
#endif // 0

PdfError PdfVariant::GetDataType( const char* pszData, long nLen, EPdfDataType* eDataType, long* pLen )
{
    PdfError eCode;
    char    c     = pszData[0];
    char*   pszStart;
    char*   pszRefStart;
    long    lRef;

    if( !eDataType )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // check for the 2 special data types null and boolean first
    if( nLen >= NULL_LENGTH && strncmp( pszData, "null", NULL_LENGTH ) == 0 )
    {
        *eDataType = ePdfDataType_Null;
        return eCode;
    }
    else if( nLen >= TRUE_LENGTH && strncmp( pszData, "true", TRUE_LENGTH ) == 0 )
    {
        *eDataType   = ePdfDataType_Bool;
        m_Data.bBoolValue = true;
        return eCode;
    }
    else if( nLen >= FALSE_LENGTH && strncmp( pszData, "false", FALSE_LENGTH ) == 0 )
    {
        *eDataType   = ePdfDataType_Bool;
        m_Data.bBoolValue = false;
        return eCode;
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
            eCode.SetError( ePdfError_InvalidDataType, __FILE__, __LINE__ );
            break;
    }

    if( eCode == ePdfError_InvalidDataType )
    {
        eCode.SetError( ePdfError_ErrOk );

        lRef = strtol( pszData, &pszStart, 10 );
        m_reference.SetObjectNumber( lRef );

        if( pszStart == pszData )
            eCode.SetError( ePdfError_InvalidDataType, __FILE__, __LINE__ );

        if( !eCode.IsError() )
        {
            // skip whitespaces
            while( PdfParserBase::IsWhitespace( *pszStart ) && pszStart - pszData < nLen )
                ++pszStart;

            pszRefStart = pszStart;
            lRef = strtol( pszRefStart, &pszStart, 10 );
            m_reference.SetGenerationNumber( lRef );

            if( pszStart == pszRefStart )
                eCode.SetError( ePdfError_InvalidDataType, __FILE__, __LINE__ );

            if( !eCode.IsError() )
            {
                while( PdfParserBase::IsWhitespace( *pszStart ) && pszStart - pszData < nLen )
                    ++pszStart;

                if( *pszStart == 'R' )
                {
                    *eDataType = ePdfDataType_Reference;
                    if( pLen )
                        *pLen = pszStart - pszData + 1;
                }
                else
                {
                    eCode.SetError( ePdfError_InvalidDataType, __FILE__, __LINE__ );
                    m_reference = PdfReference();
                }
            }
        }
        
        // check for numbers last
        if( eCode == ePdfError_InvalidDataType && (isdigit( c ) || c == '-' || c == '+' ) )
        {
            *eDataType = ePdfDataType_Number;
            eCode = ePdfError_ErrOk;            

            char*   pszBuf    = (char*)pszData;

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
        }
    }

    return eCode;
}

void PdfVariant::Clear()
{
    delete m_pDictionary;
    delete m_pName;
    delete m_pString;
    delete m_pArray;

    m_nPadding    = 0;
    m_eDataType   = ePdfDataType_Null;
    m_pDictionary = NULL;
    m_pName       = NULL;
    m_pString     = NULL;
    m_pArray      = NULL;
    m_reference   = PdfReference();

    memset( &m_Data, sizeof( UVariant ), 0 );
}

PdfError PdfVariant::Write( PdfOutputDevice* pDevice ) const
{
    return this->Write( pDevice, PdfName::KeyNull );
}

PdfError PdfVariant::Write( PdfOutputDevice* pDevice, const PdfName & keyStop ) const
{
    PdfError        eCode;

    /* Check all handles first 
     */
    if( (m_eDataType == ePdfDataType_HexString ||
        m_eDataType == ePdfDataType_String) && (!m_pString || !m_pString->String()) )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_eDataType == ePdfDataType_Array && !m_pArray ) 
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_eDataType == ePdfDataType_Dictionary && !m_pDictionary )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_eDataType == ePdfDataType_Name && !m_pName ) 
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    switch( m_eDataType ) 
    {
        case ePdfDataType_Bool:
            eCode = pDevice->Print( m_Data.bBoolValue ? "true" : "false" );
            break;
        case ePdfDataType_Number:
            eCode = pDevice->Print( "%li", m_Data.nNumber );
            break;
        case ePdfDataType_Real:
            eCode = pDevice->Print( "%g", m_Data.dNumber );
            break;
        case ePdfDataType_HexString:
        case ePdfDataType_String:
            eCode = m_pString->Write( pDevice );
            break;
        case ePdfDataType_Name:
            eCode = m_pName->Write( pDevice );
            break;
        case ePdfDataType_Array:
            eCode = m_pArray->Write( pDevice );
            break;
        case ePdfDataType_Dictionary:
            eCode = m_pDictionary->Write( pDevice, keyStop );
            break;
        case ePdfDataType_Null:
            eCode = pDevice->Print( "null" );
            break;
        case ePdfDataType_Reference:
            eCode = m_reference.Write( pDevice );
            break;
        case ePdfDataType_Unknown:
        default:
            eCode.SetError( ePdfError_InvalidDataType, __FILE__, __LINE__ );
            break;
    };

    if( !eCode.IsError() )
    {
        if( m_nPadding && (int)pDevice->Length() < m_nPadding )
        {
            std::string str( m_nPadding - pDevice->Length(), ' ' ); 
            eCode = pDevice->Print( str.c_str() );
        }
    }

    return eCode;
}

PdfError PdfVariant::ToString( std::string & rsData ) const
{
    PdfError        eCode;
    PdfOutputDevice device;
    ostringstream   out;

    SAFE_OP( device.Init( &out ) );
    SAFE_OP( this->Write( &device ) );
    
    rsData = out.str();

    return eCode;
}

const PdfVariant & PdfVariant::operator=( const PdfVariant & rhs )
{
    Clear();

    m_eDataType      = rhs.m_eDataType;
    m_Data           = rhs.m_Data;
    m_nPadding       = rhs.m_nPadding;
    
    m_reference   = rhs.m_reference;
    m_pArray      = rhs.m_pArray && m_eDataType == ePdfDataType_Array ? new PdfArray( *(rhs.m_pArray) ) : NULL;
    m_pDictionary = rhs.m_pDictionary && m_eDataType == ePdfDataType_Dictionary ? new PdfDictionary( *(rhs.m_pDictionary) ) : NULL;
    m_pName       = rhs.m_pName && m_eDataType == ePdfDataType_Name ? new PdfName( *(rhs.m_pName) ) : NULL;
    m_pString     = rhs.m_pString && 
        (m_eDataType == ePdfDataType_String || m_eDataType == ePdfDataType_HexString ) ? new PdfString( *(rhs.m_pString) ) : NULL;

    return (*this);
}

};

