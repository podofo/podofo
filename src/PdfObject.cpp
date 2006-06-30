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
    : PdfVariant( PdfDictionary() ), m_reference( objectno, generationno )
{
    Init();

    if( pszType )
        this->GetDictionary().AddKey( PdfName::KeyType, PdfName( pszType ) );
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
    PdfVariant::Clear();

    delete m_pStream;
}

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

    if( m_pStream && this->IsDictionary() )
        this->GetDictionary().AddKey( PdfName::KeyLength, (long)m_pStream->Length() );

    SAFE_OP( PdfVariant::Write( pDevice, keyStop ) );
    if( !this->IsDictionary() )
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

    PdfVariant::operator=( rhs );

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

    if( !this->GetDictionary().HasKey( pszKey ) )
    {
        RAISE_ERROR( ePdfError_InvalidKey );
    }

    SAFE_OP( device.Init() );
    SAFE_OP( this->Write( &device, pszKey ) );

    *pulOffset = device.Length();
    
    return eCode;
}

};
