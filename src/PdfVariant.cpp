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

#include "PdfOutputDevice.h"
#include "PdfParserObject.h"

#include <sstream>

#define NULL_LENGTH  4
#define TRUE_LENGTH  4
#define FALSE_LENGTH 5

namespace PoDoFo {

using namespace std;

PdfVariant::PdfVariant()
    : m_pString( NULL ), m_pName( NULL ), m_pDictionary( NULL )
{
    Clear();
}

PdfVariant::PdfVariant( const PdfVariant & rhs )
    : m_pString( NULL ), m_pName( NULL ), m_pDictionary( NULL )
{
    this->operator=(rhs);
}

PdfVariant::~PdfVariant()
{
    Clear();
}

PdfError PdfVariant::Init( const char* pszData, int nLen, long* pLen )
{
    PdfError eCode;
    char*    pszBuf    = (char*)pszData;
    long     lLen      = 0;
    long     lArrayLen;

    PdfParserObject* pParser;
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

            SAFE_OP( vVar.Init( pszBuf, (pszData+nLen)-pszBuf, &lArrayLen ) );

            pszBuf += lArrayLen;

            m_vecArray.push_back( vVar );
        }

        if( pszBuf - pszData < nLen && *pszBuf == ']' )
            ++pszBuf;
    }
    else if( m_eDataType == ePdfDataType_Dictionary )
    {
        pParser = new PdfParserObject( NULL, 0 );
        SAFE_OP( pParser->ParseDictionaryKeys( pszBuf, nLen - (pszBuf - pszData), &lArrayLen ) );
        pszBuf += lArrayLen;
        m_pDictionary = pParser;

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

PdfError PdfVariant::Init( const TVariantList & tList )
{
    Clear();

    m_eDataType = ePdfDataType_Array;
    m_vecArray = tList;
    
    return ePdfError_ErrOk;
}

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

    m_nPadding    = 0;
    m_eDataType   = ePdfDataType_Unknown;
    m_pDictionary = NULL;
    m_pName       = NULL;
    m_pString     = NULL;
    m_reference   = PdfReference();

    memset( &m_Data, sizeof( UVariant ), 0 );
    
    m_vecArray  .clear();
}

PdfError PdfVariant::ToString( std::string & rsData ) const
{
    PdfError        eCode;
    TCIVariantList  itArray; 
    PdfOutputDevice device;

    ostringstream   out;
    string          sTmp;

    if( (m_eDataType == ePdfDataType_HexString ||
        m_eDataType == ePdfDataType_String) && (!m_pString || !m_pString->String()) )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // TODO:
    //     ePdfDataType_Stream,
    switch( m_eDataType ) 
    {
        case ePdfDataType_Bool:
            out << (m_Data.bBoolValue ? "true" : "false");
            break;
        case ePdfDataType_Number:
            out << m_Data.nNumber;
            break;
        case ePdfDataType_Real:
            out << m_Data.dNumber;
            break;
        case ePdfDataType_HexString:
            out << "<" << m_pString->String() << ">";
            break;
        case ePdfDataType_String:
            out << "(" << m_pString->String() << ")";
            break;
        case ePdfDataType_Name:
            out << "/" << m_pName->Name();
            break;
        case ePdfDataType_Array:
            itArray = m_vecArray.begin();
            out << "[ ";
            while( itArray != m_vecArray.end() )
            {
                SAFE_OP( (*itArray).ToString( sTmp ) );
                out << sTmp << " ";
                ++itArray;
            }
            out << "]";
            break;
        case ePdfDataType_Dictionary:
            device.Init( sTmp );
            eCode = m_pDictionary->Write( &device );
            out << sTmp;
            break;
        case ePdfDataType_Null:
            out << "null";
            break;
        case ePdfDataType_Reference:
            out << m_reference.ToString();
            break;
        case ePdfDataType_Unknown:
        default:
            eCode.SetError( ePdfError_InvalidDataType, __FILE__, __LINE__ );
            break;
    };

    if( !eCode.IsError() )
    {
        rsData = out.str();

        if( m_nPadding && (int)rsData.length() < m_nPadding )
        {
            // pad the string with spaces
            rsData += std::string( m_nPadding - rsData.length(), ' ' ); 
        }
    }

    return eCode;
}

const TVariantList & PdfVariant::GetArray() const
{
    return m_vecArray;
}

PdfError PdfVariant::SetArray( const TVariantList & vArray )
{
    PdfError eCode;

    if( m_eDataType != ePdfDataType_Array )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    m_vecArray = vArray;

    return eCode;
}

const PdfObject & PdfVariant::GetDictionary() const
{
    return *m_pDictionary;
}

PdfError PdfVariant::GetBool( bool* pBool ) const
{
    PdfError eCode;

    if( m_eDataType != ePdfDataType_Bool )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    *pBool = m_Data.bBoolValue;

    return eCode;

}

PdfError PdfVariant::GetNumber( long* pNum ) const
{
    PdfError eCode;

    if( m_eDataType != ePdfDataType_Number )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    *pNum = m_Data.nNumber;

    return eCode;
}

PdfError PdfVariant::SetNumber( long lNum )
{
    PdfError eCode;

    if( m_eDataType != ePdfDataType_Number )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    m_Data.nNumber = lNum;

    return eCode;
}

PdfError PdfVariant::GetNumber( double* pNum ) const
{
    PdfError eCode;

    if( m_eDataType != ePdfDataType_Real )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    *pNum = m_Data.dNumber;

    return eCode;
}

PdfError PdfVariant::SetNumber( double dNum )
{
    PdfError eCode;

    if( m_eDataType != ePdfDataType_Real )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    m_Data.dNumber = dNum;

    return eCode;
}

PdfError PdfVariant::SetString( const PdfString & rsString )
{
    PdfError eCode;

    if( !(m_eDataType == ePdfDataType_String ||
         m_eDataType == ePdfDataType_HexString )) 
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    if( m_pString )
        delete m_pString;

    m_pString = new PdfString( rsString );

    return eCode;
}

PdfError PdfVariant::SetName( const PdfName & rsName )
{
    PdfError eCode;

    if( m_eDataType != ePdfDataType_Name )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    if( m_pName )
        delete m_pName;

    m_pName = new PdfName( rsName );

    return eCode;
}

const PdfReference & PdfVariant::GetReference() const
{
    if( m_eDataType != ePdfDataType_Reference )
    {
        return m_reference;
    }

    return m_reference;
}

PdfError PdfVariant::SetReference( const PdfReference & ref )
{
    PdfError eCode;

    if( m_eDataType != ePdfDataType_Reference )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    m_reference = ref;

    return eCode;
}

const PdfString & PdfVariant::GetString() const
{
    if( m_eDataType != ePdfDataType_String )
        return PdfString::StringNull;

    return *m_pString;
}

const PdfName & PdfVariant::GetName() const
{
    if( m_eDataType != ePdfDataType_Name )
        return PdfName::KeyNull;

    return *m_pName;
}

const PdfVariant & PdfVariant::operator=( const PdfVariant & rhs )
{
    TCIVariantList itArray = rhs.m_vecArray.begin();

    Clear();

    m_eDataType      = rhs.m_eDataType;
    m_Data           = rhs.m_Data;
    m_nPadding       = rhs.m_nPadding;

    while( itArray != rhs.m_vecArray.end() )
    {
        m_vecArray.push_back( (*itArray) );
        ++itArray;
    }
    
    m_pDictionary = rhs.m_pDictionary ? new PdfObject( *(rhs.m_pDictionary) ) : NULL;
    m_pName       = rhs.m_pName ? new PdfName( *(rhs.m_pName) ) : NULL;
    m_pString     = rhs.m_pString ? new PdfString( *(rhs.m_pString) ) : NULL;

    return (*this);
}

};

