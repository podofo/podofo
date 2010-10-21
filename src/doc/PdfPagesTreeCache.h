/***************************************************************************
*   Copyright (C) 2009 by Dominik Seichter                                *
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

#ifndef _PDF_PAGES_TREE_CACHE_H_
#define _PDF_PAGES_TREE_CACHE_H_

#include "base/PdfDefines.h"

namespace PoDoFo {

class PdfPage;

/**
 *  This class implements a cache infront of a PdfPagesTree
 *
 *  \see PdfCachedPagesTree
 */
class PODOFO_API PdfPagesTreeCache
{
	typedef std::deque< PdfPage* > PdfPageList;

 public:
    /** Construct a new PdfCachedPagesTree.
     *  
     *  @param nInitialSize initial size of the pagestree
     */
    PdfPagesTreeCache( int nInitialSize );
    
    /** Close/down destruct a PdfCachedPagesTree
     */
    virtual ~PdfPagesTreeCache();

    /** Return a PdfPage for the specified Page index
     *  The returned page is owned by the pages tree and
     *  deleted along with it.
     *
     *  \param nIndex page index, 0-based
     *  \returns a pointer to the requested page or NULL if it is not cached
     */
    virtual PdfPage* GetPage( int nIndex );

    /**
     * Add a PdfPage object to the cache
     * @param nIndex index of the page
     * @param pPage page object
     */
    virtual void AddPageObject( int nIndex, PdfPage* pPage );

    /**
     * A page was inserted into the pagestree,
     * therefore the cache has to be updated
     *
     * @param nIndex index where the page was inserted
     */
    virtual void InsertPage( int nIndex );

    /**
     * Delete a PdfPage from the cache
     * @param nIndex index of the page
     */
    virtual void DeletePage( int nIndex );

    /**
     * Clear cache, i.e. remove all elements from the 
     * cache.
     */
    virtual void ClearCache();

private:
    /**
     * Avoid construction of empty objects
     */
    PdfPagesTreeCache() { }

private:
    PdfPageList    m_deqPageObjs;
};

};

#endif // _PDF_PAGES_TREE_CACHE_H_


