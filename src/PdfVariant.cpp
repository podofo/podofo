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

// Do one-off initialization that should not be repeated
// in the Clear() method. Mostly useful for internal sanity checks.
inline void PdfVariant::Init() throw()
{
#if defined(PODOFO_EXTRA_CHECKS)
    m_bDelayedLoadInProgress=false;
#endif
}

PdfVariant::PdfVariant()
    : m_pData( NULL )
{
    Init();
    Clear();

    m_eDataType = ePdfDataType_Null;
}

PdfVariant::PdfVariant( bool b )
    : m_pData( NULL )
{
    Init();
    Clear();

    m_eDataType       = ePdfDataType_Bool;
    m_Data.bBoolValue = b;
}

PdfVariant::PdfVariant( long l )
    : m_pData( NULL )
{
    Init();
    Clear();

    m_eDataType       = ePdfDataType_Number;
    m_Data.nNumber    = l;
}

PdfVariant::PdfVariant( double d )
    : m_pData( NULL )
{
    Init();
    Clear();

    m_eDataType       = ePdfDataType_Real;
    m_Data.dNumber    = d;    
}

PdfVariant::PdfVariant( const PdfString & rsString )
    : m_pData( NULL )
{
    Init();
    Clear();

    m_eDataType = rsString.IsHex() ? ePdfDataType_HexString : ePdfDataType_String;
    m_pData     = new PdfString( rsString );
}

PdfVariant::PdfVariant( const PdfName & rName )
    : m_pData( NULL )
{
    Init();
    Clear();

    m_eDataType = ePdfDataType_Name;
    m_pData     = new PdfName( rName );
}

PdfVariant::PdfVariant( const PdfReference & rRef )
    : m_pData( NULL )
{
    Init();
    Clear();

    m_eDataType = ePdfDataType_Reference;
    m_pData     = new PdfReference( rRef );
}

PdfVariant::PdfVariant( const PdfArray & rArray )
    : m_pData( NULL )
{
    Init();
    Clear();

    m_eDataType = ePdfDataType_Array;
    m_pData     = new PdfArray( rArray );
}

PdfVariant::PdfVariant( const PdfDictionary & rObj )
    : m_pData( NULL )
{
    Init();
    Clear();

    m_eDataType = ePdfDataType_Dictionary;
    m_pData     = new PdfDictionary( rObj );
}

PdfVariant::PdfVariant( const PdfVariant & rhs )
    : m_pData( NULL )
{
    Init();
    this->operator=(rhs);
}

PdfVariant::~PdfVariant()
{
    Clear();
}

void PdfVariant::Clear()
{
    if (m_pData)
        delete m_pData;

    m_bDelayedLoadDone = true;
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

    nPad = static_cast<int>(pDevice->GetLength() - lLen);
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

            case ePdfDataType_Bool:
            case ePdfDataType_Null:
            case ePdfDataType_Number:
            case ePdfDataType_Real:
            case ePdfDataType_Unknown:
            default:
                break;
        };
    }

    return (*this);
}

const char * PdfVariant::GetDataTypeString() const
{
    switch(GetDataType())
    {
        case ePdfDataType_Bool: return "Bool";
        case ePdfDataType_Number: return "Number";
        case ePdfDataType_Real: return "Real";
        case ePdfDataType_String: return "String";
        case ePdfDataType_HexString: return "HexString";
        case ePdfDataType_Name: return "Name";
        case ePdfDataType_Array: return "Array";
        case ePdfDataType_Dictionary: return "Dictionary";
        case ePdfDataType_Null: return "Null";
        case ePdfDataType_Reference: return "Reference";
        case ePdfDataType_Unknown: return "Unknown";
    }
    return "INVALID_TYPE_ENUM";
}

};


