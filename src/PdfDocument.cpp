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

#include "PdfDocument.h"

namespace PoDoFo {


typedef std::map< int, PdfObject* >	PdfPageObjectMap;

/** Class for managing the tree of Pages in a PDF document
*/
class PdfPagesTree 
{
public:
	/** Construct a PdfPagesTree from the root /Pages object
	*  \param sFilename filename of the file which is going to be parsed/opened
	*/
	PdfPagesTree( PdfObject* inRootObj ) : mPagesRoot( inRootObj ) {}

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

	PdfObject*          mPagesRoot;
	PdfPageObjectMap	mPageObjMap;

	/** Private method for actually traversing the /Pages tree
	*/
	PdfObject* GetPageNode( int nPageNum, PdfObject* pPagesObject );
};


PdfPagesTree::~PdfPagesTree() 
{
	// at the moment, nothing to do...
}

int PdfPagesTree::GetTotalNumberOfPages() const
{
	if ( mPagesRoot ) {
		PdfVariant	countVar;

		if ( mPagesRoot->HasSingleValue() ) {
			PdfVariant	prVar = mPagesRoot->GetSingleValueVariant();
			if ( prVar.IsDictionary() ) {	// better be!
				prVar.GetDictionary().GetKeyValueVariant( "Count", countVar );
			}
		} else if ( mPagesRoot->HasKey( "Count" ) ) {
			mPagesRoot->GetKeyValueVariant( "Count", countVar );
		}

		if ( countVar.IsNumber() ) {
			return countVar.GetNumber();
		} else {
			// TODO: probably should throw an error here...	
			return 0;
		}
	} else {
		// it's a new document w/o any pages
		return 0;
	}
}

PdfObject* PdfPagesTree::GetPageNode( int nPageNum, PdfObject* pPagesObject )
{
	// recurse through the pages tree nodes

	int nPagesSeenSoFar = -1 ;	// initialize to -1 because nPageNum is 0-based

	PdfObject*	kidsArrayObj = pPagesObject->GetKeyValueObject( "Kids" );

	/*
	assert( NULL != inPagesDictionary ) ;
	CosObj parentPagesDictionary = inPagesDictionary ;
	CosObj kidsArray = CPagesTreeHelper::GetKidsArray( parentPagesDictionary ) ;
	ASInt32 kidsArrayLength = ::CosArrayLength( kidsArray ) ;
	ASInt32 kidsArrayCount = ::CosDictGetIntegerValue( parentPagesDictionary, SPDF_ATOM( kSPDFASAtom_Count ) ) ;
	if( kidsArrayLength == kidsArrayCount )
	{
	// the pages tree node represented by inPagesDictionary has only page nodes in its kids array,
	// or pages nodes with a kid count of 1, so we can speed things up by going straight to the desired node
	CosObj node = ::CosArrayGetDict( kidsArray, inPageNumber ) ;
	while( true )
	{
	// is node a Page object? if so, we're done
	if( SPDF_ATOM( kSPDFASAtom_Page ) == ::CosDictGetNameValue( node, SPDF_ATOM( kSPDFASAtom_Type ) ) )
	return node ;

	// if node is not a Page object, then it must be a Pages object with a single kid, so we'll dereference it
	// and try again
	kidsArray = CPagesTreeHelper::GetKidsArray( node ) ;
	node = ::CosArrayGetDict( kidsArray, 0 ) ;
	}
	}
	else
	{
	for( ASInt32 i = 0 ; i < kidsArrayLength ; i++ )
	{
	CosObj oneKidDictionary = ::CosArrayGetDict( kidsArray, i ) ;

	// is the kid a Pages tree node or a Page object?
	if( SPDF_ATOM( kSPDFASAtom_Page ) == ::CosDictGetNameValue( oneKidDictionary, SPDF_ATOM( kSPDFASAtom_Type ) ) )
	{
	numPagesSeenSoFar++ ;
	if( numPagesSeenSoFar == inPageNumber )
	{
	return oneKidDictionary ;
	}
	}
	else
	{
	ASInt32 theNumKidPages = ::CosDictGetIntegerValue( oneKidDictionary, SPDF_ATOM( kSPDFASAtom_Count ) ) ;
	if( ( numPagesSeenSoFar + theNumKidPages ) >= inPageNumber )
	return this->GetPageNode( inPageNumber - ( numPagesSeenSoFar + 1 ), oneKidDictionary ) ;
	else
	numPagesSeenSoFar += theNumKidPages ;
	}
	}
	}

	// we should never exit from here - we should always have been able to return a page from above
	assert( false ) ;
	return ( CosObj )NULL ;
	*/

	return NULL;
}

PdfObject* PdfPagesTree::GetPage( int nIndex )
{
	PdfPageObjectMap::iterator	pItor = mPageObjMap.find( nIndex );
	if ( pItor != mPageObjMap.end() ) {
		// object already exists in the map, so just return it
		return pItor->second;
	} else {
		// create me a new one

	}
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
		mPagesTree = new PdfPagesTree( pagesRootObj );
	}
}

PdfObject* PdfDocument::GetNamedObjectFromCatalog( const char* pszName ) const 
{
    PdfObject*	anObj   = NULL;
    PdfObject*	rootObj = GetCatalog();
    if ( rootObj ) 
    {
        anObj = rootObj->GetKeyValueObject( PdfName( pszName ) );
		if ( !anObj ) 
		{
			PdfVariant	var;
			rootObj->GetKeyValueVariant( PdfName( pszName ), var );
			if ( var.IsDictionary() )
				anObj = new PdfObject( var.GetDictionary() );
			else if ( var.IsReference() ) 
				anObj = new PdfObject( var.GetReference() );
		}
    }

    return anObj;
}

int PdfDocument::GetPageCount() const
{
	return mPagesTree->GetTotalNumberOfPages();
}

//---------------------------------------------------------------

};

