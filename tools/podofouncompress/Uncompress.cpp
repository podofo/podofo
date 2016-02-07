/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "Uncompress.h"

#include <cstdio>

using namespace PoDoFo;

UnCompress::UnCompress()
    : m_pDocument( NULL )
{
}

UnCompress::~UnCompress()
{
    delete m_pDocument;
}

void UnCompress::Init( const char* pszInput, const char* pszOutput )
{
    if( m_pDocument )
        delete m_pDocument;

    m_pDocument = new PdfMemDocument( pszInput );

    this->UncompressObjects();

    PdfWriter writer( &(m_pDocument->GetObjects()), new PdfObject( *(m_pDocument->GetTrailer() ) ) );
    writer.SetWriteMode( ePdfWriteMode_Clean );
    writer.Write( pszOutput );
}

void UnCompress::UncompressObjects()
{
    TIVecObjects it     = m_pDocument->GetObjects().begin();

    while( it != m_pDocument->GetObjects().end() )
    {
        printf("Reading %i %i R\n", (*it)->Reference().ObjectNumber(), (*it)->Reference().GenerationNumber() );
        if( (*it)->HasStream() )
        {
            try {
                printf("-> Uncompressing object %i %i\n", 
                       (*it)->Reference().ObjectNumber(), (*it)->Reference().GenerationNumber() );
                PdfMemStream* pStream = dynamic_cast<PdfMemStream*>((*it)->GetStream());
                printf("-> Original Length: %" PDF_FORMAT_INT64 "\n", 
                       static_cast<pdf_int64>(pStream->GetLength()) );
                try {
                    pStream->Uncompress();
                } catch( const PdfError & e ) {
                    if( e.GetError() == ePdfError_Flate )
                    {
                        // Ignore ZLib errors
                        fprintf( stderr, "WARNING: ZLib error ignored for this object.\n");
                    }
                    else
                        throw e;
                }
                printf("-> Uncompressed Length: %" PDF_FORMAT_INT64 "\n", 
                       static_cast<pdf_int64>(pStream->GetLength()) );
            } catch( const PdfError & e ) {
                e.PrintErrorMsg();
                if( e.GetError() != ePdfError_UnsupportedFilter )
                    throw e;
            }
        }

        ++it;
    }
}
