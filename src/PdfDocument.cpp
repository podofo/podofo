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

#ifdef _MSC_VER
#pragma warning(disable: 4786)
#endif

#include <algorithm>
#include <deque>
#include <iostream>


#include "PdfDocument.h"

#include "PdfAcroForm.h"
#include "PdfArray.h"
#include "PdfDestination.h"
#include "PdfDictionary.h"
#include "PdfFileSpec.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfImmediateWriter.h"
#include "PdfInfo.h"
#include "PdfMemDocument.h"
#include "PdfNamesTree.h"
#include "PdfObject.h"
#include "PdfOutlines.h"
#include "PdfPage.h"
#include "PdfPagesTree.h"
#include "PdfStream.h"
#include "PdfVecObjects.h"

using namespace std;

namespace PoDoFo {

PdfDocument::PdfDocument()
    : m_pOutlines( NULL ), m_pNamesTree( NULL ), m_pPagesTree( NULL ), 
      m_pAcroForms( NULL ), m_fontCache( &m_vecObjects )
{
    m_vecObjects.SetParentDocument( this );

    m_pTrailer = new PdfObject(); // The trailer is NO part of the vector of objects
    m_pTrailer->SetOwner( &m_vecObjects );
    m_pCatalog = m_vecObjects.CreateObject( "Catalog" );

    m_pInfo = new PdfInfo( &m_vecObjects );

    m_pTrailer->GetDictionary().AddKey( "Root", m_pCatalog->Reference() );
    m_pTrailer->GetDictionary().AddKey( "Info", m_pInfo->GetObject()->Reference() );

    InitPagesTree();
}

PdfDocument::~PdfDocument()
{
    this->Clear();
}

void PdfDocument::Clear() 
{
    TIVecObjects     it     = m_vecObjects.begin();

    m_fontCache.EmptyCache();

    while( it != m_vecObjects.end() )
    {
        delete (*it);
        ++it;
    }

    m_vecObjects.Clear();

    if( m_pInfo ) 
    {
        delete m_pInfo;
        m_pInfo = NULL;
    }

    if( m_pNamesTree ) 
    {
        delete m_pNamesTree;
        m_pNamesTree = NULL;
    }

    if( m_pPagesTree ) 
    {
        delete m_pPagesTree;
        m_pPagesTree = NULL;
    }

    if( m_pOutlines ) 
    {
        delete m_pOutlines;
        m_pOutlines = NULL;
    }

    if( m_pAcroForms ) 
    {
        delete m_pAcroForms;
        m_pAcroForms = NULL;
    }

    if( m_pTrailer ) 
    {
        delete m_pTrailer;
        m_pTrailer = NULL;
    }

    m_pCatalog = NULL;
}

void PdfDocument::InitPagesTree()
{
    PdfObject* pagesRootObj = this->GetCatalog()->GetIndirectKey( PdfName( "Pages" ) );
    if ( pagesRootObj ) 
    {
        m_pPagesTree = new PdfPagesTree( pagesRootObj );
    }
    else
    {
        m_pPagesTree = new PdfPagesTree( &m_vecObjects );
        m_pCatalog->GetDictionary().AddKey( "Pages", m_pPagesTree->GetObject()->Reference() );
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
    if( nIndex < 0 || nIndex > m_pPagesTree->GetTotalNumberOfPages() )
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    return m_pPagesTree->GetPage( nIndex );
}

PdfFont* PdfDocument::CreateFont( const char* pszFontName, bool bEmbedd )
{
    return m_fontCache.GetFont( pszFontName, bEmbedd );
}

PdfFont* PdfDocument::CreateFont( FT_Face face, bool bEmbedd )
{
    return m_fontCache.GetFont( face, bEmbedd );
}

PdfPage* PdfDocument::CreatePage( const PdfRect & rSize )
{
    return m_pPagesTree->CreatePage( rSize );
}

const PdfDocument & PdfDocument::Append( const PdfMemDocument & rDoc )
{
    int difference = m_vecObjects.GetSize() + m_vecObjects.GetFreeObjects().size();

    // append all objects first and fix their references
    TCIVecObjects it           = rDoc.GetObjects().begin();
    while( it != rDoc.GetObjects().end() )
    {
        PdfObject* pObj = new PdfObject( PdfReference( (*it)->Reference().ObjectNumber() + difference, 0 ), *(*it) );
        m_vecObjects.push_back( pObj );

        if( (*it)->IsDictionary() && (*it)->HasStream() )
            *(pObj->GetStream()) = *((*it)->GetStream());

        FixObjectReferences( pObj, difference );

        ++it;
    }

    // create all free objects again, to have a clean free object list
    TCIPdfReferenceList itFree = rDoc.GetObjects().GetFreeObjects().begin();
    while( itFree != rDoc.GetObjects().GetFreeObjects().end() )
    {
        m_vecObjects.AddFreeObject( PdfReference( (*itFree).ObjectNumber() + difference, 0 ) );

        ++itFree;
    }

    // append all pages now to our page tree
    for(int i=0;i<rDoc.GetPageCount();i++ )
    {
        PdfPage*      pPage = rDoc.GetPage( i );
        PdfObject*    pObj  = m_vecObjects.GetObject( PdfReference( pPage->GetObject()->Reference().ObjectNumber() + difference, 0 ) );
        if( pObj->IsDictionary() && pObj->GetDictionary().HasKey( "Parent" ) )
            pObj->GetDictionary().RemoveKey( "Parent" );

        printf("Inserting at: %i\n", this->GetPageCount()-1 );
        m_pPagesTree->InsertPage( this->GetPageCount()-1, pObj );
    }

    // append all outlines
    PdfOutlineItem* pRoot       = this->GetOutlines();
    PdfOutlines*    pAppendRoot = const_cast<PdfMemDocument&>(rDoc).GetOutlines( PoDoFo::ePdfDontCreateObject );
    if( pAppendRoot && pAppendRoot->First() ) 
    {
        // only append outlines if appended document has outlines
        while( pRoot && pRoot->Next() ) 
            pRoot = pRoot->Next();

        printf("Reached last node difference=%i\n", difference);
        printf("First: %li 0 R\n", pAppendRoot->First()->GetObject()->Reference().ObjectNumber() );
        PdfReference ref( pAppendRoot->First()->GetObject()->Reference().ObjectNumber() + difference, 0 );
        pRoot->InsertChild( new PdfOutlines( m_vecObjects.GetObject( ref ) ) );
    }

    // TODO: merge name trees
    // ToDictionary -> then iteratate over all keys and add them to the new one
    return *this;
}

void PdfDocument::FixObjectReferences( PdfObject* pObject, int difference )
{
    if( !pObject ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( pObject->IsReference() )
    {
        pObject->GetReference().SetObjectNumber( pObject->GetReference().ObjectNumber() + difference );
    }
    else if( pObject->IsDictionary() )
    {
        TKeyMap::iterator it = pObject->GetDictionary().GetKeys().begin();

        while( it != pObject->GetDictionary().GetKeys().end() )
        {
            FixObjectReferences( (*it).second, difference );
            ++it;
        }
    }
    else if( pObject->IsArray() )
    {
        PdfArray::iterator it = pObject->GetArray().begin();

        while( it != pObject->GetArray().end() )
        {
            FixObjectReferences( &(*it), difference );
            ++it;
        }
    }
}

EPdfPageMode PdfDocument::GetPageMode( void ) const
{
    // PageMode is optional; the default value is UseNone
    EPdfPageMode thePageMode = ePdfPageModeUseNone;
    
    PdfObject* pageModeObj = GetCatalog()->GetIndirectKey( PdfName( "PageMode" ) );
    if ( pageModeObj != NULL ) {
        PdfName pmName = pageModeObj->GetName();
        
        if( PdfName( "UseNone" ) == pmName )
            thePageMode = ePdfPageModeUseNone ;
        else if( PdfName( "UseThumbs" ) == pmName )
            thePageMode = ePdfPageModeUseThumbs ;
        else if( PdfName( "UseOutlines" ) == pmName )
            thePageMode = ePdfPageModeUseBookmarks ;
        else if( PdfName( "FullScreen" ) == pmName )
            thePageMode = ePdfPageModeFullScreen ;
        else if( PdfName( "UseOC" ) == pmName )
            thePageMode = ePdfPageModeUseOC ;
        else if( PdfName( "UseAttachments" ) == pmName )
            thePageMode = ePdfPageModeUseAttachments ;
        else
            PODOFO_RAISE_ERROR( ePdfError_InvalidName );
    }
    
    return thePageMode ;
}

void PdfDocument::SetPageMode( EPdfPageMode inMode )
{
    switch ( inMode ) {
        default:
        case ePdfPageModeUnknown:
        case ePdfPageModeDontCare:	
            // GetCatalog()->RemoveKey( PdfName( "PageMode" ) );
            // this value means leave it alone!
            break;
            
        case ePdfPageModeUseNone:
            GetCatalog()->GetDictionary().AddKey( PdfName( "PageMode" ), PdfName( "UseNone" ) );
            break;
            
        case ePdfPageModeUseThumbs:
            GetCatalog()->GetDictionary().AddKey( PdfName( "PageMode" ), PdfName( "UseThumbs" ) );
            break;
            
        case ePdfPageModeUseBookmarks:
            GetCatalog()->GetDictionary().AddKey( PdfName( "PageMode" ), PdfName( "UseOutlines" ) );
            break;
            
        case ePdfPageModeFullScreen:
            GetCatalog()->GetDictionary().AddKey( PdfName( "PageMode" ), PdfName( "FullScreen" ) );
            break;
            
        case ePdfPageModeUseOC:
            GetCatalog()->GetDictionary().AddKey( PdfName( "PageMode" ), PdfName( "UseOC" ) );
            break;
            
        case ePdfPageModeUseAttachments:
            GetCatalog()->GetDictionary().AddKey( PdfName( "PageMode" ), PdfName( "UseAttachments" ) );
            break;
    }
}

void PdfDocument::SetUseFullScreen( void )
{
    // first, we get the current mode
    EPdfPageMode	curMode = GetPageMode();
    
    // if current mode is anything but "don't care", we need to move that to non-full-screen
    if ( curMode != ePdfPageModeDontCare )
        SetViewerPreference( PdfName( "NonFullScreenPageMode" ), PdfObject( *(GetCatalog()->GetIndirectKey( PdfName( "PageMode" ) )) ) );
    
    SetPageMode( ePdfPageModeFullScreen );
}

void PdfDocument::SetViewerPreference( const PdfName& whichPref, const PdfObject & valueObj )
{
    PdfObject* prefsObj = GetCatalog()->GetIndirectKey( PdfName( "ViewerPreferences" ) );
    if ( prefsObj == NULL ) {
        // make me a new one and add it
        PdfDictionary	vpDict;
        vpDict.AddKey( whichPref, valueObj );
        
        GetCatalog()->GetDictionary().AddKey( PdfName( "ViewerPreferences" ), PdfObject( vpDict ) );
    } else {
        // modify the existing one
        prefsObj->GetDictionary().AddKey( whichPref, valueObj );
    }
}

void PdfDocument::SetViewerPreference( const PdfName& whichPref, bool inValue )
{
    SetViewerPreference( whichPref, PdfObject( inValue ) );
}

void PdfDocument::SetHideToolbar( void )
{
    SetViewerPreference( PdfName( "HideToolbar" ), true );
}

void PdfDocument::SetHideMenubar( void )
{
    SetViewerPreference( PdfName( "HideMenubar" ), true );
}

void PdfDocument::SetHideWindowUI( void )
{
    SetViewerPreference( PdfName( "HideWindowUI" ), true );
}

void PdfDocument::SetFitWindow( void )
{
    SetViewerPreference( PdfName( "FitWindow" ), true );
}

void PdfDocument::SetCenterWindow( void )
{
    SetViewerPreference( PdfName( "CenterWindow" ), true );
}

void PdfDocument::SetDisplayDocTitle( void )
{
    SetViewerPreference( PdfName( "DisplayDocTitle" ), true );
}

void PdfDocument::SetPrintScaling( PdfName& inScalingType )
{
    SetViewerPreference( PdfName( "PrintScaling" ), inScalingType );
}

void PdfDocument::SetBaseURI( const std::string& inBaseURI )
{
    PdfDictionary	uriDict;
    uriDict.AddKey( PdfName( "Base" ), new PdfObject( PdfString( inBaseURI ) ) );
    GetCatalog()->GetDictionary().AddKey( PdfName( "URI" ), new PdfObject( uriDict ) );
}

void PdfDocument::SetLanguage( const std::string& inLanguage )
{
    GetCatalog()->GetDictionary().AddKey( PdfName( "Lang" ), new PdfObject( PdfString( inLanguage ) ) );
}

void PdfDocument::SetBindingDirection( PdfName& inDirection )
{
    SetViewerPreference( PdfName( "Direction" ), inDirection );
}

void PdfDocument::SetPageLayout( EPdfPageLayout inLayout )
{
    switch ( inLayout ) {
        default:
        case ePdfPageLayoutIgnore:
        case ePdfPageLayoutUnknown:
            break;	// means do nothing
        case ePdfPageLayoutDefault:			
            GetCatalog()->GetDictionary().RemoveKey( PdfName( "PageLayout" ) );
            break;
        case ePdfPageLayoutSinglePage:		
            GetCatalog()->GetDictionary().AddKey( PdfName( "PageLayout" ), PdfName( "SinglePage" ) );
            break;
        case ePdfPageLayoutOneColumn:		
            GetCatalog()->GetDictionary().AddKey( PdfName( "PageLayout" ), PdfName( "OneColumn" ) );
            break;
        case ePdfPageLayoutTwoColumnLeft:	
            GetCatalog()->GetDictionary().AddKey( PdfName( "PageLayout" ), PdfName( "TwoColumnLeft" ) );
            break;
        case ePdfPageLayoutTwoColumnRight: 	
            GetCatalog()->GetDictionary().AddKey( PdfName( "PageLayout" ), PdfName( "TwoColumnRight" ) );
            break;
        case ePdfPageLayoutTwoPageLeft: 	
            GetCatalog()->GetDictionary().AddKey( PdfName( "PageLayout" ), PdfName( "TwoPageLeft" ) );
            break;
        case ePdfPageLayoutTwoPageRight: 	
            GetCatalog()->GetDictionary().AddKey( PdfName( "PageLayout" ), PdfName( "TwoPageRight" ) );
            break;
    }
}

PdfOutlines* PdfDocument::GetOutlines( bool bCreate )
{
    PdfObject* pObj;

    if( !m_pOutlines )
    {
        pObj = GetNamedObjectFromCatalog( "Outlines" );
        if( !pObj ) 
        {
            if ( !bCreate )	return NULL;
            
            m_pOutlines = new PdfOutlines( &m_vecObjects );
            this->GetCatalog()->GetDictionary().AddKey( "Outlines", m_pOutlines->GetObject()->Reference() );
        } else if ( pObj->GetDataType() != ePdfDataType_Dictionary ) {
            PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
        } else
            m_pOutlines = new PdfOutlines( pObj );
    }        
    
    return m_pOutlines;
}

PdfNamesTree* PdfDocument::GetNamesTree( bool bCreate )
{
    PdfObject* pObj;

    if( !m_pNamesTree )
    {
        pObj = GetNamedObjectFromCatalog( "Names" );
        if( !pObj ) 
        {
            if ( !bCreate )
                return NULL;

            PdfNamesTree tmpTree ( &m_vecObjects );
            pObj = tmpTree.GetObject();
            this->GetCatalog()->GetDictionary().AddKey( "Names", pObj->Reference() );
            m_pNamesTree = new PdfNamesTree( pObj, this->GetCatalog() );
        } else if ( pObj->GetDataType() != ePdfDataType_Dictionary ) {
            PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
        } else
            m_pNamesTree = new PdfNamesTree( pObj, this->GetCatalog() );
    }        
    
    return m_pNamesTree;
}

PdfAcroForm* PdfDocument::GetAcroForm( bool bCreate )
{
    PdfObject* pObj;

    if( !m_pAcroForms )
    {
        pObj = GetNamedObjectFromCatalog( "AcroForm" );
        if( !pObj ) 
        {
            if ( !bCreate )	return NULL;
            
            m_pAcroForms = new PdfAcroForm( this );
            this->GetCatalog()->GetDictionary().AddKey( "AcroForm", m_pAcroForms->GetObject()->Reference() );
        } else if ( pObj->GetDataType() != ePdfDataType_Dictionary ) {
            PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
        } else
            m_pAcroForms = new PdfAcroForm( this, pObj );
    }        
    
    return m_pAcroForms;
}

void PdfDocument::AddNamedDestination( const PdfDestination& rDest, const PdfString & rName )
{
    PdfNamesTree* nameTree = GetNamesTree();
    nameTree->AddValue( PdfName("Dests"), rName, rDest.GetObject()->Reference() );
}

void PdfDocument::AttachFile( const PdfFileSpec & rFileSpec )
{
    PdfNamesTree* pNames = this->GetNamesTree( true );

    if( !pNames ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    pNames->AddValue( "EmbeddedFiles", rFileSpec.GetFilename(), rFileSpec.GetObject()->Reference() );
}

};

