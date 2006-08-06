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
#include "PdfObject.h"
#include "PdfPage.h"
#include "PdfPagesTree.h"
#include "PdfVecObjects.h"

namespace PoDoFo {


PdfDocument::PdfDocument()
    : m_pPagesTree( NULL ), m_pTrailer( NULL )
{
    m_eVersion    = ePdfVersion_1_3;
    m_bLinearized = false;

    m_pTrailer = new PdfObject();
    m_pCatalog = m_vecObjects.CreateObject( "Catalog" );

    m_pTrailer->GetDictionary().AddKey( "Root", m_pCatalog->Reference() );

    InitPagesTree();
}

PdfDocument::PdfDocument( const char* pszFilename )
    : m_pPagesTree( NULL ), m_pTrailer( NULL )
{
    // TODO: Dom:
    // Use loading on demand here!
    // Currently this will crash as the file handle used
    // by the PdfParserObjects is closed in the PdfParser
    // destructor. We need a way so that they file is open as long
    // as it is needed by any object.
    // ---
    PdfParser parser( pszFilename, false );

    InitFromParser( &parser );
    InitPagesTree();
}

PdfDocument::PdfDocument( PdfParser* pParser )
    : m_pPagesTree( NULL ), m_pTrailer( NULL )
{
    InitFromParser( pParser );
    InitPagesTree();
}

PdfDocument::~PdfDocument()
{
    TIVecObjects it = m_vecObjects.begin();

    while( it != m_vecObjects.end() )
    {
        delete (*it);
        ++it;
    }

    m_vecObjects.clear();

    if ( m_pPagesTree ) 
    {
        delete m_pPagesTree;
        m_pPagesTree = NULL;
    }

    if ( m_pTrailer ) 
    {
        delete m_pTrailer;
        m_pTrailer = NULL;
    }
}

void PdfDocument::InitFromParser( PdfParser* pParser )
{
    PdfObject* pRoot;

    m_vecObjects   = pParser->GetObjects();
    m_eVersion     = pParser->GetPdfVersion();
    m_bLinearized  = pParser->IsLinearized();

    // clear the parsers object value
    // other PdfWriter and PdfParser
    // would delete the same objects
    pParser->m_vecObjects.clear();

    m_pTrailer = new PdfObject( *(pParser->GetTrailer()) );

    pRoot      = m_pTrailer->GetDictionary().GetKey( "Root" );
    if( pRoot && pRoot->IsReference() )
        m_pCatalog = m_vecObjects.GetObject( pRoot->GetReference() );
    else
    {
        RAISE_ERROR( ePdfError_NoObject );
    }
}

void PdfDocument::InitPagesTree()
{
    if( !m_pCatalog->GetDictionary().HasKey( "Pages" ) )
        printf("Kein Pages");

    printf("m_pCatalog->GetParent() == %p m_vecObjects == %p\n", m_pCatalog->GetParent(), &m_vecObjects );

    PdfObject* pagesRootObj = m_pCatalog->GetIndirectKey( PdfName( "Pages" ) );
    printf("PagesRootObj: %p\n", pagesRootObj );
    if ( pagesRootObj ) 
    {
        printf("Ja: %i\n", pagesRootObj->IsDictionary() );
        m_pPagesTree = new PdfPagesTree( pagesRootObj );
    }
}

PdfObject* PdfDocument::GetNamedObjectFromCatalog( const char* pszName ) const 
{
    return m_pCatalog->GetIndirectKey( PdfName( pszName ) );
}

int PdfDocument::GetPageCount() const
{
    return m_pPagesTree->GetTotalNumberOfPages();
}

PdfPage* PdfDocument::GetPage( int nIndex ) const
{
    PdfPage*    thePage = NULL;
    PdfObject*	pgObj = m_pPagesTree->GetPage( nIndex );
    if ( pgObj ) 
    {
        thePage = new PdfPage( pgObj );
    }

    return thePage;
}

};

