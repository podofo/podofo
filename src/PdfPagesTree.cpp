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
#include "PdfPage.h"
#include "PdfVecObjects.h"

namespace PoDoFo {

PdfPagesTree::PdfPagesTree( PdfVecObjects* pParent )
    : PdfElement( "Pages", pParent )
{

}

PdfPagesTree::PdfPagesTree( PdfObject* pPagesRoot )
    : PdfElement( "Pages", pPagesRoot )
{
    if( !m_pObject ) 
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // pre-allocate enough elements
    // better performance and allows for a "sparse array"
    m_deqPageObjs.resize( GetTotalNumberOfPages() );
}

PdfPagesTree::~PdfPagesTree() 
{
    PdfPageObjects::iterator it = m_deqPageObjs.begin();

    while( it != m_deqPageObjs.end() )
    {
        delete (*it);
        ++it;
    }
        
    m_deqPageObjs.clear();
}

int PdfPagesTree::GetTotalNumberOfPages() const
{
    return ((m_pObject->GetDictionary().HasKey( "Count" ) ) ?
            m_pObject->GetDictionary().GetKeyAsLong( "Count", 0 ) : 0);
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
    size_t	    numKids = kidsArray.size(),
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

            PdfObject* pgObject = m_pObject->GetParent()->GetObject( pgVar.GetReference() );

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
        for( unsigned int i = 0 ; i < numKids ; i++ )
        {
            PdfVariant	kidsVar = kidsArray[ i ];
            
            // is the kid a Pages tree node or a Page object?
            if ( !kidsVar.IsReference() ) 
            {
                return NULL;	// can't handle inline pages just yet...
            }

            PdfObject* pgObject = m_pObject->GetParent()->GetObject( kidsVar.GetReference() );
            
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

PdfPage* PdfPagesTree::GetPage( int nIndex )
{
    PdfObject* pObj;
    PdfPage*   pPage;

    // if you try to get a page past the end, return NULL
    // we use >= since nIndex is 0 based
    if ( nIndex >= GetTotalNumberOfPages() )
        return NULL;
    
    // if we already have the page in our list, return it
    // otherwise, we need to find it, add it and return it
    pPage = m_deqPageObjs[ nIndex ];
    if ( !pPage ) 
    {
        pObj = GetPageNode( nIndex, m_pObject );
        if ( pObj )
        {
            pPage = new PdfPage( pObj );
            m_deqPageObjs[ nIndex ] = pPage;
        }
    }
    
    return pPage;
}

PdfPage* PdfPagesTree::CreatePage( const PdfRect & rSize )
{
    PdfObject* pObj;
    PdfArray   array;
    size_t     last = m_deqPageObjs.size();
    PdfPage*  pPage = new PdfPage( rSize, m_pObject->GetParent() );

    // TODO: 
    // --- buggy old code to insert into the page tree
    // --- will not work for trees but only for arrays
    // --- START BUGGY
    pObj = m_pObject->GetDictionary().GetKey( "Kids" );
    if( pObj && pObj->IsArray() )
        array = pObj->GetArray();
    
    array.push_back( pPage->Object()->Reference() );
    m_pObject->GetDictionary().AddKey( "Kids", array );

    m_deqPageObjs[last] = pPage;
    m_pObject->GetDictionary().AddKey( "Count", PdfObject( (long)GetTotalNumberOfPages() + 1) );
    // -- END BUGGY
    
    return pPage;
}

};
