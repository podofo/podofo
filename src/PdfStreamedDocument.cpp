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

#include "PdfStreamedDocument.h"

#include "PdfImmediateWriter.h"

namespace PoDoFo {

PdfStreamedDocument::PdfStreamedDocument( PdfOutputDevice* pDevice )
    : m_pWriter( NULL ), m_pDevice( NULL )
{
    Init( pDevice );
}

PdfStreamedDocument::PdfStreamedDocument( const char* pszFilename )
    : m_pWriter( NULL )
{
    m_pDevice = new PdfOutputDevice( pszFilename );
    Init( m_pDevice );
}

PdfStreamedDocument::~PdfStreamedDocument()
{
    this->Close();

    delete m_pWriter;
    delete m_pDevice;
}

void PdfStreamedDocument::Init( PdfOutputDevice* pDevice )
{
    m_pWriter = new PdfImmediateWriter( pDevice, &(m_doc.GetObjects()), m_doc.GetTrailer() );
}

void PdfStreamedDocument::Close()
{

}



};
