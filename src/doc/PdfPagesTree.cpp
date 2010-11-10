/***************************************************************************
*   Copyriht (C) 2006 by Dominik Seichter                                *
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

#include "PdfPagesTree.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfObject.h"
#include "base/PdfOutputDevice.h"
#include "base/PdfVecObjects.h"

#include "PdfPage.h"

#include <iostream>
namespace PoDoFo {

PdfPagesTree::PdfPagesTree( PdfVecObjects* pParent )
    : PdfElement( "Pages", pParent ),
      m_cache( 0 )
{
    GetObject()->GetDictionary().AddKey( "Kids", PdfArray() ); // kids->Reference() 
    GetObject()->GetDictionary().AddKey( "Count", PdfObject( static_cast<pdf_int64>(0LL) ) );
}

PdfPagesTree::PdfPagesTree( PdfObject* pPagesRoot )
    : PdfElement( "Pages", pPagesRoot ),
      m_cache( static_cast<int>(pPagesRoot->GetDictionary().GetKeyAsLong( "Count", static_cast<pdf_int64>(0LL) )) )
{
    if( !this->GetObject() ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
}

PdfPagesTree::~PdfPagesTree() 
{
    m_cache.ClearCache();
}

int PdfPagesTree::GetTotalNumberOfPages() const
{
    return ( ( this->GetObject()->GetDictionary().HasKey( "Count" ) ) ?
             static_cast<int>(this->GetObject()->GetDictionary().GetKeyAsLong( "Count", 0LL )) : 0 );
}

PdfPage* PdfPagesTree::GetPage( int nIndex )
{
    // if you try to get a page past the end, return NULL
    // we use >= since nIndex is 0 based
    if ( nIndex >= GetTotalNumberOfPages() )
        return NULL;

    // Take a look into the cache first
    PdfPage* pPage = m_cache.GetPage( nIndex );
    if( pPage )
        return pPage;

    // Not in cache -> search tree
    PdfObjectList lstParents;
    PdfObject* pObj = this->GetPageNode(nIndex, this->GetRoot(), lstParents);
    if( pObj ) 
    {
        pPage = new PdfPage( pObj, lstParents );
        m_cache.AddPageObject( nIndex, pPage );
        return pPage;
    }

    return NULL;
}

PdfPage* PdfPagesTree::GetPage( const PdfReference & ref )
{
    // We have to search through all pages,
    // as this is the only way
    // to instantiate the PdfPage with a correct list of parents
    for( int i=0;i<this->GetTotalNumberOfPages();i++ ) 
    {
        PdfPage* pPage = this->GetPage( i );
        if( pPage && pPage->GetObject()->Reference() == ref ) 
            return pPage;
    }
    
    return NULL;
}


void PdfPagesTree::InsertPage( int inAfterPageNumber, PdfPage* inPage )
{
    this->InsertPage( inAfterPageNumber, inPage->GetObject() );
}

void PdfPagesTree::InsertPage( int nAfterPageNumber, PdfObject* pPage )
{
    bool bInsertBefore = false;

    if( ePdfPageInsertionPoint_InsertBeforeFirstPage == nAfterPageNumber )
    {
        bInsertBefore = true;
        nAfterPageNumber = 0;
    }
    else if( nAfterPageNumber < 0 ) 
    {
        // Only ePdfPageInsertionPoint_InsertBeforeFirstPage is valid here
        PdfError::LogMessage( eLogSeverity_Information,
                              "Invalid argument to PdfPagesTree::InsertPage: %i (Only ePdfPageInsertionPoint_InsertBeforeFirstPage is valid here).",
                              nAfterPageNumber );
        return;
    }

    //printf("Fetching page node: %i\n", nAfterPageNumber);
    PdfObjectList lstParents;
    //printf("Searching page=%i\n", nAfterPageNumber );
    PdfObject* pPageBefore = this->GetPageNode( nAfterPageNumber, this->GetRoot(), lstParents );
    
    //printf("pPageBefore=%p lstParents=%i\n", pPageBefore,lstParents.size() );
    if( !pPageBefore || lstParents.size() == 0 ) 
    {
        if( this->GetTotalNumberOfPages() != 0 ) 
        {
            PdfError::LogMessage( eLogSeverity_Critical,
                                  "Cannot find page %i or page %i has no parents. Cannot insert new page.",
                                  nAfterPageNumber, nAfterPageNumber );
            return;
        }
        else
        {
            // We insert the first page into an empty pages tree
            PdfObjectList lstPagesTree;
            lstPagesTree.push_back( this->GetObject() );
            // Use -1 as index to insert before the empty kids array
            InsertPageIntoNode( this->GetObject(), lstPagesTree, -1, pPage );
        }
    }
    else
    {
        PdfObject* pParent = lstParents.back();
        //printf("bInsertBefore=%i\n", bInsertBefore );
        int nKidsIndex = bInsertBefore  ? -1 : this->GetPosInKids( pPageBefore, pParent );
        //printf("Inserting into node: %p at pos %i\n", pParent, nKidsIndex );

        InsertPageIntoNode( pParent, lstParents, nKidsIndex, pPage );
    }

    m_cache.InsertPage( nAfterPageNumber );
}


PdfPage* PdfPagesTree::CreatePage( const PdfRect & rSize )
{
    PdfPage* pPage = new PdfPage( rSize, GetRoot()->GetOwner() );

    InsertPage( this->GetTotalNumberOfPages() - 1, pPage );
    m_cache.AddPageObject( this->GetTotalNumberOfPages() - 1, pPage );
    
    return pPage;
}

void PdfPagesTree::DeletePage( int nPageNumber )
{
    // Delete from cache
    m_cache.DeletePage( nPageNumber );
    
    // Delete from pages tree
    PdfObjectList lstParents;
    PdfObject* pPageNode = this->GetPageNode( nPageNumber, this->GetRoot(), lstParents );

    if( !pPageNode ) 
    {
        PdfError::LogMessage( eLogSeverity_Information,
                              "Invalid argument to PdfPagesTree::DeletePage: %i - Page not found\n",
                              nPageNumber );
        PODOFO_RAISE_ERROR( ePdfError_PageNotFound );
    }

    if( lstParents.size() > 0 ) 
    {
        PdfObject* pParent = lstParents.back();
        int nKidsIndex = this->GetPosInKids( pPageNode, pParent );
        
        DeletePageFromNode( pParent, lstParents, nKidsIndex, pPageNode );
    }
    else
    {
        PdfError::LogMessage( eLogSeverity_Error,
                              "PdfPagesTree::DeletePage: Page %i has no parent - cannot be deleted.\n",
                              nPageNumber );
        PODOFO_RAISE_ERROR( ePdfError_PageNotFound );
    }
}


////////////////////////////////////////////////////
// Private methods
////////////////////////////////////////////////////

PdfObject* PdfPagesTree::GetPageNode( int nPageNum, PdfObject* pParent, 
                                      PdfObjectList & rLstParents ) 
{
    if( !pParent ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( !pParent->GetDictionary().HasKey( PdfName("Kids") ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidKey );
    }

    
    const PdfObject* pObj = pParent->GetIndirectKey( "Kids" );
    if( !pObj->IsArray() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
    }

    const PdfArray & rKidsArray = pObj->GetArray(); 
    PdfArray::const_iterator it = rKidsArray.begin();

    const size_t numDirectKids = rKidsArray.size();
    const size_t numKids = static_cast<size_t>(pParent->GetDictionary().GetKeyAsLong( "Count", 0LL ));

    if( static_cast<int>(numKids) < nPageNum ) 
    {
        PdfError::LogMessage( eLogSeverity_Critical, "Cannot retrieve page %i from a document with only %i pages.",
                              nPageNum, static_cast<int>(numKids) );
        return NULL;
    }

    //printf("Fetching: %i %i %i\n", numDirectKids, numKids, nPageNum );
    if( numDirectKids == numKids && static_cast<size_t>(nPageNum) < numDirectKids )
    {
        // This node has only page nodes as kids,
        // so we can access the array directly
        rLstParents.push_back( pParent );
        return GetPageNodeFromArray( nPageNum, rKidsArray, rLstParents );
    } 
    else if( numDirectKids == numKids && static_cast<size_t>(nPageNum) < numDirectKids )
    {
        // This node has only page nodes as kids,
        // but does not contain our page,
        // skip it - this case should never occur because
        // of handling of childs in the else part below.
        return NULL;
    }
    else
    {
        // We have to traverse the tree
        while( it != rKidsArray.end() ) 
        {
            if( (*it).IsArray() ) 
            {
                // Fixes some broken PDFs who have trees with 1 element kids arrays
                rLstParents.push_back( pParent );
                return GetPageNodeFromArray( nPageNum, (*it).GetArray(), rLstParents ); 
            }
            else if( (*it).IsReference() ) 
            {
                PdfObject* pChild = GetRoot()->GetOwner()->GetObject( (*it).GetReference() );
                if (!pChild) 
                {
                    PdfError::LogMessage( eLogSeverity_Critical, "Requesting page index %i. Child not found: %s\n", 
                                          nPageNum, (*it).GetReference().ToString().c_str()); 
                    return NULL;
                }

                if( this->IsTypePages(pChild) ) 
                {
                    int childCount = this->GetChildCount( pChild );
                    if( childCount < nPageNum + 1 ) // Pages are 0 based, but count is not
                    {
                        // skip this page node
                        // and go to the next one
                        nPageNum -= childCount;
                    }
                    else
                    {
                        rLstParents.push_back( pParent );
                        return this->GetPageNode( nPageNum, pChild, rLstParents );
                    }
                }
                else // Type == Page
                {
                    if( 0 == nPageNum )
                    {
                        rLstParents.push_back( pParent );
                        return pChild;
                    } 

                    // Skip a normal page
                    if(nPageNum > 0 )
                        nPageNum--;
                }
            }
            else
            {
                PdfError::LogMessage( eLogSeverity_Critical, "Requesting page index %i. Invalid datatype in kids array: %s\n", 
                                      nPageNum, (*it).GetDataTypeString()); 
                return NULL;
            }
            
            ++it;
        }
    }

    return NULL;
}

PdfObject* PdfPagesTree::GetPageNodeFromArray( int nPageNum, const PdfArray & rKidsArray, PdfObjectList & rLstParents )
{
    if( static_cast<size_t>(nPageNum) >= rKidsArray.GetSize() )
    {
        PdfError::LogMessage( eLogSeverity_Critical, "Requesting page index %i from array of size %i\n", 
                              nPageNum, rKidsArray.size() );
        return NULL;
    }

    // TODO: Fill cache immediately with all pages 
    //       in this kids array
    PdfVariant rVar = rKidsArray[nPageNum];
    while( true ) 
    {
        if( rVar.IsArray() ) 
        {
            // Fixes some broken PDFs who have trees with 1 element kids arrays
            return GetPageNodeFromArray( 0, rVar.GetArray(), rLstParents );
        }
        else if( !rVar.IsReference() )
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_NotImplemented, "Cannot handle inline pages." );
        }

        PdfObject* pgObject = GetRoot()->GetOwner()->GetObject( rVar.GetReference() );
        //printf("Reading %s\n", pgObject->Reference().ToString().c_str());
        // make sure the object is a /Page and not a /Pages with a single kid
        if( this->IsTypePage(pgObject) ) 
        {
            return pgObject;
        }

        // it's a /Pages with a single kid, so dereference and try again...
        if (this->IsTypePages(pgObject) ) 
        {
            if( !pgObject->GetDictionary().HasKey( "Kids" ) )
                return NULL;

            rLstParents.push_back( pgObject );
            rVar = *(pgObject->GetDictionary().GetKey( "Kids" ));
        }
    }

    return NULL;
}

bool PdfPagesTree::IsTypePage(const PdfObject* pObject) const 
{
    if( !pObject )
        return false;

    if( pObject->GetDictionary().GetKeyAsName( PdfName( "Type" ) ) == PdfName( "Page" ) )
        return true;

    return false;
}

bool PdfPagesTree::IsTypePages(const PdfObject* pObject) const 
{
    if( !pObject )
        return false;

    if( pObject->GetDictionary().GetKeyAsName( PdfName( "Type" ) ) == PdfName( "Pages" ) )
        return true;

    return false;
}

int PdfPagesTree::GetChildCount( const PdfObject* pNode ) const
{
    if( !pNode ) 
        return 0;

    return static_cast<int>(pNode->GetDictionary().GetKeyAsLong("Count", 0L));
}

int PdfPagesTree::GetPosInKids( PdfObject* pPageObj, PdfObject* pPageParent )
{
    if( !pPageParent )
    {
        //printf("pPageParent=%p\n", pPageParent );
        return -1;
    }

    const PdfArray & rKids = pPageParent->GetDictionary().GetKey( PdfName("Kids") )->GetArray();
    PdfArray::const_iterator it = rKids.begin();

    int index = 0;
    while( it != rKids.end() ) 
    {
        if( (*it).GetReference() == pPageObj->Reference() )
        {
            //printf("Found at: %i \n", index );
            return index;
        }

        ++index;
        ++it;
    }

    //printf("Not found %i 0 R in %i 0 R\n", pPageObj->Reference().ObjectNumber(),
    //       pPageParent->Reference().ObjectNumber());
    return -1;
}

void PdfPagesTree::InsertPageIntoNode( PdfObject* pParent, const PdfObjectList & rlstParents, 
                                       int nIndex, PdfObject* pPage )
{
    if( !pParent || !pPage ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // 1. Add the reference of the new page to the kids array of pParent
    // 2. Increase count of every node in lstParents (which also includes pParent)
    // 3. Add Parent key to the page

    // 1. Add reference
    const PdfArray oldKids = pParent->GetDictionary().GetKey( PdfName("Kids") )->GetArray();
    PdfArray::const_iterator it = oldKids.begin();
    PdfArray newKids;

    newKids.reserve( oldKids.GetSize() + 1 );

    if( nIndex < 0 ) 
    {
        newKids.push_back( pPage->Reference() );
    }

    int i = 0;
    while( it != oldKids.end() ) 
    {
        newKids.push_back( *it );

        if( i == nIndex ) 
            newKids.push_back( pPage->Reference() );

        ++i;
        ++it;
    }

    /*
    PdfVariant var2( newKids );
    std::string str2;
    var2.ToString(str2);
    printf("newKids= %s\n", str2.c_str() );
    */

    pParent->GetDictionary().AddKey( PdfName("Kids"), newKids );
 
    // 2. increase count
    PdfObjectList::const_reverse_iterator itParents = rlstParents.rbegin();
    while( itParents != rlstParents.rend() )
    {
        this->ChangePagesCount( *itParents, 1 );

        ++itParents;
    } 

    // 3. add parent key to the page
    pPage->GetDictionary().AddKey( PdfName("Parent"), pParent->Reference() );
}

void PdfPagesTree::DeletePageFromNode( PdfObject* pParent, const PdfObjectList & rlstParents, 
                                       int nIndex, PdfObject* pPage )
{
    if( !pParent || !pPage ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // 1. Delete the reference from the kids array of pParent
    // 2. Decrease count of every node in lstParents (which also includes pParent)
    // 3. Remove empty page nodes

    // TODO: Tell cache to free page object

    // 1. Delete reference
    this->DeletePageNode( pParent, nIndex ) ;

    // 2. Decrease count
    PdfObjectList::const_reverse_iterator itParents = rlstParents.rbegin();
    while( itParents != rlstParents.rend() )
    {
        this->ChangePagesCount( *itParents, -1 );

        ++itParents;
    } 

    // 3. Remove empty pages nodes
    itParents = rlstParents.rbegin();
    while( itParents != rlstParents.rend() )
    {
        // Never delete root node
        if( IsEmptyPageNode( *itParents ) && *itParents != GetRoot() ) 
        {
            PdfObject* pParentOfNode = *(itParents + 1);
            int nKidsIndex = this->GetPosInKids( *itParents, pParentOfNode );
            DeletePageNode( pParentOfNode, nKidsIndex );

            // Delete empty page nodes
            delete this->GetObject()->GetOwner()->RemoveObject( (*itParents)->Reference() );
        }

        ++itParents;
    } 
}

void PdfPagesTree::DeletePageNode( PdfObject* pParent, int nIndex ) 
{
    PdfArray kids = pParent->GetDictionary().GetKey( PdfName("Kids") )->GetArray();
    kids.erase( kids.begin() + nIndex );
    pParent->GetDictionary().AddKey( PdfName("Kids"), kids );
}

int PdfPagesTree::ChangePagesCount( PdfObject* pPageObj, int nDelta )
{
    // Increment or decrement inPagesDict's Count by inDelta, and return the new count.
    // Simply return the current count if inDelta is 0.
    int	cnt = static_cast<int>(pPageObj->GetDictionary().GetKey( "Count" )->GetNumber());
    if( 0 != nDelta ) 
    {
        cnt += nDelta ;
        pPageObj->GetDictionary().AddKey( "Count", PdfVariant( static_cast<pdf_int64>(cnt) ) );
    }

    return cnt ;
}

bool PdfPagesTree::IsEmptyPageNode( PdfObject* pPageNode ) 
{
    long lCount = static_cast<long>(pPageNode->GetDictionary().GetKeyAsLong( PdfName("Count"), static_cast<pdf_int64>(0LL) ));
    bool bKidsEmpty = true;

    if( pPageNode->GetDictionary().HasKey( PdfName("Kids") ) )
    {
        bKidsEmpty = pPageNode->GetDictionary().GetKey( PdfName("Kids") )->GetArray().empty();
    }

    return ( lCount == 0L || bKidsEmpty );
}

/*
PdfObject* PdfPagesTree::GetPageNode( int nPageNum, PdfObject* pPagesObject, 
                                      std::deque<PdfObject*> & rListOfParents )
{
    // recurse through the pages tree nodes
    PdfObject* pObj            = NULL;

    if( !pPagesObject->GetDictionary().HasKey( "Kids" ) )
        return NULL;

    pObj = pPagesObject->GetDictionary().GetKey( "Kids" );
    if( !pObj->IsArray() )
        return NULL;

    PdfArray&	kidsArray = pObj->GetArray();
    size_t	numKids   = kidsArray.size();
    size_t      kidsCount = pPagesObject->GetDictionary().GetKeyAsLong( "Count", 0 );

    // All parents of the page node will be added to this lists,
    // so that the PdfPage can later access inherited attributes
    rListOfParents.push_back( pPagesObject );

    // the pages tree node represented by pPagesObject has only page nodes in its kids array,
    // or pages nodes with a kid count of 1, so we can speed things up
    // by going straight to the desired node
    if ( numKids == kidsCount )
    {
        if( nPageNum >= static_cast<int>(kidsArray.size()) )
        {
            PdfError::LogMessage( eLogSeverity_Critical, "Requesting page index %i from array of size %i\n", nPageNum, kidsArray.size() );
            nPageNum--;
        }

        PdfVariant pgVar = kidsArray[ nPageNum ];
        while ( true ) 
        {
            if ( pgVar.IsArray() ) 
            {
                // Fixes some broken PDFs who have trees with 1 element kids arrays
                return GetPageNodeFromTree( nPageNum, pgVar.GetArray(), rListOfParents );
            }
            else if ( !pgVar.IsReference() )
                return NULL;	// can't handle inline pages just yet...

            PdfObject* pgObject = GetRoot()->GetOwner()->GetObject( pgVar.GetReference() );
            // make sure the object is a /Page and not a /Pages with a single kid
            if ( pgObject->GetDictionary().GetKeyAsName( PdfName( "Type" ) ) == PdfName( "Page" ) )
                return pgObject;

            // it's a /Pages with a single kid, so dereference and try again...
            if( !pgObject->GetDictionary().HasKey( "Kids" ) )
                return NULL;

            rListOfParents.push_back( pgObject );
            pgVar = *(pgObject->GetDictionary().GetKey( "Kids" ));
        }
    } 
    else 
    {
        return GetPageNodeFromTree( nPageNum, kidsArray, rListOfParents );
    }

    // we should never exit from here - we should always have been able to return a page from above
    // assert( false ) ;
    return NULL;
}
*/

};
