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

#include "PdfObject.h"

#include "PdfDictionary.h"
#include "PdfOutputDevice.h"
#include "PdfStream.h"
#include "PdfVariant.h"

#include <sstream>
#include <fstream>

#include <string.h>

using namespace std;

namespace PoDoFo {

PdfObject::PdfObject( unsigned int objectno, unsigned int generationno, const char* pszType )
    : m_reference( objectno, generationno ), m_value( PdfDictionary() )
{
    Init();

    if( pszType )
        this->AddKey( PdfName::KeyType, PdfName( pszType ) );
}

PdfObject::PdfObject( const PdfObject & rhs )
{
    Init();

    operator=( rhs );
}

PdfObject::~PdfObject()
{
    Clear();
}

void PdfObject::Init()
{
    m_bEmptyEntry             = false;

    m_pStream                 = NULL;

    m_bLoadOnDemandDone       = false;
    m_bLoadStreamOnDemandDone = false;
}

void PdfObject::Clear()
{
    m_value.Clear();

    delete m_pStream;
}

/*
PdfError PdfObject::AddKey( const PdfName & identifier, const std::string & rValue )
{
    return this->AddKey( identifier, rValue.c_str() );
}

PdfError PdfObject::AddKey( const  PdfName & identifier, const char* pszValue )
{
    PdfError   eCode;
    PdfVariant cVariant;

    SAFE_OP( cVariant.Parse( pszValue ) );
    SAFE_OP( this->AddKey( identifier, cVariant ) );

    return eCode;
}

PdfError PdfObject::AddKey( const PdfName & identifier, double dValue )
{
    return this->AddKey( identifier, PdfVariant( dValue ) );
}

PdfError PdfObject::AddKey( const PdfName & identifier, long nValue )
{
    return this->AddKey( identifier, PdfVariant( nValue ) );
}

PdfError PdfObject::AddKey( const PdfName & identifier, const PdfString & rValue )
{
    return this->AddKey( identifier, PdfVariant( rValue ) );
}

PdfError PdfObject::AddKey( const PdfName & identifier, const PdfName & rValue )
{
    return this->AddKey( identifier, PdfVariant( rValue ) );
}

PdfError PdfObject::AddKey( const PdfName & identifier, const PdfReference & rValue )
{
    return this->AddKey( identifier, PdfVariant( rValue ) );
}
*/

PdfError PdfObject::AddKey( const PdfName & identifier, const PdfVariant & rValue )
{
    PdfError eCode;

    SAFE_OP( DelayedLoad() );

    if( m_value.GetDataType() == ePdfDataType_Dictionary ) 
    {
        return m_value.GetDictionary().AddKey( identifier, rValue );
    }
    else 
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    return eCode;
}

const PdfVariant & PdfObject::GetKey( const PdfName & key ) const
{
    PdfError eCode;

    if( DelayedLoad().IsError() )
        return PdfVariant::NullValue;

    if( m_value.GetDataType() == ePdfDataType_Dictionary ) 
    {
        return m_value.GetDictionary().GetKey( key );
    }
    else 
    {
        //RAISE_ERROR( ePdfError_InvalidDataType );
    }

    return PdfVariant::NullValue;

}

bool PdfObject::HasKey( const PdfName & key  ) const
{
    if( DelayedLoad().IsError() )
        return false;

    if( m_value.GetDataType() == ePdfDataType_Dictionary ) 
    {
        return m_value.GetDictionary().HasKey( key );
    }
    else 
        return false;
}

bool PdfObject::RemoveKey( const PdfName & identifier )
{
    if( DelayedLoad().IsError() )
        return false;

    if( m_value.GetDataType() == ePdfDataType_Dictionary ) 
    {
        return m_value.GetDictionary().RemoveKey( identifier );
    }
    else 
        return false;
}

/*
const PdfString & PdfObject::GetKeyValueString ( const PdfName & key, const PdfString & sDefault ) const
{
    TCIKeyMap it;

    // HasKey calls DelayedLoad()
    if( HasKey( key ) )
    {
        it = m_mapKeys.find( key );
        return (*it).second.GetString();
    }
    else
        return sDefault;
}

long PdfObject::GetKeyValueLong   ( const PdfName & key, long lDefault ) const
{
    TCIKeyMap it;

    // HasKey calls DelayedLoad()
    if( HasKey( key ) )
    {
        it = m_mapKeys.find( key );
        if( (*it).second.GetDataType() == ePdfDataType_Number )
            return (*it).second.GetNumber();
    }

    return lDefault;
}

bool PdfObject::GetKeyValueBool   ( const PdfName & key, bool bDefault ) const
{
    PdfError    eCode;
    PdfVariant  var;

    // GetKeyValueVariant calls DelayedLoad()
    eCode = this->GetKeyValueVariant( key, var );
    if( eCode.IsError() || var.GetDataType() != ePdfDataType_Bool )
        return bDefault;

    return var.GetBool();
}

PdfError PdfObject::GetKeyValueVariant( const PdfName & key, PdfVariant & rVariant ) const
{
    PdfError   eCode;
    TCIKeyMap it;

    // HasKey calls DelayedLoad()
    if( HasKey( key ) )
    {
        it = m_mapKeys.find( key );
        rVariant = (*it).second;
    }
    else
    {
        RAISE_ERROR( ePdfError_InvalidKey );
    }

    return eCode;
}
*/

PdfError PdfObject::Write( PdfOutputDevice* pDevice, const PdfName & keyStop ) 
{
    PdfError      eCode;

    // do not write empty objects to disc
    if( m_bEmptyEntry )
        return eCode;

    DelayedStreamLoad();

    if( !pDevice )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    SAFE_OP( pDevice->Print( "%i %i obj\n", m_reference.ObjectNumber(), m_reference.GenerationNumber() ) );

    if( m_pStream && m_value.IsDictionary() )
        this->AddKey( PdfName::KeyLength, (long)m_pStream->Length() );

    SAFE_OP( m_value.Write( pDevice, keyStop ) );
    if( !m_value.IsDictionary() )
    {
        SAFE_OP( pDevice->Print( "\n" ) );
    }

    if( m_pStream )
    {
        SAFE_OP( pDevice->Print( "stream\n" ) );
        SAFE_OP( pDevice->Write( m_pStream->Get(), m_pStream->Length() ) );
        SAFE_OP( pDevice->Print( "\nendstream\n" ) );
    }

    SAFE_OP( pDevice->Print( "endobj\n" ) );

    return eCode;
}

PdfError PdfObject::GetObjectLength( unsigned long* pulLength )
{
    PdfError        eCode;
    PdfOutputDevice device;

    if( !pulLength )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    SAFE_OP( device.Init() );

    SAFE_OP( this->Write( &device ) );

    *pulLength = device.Length();

    return eCode;
}

PdfStream* PdfObject::Stream()
{
    DelayedStreamLoad();

    if( !m_pStream )
        m_pStream = new PdfStream( this );
    
    return m_pStream;
}

const PdfStream* PdfObject::Stream() const
{
    DelayedStreamLoad();

    return m_pStream;
}

PdfError PdfObject::FlateDecodeStream() 
{
    DelayedStreamLoad();

    return m_pStream ? m_pStream->FlateDecode() : ePdfError_ErrOk;
}

const PdfObject & PdfObject::operator=( const PdfObject & rhs )
{
    Clear();
    Init();

    if( !rhs.m_bLoadOnDemandDone || !rhs.m_bLoadStreamOnDemandDone ) 
    {
        PdfObject* p = const_cast<PdfObject*>(&rhs); \
        p->LoadStreamOnDemand();
    }

    m_reference     = rhs.m_reference;
    m_bEmptyEntry   = rhs.m_bEmptyEntry;
    m_value         = rhs.m_value;

    m_bLoadOnDemandDone       = true;
    m_bLoadStreamOnDemandDone = true;

    if( rhs.m_pStream )
        m_pStream = new PdfStream( *(rhs.m_pStream) ); 

    return *this;
}

PdfError PdfObject::GetByteOffset( const char* pszKey, unsigned long* pulOffset )
{
    PdfError        eCode;
    PdfOutputDevice device;

    if( !pszKey || !pulOffset ) 
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( !HasKey( pszKey ) )
    {
        RAISE_ERROR( ePdfError_InvalidKey );
    }

    SAFE_OP( device.Init() );
    SAFE_OP( this->Write( &device, pszKey ) );

    *pulOffset = device.Length();
    
    return eCode;
}

};
