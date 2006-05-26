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

UnCompress::UnCompress()
{
    m_pParser = new PdfParser();
    m_pWriter = new PdfWriter();
}

UnCompress::~UnCompress()
{
    delete m_pParser;
    delete m_pWriter;
}

PdfError UnCompress::Init( const char* pszInput, const char* pszOutput )
{
    PdfError eCode;

    SAFE_OP( m_pParser->Init( pszInput ) );

    SAFE_OP( this->UncompressObjects() );

    SAFE_OP( m_pWriter->Init( m_pParser ) );

    m_pWriter->SetPdfCompression( false );

    SAFE_OP( m_pWriter->Write( pszOutput ) );

    return eCode;
}

PdfError UnCompress::UncompressObjects()
{
    PdfError     eCode;
    TVecObjects  vecObj = m_pParser->GetObjects();
    TIVecObjects it     = vecObj.begin();

    long         lLen;
    char*        pBuffer;

    while( it != vecObj.end() )
    {
        if( (*it)->HasStream() )
        {
            SAFE_OP( (*it)->Stream()->GetFilteredCopy( &pBuffer, &lLen ) );
            SAFE_OP( (*it)->Stream()->Set( pBuffer, lLen ) );

            (*it)->RemoveKey( "Filter" );            
        }

        ++it;
    }

    return eCode;
}
