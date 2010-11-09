/***************************************************************************
*   Copyriht (C) 2009 by Dominik Seichter                                *
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

#include "PdfPagesTreeCache.h"

#include "base/PdfDefinesPrivate.h"

#include "PdfPage.h"
#include "PdfPagesTree.h"

namespace PoDoFo {

PdfPagesTreeCache::PdfPagesTreeCache( int nInitialSize )
{
    m_deqPageObjs.resize( nInitialSize );
}

PdfPagesTreeCache::~PdfPagesTreeCache()
{
    this->ClearCache();
}

PdfPage* PdfPagesTreeCache::GetPage( int nIndex )
{
    if( nIndex < 0 || nIndex >= static_cast<int>(m_deqPageObjs.size()) ) 
    {
        PdfError::LogMessage( eLogSeverity_Error,
                              "PdfPagesTreeCache::GetPage( %i ) index out of range. Size of cache is %i\n",
                              nIndex, m_deqPageObjs.size() );
        return NULL;
    }

    return m_deqPageObjs[nIndex];
}

void PdfPagesTreeCache::AddPageObject( int nIndex, PdfPage* pPage )
{
    // Delete an old page if it is at the same position
    PdfPage* pOldPage = GetPage( nIndex );
    delete pOldPage;

    if( nIndex+1 > static_cast<int>(m_deqPageObjs.size()) )
    {
        m_deqPageObjs.resize( nIndex+1 );
    }

    m_deqPageObjs[nIndex] = pPage;
}

void PdfPagesTreeCache::InsertPage( int nIndex ) 
{
    if( nIndex == ePdfPageInsertionPoint_InsertBeforeFirstPage ) 
    {
        m_deqPageObjs.push_front( NULL );
    } 
    else
    {
        if( nIndex > static_cast<int>(m_deqPageObjs.size()) )
        {
            m_deqPageObjs.resize( nIndex );
        }
        
        m_deqPageObjs.insert( m_deqPageObjs.begin() + nIndex, static_cast<PdfPage*>(NULL) );
    }
}

void PdfPagesTreeCache::DeletePage( int nIndex )
{
    if( nIndex < 0 || nIndex >= static_cast<int>(m_deqPageObjs.size()) ) 
    {
        PdfError::LogMessage( eLogSeverity_Error,
                              "PdfPagesTreeCache::DeletePage( %i ) index out of range. Size of cache is %i\n",
                              nIndex, m_deqPageObjs.size() );
        return;
    }

    delete m_deqPageObjs[nIndex];
    m_deqPageObjs.erase( m_deqPageObjs.begin() + nIndex );
}

void PdfPagesTreeCache::ClearCache() 
{
    PdfPageList::iterator it = m_deqPageObjs.begin();

    while( it != m_deqPageObjs.end() )
    {
        delete (*it);
        ++it;
    }
        
    m_deqPageObjs.clear();
}

};
