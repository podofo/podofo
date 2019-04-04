/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#ifndef _PDF_PAGES_TREE_H_
#define _PDF_PAGES_TREE_H_

#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfArray.h"

#include "PdfElement.h"
#include "PdfPagesTreeCache.h"

namespace PoDoFo {

class PdfObject;
class PdfPage;
class PdfRect;

enum EPdfPageInsertionPoint {
    ePdfPageInsertionPoint_InsertBeforeFirstPage	= -1,
    ePdfPageInsertionPoint_InsertLastPage		= -2,
    ePdfPageInsertionPoint_InsertAllPages		= -3,
    ePdfPageInsertionPoint_InsertOddPagesOnly	= -4,
    ePdfPageInsertionPoint_InsertEvenPagesOnly	= -5
};

/** Class for managing the tree of Pages in a PDF document
 *  Don't use this class directly. Use PdfDocument instead.
 *  
 *  \see PdfDocument
 */
class PODOFO_DOC_API PdfPagesTree : public PdfElement
{
	typedef std::deque< PdfObject* >  PdfObjectList;

 public:
    /** Construct a new PdfPagesTree
     */
    PdfPagesTree( PdfVecObjects* pParent );

    /** Construct a PdfPagesTree from the root /Pages object
     *  \param pPagesRoot pointer to page tree dictionary
     */
    PdfPagesTree( PdfObject* pPagesRoot );
    
    /** Close/down destruct a PdfPagesTree
     */
    virtual ~PdfPagesTree();
    
    /** Return the number of pages in the entire tree
     *  \returns number of pages
     */
    int GetTotalNumberOfPages() const;
    
    /** Return a PdfPage for the specified Page index
     *  The returned page is owned by the pages tree and
     *  deleted along with it.
     *
     *  \param nIndex page index, 0-based
     *  \returns a pointer to the requested page
     */
    PdfPage* GetPage( int nIndex );

    /** Return a PdfPage for the specified Page reference.
     *  The returned page is owned by the pages tree and
     *  deleted along with it.
     *
     *  \param ref the reference of the pages object
     *  \returns a pointer to the requested page
     */
    PdfPage* GetPage( const PdfReference & ref );

    /** Inserts an existing page object into the internal page tree. 
     *	after the specified page number
     *
     *  \param nAfterPageIndex an integer specifying after what page
     *         - may be one of the special values from EPdfPageInsertionPoint.
     *         Pages are 0 based.
     *         
     *  \param pPage musst be a PdfObject with type /Page
     */
    void InsertPage( int nAfterPageIndex, PdfObject* pPage );

    /** Inserts an existing page object into the internal page tree. 
     *	after the specified page number
     *
     *  \param nAfterPageIndex an integer specifying after what page
     *         - may be one of the special values  from EPdfPageInsertionPoint.
     *         Pages are 0 based.
     *  \param pPage a PdfPage to be inserted, the PdfPage will not get owned by the PdfPagesTree
     */
    void InsertPage( int nAfterPageIndex, PdfPage* pPage );

    /** Inserts a vector of page objects at once into the internal page tree
     *  after the specified page index (zero based index)
     *
     *  \param nAfterPageIndex a zero based integer index specifying after what page to insert
     *         - you need to pass ePdfPageInsertionPoint_InsertBeforeFirstPage if you want to insert before the first page.
     *         
     *  \param vecPages must be a vector of PdfObjects with type /Page
     */
    void InsertPages( int nAfterPageIndex, const std::vector<PdfObject*>& vecPages );

    /** Creates a new page object and inserts it into the internal
     *  page tree.
     *  The returned page is owned by the pages tree and will get deleted along
     *  with it!
     *
     *  \param rSize a PdfRect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \returns a pointer to a PdfPage object
     */
    PdfPage* CreatePage( const PdfRect & rSize );

    /** Creates several new page objects and inserts them into the internal
     *  page tree.
     *  The new pages are owned by the pages tree and will get deleted along
     *  with it!
	 *  Note: this function will attach all new pages onto the same page node
	 *  which can cause the tree to be unbalanced if 
     *
     *  \param vecSizes a vector of PdfRect specifying the size of each of the pages to create (i.e the /MediaBox key) in PDF units
     */
    void CreatePages( const std::vector<PdfRect>& vecSizes );

    /** Creates a new page object and inserts it at index atIndex.
     *  The returned page is owned by the pages tree and will get deleted along
     *  with it!
     *
     *  \param rSize a PdfRect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \param atIndex index where to insert the new page (0-based)
     *  \returns a pointer to a PdfPage object
     */
    PdfPage* InsertPage( const PdfRect & rSize, int atIndex);

    /**  Delete the specified page object from the internal pages tree.
     *   It does NOT remove any PdfObjects from memory - just the reference from the tree
     *
     *   \param inPageNumber the page number (0-based) to be removed
     *
     *   The PdfPage object refering to this page will be deleted by this call!
     *   Empty page nodes will also be deleted.
     *
     *   \see PdfMemDocument::DeletePages
     */
    void DeletePage( int inPageNumber );

    /**
     * Clear internal cache of PdfPage objects.
     * All references to PdfPage object will become invalid
     * when calling this method. All PdfPages will be deleted.
     *
     * You normally will never have to call this method.
     * It is only useful if one modified the page nodes 
     * of the pagestree manually.
     *
     */
    inline void ClearCache();

 private:
    PdfPagesTree();	// don't allow construction from nothing!

    PdfObject* GetPageNode( int nPageNum, PdfObject* pParent, PdfObjectList & rLstParents );

    int GetChildCount( const PdfObject* pNode ) const;

    /**
     * Test if a PdfObject is a page node
     * @return true if PdfObject is a page node
     */
    bool IsTypePage( const PdfObject* pObject ) const; 

    /**
     * Test if a PdfObject is a pages node
     * @return true if PdfObject is a pages node
     */
    bool IsTypePages( const PdfObject* pObject ) const; 

    /**
     * Find the position of pPageObj in the kids array of pPageParent
     *
     * @returns the index in the kids array or -1 if pPageObj is no child of pPageParent
     */
    int GetPosInKids( PdfObject* pPageObj, PdfObject* pPageParent );

    /** Private method for adjusting the page count in a tree
     */
    int ChangePagesCount( PdfObject* inPageObj, int inDelta );

    /**
     * Insert a page object into a pages node
     *
     * @param pNode the pages node whete pPage is to be inserted
     * @param rlstParents list of all (future) parent pages nodes in the pages tree
     *                   of pPage
     * @param nIndex index where pPage is to be inserted in pNode's kids array
     * @param pPage the page object which is to be inserted
     */
    void InsertPageIntoNode( PdfObject* pNode, const PdfObjectList & rlstParents, 
                             int nIndex, PdfObject* pPage );

     /**
     * Insert a vector of page objects into a pages node
     * Same as InsertPageIntoNode except that it allows for adding multiple pages at one time
	 * Note that adding many pages onto the same node will create an unbalanced page tree
     *
     * @param pNode the pages node whete pPage is to be inserted
     * @param rlstParents list of all (future) parent pages nodes in the pages tree
     *                   of pPage
     * @param nIndex index where pPage is to be inserted in pNode's kids array
     * @param vecPages a vector of the page objects which are to be inserted
     */
    void InsertPagesIntoNode( PdfObject* pParent, const PdfObjectList & rlstParents, 
                              int nIndex, const std::vector<PdfObject*>& vecPages );
    
    /**
     * Delete a page object from a pages node
     *
     * @param pNode which is the direct parent of pPage and where the page must be deleted
     * @param rlstParents list of all parent pages nodes in the pages tree
     *                   of pPage
     * @param nIndex index where pPage is to be deleted in pNode's kids array
     * @param pPage the page object which is to be deleted
     */
    void DeletePageFromNode( PdfObject* pNode, const PdfObjectList & rlstParents, 
                             int nIndex, PdfObject* pPage );

    /**
     * Delete a single page node or page object from the kids array of pParent
     *
     * @param pParent the parent of the page node which is deleted
     * @param nIndex index to remove from the kids array of pParent
     */
    void DeletePageNode( PdfObject* pParent, int nIndex );

    /**
     * Tests if a page node is emtpy
     *
     * @returns true if Count of page is 0 or the Kids array is empty
     */
    bool IsEmptyPageNode( PdfObject* pPageNode );

    /** Private method for actually traversing the /Pages tree
     *
     *  \param rListOfParents all parents of the page node will be added to this lists,
     *                        so that the PdfPage can later access inherited attributes
     */
    /*
    PdfObject* GetPageNode( int nPageNum, PdfObject* pPagesObject, 
                            std::deque<PdfObject*> & rListOfParents );
    */

    /** Private method for actually traversing the /Pages tree
     *  This method directly traverses the tree and does no
     *  optimization for nodes with only one element like GetPageNode does.
     *
     *  \param rListOfParents all parents of the page node will be added to this lists,
     *                        so that the PdfPage can later access inherited attributes
     */
    /*
    PdfObject* GetPageNodeFromTree( int nPageNum, const PdfArray & kidsArray, 
                                    std::deque<PdfObject*> & rListOfParents );

    */
    /** Private method to access the Root of the tree using a logical name
     */
    PdfObject* GetRoot()	{ return this->GetObject(); }
    const PdfObject* GetRoot() const	{ return this->GetObject(); }

private:
    PdfPagesTreeCache m_cache;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfPagesTree::ClearCache() 
{
    m_cache.ClearCache();
}

};

#endif // _PDF_PAGES_TREE_H_


