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

#include "PdfPagesTree.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfObject.h"
#include "PdfVecObjects.h"

namespace PoDoFo {

PdfPagesTree::PdfPagesTree( PdfObject* pPagesRoot )
    : m_pPagesRoot( pPagesRoot )
{
    if( !m_pPagesRoot ) 
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // pre-allocate enough elements
    // better performance and allows for a "sparse array"
    m_deqPageObjs.resize( GetTotalNumberOfPages() );
}

PdfPagesTree::~PdfPagesTree() 
{
    // at the moment, nothing to do...
}

int PdfPagesTree::GetTotalNumberOfPages() const
{
    return ((m_pPagesRoot->GetDictionary().HasKey( "Count" ) ) ?
            m_pPagesRoot->GetDictionary().GetKeyAsLong( "Count", 0 ) : 0);
}

PdfObject* PdfPagesTree::GetPageNode( int nPageNum, PdfObject* pPagesObject )
{
    // recurse through the pages tree nodes
    int        nPagesSeenSoFar = -1;	// initialize to -1 because nPageNum is 0-based
    PdfObject* pObj            = NULL;

    if( !pPagesObject->GetDictionary().HasKey( "Kids" ) )
        return NULL;

    pObj = pPagesObject->GetDictionary().GetKey( "Kids" );
    if( !pObj->IsArray() )
        return NULL;
    
    PdfArray	kidsArray = pObj->GetArray();
    int	        numKids = kidsArray.size(),
    kidsCount = pPagesObject->GetDictionary().GetKeyAsLong( "Count", 0 );

    // the pages tree node represented by pPagesObject has only page nodes in its kids array,
    // or pages nodes with a kid count of 1, so we can speed things up by going straight to the desired node
    if ( numKids == kidsCount )
    {
        PdfVariant pgVar = kidsArray[ nPageNum ];
        std::string str;
        pgVar.ToString( str );
        while ( true ) 
        {
            if ( !pgVar.IsReference() ) 
                return NULL;	// can't handle inline pages just yet...

            PdfObject* pgObject = m_pPagesRoot->GetParent()->GetObject( pgVar.GetReference() );

            // make sure the object is a /Page and not a /Pages with a single kid
            if ( pgObject->GetDictionary().GetKeyAsName( PdfName( "Type" ) ) == PdfName( "Page" ) )
                return pgObject;

            // it's a /Pages with a single kid, so dereference and try again...
            if( !pgObject->GetDictionary().HasKey( "Kids" ) )
                return NULL;

            pgVar = *(pgObject->GetDictionary().GetKey( "Kids" ));
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

            PdfObject* pgObject = m_pPagesRoot->GetParent()->GetObject( kidsVar.GetReference() );
            
            // if it's a Page, then is it the right page??
            // otherwise, it's a Pages, and we need to recurse
            if ( pgObject->GetDictionary().GetKeyAsName( PdfName( "Type" ) ) == PdfName( "Page" ) )
            {
                nPagesSeenSoFar++;
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
    PdfObject*	pObject = m_deqPageObjs[ nIndex ];
    if ( !pObject ) 
    {
        pObject = GetPageNode( nIndex, m_pPagesRoot );
        if ( pObject )
            m_deqPageObjs[ nIndex ] = pObject;
    }
    
    return pObject;
}

};
