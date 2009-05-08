/***************************************************************************
 *   Copyright (C) 2009 by Dominik Seichter                                *
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

#include "MoveOperation.h"

#include <podofo.h>

MoveOperation::MoveOperation( int nFrom, int nTo )
    : m_nFrom( nFrom ), m_nTo( nTo ) 
{
}

void MoveOperation::Perform( PoDoFo::PdfDocument & rDoc )
{
    PoDoFo::PdfPagesTree* pTree = rDoc.GetPagesTree();
    PoDoFo::PdfPage* pPage = pTree->GetPage( m_nFrom );

    int from = m_nFrom;
    pTree->InsertPage( m_nTo, pPage );
    
    if( m_nTo < from ) 
    {
        // If we inserted the page before the old 
        // position we have to increment the from position
        from++;
    }

    pTree->DeletePage( from );
}

std::string MoveOperation::ToString() const
{
    std::ostringstream oss;

    oss << "Moving page " << m_nFrom << " to " << m_nTo << "." << std::endl;

    return oss.str();
}

