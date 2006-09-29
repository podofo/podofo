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

    m_pDocument = new PdfDocument( pszInput );

    this->UncompressObjects();

    PdfWriter writer( m_pDocument );
    writer.SetPdfCompression( false );
    writer.Write( pszOutput );
}

void UnCompress::UncompressObjects()
{
    TVecObjects  vecObj = m_pDocument->GetObjects();
    TIVecObjects it     = vecObj.begin();

    while( it != vecObj.end() )
    {
        if( (*it)->HasStream() )
        {
            (*it)->GetStream()->Uncompress();
        }

        ++it;
    }
}
