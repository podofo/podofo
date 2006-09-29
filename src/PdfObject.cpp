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

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfOutputDevice.h"
#include "PdfStream.h"
#include "PdfVariant.h"

#include <sstream>
#include <fstream>

#include <string.h>

using namespace std;

namespace PoDoFo {

PdfObject::PdfObject()
    : PdfVariant( PdfDictionary() ), m_reference( (unsigned long)-1, (unsigned long)-1 )
{
    Init( true );
}

PdfObject::PdfObject( const PdfReference & rRef, const char* pszType )
    : PdfVariant( PdfDictionary() ), m_reference( rRef )
{
    Init( true );

    if( pszType )
        this->GetDictionary().AddKey( PdfName::KeyType, PdfName( pszType ) );
}

PdfObject::PdfObject( const PdfReference & rRef, const PdfVariant & rVariant )
    : PdfVariant( rVariant ), m_reference( rRef )
{
    Init( true );
}

PdfObject::PdfObject( const PdfVariant & var )
    : PdfVariant( var ), m_reference( (unsigned long)-1, (unsigned long)-1 )
{
    Init( true );
}

PdfObject::PdfObject( bool b )
    : PdfVariant( b ), m_reference( (unsigned long)-1, (unsigned long)-1 )
{
    Init( false );
}

PdfObject::PdfObject( long l )
    : PdfVariant( l ), m_reference( (unsigned long)-1, (unsigned long)-1 )
{
    Init( false );
}

PdfObject::PdfObject( double d )
    : PdfVariant( d ), m_reference( (unsigned long)-1, (unsigned long)-1 )
{
    Init( false );
}

PdfObject::PdfObject( const PdfString & rsString )
    : PdfVariant( rsString ), m_reference( (unsigned long)-1, (unsigned long)-1 )
{
    Init( false );
}

PdfObject::PdfObject( const PdfName & rName )
    : PdfVariant( rName ), m_reference( (unsigned long)-1, (unsigned long)-1 )
{
    Init( false );
}

PdfObject::PdfObject( const PdfReference & rRef )
    : PdfVariant( rRef ), m_reference( (unsigned long)-1, (unsigned long)-1 )
{
    Init( false );
}

PdfObject::PdfObject( const PdfArray & tList )
    : PdfVariant( tList ), m_reference( (unsigned long)-1, (unsigned long)-1 )
{
    Init( false );
}

PdfObject::PdfObject( const PdfDictionary & rDict )
    : PdfVariant( rDict ), m_reference( (unsigned long)-1, (unsigned long)-1 )
{
    Init( false );
}

PdfObject::PdfObject( const PdfObject & rhs )
{
    Init( false );

    operator=( rhs );
}

PdfObject::~PdfObject()
{
    delete m_pStream;
    m_pStream = NULL;
}

void PdfObject::Init( bool bLoadOnDemandDone )
{
    m_pStream                 = NULL;

    m_bLoadOnDemandDone       = bLoadOnDemandDone;
    m_bLoadStreamOnDemandDone = bLoadOnDemandDone;
}

void PdfObject::WriteObject( PdfOutputDevice* pDevice, const PdfName & keyStop ) const
{
    bool          bIndirect = ( (long)m_reference.ObjectNumber() != -1  && (long)m_reference.GenerationNumber() != -1 );
    bool          bIsTrailer = ( (long)m_reference.ObjectNumber() == 0  && (long)m_reference.GenerationNumber() == 0 );

    DelayedStreamLoad();

    if( !pDevice )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( bIndirect && !bIsTrailer )
        pDevice->Print( "%i %i obj\n", m_reference.ObjectNumber(), m_reference.GenerationNumber() );

    this->Write( pDevice, keyStop );
    if( !this->IsDictionary() )
        pDevice->Print( "\n" );

    if( m_pStream )
    {
        pDevice->Print( "stream\n" );
        pDevice->Write( m_pStream->Get(), m_pStream->Length() );
        pDevice->Print( "\nendstream\n" );
    }

    if( bIndirect )
        pDevice->Print( "endobj\n" );
}

PdfObject* PdfObject::GetIndirectKey( const PdfName & key )
{
    PdfObject* pObj = NULL;

    if( this->IsDictionary() && this->GetDictionary().HasKey( key ) )
    {
        pObj = this->GetDictionary().GetKey( key );
        if( pObj->IsReference() ) 
        {
            if( !m_pParent )
            {
                RAISE_ERROR( ePdfError_InvalidHandle );
            }
            pObj = m_pParent->GetObject( pObj->GetReference() );
        }
        else
            pObj->SetParent( GetParent() );	// even directs might want a parent...
    }

    return pObj;
}

unsigned long PdfObject::GetObjectLength()
{
    PdfOutputDevice device;

    this->WriteObject( &device );

    return device.Length();
}

PdfStream* PdfObject::GetStream()
{
    DelayedStreamLoad();

    if( !m_pStream )
        m_pStream = new PdfStream( this );
    
    return m_pStream;
}

const PdfStream* PdfObject::GetStream() const
{
    DelayedStreamLoad();

    return m_pStream;
}

void PdfObject::FlateCompressStream() 
{
    DelayedStreamLoad();

    if( m_pStream )
        m_pStream->FlateCompress();
}

const PdfObject & PdfObject::operator=( const PdfObject & rhs )
{
    delete m_pStream;

    Init( true );

    if( !rhs.m_bLoadOnDemandDone || !rhs.m_bLoadStreamOnDemandDone ) 
    {
        PdfObject* p = const_cast<PdfObject*>(&rhs);
        p->LoadStreamOnDemand();
    }

    m_reference     = rhs.m_reference;

    PdfVariant::operator=( rhs );

    m_bLoadOnDemandDone       = true;
    m_bLoadStreamOnDemandDone = true;

    if( rhs.m_pStream )
        m_pStream = new PdfStream( *(rhs.m_pStream) ); 

    return *this;
}

unsigned long PdfObject::GetByteOffset( const char* pszKey )
{
    PdfOutputDevice device;

    if( !pszKey )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( !this->GetDictionary().HasKey( pszKey ) )
    {
        RAISE_ERROR( ePdfError_InvalidKey );
    }

    this->Write( &device, pszKey );
    
    return device.Length();
}

};
