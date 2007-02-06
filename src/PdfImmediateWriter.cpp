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
 ***************************************************************************/

#include "PdfImmediateWriter.h"

#include "PdfFileStream.h"
#include "PdfObject.h"
#include "PdfXRef.h"
#include "PdfXRefStream.h"

namespace PoDoFo {

PdfImmediateWriter::PdfImmediateWriter( PdfOutputDevice* pDevice, PdfVecObjects* pVecObjects, 
                                        const PdfObject* pTrailer, EPdfVersion eVersion )
    : PdfWriter( pVecObjects ), m_pParent( pVecObjects ), 
      m_pDevice( pDevice ), m_pLast( NULL )
{
    m_pTrailer = new PdfObject( *pTrailer );

    // register as observer for PdfVecObjects
    m_pParent->Attach( this );
    // register as stream factory for PdfVecObjects
    m_pParent->SetStreamFactory( this );

    // start with writing the header
    this->SetPdfVersion( eVersion );
    this->WritePdfHeader( m_pDevice );

    m_pXRef = m_bXRefStream ? new PdfXRefStream( m_vecObjects, this ) : new PdfXRef();
}

PdfImmediateWriter::~PdfImmediateWriter()
{
    if( m_pParent ) 
        m_pParent->Detach( this );
}

void PdfImmediateWriter::WriteObject( const PdfObject* pObject )
{
    const int endObjLenght = 7;

    this->FinishLastObject();

    m_pXRef->AddObject( pObject->Reference(), m_pDevice->GetLength(), true );
    pObject->WriteObject( m_pDevice );

    // Let's cheat a bit:
    // pObject has written an "endobj\n" as last data to the file.
    // we simply overwrite this string with "stream\n" which 
    // has excatly the same length.
    m_pDevice->Seek( m_pDevice->GetLength() - endObjLenght );
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
    this->WritePdfObjects( m_pDevice, *m_pParent, m_pXRef );

    // write the XRef
    long lXRefOffset = m_pDevice->GetLength();
    m_pXRef->Write( m_pDevice );
            
    // XRef streams contain the trailer in the XRef
    if( !m_bXRefStream ) 
    {
        PdfObject trailer;
        
        // if we have a dummy offset we write also a prev entry to the trailer
        FillTrailerObject( &trailer, m_pXRef->GetSize(), false, false );
        
        m_pDevice->Print("trailer\n");
        trailer.WriteObject( m_pDevice );
    }
    
    m_pDevice->Print( "startxref\n%li\n%%%%EOF\n", lXRefOffset );
    m_pDevice->Flush();

    // we are done now
    m_pParent->Detach( this );
    m_pParent = NULL;
}

PdfStream* PdfImmediateWriter::CreateStream( PdfObject* pParent )
{
    return new PdfFileStream( pParent, m_pDevice );
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

};

