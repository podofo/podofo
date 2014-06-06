/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#include "PdfImmediateWriter.h"

#include "PdfFileStream.h"
#include "PdfMemStream.h"
#include "PdfObject.h"
#include "PdfXRef.h"
#include "PdfXRefStream.h"
#include "PdfDefinesPrivate.h"

namespace PoDoFo {

PdfImmediateWriter::PdfImmediateWriter( PdfOutputDevice* pDevice, PdfVecObjects* pVecObjects, 
                                        const PdfObject* pTrailer, EPdfVersion eVersion, 
                                        PdfEncrypt* pEncrypt, EPdfWriteMode eWriteMode )
    : PdfWriter( pVecObjects ), m_pParent( pVecObjects ), 
      m_pDevice( pDevice ), m_pLast( NULL ), m_bOpenStream( false )
{
    if( m_pTrailer )
        delete m_pTrailer;
    m_pTrailer = new PdfObject( *pTrailer );

    // register as observer for PdfVecObjects
    m_pParent->Attach( this );
    // register as stream factory for PdfVecObjects
    m_pParent->SetStreamFactory( this );

    this->CreateFileIdentifier( m_identifier, m_pTrailer );
    // setup encryption
    if( pEncrypt )
    {
        this->SetEncrypted( *pEncrypt );
        m_pEncrypt->GenerateEncryptionKey( m_identifier );
    }

    // start with writing the header
    this->SetPdfVersion( eVersion );
    this->SetWriteMode( eWriteMode );
    this->WritePdfHeader( m_pDevice );

    m_pXRef = m_bXRefStream ? new PdfXRefStream( m_vecObjects, this ) : new PdfXRef();

}

PdfImmediateWriter::~PdfImmediateWriter()
{
    if( m_pParent ) 
        m_pParent->Detach( this );
    
    delete m_pXRef;
}

void PdfImmediateWriter::WriteObject( const PdfObject* pObject )
{
    const int endObjLenght = 7;

    this->FinishLastObject();

    m_pXRef->AddObject( pObject->Reference(), m_pDevice->Tell(), true );
    pObject->WriteObject( m_pDevice, this->GetWriteMode(), m_pEncrypt );
    // Make sure, no one will add keys now to the object
    const_cast<PdfObject*>(pObject)->SetImmutable(true);

    // Let's cheat a bit:
    // pObject has written an "endobj\n" as last data to the file.
    // we simply overwrite this string with "stream\n" which 
    // has excatly the same length.
    m_pDevice->Seek( m_pDevice->Tell() - endObjLenght );
    m_pDevice->Print( "stream\n" );
    m_pLast = const_cast<PdfObject*>(pObject);
}

void PdfImmediateWriter::ParentDestructed()
{
    m_pParent = NULL;
}

void PdfImmediateWriter::Finish()
{
    // write all objects which are still in RAM
    this->FinishLastObject();

    // setup encrypt dictionary
    if( m_pEncrypt )
    {
        // Add our own Encryption dictionary
        m_pEncryptObj = m_vecObjects->CreateObject();
        m_pEncrypt->CreateEncryptionDictionary( m_pEncryptObj->GetDictionary() );
    }

    this->WritePdfObjects( m_pDevice, *m_pParent, m_pXRef );

    // write the XRef
    pdf_long lXRefOffset = m_pDevice->Tell();
    m_pXRef->Write( m_pDevice );
            
    // XRef streams contain the trailer in the XRef
    if( !m_bXRefStream ) 
    {
        PdfObject trailer;
        
        // if we have a dummy offset we write also a prev entry to the trailer
        FillTrailerObject( &trailer, m_pXRef->GetSize(), false, false );
        
        m_pDevice->Print("trailer\n");
        trailer.WriteObject( m_pDevice, this->GetWriteMode(), NULL );
    }
    
    m_pDevice->Print( "startxref\n%li\n%%%%EOF\n", lXRefOffset );
    m_pDevice->Flush();

    // we are done now
    m_pParent->Detach( this );
    m_pParent = NULL;
}

PdfStream* PdfImmediateWriter::CreateStream( PdfObject* pParent )
{
    return m_bOpenStream ? 
        static_cast<PdfStream*>(new PdfMemStream( pParent )) :
        static_cast<PdfStream*>(new PdfFileStream( pParent, m_pDevice ));
}

void PdfImmediateWriter::FinishLastObject()
{
    if( m_pLast ) 
    {
        m_pDevice->Print( "\nendstream\n" );
        m_pDevice->Print( "endobj\n" );
        
        delete m_pParent->RemoveObject( m_pLast->Reference(), false );
        m_pLast = NULL;

    }
}

void PdfImmediateWriter::BeginAppendStream( const PdfStream* pStream )
{
    const PdfFileStream* pFileStream = dynamic_cast<const PdfFileStream*>(pStream );
    if( pFileStream ) 
    {
        // Only one open file stream is allowed at a time
        assert( !m_bOpenStream );
        m_bOpenStream = true;

        if( m_pEncrypt )
            const_cast<PdfFileStream*>(pFileStream)->SetEncrypted( m_pEncrypt );
    }
}
    
void PdfImmediateWriter::EndAppendStream( const PdfStream* pStream )
{
    const PdfFileStream* pFileStream = dynamic_cast<const PdfFileStream*>(pStream );
    if( pFileStream ) 
    {
        // A PdfFileStream has to be opened before
        assert( m_bOpenStream );
        m_bOpenStream = false;
    }
}

};

