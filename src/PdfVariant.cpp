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
#include "PdfData.h"
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
inline void PdfVariant::Init()
{
    memset( &m_Data, 0, sizeof( UVariant ) );
    m_eDataType = ePdfDataType_Null;
#if defined(PODOFO_EXTRA_CHECKS)
    m_bDelayedLoadInProgress=false;
#endif
}

PdfVariant::PdfVariant()
{
    Init();
    Clear();

    m_eDataType = ePdfDataType_Null;
}

PdfVariant::PdfVariant( bool b )
{
    Init();
    Clear();

    m_eDataType       = ePdfDataType_Bool;
    m_Data.bBoolValue = b;
}

PdfVariant::PdfVariant( long l )
{
    Init();
    Clear();

    m_eDataType       = ePdfDataType_Number;
    m_Data.nNumber    = l;
}

PdfVariant::PdfVariant( double d )
{
    Init();
    Clear();

    m_eDataType       = ePdfDataType_Real;
    m_Data.dNumber    = d;    
}

PdfVariant::PdfVariant( const PdfString & rsString )
{
    Init();
    Clear();

    m_eDataType  = rsString.IsHex() ? ePdfDataType_HexString : ePdfDataType_String;
    m_Data.pData = new PdfString( rsString );
}

PdfVariant::PdfVariant( const PdfName & rName )
{
    Init();
    Clear();

    m_eDataType  = ePdfDataType_Name;
    m_Data.pData = new PdfName( rName );
}

PdfVariant::PdfVariant( const PdfReference & rRef )
{
    Init();
    Clear();

    m_eDataType  = ePdfDataType_Reference;
    m_Data.pData = new PdfReference( rRef );
}

PdfVariant::PdfVariant( const PdfArray & rArray )
{
    Init();
    Clear();

    m_eDataType  = ePdfDataType_Array;
    m_Data.pData = new PdfArray( rArray );
}

PdfVariant::PdfVariant( const PdfDictionary & rObj )
{
    Init();
    Clear();

    m_eDataType  = ePdfDataType_Dictionary;
    m_Data.pData = new PdfDictionary( rObj );
}

PdfVariant::PdfVariant( const PdfData & rData )
{
    Init();
    Clear();

    m_eDataType  = ePdfDataType_RawData;
    m_Data.pData = new PdfData( rData );
}

PdfVariant::PdfVariant( const PdfVariant & rhs )
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
    switch( m_eDataType ) 
    {
        case ePdfDataType_Array:
        case ePdfDataType_Reference:
        case ePdfDataType_Dictionary:
        case ePdfDataType_Name:
        case ePdfDataType_String:
        case ePdfDataType_HexString:
        case ePdfDataType_RawData:
        {
            if( m_Data.pData )
                delete m_Data.pData;
            break;
        }
            
        case ePdfDataType_Bool:
        case ePdfDataType_Null:
        case ePdfDataType_Number:
        case ePdfDataType_Real:
        case ePdfDataType_Unknown:
        default:
            break;
            
    }

    m_bDelayedLoadDone = true;
    m_eDataType  = ePdfDataType_Null;

    memset( &m_Data, 0, sizeof( UVariant ) );
}

void PdfVariant::Write( PdfOutputDevice* pDevice, const PdfEncrypt* pEncrypt ) const
{
    this->Write( pDevice, pEncrypt, PdfName::KeyNull );
}

void PdfVariant::Write( PdfOutputDevice* pDevice, const PdfEncrypt* pEncrypt, const PdfName & keyStop ) const
{
    DelayedLoad(); 

    /* Check all handles first 
     */
    if( (m_eDataType == ePdfDataType_HexString ||
         m_eDataType == ePdfDataType_String ||
         m_eDataType == ePdfDataType_Array ||
         m_eDataType == ePdfDataType_Dictionary ||
         m_eDataType == ePdfDataType_Name || 
         m_eDataType == ePdfDataType_RawData ) && !m_Data.pData )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
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
            //pDevice->Print( "%g", m_Data.dNumber );
            // DominikS: %g precision might write floating points
            //           numbers in exponential form (with e)
            //           which is not supported in PDF.
            //           %f fixes this but might loose precision as 
            //           it defaults to a precision of 6
            pDevice->Print( "%f", m_Data.dNumber );
            break;
        case ePdfDataType_HexString:
        case ePdfDataType_String:
        case ePdfDataType_Name:
        case ePdfDataType_Array:
        case ePdfDataType_Reference:
        case ePdfDataType_RawData:
            m_Data.pData->Write( pDevice, pEncrypt );
            break;
        case ePdfDataType_Dictionary:
            static_cast<PdfDictionary*>(m_Data.pData)->Write( pDevice, pEncrypt, keyStop );
            break;
        case ePdfDataType_Null:
            pDevice->Print( "null" );
            break;
        case ePdfDataType_Unknown:
        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
            break;
        }
    };
}

void PdfVariant::ToString( std::string & rsData ) const
{
    ostringstream   out;
    // We don't need to this stream with the safe PDF locale because
    // PdfOutputDevice will do so for us.
    PdfOutputDevice device( &out );

    this->Write( &device, NULL );
    
    rsData = out.str();
}

const PdfVariant & PdfVariant::operator=( const PdfVariant & rhs )
{
    Clear();

    rhs.DelayedLoad();

    m_eDataType      = rhs.m_eDataType;
    
    switch( m_eDataType ) 
    {
        case ePdfDataType_Array:
        {
            if( rhs.m_Data.pData ) 
                m_Data.pData = new PdfArray( *(static_cast<PdfArray*>(rhs.m_Data.pData)) );
            break;
        }
        case ePdfDataType_Reference:
        {
            if( rhs.m_Data.pData ) 
                m_Data.pData = new PdfReference( *(static_cast<PdfReference*>(rhs.m_Data.pData)) );
            break;
        }
        case ePdfDataType_Dictionary:
        {
            if( rhs.m_Data.pData ) 
                m_Data.pData = new PdfDictionary( *(static_cast<PdfDictionary*>(rhs.m_Data.pData)) );
            break;
        }
        case ePdfDataType_Name:
        {
            if( rhs.m_Data.pData ) 
                m_Data.pData = new PdfName( *(static_cast<PdfName*>(rhs.m_Data.pData)) );
            break;
        }
        case ePdfDataType_String:
        case ePdfDataType_HexString:
        {
            if( rhs.m_Data.pData ) 
                m_Data.pData = new PdfString( *(static_cast<PdfString*>(rhs.m_Data.pData)) );
            break;
        }
            
        case ePdfDataType_RawData: 
        {
            if( rhs.m_Data.pData ) 
                m_Data.pData = new PdfData( *(static_cast<PdfData*>(rhs.m_Data.pData)) );
            break;
        }
        case ePdfDataType_Bool:
        case ePdfDataType_Null:
        case ePdfDataType_Number:
        case ePdfDataType_Real:
            m_Data = rhs.m_Data;
            break;
            
        case ePdfDataType_Unknown:
        default:
            break;
    };

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
        case ePdfDataType_RawData: return "RawData";
        case ePdfDataType_Unknown: return "Unknown";
    }
    return "INVALID_TYPE_ENUM";
}

};



