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

typedef std::deque< PdfPage* >	PdfPageObjects;

typedef enum {
	PageInsertBeforeFirstPage	= -1,
	PageInsertLastPage			= -2,
	PageInsertAllPages			= -3,
	PageInsertOddPagesOnly		= -4,
	PageInsertEvenPagesOnly		= -5
} PageInsertionPoints;

/** Class for managing the tree of Pages in a PDF document
 *  Don't use this class directly. Use PdfDocument instead.
 *  
 *  \see PdfDocument
 */
class PdfPagesTree : public PdfElement
{
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

    /** Inserts an existing page object into the internal page tree. 
	 *	after the specified page number
     *  The returned page is owned by the pages tree
     *  and will get deleted along with it!
     *
     *  \param inAfterPageNumber an integer specifying after what page - may be one of the special values
	 *  \param inPage a PdfPage to be inserted
     *  \returns a pointer to a PdfPage object
     */
	void InsertPage( int inAfterPageNumber, PdfPage* inPage );

   /** Creates a new page object and inserts it into the internal
     *  page tree. 
     *  The returned page is owned by the pages tree
     *  and will get deleted along with it!
     *
     *  \param rSize a PdfRect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \returns a pointer to a PdfPage object
     */
    PdfPage* CreatePage( const PdfRect & rSize );

 private:
    PdfPagesTree();	// don't allow construction from nothing!

    /** Private method for actually traversing the /Pages tree
     */
    PdfObject* GetPageNode( int nPageNum, PdfObject* pPagesObject );

	/** Private method to access the Root of the tree using a logical name
	 */
	PdfObject* GetRoot()	{ return m_pObject; }
    
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
	PdfObject* GetPageFromKidArray( PdfArray& inArray, int inIndex );

  private:
    PdfPageObjects    m_deqPageObjs;
    
};

};

#endif // _PDF_PAGES_TREE_H_


