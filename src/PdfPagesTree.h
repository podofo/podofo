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

#include "PdfDefines.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfObject;

typedef std::deque< PdfObject* >	PdfPageObjects;

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
    
    /** Return a PdfObject* for the specified Page index
     *  \param nIndex page index, 0-based
     *  \returns a PDFObject* for the given page
     */
    PdfObject* GetPage( int nIndex );

    /** Inserts a PdfObject in the pages tree
     *  \param pObject pointer to a pdf page object
     */
    void InsertPage( PdfObject* pObject );

 private:
    PdfPagesTree();	// don't allow construction from nothing!

    /** Private method for actually traversing the /Pages tree
     */
    PdfObject* GetPageNode( int nPageNum, PdfObject* pPagesObject );
    
 private:
    PdfPageObjects    m_deqPageObjs;
    
};

};

#endif // _PDF_PAGES_TREE_H_


