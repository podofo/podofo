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

#include "PdfObject.h"

namespace PoDoFo {

PdfImmediateWriter::PdfImmediateWriter( PdfOutputDevice* pDevice, PdfVecObjects* pVecObjects, EPdfVersion eVersion )
    : PdfWriter( pVecObjects ), m_pParent( pVecObjects ), m_pDevice( pDevice )
{
    // register as observer for PdfVecObjects

    // start with writing the header
    this->SetPdfVersion( eVersion );
    this->WritePdfHeader( m_pDevice );
}

PdfImmediateWriter::~PdfImmediateWriter()
{
    // calling here is too late, as the PdfVecObjects in PdfDocument
    // is already cleared.
    // 
    this->WriteOut();
}

void PdfImmediateWriter::WriteOut()
{
    TCIVecObjects  itObjects  = m_pParent->begin();

    this->CompressObjects( *m_pParent );

    while( itObjects != m_pParent->end() )
    {
        this->WriteObject( *itObjects );
        ++itObjects;
    }
}

void PdfImmediateWriter::WriteObject( const PdfObject* pObject )
{
    pObject->WriteObject( m_pDevice );
}

};

