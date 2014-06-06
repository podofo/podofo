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

#ifndef _PDF_PAGES_TREE_CACHE_H_
#define _PDF_PAGES_TREE_CACHE_H_

#include "podofo/base/PdfDefines.h"

namespace PoDoFo {

class PdfPage;

/**
 *  This class implements a cache infront of a PdfPagesTree
 *
 *  \see PdfCachedPagesTree
 */
class PODOFO_DOC_API PdfPagesTreeCache
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
     * Add several PdfPage objects to the cache, replacing any existing at the given index
     * @param nIndex zero based index of where the first page will be placed
     * @param vecPages vector of the page objects to add
     */
    virtual void AddPageObjects( int nIndex, std::vector<PdfPage*> vecPages );

    /**
     * A page was inserted into the pagestree,
     * therefore the cache has to be updated
     *
     * @param nAfterPageIndex zero based index of the page we are inserting after
	 *         - may be one of the special values  from EPdfPageInsertionPoint.
     */
    virtual void InsertPage( int nAfterPageIndex );

    /**
     * Insert several pages into the pagestree, after the given index
     * therefore the cache has to be updated
     *
     * @param nAfterPageIndex zero based index of the page we are inserting after
	 *         - may be one of the special values  from EPdfPageInsertionPoint.
     * @param nCount number of pages that were inserted
     */
    virtual void InsertPages( int nAfterPageIndex, int nCount );

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


