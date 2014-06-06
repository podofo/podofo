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

    if( nIndex >= static_cast<int>(m_deqPageObjs.size()) )
    {
        m_deqPageObjs.resize( nIndex + 1 );
    }

    m_deqPageObjs[nIndex] = pPage;
}

void PdfPagesTreeCache::AddPageObjects( int nIndex, std::vector<PdfPage*> vecPages )
{
    if( (nIndex + static_cast<int>(vecPages.size())) >= static_cast<int>(m_deqPageObjs.size()) )
    {
        m_deqPageObjs.resize( nIndex + vecPages.size() + 1 );
    }
    
    for (size_t i=0; i<vecPages.size(); ++i)
    {
        // Delete any old pages if it is at the same position
        PdfPage* pOldPage = GetPage( nIndex + i );
        delete pOldPage;

        // Assign the new page
        m_deqPageObjs[nIndex + i]  = vecPages.at(i);
    }
}

void PdfPagesTreeCache::InsertPage( int nAfterPageIndex ) 
{
    const int nBeforeIndex = ( nAfterPageIndex == ePdfPageInsertionPoint_InsertBeforeFirstPage ) ? 0 : nAfterPageIndex+1;

    if( nBeforeIndex >= static_cast<int>(m_deqPageObjs.size()) )
        m_deqPageObjs.resize( nBeforeIndex + 1 );

    m_deqPageObjs.insert( m_deqPageObjs.begin() + nBeforeIndex, static_cast<PdfPage*>(NULL) );
}

void PdfPagesTreeCache::InsertPages( int nAfterPageIndex, int nCount ) 
{
    const int nBeforeIndex = ( nAfterPageIndex == ePdfPageInsertionPoint_InsertBeforeFirstPage ) ? 0 : nAfterPageIndex+1;

    if( nBeforeIndex+nCount >= static_cast<int>(m_deqPageObjs.size()) )
        m_deqPageObjs.resize( nBeforeIndex + nCount + 1 );

    for (int i=0; i<nCount; ++i)
        m_deqPageObjs.insert( m_deqPageObjs.begin() + nBeforeIndex + i, static_cast<PdfPage*>(NULL) );
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
