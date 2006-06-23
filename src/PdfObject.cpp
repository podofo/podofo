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

#include "PdfOutputDevice.h"
#include "PdfStream.h"
#include "PdfVariant.h"

#include <sstream>
#include <fstream>

#include <string.h>

using namespace std;

namespace PoDoFo {

PdfObject::PdfObject( unsigned int objectno, unsigned int generationno, const char* pszType )
    : m_reference( objectno, generationno )
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
    m_bDirect                 = false;;
    m_bEmptyEntry             = false;

    m_pStream                 = NULL;

    m_bLoadOnDemandDone       = false;
    m_bLoadStreamOnDemandDone = false;
}

void PdfObject::Clear()
{
    ClearKeys();
    ClearObjectKeys();

    delete m_pStream;
}

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

PdfError PdfObject::AddKey( const PdfName & identifier, const PdfVariant & rVariant )
{
    PdfError eCode;

    SAFE_OP( DelayedLoad() );

    if( !identifier.Length() )
    {
        RAISE_ERROR( ePdfError_InvalidDataType );
    }

    if( m_mapKeys.find( identifier ) != m_mapKeys.end() )
        m_mapKeys.erase( identifier );

    m_mapKeys[identifier] = rVariant;
    m_singleValue.Clear();

    return eCode;
}

PdfError PdfObject::AddKey( const PdfName & identifier, PdfObject* pObj )
{
    PdfError eCode;

    SAFE_OP( DelayedLoad() );

    PdfObject* pTmp = m_mapObjKeys[identifier];
    if( pTmp )
        delete pTmp;

    m_mapObjKeys[identifier] = pObj;
    m_singleValue.Clear();

    return eCode;
}

bool PdfObject::RemoveKey( const PdfName & identifier )
{
    // HasKey calls DelayedLoad()
    if( HasKey( identifier ) )
    {
        m_mapKeys.erase( identifier );
        return true;
    }

    return false;
}

bool PdfObject::RemoveObjectKey( const PdfName & identifier )
{
    // HasObjectKey calls DelayedLoad()
    if( HasObjectKey( identifier ) )
    {
        delete GetKeyValueObject( identifier );
        m_mapObjKeys.erase( identifier );

        return true;
    }

    return false;
}

void PdfObject::ClearKeys()
{
    if( !m_mapKeys.empty() )
        m_mapKeys.clear();
}

void PdfObject::ClearObjectKeys()
{
    TIObjKeyMap itObjects = m_mapObjKeys.begin();

    while( itObjects != m_mapObjKeys.end() )
    {
        delete (*itObjects).second;
        ++itObjects;
    }

    m_mapObjKeys.clear();
}

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

PdfObject* PdfObject::GetKeyValueObject( const PdfName & key ) const
{
    TCIObjKeyMap it;

    // HasObjectKey calls DelayedLoad()
    if( HasObjectKey( key ) )
    {
        it = m_mapObjKeys.find( key );
        return (*it).second;
    }
    else
        return NULL;
}

bool PdfObject::HasKey( const PdfName & key ) const
{
    DelayedLoad();

    if( !key.Length() )
        return false;
    
    return ( m_mapKeys.find( key ) != m_mapKeys.end() );
}

bool PdfObject::HasObjectKey( const PdfName & key ) const
{
    DelayedLoad();

    if( !key.Length() )
        return false;
    
    return ( m_mapObjKeys.find( key ) != m_mapObjKeys.end() );
}

void PdfObject::SetSingleValue( const PdfVariant & var  )
{
    TIObjKeyMap itObjects = m_mapObjKeys.begin();

    DelayedLoad();

    while( itObjects != m_mapObjKeys.end() )
    {
        delete (*itObjects).second;
        ++itObjects;
    }

    m_mapKeys   .clear();
    m_mapObjKeys.clear();

    m_singleValue = var;
}

const PdfString & PdfObject::GetSingleValueString() const
{
    DelayedLoad();

    return m_singleValue.GetString();
}

long PdfObject::GetSingleValueLong() const
{
    DelayedLoad();

    return m_singleValue.GetNumber();
}

bool PdfObject::GetSingleValueBool() const
{
    DelayedLoad();

    return m_singleValue.GetBool();
}

const PdfVariant & PdfObject::GetSingleValueVariant () const
{
    DelayedLoad();

    return m_singleValue;
}

PdfError PdfObject::Write( PdfOutputDevice* pDevice, const PdfName & keyStop ) 
{
    PdfError      eCode;
    TCIKeyMap     itKeys;
    TCIObjKeyMap  itObjKeys;
    ostringstream out;
    std::string   sData;
    PdfVariant    var;

    // do not write empty objects to disc
    if( m_bEmptyEntry )
        return eCode;

    DelayedStreamLoad();

    if( !pDevice )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( !m_bDirect )
        SAFE_OP( pDevice->Print( "%i %i obj\n", m_reference.ObjectNumber(), m_reference.GenerationNumber() ) );

    if( m_pStream )
        this->AddKey( PdfName::KeyLength, (long)m_pStream->Length() );

    if( m_singleValue.IsEmpty() )
    {
        SAFE_OP( pDevice->Print( "<<\n" ) );

        itKeys     = m_mapKeys.begin();
        itObjKeys  = m_mapObjKeys.begin();

        if( keyStop != PdfName::KeyNull && keyStop.Length() && keyStop == PdfName::KeyType )
            return eCode;

        if( this->HasKey( PdfName::KeyType ) ) 
        {
            SAFE_OP( this->GetKeyValueVariant( PdfName::KeyType, var ) );
            SAFE_OP( var.ToString( sData ) );
            SAFE_OP( pDevice->Print( "/Type %s\n", sData.c_str() ) );
        }

        while( itKeys != m_mapKeys.end() )
        {
            if( (*itKeys).first != PdfName::KeyType )
            {
                if( keyStop != PdfName::KeyNull && keyStop.Length() && (*itKeys).first == keyStop )
                    return eCode;

                SAFE_OP( (*itKeys).second.ToString( sData ) );
                SAFE_OP( pDevice->Print( "/%s %s\n", (*itKeys).first.Name().c_str(), sData.c_str() ) );
            }

            ++itKeys;
        }
        
        while( itObjKeys != m_mapObjKeys.end() )
        {
            if( keyStop.Length() && (*itObjKeys).first == keyStop )
                return eCode;

            SAFE_OP( pDevice->Print( "/%s ", (*itObjKeys).first.Name().c_str() ) );
            SAFE_OP( (*itObjKeys).second->Write( pDevice ) );
            ++itObjKeys;
        }
        
        SAFE_OP( pDevice->Print( ">>\n" ) );
        if( m_pStream )
        {
            SAFE_OP( pDevice->Print( "stream\n" ) );
            SAFE_OP( pDevice->Write( m_pStream->Get(), m_pStream->Length() ) );
            SAFE_OP( pDevice->Print( "\nendstream\n" ) );
        }
    }
    else
    {
        SAFE_OP( m_singleValue.ToString( sData ) );
        SAFE_OP( pDevice->Print( "%s\n", sData.c_str()  ) );
    }

    if( !m_bDirect )
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
    TCIKeyMap    itKeys;
    TCIObjKeyMap itObj;

    Clear();
    Init();

    if( !rhs.m_bLoadOnDemandDone || !rhs.m_bLoadStreamOnDemandDone ) 
    {
        PdfObject* p = const_cast<PdfObject*>(&rhs); \
        p->LoadStreamOnDemand();
    }

    m_reference     = rhs.m_reference;
    m_bDirect       = rhs.m_bDirect;
    m_bEmptyEntry   = rhs.m_bEmptyEntry;

    m_bLoadOnDemandDone       = true;
    m_bLoadStreamOnDemandDone = true;

    m_singleValue  = rhs.m_singleValue;

    itKeys = rhs.m_mapKeys.begin();
    while( itKeys != rhs.m_mapKeys.end() )
    {
        m_mapKeys[(*itKeys).first] = (*itKeys).second;

        ++itKeys;
    }

    itObj = rhs.m_mapObjKeys.begin();
    while( itObj != rhs.m_mapObjKeys.end() )
    {
        m_mapObjKeys[(*itObj).first] = new PdfObject( *((*itObj).second) );

        ++itObj;
    }

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
