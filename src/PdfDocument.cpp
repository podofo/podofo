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

#include <algorithm>
#include <deque>
#include <iostream>

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfDocument.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfObject.h"
#include "PdfOutlines.h"
#include "PdfPage.h"
#include "PdfPagesTree.h"
#include "PdfStream.h"
#include "PdfVecObjects.h"

#ifndef _WIN32
#include <fontconfig.h>
#endif

namespace PoDoFo {

using namespace std;

class FontComperator { 
public:
    FontComperator( const string & sPath )
        {
            m_sPath = sPath;
        }
    
    bool operator()(const PdfFont* pFont) 
        { 
            return (m_sPath == pFont->FontMetrics()->Filename());
        }
private:
    string m_sPath;
};


PdfDocument::PdfDocument()
    : m_pOutlines( NULL ), m_pPagesTree( NULL ), m_pTrailer( NULL ), m_ftLibrary( NULL )
{
    m_eVersion    = ePdfVersion_1_3;
    m_bLinearized = false;

    m_pTrailer = new PdfObject();
    m_pTrailer->SetParent( &m_vecObjects );
    m_pCatalog = m_vecObjects.CreateObject( "Catalog" );

    m_pTrailer->GetDictionary().AddKey( "Root", m_pCatalog->Reference() );

    InitPagesTree();
    InitFonts();
}

PdfDocument::PdfDocument( const char* pszFilename )
    : m_pOutlines( NULL ), m_pPagesTree( NULL ), m_pTrailer( NULL ), m_ftLibrary( NULL )
{
    this->Load( pszFilename );
}

PdfDocument::~PdfDocument()
{
    this->Clear();

#ifndef _WIN32
    FcConfigDestroy( (FcConfig*)m_pFcConfig );
#endif

    if( m_ftLibrary ) 
    {
        FT_Done_FreeType( m_ftLibrary );
        m_ftLibrary = NULL;
    }
}

void PdfDocument::Clear() 
{
    TIVecObjects     it     = m_vecObjects.begin();
    TISortedFontList itFont = m_vecFonts.begin();

    while( it != m_vecObjects.end() )
    {
        delete (*it);
        ++it;
    }

    while( itFont != m_vecFonts.end() )
    {
        delete (*itFont);
        ++itFont;
    }

    m_vecObjects.clear();
    m_vecFonts.clear();

    if ( m_pPagesTree ) 
    {
        delete m_pPagesTree;
        m_pPagesTree = NULL;
    }

    if( m_pOutlines ) 
    {
        delete m_pOutlines;
        m_pOutlines = NULL;
    }

    if ( m_pTrailer ) 
    {
        delete m_pTrailer;
        m_pTrailer = NULL;
    }
}

void PdfDocument::InitFonts()
{
#ifndef _WIN32
    m_pFcConfig     = (void*)FcInitLoadConfigAndFonts();
#endif

    if( FT_Init_FreeType( &m_ftLibrary ) )
    {
        RAISE_ERROR( ePdfError_FreeType );
    }
}

void PdfDocument::InitFromParser( PdfParser* pParser )
{
    m_eVersion     = pParser->GetPdfVersion();
    m_bLinearized  = pParser->IsLinearized();

    m_pTrailer = new PdfObject( *(pParser->GetTrailer()) );
    m_pTrailer->SetParent( &m_vecObjects );

    m_pCatalog  = m_pTrailer->GetIndirectKey( "Root" );
    if( !m_pCatalog )
    {
        RAISE_ERROR( ePdfError_NoObject );
    }
}

void PdfDocument::InitPagesTree()
{
    PdfObject* pagesRootObj = m_pCatalog->GetIndirectKey( PdfName( "Pages" ) );
    if ( pagesRootObj ) 
    {
        m_pPagesTree = new PdfPagesTree( pagesRootObj );
    }
    else
    {
        m_pPagesTree = new PdfPagesTree( &m_vecObjects );
        m_pCatalog->GetDictionary().AddKey( "Pages", m_pPagesTree->Object()->Reference() );
    }
}

void PdfDocument::Load( const char* pszFilename )
{
    this->Clear();

    PdfParser parser( &m_vecObjects, pszFilename, true );
    InitFromParser( &parser );
    InitPagesTree();
    InitFonts();
}

void PdfDocument::Write( const char* pszFilename )
{
    PdfOutputDevice device( pszFilename );

    /** TODO:
     *  We will get problems here on linux,
     *  if we write to the same filename we read the 
     *  document from.
     *  Because the PdfParserObjects will read there streams 
     *  data from the file while we are writing it.
     *  The problem is that the stream data won't exist at this time
     *  as we truncated the file already to zero length by opening
     *  it writeable.
     */
    PdfWriter       writer( this );

    writer.Write( &device );    
}

PdfObject* PdfDocument::GetNamedObjectFromCatalog( const char* pszName ) const 
{
    return m_pCatalog->GetIndirectKey( PdfName( pszName ) );
}

PdfObject* PdfDocument::GetInfo( bool bCreate )
{ 
    PdfObject* pObj = m_pTrailer->GetIndirectKey( PdfName( "Info" ) );

    if( !pObj && bCreate ) 
    {
        pObj = m_vecObjects.CreateObject( "Info" );
        m_pTrailer->GetDictionary().AddKey( PdfName( "Info" ), pObj->Reference() );
    }

    return pObj; 
}

int PdfDocument::GetPageCount() const
{
    return m_pPagesTree->GetTotalNumberOfPages();
}

PdfPage* PdfDocument::GetPage( int nIndex ) const
{
    if( nIndex < 0 || nIndex > m_pPagesTree->GetTotalNumberOfPages() )
    {
        RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    return m_pPagesTree->GetPage( nIndex );
}

PdfFont* PdfDocument::CreateFont( const char* pszFontName, bool bEmbedd )
{
#ifdef _WIN32
    std::string       sPath = PdfFontMetrics::GetFilenameForFont( pszFontName );
#else
    std::string       sPath = PdfFontMetrics::GetFilenameForFont( (FcConfig*)m_pFcConfig, pszFontName );
#endif
    PdfFont*          pFont;
    PdfFontMetrics*   pMetrics;
    TCISortedFontList it;

    if( sPath.empty() )
    {
        PdfError::LogMessage( eLogSeverity_Critical, "No path was found for the specified fontname: %s\n", pszFontName );
        return NULL;
    }

    it = std::find_if( m_vecFonts.begin(), m_vecFonts.end(), FontComperator( sPath ) );

    if( it == m_vecFonts.end() )
    {
        pMetrics = new PdfFontMetrics( &m_ftLibrary, sPath.c_str() );

        try {
            pFont    = new PdfFont( pMetrics, bEmbedd, &m_vecObjects );

            m_vecFonts  .push_back( pFont );

            // Now sort the font list
            std::sort( m_vecFonts.begin(), m_vecFonts.end() );
        } catch( PdfError & e ) {
            e.AddToCallstack( __FILE__, __LINE__ );
            e.PrintErrorMsg();
            PdfError::LogMessage( eLogSeverity_Error, "Cannot initialize font: %s\n", pszFontName );
            return NULL;
        }
    }
    else
        pFont = *it;

    return pFont;
}

PdfPage* PdfDocument::CreatePage( const PdfRect & rSize )
{
    return m_pPagesTree->CreatePage( rSize );
}

const PdfString & PdfDocument::GetStringFromInfoDict( const PdfName & rName ) const
{
    PdfObject* pObj = this->GetInfo();
    if( pObj )
        pObj = pObj->GetDictionary().GetKey( rName );
    
    return pObj && pObj->IsString() ? pObj->GetString() : PdfString::StringNull;
}

void PdfDocument::SetAuthor( const PdfString & sAuthor )
{
    this->GetInfo( true )->GetDictionary().AddKey( "Author", sAuthor );
}

void PdfDocument::SetCreator( const PdfString & sCreator )
{
    this->GetInfo( true )->GetDictionary().AddKey( "Creator", sCreator );
}

void PdfDocument::SetKeywords( const PdfString & sKeywords )
{
    this->GetInfo( true )->GetDictionary().AddKey( "Keywords", sKeywords );
}

void PdfDocument::SetSubject( const PdfString & sSubject )
{
    this->GetInfo( true )->GetDictionary().AddKey( "Subject", sSubject );
}

void PdfDocument::SetTitle( const PdfString & sTitle )
{
    this->GetInfo( true )->GetDictionary().AddKey( "Title", sTitle );
}

const PdfDocument & PdfDocument::Append( const PdfDocument & rDoc )
{
    TCIVecObjects it;
    PdfObject*    pObj;
    PdfPage*      pPage;
    PdfReference  ref;
    int           difference = m_vecObjects.size();
    
    // append all objects first and fix their references
    it = rDoc.GetObjects().begin();
    while( it != rDoc.GetObjects().end() )
    {
        pObj  = m_vecObjects.CreateObject( (const PdfVariant &)*(*it) );
        if( (*it)->HasStream() )
            *(pObj->Stream()) = *((*it)->Stream());
        
        FixObjectReferences( pObj, difference );

        ++it;
    }
    
    // append all pages now to our page tree
    for(int i=0;i<rDoc.GetPageCount();i++ )
    {
        pPage = rDoc.GetPage( i );
        pObj  = m_vecObjects.GetObject( PdfReference( pPage->Object()->Reference().ObjectNumber() + difference, 0 ) );
        if( pObj->IsDictionary() && pObj->GetDictionary().HasKey( "Parent" ) )
            pObj->GetDictionary().RemoveKey( "Parent" );

        m_pPagesTree->InsertPage( this->GetPageCount(), pObj );
    }

    return *this;
}

void PdfDocument::FixObjectReferences( PdfObject* pObject, int difference )
{
    if( !pObject ) 
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
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

void PdfDocument::DeletePages( int inFirstPage, int inNumPages )
{
    for( int i = 0 ; i < inNumPages ; i++ )
    {
        m_pPagesTree->DeletePage( inFirstPage ) ;
    }
}

const PdfDocument & PdfDocument::InsertPages( const PdfDocument & rDoc, int inFirstPage, int inNumPages )
{
    /*
      This function works a bit different than one might expect. 
      Rather than copying one page at a time - we copy the ENTIRE document
      and then delete the pages we aren't interested in.
      
      We do this because 
      1) SIGNIFICANTLY simplifies the process
      2) Guarantees that shared objects aren't copied multiple times
      3) offers MUCH faster performance for the common cases
      
      HOWEVER: because PoDoFo doesn't currently do any sort of "object garbage collection" during
      a Write() - we will end up with larger documents, since the data from unused pages
      will also be in there.
    */

    // calculate preliminary "left" and "right" page ranges to delete
    // then offset them based on where the pages were inserted
    // NOTE: some of this will change if/when we support insertion at locations
    //       OTHER than the end of the document!
    int leftStartPage = 0 ;
    int leftCount = inFirstPage ;
    int rightStartPage = inFirstPage + inNumPages ;
    int rightCount = rDoc.GetPageCount() - rightStartPage ;
    int pageOffset = this->GetPageCount();	

    leftStartPage += pageOffset ;
    rightStartPage += pageOffset ;
    
    // append in the whole document
    this->Append( rDoc );

    // delete
    if( rightCount > 0 )
        this->DeletePages( rightStartPage, rightCount ) ;
    if( leftCount > 0 )
        this->DeletePages( leftStartPage, leftCount ) ;
    
    return *this;
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
            RAISE_ERROR( ePdfError_InvalidName );
    }
    
    return thePageMode ;
}

void PdfDocument::SetPageMode( EPdfPageMode inMode ) const
{
    switch ( inMode ) {
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

void PdfDocument::SetUseFullScreen( void ) const
{
    // first, we get the current mode
    EPdfPageMode	curMode = GetPageMode();
    
    // if current mode is anything but "don't care", we need to move that to non-full-screen
    if ( curMode != ePdfPageModeDontCare )
        SetViewerPreference( PdfName( "NonFullScreenPageMode" ), PdfObject( *(GetCatalog()->GetIndirectKey( PdfName( "PageMode" ) )) ) );
    
    SetPageMode( ePdfPageModeFullScreen );
}

void PdfDocument::SetViewerPreference( const PdfName& whichPref, const PdfObject & valueObj ) const
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

void PdfDocument::SetViewerPreference( const PdfName& whichPref, bool inValue ) const
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
        case ePdfPageLayoutIgnore:			break;	// means do nothing
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

PdfOutlines* PdfDocument::GetOutlines()
{
    PdfObject* pObj;

    if( !m_pOutlines )
    {
        pObj = GetNamedObjectFromCatalog( "Outlines" );
        if( !pObj ) 
        {
            m_pOutlines = new PdfOutlines( &m_vecObjects );
            m_pCatalog->GetDictionary().AddKey( "Outlines", m_pOutlines->Object()->Reference() );
        }
        else
            m_pOutlines = new PdfOutlines( pObj );
    }        

    return m_pOutlines;
}
 
};

