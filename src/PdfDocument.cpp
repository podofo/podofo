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

#include <deque>

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfDocument.h"
#include "PdfPage.h"

namespace PoDoFo {

typedef std::deque< PdfObject* >	PdfPageObjects;

/** Class for managing the tree of Pages in a PDF document
*/
class PdfPagesTree 
{
public:
    /** Construct a PdfPagesTree from the root /Pages object
     *  \param sFilename filename of the file which is going to be parsed/opened
     */
    PdfPagesTree( PdfDocument* inOwningDoc, PdfObject* inRootObj );
    
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
    
private:
    PdfPagesTree();	// don't allow construction from nothing!
    
	PdfDocument*	  mOwningDoc;
    PdfObject*        mPagesRoot;
    PdfPageObjects	  mPageObjs;
    
    /** Private method for actually traversing the /Pages tree
     */
    PdfObject* GetPageNode( int nPageNum, PdfObject* pPagesObject );
};


PdfPagesTree::PdfPagesTree( PdfDocument* inOwningDoc, PdfObject* inRootObj ) 
: mOwningDoc( inOwningDoc ), mPagesRoot( inRootObj )
{
	// pre-allocate enough elements
	// better performance and allows for a "sparse array"
	mPageObjs.resize( GetTotalNumberOfPages() );
}

PdfPagesTree::~PdfPagesTree() 
{
    // at the moment, nothing to do...
}

int PdfPagesTree::GetTotalNumberOfPages() const
{
	int	pgCount = 0;	// assume a new document w/o any pages

    if ( mPagesRoot ) 
    {
        if( mPagesRoot->GetDictionary().HasKey( "Count" ) )
            return mPagesRoot->GetDictionary().GetKeyAsLong( "Count", 0 );
    }

	return pgCount;
}

PdfObject* PdfPagesTree::GetPageNode( int nPageNum, PdfObject* pPagesObject )
{
    // recurse through the pages tree nodes
    
    int nPagesSeenSoFar = -1 ;	// initialize to -1 because nPageNum is 0-based
    
    PdfVariant var;
    if( !pPagesObject->GetDictionary().HasKey( "Kids" ) )
        return NULL;

    var = pPagesObject->GetDictionary().GetKey( "Kids" );
    if( !var.IsArray() )
    {
        return NULL;
    }
    
    PdfArray	kidsArray = var.GetArray();
    int	        numKids = kidsArray.size(),
    kidsCount = pPagesObject->GetDictionary().GetKeyAsLong( "Count", 0 );
    
    // the pages tree node represented by pPagesObject has only page nodes in its kids array,
    // or pages nodes with a kid count of 1, so we can speed things up by going straight to the desired node
    if ( numKids == kidsCount )
    {
        PdfVariant	pgVar = kidsArray[ nPageNum ];

        while ( true ) 
        {
            if ( !pgVar.IsReference() ) 
            {
                return NULL;	// can't handle inline pages just yet...
            }
            PdfObject* pgObject = mOwningDoc->GetObject( pgVar.GetReference() );

            // make sure the object is a /Page and not a /Pages with a single kid
            if ( pgObject->GetDictionary().GetKeyAsName( PdfName( "Type" ) ) == PdfName( "Page" ) )
                return pgObject;

            // it's a /Pages with a single kid, so dereference and try again...
            if( !pgObject->GetDictionary().HasKey( "Kids" ) )
                return NULL;
            pgVar = pgObject->GetDictionary().GetKey( "Kids" );
        }
    } 
    else 
    {
        for( int i = 0 ; i < numKids ; i++ )
        {
            PdfVariant	kidsVar = kidsArray[ i ];
            
            // is the kid a Pages tree node or a Page object?
            if ( !kidsVar.IsReference() ) 
            {
                return NULL;	// can't handle inline pages just yet...
            }
            PdfObject* pgObject = mOwningDoc->GetObject( kidsVar.GetReference() );
            
            // if it's a Page, then is it the right page??
            // otherwise, it's a Pages, and we need to recurse
            if ( pgObject->GetDictionary().GetKeyAsName( PdfName( "Type" ) ) == PdfName( "Page" ) )
            {
                nPagesSeenSoFar++ ;
                if( nPagesSeenSoFar == nPageNum )
                {
                    return pgObject;
                }
            }
            else 
            {
                int thisKidCount = pgObject->GetDictionary().GetKeyAsLong( "Count", 0 );
                if( ( nPagesSeenSoFar + thisKidCount ) >= nPageNum )
                    return this->GetPageNode( nPageNum - ( nPagesSeenSoFar + 1 ), pgObject ) ;
                else
                    nPagesSeenSoFar += thisKidCount ;
            }
        }
    }

    // we should never exit from here - we should always have been able to return a page from above
    // assert( false ) ;
    return NULL;
}

PdfObject* PdfPagesTree::GetPage( int nIndex )
{
    // if you try to get a page past the end, return NULL
    // we use >= since nIndex is 0 based
    if ( nIndex >= GetTotalNumberOfPages() )
        return NULL;
    
    // if we already have the page in our list, return it
    // otherwise, we need to find it, add it and return it
    PdfObject*	pObject = mPageObjs[ nIndex ];
    if ( !pObject ) 
    {
        pObject = GetPageNode( nIndex, mPagesRoot );
        if ( pObject )
            mPageObjs[ nIndex ] = pObject;
    }
    
    return pObject;
}

//------------------------------------------------------------------------------------
PdfDocument::PdfDocument()
    : mParser( NULL ), mPagesTree( NULL )
{
    mWriter.Init();	// initialize a new document
	InitPagesTree();
}

PdfDocument::PdfDocument( const std::string& sPathname )
    : mParser( new PdfParser() ), mPagesTree( NULL )
{
    mParser->Init( sPathname.c_str(), true );	// load the file, with objects on demand
    mWriter.Init( mParser );
	InitPagesTree();
}

PdfDocument::~PdfDocument()
{
    if ( mPagesTree ) 
    {
        delete mPagesTree;
        mPagesTree = NULL;
    }

	if ( mParser ) 
	{
		delete mParser;
		mParser = NULL;
	}
}

void PdfDocument::InitPagesTree()
{
	PdfObject*	pagesRootObj = GetNamedObjectFromCatalog( "Pages" );
	if ( pagesRootObj ) {
		mPagesTree = new PdfPagesTree( this, pagesRootObj );
	}
}

PdfObject* PdfDocument::GetNamedObjectFromCatalog( const char* pszName ) const 
{
    PdfVariant ref;
    PdfObject* rootObj = GetCatalog();
    if ( rootObj ) 
    {
        ref = rootObj->GetDictionary().GetKey( PdfName( pszName ) );
        if( !ref.IsReference() ) {
            return NULL;
        }
        
        return GetObject( ref.GetReference() );
    }

    return NULL;
}

int PdfDocument::GetPageCount() const
{
	return mPagesTree->GetTotalNumberOfPages();
}

PdfPage* PdfDocument::GetPage( int nIndex ) const
{
	PdfPage*    thePage = NULL;
	PdfObject*	pgObj = mPagesTree->GetPage( nIndex );
	if ( pgObj ) 
	{
		thePage = new PdfPage( const_cast<PdfDocument*>(this), pgObj );
	}

	return thePage;
}

//---------------------------------------------------------------

};

