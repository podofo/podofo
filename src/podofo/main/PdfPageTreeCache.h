/**
 * SPDX-FileCopyrightText: (C) 2009 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_PAGES_TREE_CACHE_H
#define PDF_PAGES_TREE_CACHE_H

#include "PdfPage.h"

namespace PoDoFo {

/**
 *  This class implements a cache infront of a PdfPageTree
 *
 *  \see PdfCachedPagesTree
 */
class PODOFO_API PdfPageTreeCache final
{
    using List = std::vector<PdfPage*>;

public:
    /** Construct a new PdfCachedPagesTree.
     *
     *  \param nInitialSize initial size of the pagestree
     */
    PdfPageTreeCache(unsigned initialSize);

    /** Return a PdfPage for the specified Page index
     *  The returned page is owned by the pages tree and
     *  deleted along with it.
     *
     *  \param atIndex page index, 0-based
     *  \returns a pointer to the requested page or nullptr if it is not cached
     */
    PdfPage* GetPage(unsigned index);

    /**
     * Set a PdfPage object in the cache at the given index.
     * Existing object will be replaced
     *
     * \param atIndex index of the page
     * \param page page object
     */
    void SetPage(unsigned atIndex, PdfPage* page);

    /**
     * Add several PdfPage objects to the cache, replacing any existing at the given index
     * \param atIndex zero based index of where the first page will be placed
     * \param pages vector of the page objects to add
     */
    void SetPages(unsigned atIndex, const std::vector<PdfPage*>& pages);

    /**
     * Insert several page placeholders at the given index
     * therefore the cache has to be updated
     *
     * \param atIndex zero based index of the page we are inserting
     * \param count number of pages that were inserted
     */
    void InsertPlaceHolders(unsigned atIndex, unsigned count);

    /**
     * Delete a PdfPage from the cache
     * \param atIndex index of the page
     */
    void DeletePage(unsigned atIndex);

    /**
     * Clear cache, i.e. remove all elements from the
     * cache.
     */
    void ClearCache();

private:
    List m_PageObjs;
};

};

#endif // PDF_PAGES_TREE_CACHE_H
