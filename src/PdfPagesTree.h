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
***************************************************************************/

#ifndef _PDF_PAGES_TREE_H_
#define _PDF_PAGES_TREE_H_

#include "PdfArray.h"
#include "PdfDefines.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfObject;
class PdfPage;
class PdfRect;

typedef enum {
    PageInsertBeforeFirstPage	= -1,
    PageInsertLastPage		= -2,
    PageInsertAllPages		= -3,
    PageInsertOddPagesOnly	= -4,
    PageInsertEvenPagesOnly	= -5
} PageInsertionPoints;

/** Class for managing the tree of Pages in a PDF document
 *  Don't use this class directly. Use PdfDocument instead.
 *  
 *  \see PdfDocument
 */
class PODOFO_API PdfPagesTree : public PdfElement
{
	typedef std::deque< PdfPage* >	PdfPageObjects;
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
     *  \param inAfterPageNumber an integer specifying after what page - may be one of the special values
     *  \param pPage musst be a PdfObject with type /Page
     */
    void InsertPage( int inAfterPageNumber, PdfObject* pPage );

    /** Inserts an existing page object into the internal page tree. 
     *	after the specified page number
     *
     *  \param inAfterPageNumber an integer specifying after what page - may be one of the special values
     *  \param inPage a PdfPage to be inserted
     */
    void InsertPage( int inAfterPageNumber, PdfPage* inPage );

    /** Creates a new page object and inserts it into the internal
     *  page tree.
     *  The returned page is owned by the pages tree and will get deleted along
     *  with it!
     *
     *  \param rSize a PdfRect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \returns a pointer to a PdfPage object
     */
    PdfPage* CreatePage( const PdfRect & rSize );
    
    /**  Delete the specified page object from the internal pages tree.
     *   It does NOT remove any PdfObjects from memory - just the reference from the tree
     *
     *   \param inPageNumber the page number (0-based) to be removed
     *
     *   The PdfPage object refering to this page will be deleted by this call!
     */
    void DeletePage( int inPageNumber );

 private:
    PdfPagesTree();	// don't allow construction from nothing!

    /** Private method for actually traversing the /Pages tree
     *
     *  \param rListOfParents all parents of the page node will be added to this lists,
     *                        so that the PdfPage can later access inherited attributes
     */
    PdfObject* GetPageNode( int nPageNum, PdfObject* pPagesObject, std::deque<PdfObject*> & rListOfParents );

    /** Private method for actually traversing the /Pages tree
     *  This method directly traverses the tree and does no
     *  optimization for nodes with only one element like GetPageNode does.
     *
     *  \param rListOfParents all parents of the page node will be added to this lists,
     *                        so that the PdfPage can later access inherited attributes
     */
    PdfObject* GetPageNodeFromTree( int nPageNum, const PdfArray & kidsArray, std::deque<PdfObject*> & rListOfParents );

    /** Private method to access the Root of the tree using a logical name
     */
    PdfObject* GetRoot()	{ return m_pObject; }
    const PdfObject* GetRoot() const	{ return m_pObject; }
    
    /** Private method for getting the Parent of a node in the /Pages tree
     */
    static PdfObject* GetParent( PdfObject* inObject );
    
    /** Private method for getting the Kids of a node in the /Pages tree
     */
    static PdfObject* GetKids( PdfObject* inObject );
    
    /** Private method for determining where a page is in the /Pages tree
     */
    int GetPosInKids( PdfObject* inPageObj );
    
    /** Private method for adjusting the page count in a tree
     */
    int ChangePagesCount( PdfObject* inPageObj, int inDelta );
    
    /** Private method for cleaning up after an insertion
     */
    void InsertPages( int inAfterIndex, PdfObject* inPageOrPagesObj, PdfObject* inParentObj, int inNumPages );

    /** Private method for getting the PdfObject* for a Page from a specific /Kids array
     */
    PdfObject* GetPageFromKidArray( const PdfArray& inArray, int inIndex ) const;

  private:
    PdfPageObjects    m_deqPageObjs;
    
};

};

#endif // _PDF_PAGES_TREE_H_


