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
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#include "PdfObject.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfFileStream.h"
#include "PdfOutputDevice.h"
#include "PdfStream.h"
#include "PdfVariant.h"
#include "PdfDefinesPrivate.h"

#include <sstream>
#include <fstream>
#include <cassert>

#include <string.h>

using namespace std;

namespace PoDoFo {

PdfObject::PdfObject()
    : PdfVariant( PdfDictionary() )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfReference & rRef, const char* pszType )
    : PdfVariant( PdfDictionary() ), m_reference( rRef )
{
    InitPdfObject();

    if( pszType )
        this->GetDictionary().AddKey( PdfName::KeyType, PdfName( pszType ) );
}

PdfObject::PdfObject( const PdfReference & rRef, const PdfVariant & rVariant )
    : PdfVariant( rVariant ), m_reference( rRef )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfVariant & var )
    : PdfVariant( var )
{
    InitPdfObject();
}

PdfObject::PdfObject( bool b )
    : PdfVariant( b )
{
    InitPdfObject();
}

PdfObject::PdfObject( pdf_int64 l )
    : PdfVariant( l )
{
    InitPdfObject();
}

PdfObject::PdfObject( double d )
    : PdfVariant( d )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfString & rsString )
    : PdfVariant( rsString )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfName & rName )
    : PdfVariant( rName )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfReference & rRef )
    : PdfVariant( rRef )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfArray & tList )
    : PdfVariant( tList )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfDictionary & rDict )
    : PdfVariant( rDict )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfObject & rhs ) 
    : PdfVariant( rhs ), m_reference( rhs.m_reference )
{
    InitPdfObject();

    // DS: If you change this code, also change the assignment operator.
    //     As the copy constructor is called very often,
    //     it contains a copy of parts of the assignment code to be faster.
    const_cast<PdfObject*>(&rhs)->DelayedStreamLoad();
    m_bDelayedStreamLoadDone = rhs.DelayedStreamLoadDone();

    if( rhs.m_pStream && m_pOwner )
        m_pStream = m_pOwner->CreateStream( *(rhs.m_pStream) );

#if defined(PODOFO_EXTRA_CHECKS)
    // Must've been demand loaded or already done
    assert(DelayedLoadDone());
    assert(DelayedStreamLoadDone());
#endif
}

PdfObject::~PdfObject()
{
    delete m_pStream;
    m_pStream = NULL;
}

void PdfObject::InitPdfObject()
{
    m_pStream                 = NULL;
    m_pOwner                  = NULL;

    m_bDelayedStreamLoadDone  = true;

#if defined(PODOFO_EXTRA_CHECKS)
    m_bDelayedStreamLoadInProgress = false;
#endif
}

void PdfObject::WriteObject( PdfOutputDevice* pDevice, EPdfWriteMode eWriteMode,
                             PdfEncrypt* pEncrypt, const PdfName & keyStop ) const
{
    DelayedStreamLoad();

    if( !pDevice )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_reference.IsIndirect() )
    {
        if( (eWriteMode & ePdfWriteMode_Clean) == ePdfWriteMode_Clean ) 
        {
            pDevice->Print( "%i %i obj\n", m_reference.ObjectNumber(), m_reference.GenerationNumber() );
        }
        else 
        {
            pDevice->Print( "%i %i obj", m_reference.ObjectNumber(), m_reference.GenerationNumber() );
        }
    }

    if( pEncrypt ) 
    {
        pEncrypt->SetCurrentReference( m_reference );
    }

    if( pEncrypt && m_pStream )
    {
        // Set length if it is a key
        PdfFileStream* pFileStream = dynamic_cast<PdfFileStream*>(m_pStream);
        if( !pFileStream )
        {
            // PdfFileStream handles encryption internally
            pdf_long lLength = pEncrypt->CalculateStreamLength(m_pStream->GetLength());
            PdfVariant varLength = static_cast<pdf_int64>(lLength);
            *(const_cast<PdfObject*>(this)->GetIndirectKey( PdfName::KeyLength )) = varLength;
        }
    }

    this->Write( pDevice, eWriteMode, pEncrypt, keyStop );
    pDevice->Print( "\n" );

    if( m_pStream )
    {
        m_pStream->Write( pDevice, pEncrypt );
    }

    if( m_reference.IsIndirect() )
    {
        pDevice->Print( "endobj\n" );
    }
}

PdfObject* PdfObject::GetIndirectKey( const PdfName & key ) const
{
    const PdfObject* pObj = NULL;

    if( this->IsDictionary() && this->GetDictionary().HasKey( key ) )
    {
        pObj = this->GetDictionary().GetKey( key );
        if( pObj->IsReference() ) 
        {
            if( !m_pOwner )
            {
                PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, "Object is a reference but does not have an owner!" );
            }

            pObj = m_pOwner->GetObject( pObj->GetReference() );
        }
        else
            const_cast<PdfObject*>(pObj)->SetOwner( GetOwner() );// even directs might want an owner...
    }

    // DominikS: TODO Remove const on GetIndirectKey
    return const_cast<PdfObject*>(pObj);
}

pdf_long PdfObject::GetObjectLength( EPdfWriteMode eWriteMode )
{
    PdfOutputDevice device;

    this->WriteObject( &device, eWriteMode, NULL  );

    return device.GetLength();
}

PdfStream* PdfObject::GetStream()
{
    DelayedStreamLoad();
    return GetStream_NoDL();
}

PdfStream* PdfObject::GetStream_NoDL()
{
    if( !m_pStream )
    {
        if ( GetDataType() != ePdfDataType_Dictionary )
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "Tried to get stream of non-dictionary object");
	    }
        if ( !m_reference.IsIndirect() )
		{
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "Tried to get stream of non-indirect PdfObject");
		}
        if( !m_pOwner ) 
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, "Tried to create stream on PdfObject lacking owning document/PdfVecObjects" );
        }

        m_pStream = m_pOwner->CreateStream( this );
    }

    SetDirty( true );
    return m_pStream;
}

const PdfStream* PdfObject::GetStream() const
{
    DelayedStreamLoad();

    return m_pStream;
}

void PdfObject::FlateCompressStream() 
{
    // TODO: If the stream isn't already in memory, defer loading and compression until first read of the stream to save some memory.
    DelayedStreamLoad();

    /*
    if( m_pStream )
        m_pStream->FlateCompress();
    */
}

const PdfObject & PdfObject::operator=( const PdfObject & rhs )
{
    // DS: If you change this code, also change the copy constructor.
    //     As the copy constructor is called very often,
    //     it contains a copy of parts of this code to be faster.

    delete m_pStream;

    const_cast<PdfObject*>(&rhs)->DelayedStreamLoad();

    m_reference     = rhs.m_reference;
    m_pOwner        = rhs.m_pOwner;

    PdfVariant::operator=( rhs );

    m_bDelayedStreamLoadDone = rhs.DelayedStreamLoadDone();

    if( rhs.m_pStream )
        m_pStream = m_pOwner->CreateStream( *(rhs.m_pStream) );

#if defined(PODOFO_EXTRA_CHECKS)
    // Must've been demand loaded or already done
    assert(DelayedLoadDone());
    assert(DelayedStreamLoadDone());
#endif

    return *this;
}

pdf_long PdfObject::GetByteOffset( const char* pszKey, EPdfWriteMode eWriteMode )
{
    PdfOutputDevice device;

    if( !pszKey )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( !this->GetDictionary().HasKey( pszKey ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidKey );
    }

    this->Write( &device, eWriteMode, NULL, pszKey );
    
    return device.GetLength();
}

};
