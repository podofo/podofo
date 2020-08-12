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
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#if defined(_MSC_VER)  &&  _MSC_VER <= 1200
#pragma warning(disable: 4786)
#endif

#include <algorithm>
#include <deque>
#include <iostream>


#include "PdfDocument.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfImmediateWriter.h"
#include "base/PdfObject.h"
#include "base/PdfStream.h"
#include "base/PdfVecObjects.h"

#include "PdfAcroForm.h"
#include "PdfDestination.h"
#include "PdfFileSpec.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfInfo.h"
#include "PdfMemDocument.h"
#include "PdfNamesTree.h"
#include "PdfOutlines.h"
#include "PdfPage.h"
#include "PdfPagesTree.h"
#include "PdfXObject.h"


using namespace std;

namespace PoDoFo {

PdfDocument::PdfDocument(bool bEmpty)
    : m_fontCache( &m_vecObjects ), 
      m_pTrailer(NULL),
      m_pCatalog(NULL),
      m_pInfo(NULL),
      m_pPagesTree( NULL ),
      m_pAcroForms( NULL ),  
      m_pOutlines( NULL ), 
      m_pNamesTree( NULL )
{
    m_vecObjects.SetParentDocument( this );

    if( !bEmpty ) 
    {
        m_pTrailer = new PdfObject(); // The trailer is NO part of the vector of objects
        m_pTrailer->SetOwner( &m_vecObjects );
        m_pCatalog = m_vecObjects.CreateObject( "Catalog" );
        
        m_pInfo = new PdfInfo( &m_vecObjects );
        
        m_pTrailer->GetDictionary().AddKey( "Root", m_pCatalog->Reference() );
        m_pTrailer->GetDictionary().AddKey( "Info", m_pInfo->GetObject()->Reference() );

        InitPagesTree();
    }
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
    m_vecObjects.SetParentDocument( this );

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
    if( nIndex < 0 || nIndex >= m_pPagesTree->GetTotalNumberOfPages() )
    {
        PODOFO_RAISE_ERROR( ePdfError_PageNotFound );
    }

    return m_pPagesTree->GetPage( nIndex );
}

PdfFont* PdfDocument::CreateFont( const char* pszFontName,
                                  bool bSymbolCharset,
                                  const PdfEncoding * const pEncoding, 
                                  PdfFontCache::EFontCreationFlags eFontCreationFlags, 
                                  bool bEmbedd )
{
    return m_fontCache.GetFont( pszFontName, false, false, bSymbolCharset, bEmbedd, eFontCreationFlags, pEncoding );
}

PdfFont* PdfDocument::CreateFont( const char* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                                  const PdfEncoding * const pEncoding, 
                                  PdfFontCache::EFontCreationFlags eFontCreationFlags,
                                  bool bEmbedd, const char* pszFileName )
{
    return m_fontCache.GetFont( pszFontName, bBold, bItalic, bSymbolCharset, bEmbedd, eFontCreationFlags, pEncoding, pszFileName );
}

#if defined(_WIN32) && !defined(PODOFO_NO_FONTMANAGER)
PdfFont* PdfDocument::CreateFont( const wchar_t* pszFontName, bool bSymbolCharset, const PdfEncoding * const pEncoding, 
                                  bool bEmbedd )
{
    return m_fontCache.GetFont( pszFontName, false, false, bSymbolCharset, bEmbedd, pEncoding );
}

PdfFont* PdfDocument::CreateFont( const wchar_t* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                                  const PdfEncoding * const pEncoding, bool bEmbedd )
{
    return m_fontCache.GetFont( pszFontName, bBold, bItalic, bSymbolCharset, bEmbedd, pEncoding );
}

PdfFont* PdfDocument::CreateFont( const LOGFONTA &logFont, const PdfEncoding * const pEncoding, bool bEmbedd )
{
    return m_fontCache.GetFont( logFont, bEmbedd, pEncoding );
}

PdfFont* PdfDocument::CreateFont( const LOGFONTW &logFont, const PdfEncoding * const pEncoding, bool bEmbedd )
{
    return m_fontCache.GetFont( logFont, bEmbedd, pEncoding );
}
#endif // _WIN32

PdfFont* PdfDocument::CreateFontSubset( const char* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                                        const PdfEncoding * const pEncoding, const char* pszFileName )
{
    return m_fontCache.GetFontSubset( pszFontName, bBold, bItalic, bSymbolCharset, pEncoding, pszFileName );
}

#if defined(_WIN32) && !defined(PODOFO_NO_FONTMANAGER)
PdfFont* PdfDocument::CreateFontSubset( const wchar_t* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                                        const PdfEncoding * const pEncoding)
{
    PODOFO_RAISE_ERROR_INFO( ePdfError_Unknown, "Subsets are not yet implemented for unicode on windows." );
}
#endif // _WIN32

PdfFont* PdfDocument::CreateFont( FT_Face face, bool bSymbolCharset, const PdfEncoding * const pEncoding, bool bEmbedd )
{
    return m_fontCache.GetFont( face, bSymbolCharset, bEmbedd, pEncoding );
}

PdfFont* PdfDocument::CreateDuplicateFontType1( PdfFont * pFont, const char * pszSuffix )
{
	return m_fontCache.GetDuplicateFontType1( pFont, pszSuffix );
}

PdfPage* PdfDocument::CreatePage( const PdfRect & rSize )
{
    return m_pPagesTree->CreatePage( rSize );
}

void PdfDocument::CreatePages( const std::vector<PdfRect>& vecSizes )
{
    m_pPagesTree->CreatePages( vecSizes );
}

PdfPage* PdfDocument::InsertPage( const PdfRect & rSize, int atIndex)
{
	return m_pPagesTree->InsertPage( rSize, atIndex );
}

void PdfDocument::EmbedSubsetFonts()
{
	m_fontCache.EmbedSubsetFonts();
}

const PdfDocument & PdfDocument::Append( const PdfMemDocument & rDoc, bool bAppendAll )
{
    unsigned int difference = static_cast<unsigned int>(m_vecObjects.GetSize() + m_vecObjects.GetFreeObjects().size());


    // Ulrich Arnold 30.7.2009: Because GetNextObject uses m_nObjectCount instead 
    //                          of m_vecObjects.GetSize()+m_vecObjects.GetFreeObjects().size()+1
    //                          make sure the free objects are already present before appending to
	//                          prevent overlapping obj-numbers

    // create all free objects again, to have a clean free object list
    TCIPdfReferenceList itFree = rDoc.GetObjects().GetFreeObjects().begin();
    while( itFree != rDoc.GetObjects().GetFreeObjects().end() )
    {
        m_vecObjects.AddFreeObject( PdfReference( (*itFree).ObjectNumber() + difference, (*itFree).GenerationNumber() ) );

        ++itFree;
    }

	// append all objects first and fix their references
    TCIVecObjects it           = rDoc.GetObjects().begin();
    while( it != rDoc.GetObjects().end() )
    {
        PdfObject* pObj = new PdfObject( PdfReference( 
                                             static_cast<unsigned int>((*it)->Reference().ObjectNumber() + difference), (*it)->Reference().GenerationNumber() ), *(*it) );
        m_vecObjects.push_back( pObj );

        if( (*it)->IsDictionary() && (*it)->HasStream() )
            *(pObj->GetStream()) = *(static_cast<const PdfObject*>(*it)->GetStream());

        PdfError::LogMessage( eLogSeverity_Information,
                              "Fixing references in %i %i R by %i\n", pObj->Reference().ObjectNumber(), pObj->Reference().GenerationNumber(), difference );
        FixObjectReferences( pObj, difference );

        ++it;
    }

    if( bAppendAll )
    {
        const PdfName inheritableAttributes[] = {
            PdfName("Resources"),
            PdfName("MediaBox"),
            PdfName("CropBox"),
            PdfName("Rotate"),
            PdfName::KeyNull
        };

        // append all pages now to our page tree
        for(int i=0;i<rDoc.GetPageCount();i++ )
        {
            PdfPage*      pPage = rDoc.GetPage( i );
            if (NULL == pPage)
            {
                std::ostringstream oss;
                oss << "No page " << i << " (the first is 0) found.";
                PODOFO_RAISE_ERROR_INFO( ePdfError_PageNotFound, oss.str() );
            }
            PdfObject*    pObj  = m_vecObjects.MustGetObject( PdfReference( pPage->GetObject()->Reference().ObjectNumber() + difference, pPage->GetObject()->Reference().GenerationNumber() ) );
            if( pObj->IsDictionary() && pObj->GetDictionary().HasKey( "Parent" ) )
                pObj->GetDictionary().RemoveKey( "Parent" );

            // Deal with inherited attributes
            const PdfName* pInherited = inheritableAttributes;
            while( pInherited->GetLength() != 0 ) 
            {
                const PdfObject* pAttribute = pPage->GetInheritedKey( *pInherited ); 
                if( pAttribute )
                {
                    PdfObject attribute( *pAttribute );
                    FixObjectReferences( &attribute, difference );
                    pObj->GetDictionary().AddKey( *pInherited, attribute );
                }

                ++pInherited;
            }

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
            
            PdfReference ref( pAppendRoot->First()->GetObject()->Reference().ObjectNumber() + difference, pAppendRoot->First()->GetObject()->Reference().GenerationNumber() );
            pRoot->InsertChild( new PdfOutlines( m_vecObjects.MustGetObject( ref ) ) );
        }
    }
    
    // TODO: merge name trees
    // ToDictionary -> then iteratate over all keys and add them to the new one
    return *this;
}

const PdfDocument &PdfDocument::InsertExistingPageAt( const PdfMemDocument & rDoc, int nPageIndex, int nAtIndex)
{
	/* copy of PdfDocument::Append, only restricts which page to add */
    unsigned int difference = static_cast<unsigned int>(m_vecObjects.GetSize() + m_vecObjects.GetFreeObjects().size());


    // Ulrich Arnold 30.7.2009: Because GetNextObject uses m_nObjectCount instead 
    //                          of m_vecObjects.GetSize()+m_vecObjects.GetFreeObjects().size()+1
    //                          make sure the free objects are already present before appending to
	//                          prevent overlapping obj-numbers

    // create all free objects again, to have a clean free object list
    TCIPdfReferenceList itFree = rDoc.GetObjects().GetFreeObjects().begin();
    while( itFree != rDoc.GetObjects().GetFreeObjects().end() )
    {
        m_vecObjects.AddFreeObject( PdfReference( (*itFree).ObjectNumber() + difference, (*itFree).GenerationNumber() ) );

        ++itFree;
    }

	// append all objects first and fix their references
    TCIVecObjects it           = rDoc.GetObjects().begin();
    while( it != rDoc.GetObjects().end() )
    {
        PdfObject* pObj = new PdfObject( PdfReference( 
                                             static_cast<unsigned int>((*it)->Reference().ObjectNumber() + difference), (*it)->Reference().GenerationNumber() ), *(*it) );
        m_vecObjects.push_back( pObj );

        if( (*it)->IsDictionary() && (*it)->HasStream() )
            *(pObj->GetStream()) = *(static_cast<const PdfObject*>(*it)->GetStream());

        PdfError::LogMessage( eLogSeverity_Information,
                              "Fixing references in %i %i R by %i\n", pObj->Reference().ObjectNumber(), pObj->Reference().GenerationNumber(), difference );
        FixObjectReferences( pObj, difference );

        ++it;
    }

    const PdfName inheritableAttributes[] = {
        PdfName("Resources"),
        PdfName("MediaBox"),
        PdfName("CropBox"),
        PdfName("Rotate"),
        PdfName::KeyNull
    };

    // append all pages now to our page tree
    for(int i=0;i<rDoc.GetPageCount();i++ )
    {
        if (i != nPageIndex) {
            continue;
        }

        PdfPage*      pPage = rDoc.GetPage( i );
        PdfObject*    pObj  = m_vecObjects.MustGetObject( PdfReference( pPage->GetObject()->Reference().ObjectNumber() + difference, pPage->GetObject()->Reference().GenerationNumber() ) );
        if( pObj->IsDictionary() && pObj->GetDictionary().HasKey( "Parent" ) )
            pObj->GetDictionary().RemoveKey( "Parent" );

        // Deal with inherited attributes
        const PdfName* pInherited = inheritableAttributes;
        while( pInherited->GetLength() != 0 ) 
        {
	    const PdfObject* pAttribute = pPage->GetInheritedKey( *pInherited ); 
	    if( pAttribute )
	    {
	        PdfObject attribute( *pAttribute );
	        FixObjectReferences( &attribute, difference );
	        pObj->GetDictionary().AddKey( *pInherited, attribute );
	    }

	    ++pInherited;
        }

        m_pPagesTree->InsertPage( nAtIndex <= 0 ? ePdfPageInsertionPoint_InsertBeforeFirstPage : nAtIndex - 1, pObj );
    }

    // append all outlines
    PdfOutlineItem* pRoot       = this->GetOutlines();
    PdfOutlines*    pAppendRoot = const_cast<PdfMemDocument&>(rDoc).GetOutlines( PoDoFo::ePdfDontCreateObject );
    if( pAppendRoot && pAppendRoot->First() ) 
    {
        // only append outlines if appended document has outlines
        while( pRoot && pRoot->Next() ) 
	    pRoot = pRoot->Next();
    
        PdfReference ref( pAppendRoot->First()->GetObject()->Reference().ObjectNumber() + difference, pAppendRoot->First()->GetObject()->Reference().GenerationNumber() );
        pRoot->InsertChild( new PdfOutlines( m_vecObjects.MustGetObject( ref ) ) );
    }
    
    // TODO: merge name trees
    // ToDictionary -> then iteratate over all keys and add them to the new one
    return *this;
}

PdfRect PdfDocument::FillXObjectFromDocumentPage( PdfXObject * pXObj, const PdfMemDocument & rDoc, int nPage, bool bUseTrimBox )
{
    unsigned int difference = static_cast<unsigned int>(m_vecObjects.GetSize() + m_vecObjects.GetFreeObjects().size());
    Append( rDoc, false );
    PdfPage* pPage = rDoc.GetPage( nPage );

    return FillXObjectFromPage( pXObj, pPage, bUseTrimBox, difference );
}

PdfRect PdfDocument::FillXObjectFromExistingPage( PdfXObject * pXObj, int nPage, bool bUseTrimBox )
{
    PdfPage* pPage = GetPage( nPage );

    return FillXObjectFromPage( pXObj, pPage, bUseTrimBox, 0 );
}

PdfRect PdfDocument::FillXObjectFromPage( PdfXObject * pXObj, const PdfPage * pPage, bool bUseTrimBox, unsigned int difference )
{
    // TODO: remove unused objects: page, ...

    PdfObject*    pObj  = m_vecObjects.MustGetObject( PdfReference( pPage->GetObject()->Reference().ObjectNumber() + difference, pPage->GetObject()->Reference().GenerationNumber() ) );
    PdfRect       box  = pPage->GetMediaBox();

    // intersect with crop-box
    box.Intersect( pPage->GetCropBox() );

    // intersect with trim-box according to parameter
    if ( bUseTrimBox )
        box.Intersect( pPage->GetTrimBox() );

    // link resources from external doc to x-object
    if( pObj->IsDictionary() && pObj->GetDictionary().HasKey( "Resources" ) )
        pXObj->GetContentsForAppending()->GetDictionary().AddKey( "Resources" , pObj->GetDictionary().GetKey( "Resources" ) );

    // copy top-level content from external doc to x-object
    if( pObj->IsDictionary() && pObj->GetDictionary().HasKey( "Contents" ) )
    {
        // get direct pointer to contents
        const PdfObject* pContents = pObj->MustGetIndirectKey( "Contents" );

        if( pContents->IsArray() )
        {
            // copy array as one stream to xobject
            PdfArray pArray = pContents->GetArray();

            PdfObject*  pObj = pXObj->GetContentsForAppending();
            PdfStream*  pObjStream = pObj->GetStream();

            TVecFilters vFilters;
            vFilters.push_back( ePdfFilter_FlateDecode );
            pObjStream->BeginAppend( vFilters );

            TIVariantList it;
            for(it = pArray.begin(); it != pArray.end(); it++)
            {
                if ( it->IsReference() )
                {
                    // TODO: not very efficient !!
                    const PdfObject*  pObj = GetObjects()->GetObject( it->GetReference() );

                    while (pObj!=NULL)
                    {
                        if (pObj->IsReference())    // Recursively look for the stream
                        {
                            pObj = GetObjects()->GetObject( pObj->GetReference() );
                        }
                        else if (pObj->HasStream())
                        {
                            const PdfStream*  pcontStream = pObj->GetStream();

                            char*       pcontStreamBuffer;
                            pdf_long    pcontStreamLength;
                            pcontStream->GetFilteredCopy( &pcontStreamBuffer, &pcontStreamLength );
    
                            pObjStream->Append( pcontStreamBuffer, pcontStreamLength );
                            podofo_free( pcontStreamBuffer );
                            break;
                        }
                        else
                        {
                            PODOFO_RAISE_ERROR( ePdfError_InvalidStream );
                            break;
                        }
                    }
                }
                else
                {
                    string str;
                    it->ToString( str );
                    pObjStream->Append( str );
                    pObjStream->Append( " " );
                }
            }
            pObjStream->EndAppend();
        }
        else if( pContents->HasStream() )
        {
            // copy stream to xobject
            PdfObject*  pObj = pXObj->GetContentsForAppending();
            PdfStream*  pObjStream = pObj->GetStream();
            const PdfStream*  pcontStream = pContents->GetStream();
            char*       pcontStreamBuffer;
            pdf_long    pcontStreamLength;

            TVecFilters vFilters;
            vFilters.push_back( ePdfFilter_FlateDecode );
            pObjStream->BeginAppend( vFilters );
            pcontStream->GetFilteredCopy( &pcontStreamBuffer, &pcontStreamLength );
            pObjStream->Append( pcontStreamBuffer, pcontStreamLength );
            podofo_free( pcontStreamBuffer );
            pObjStream->EndAppend();
        }
        else
        {
            PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
        }
    }

    return box;
}

void PdfDocument::FixObjectReferences( PdfObject* pObject, int difference )
{
    if( !pObject ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( pObject->IsDictionary() )
    {
        TKeyMap::iterator it = pObject->GetDictionary().GetKeys().begin();

        while( it != pObject->GetDictionary().GetKeys().end() )
        {
            if( (*it).second->IsReference() )
            {
                *(*it).second = PdfReference( (*it).second->GetReference().ObjectNumber() + difference,
                                              (*it).second->GetReference().GenerationNumber() );
            }
            else if( (*it).second->IsDictionary() || 
                     (*it).second->IsArray() )
            {
                FixObjectReferences( (*it).second, difference );
            }

            ++it;
        }
    }
    else if( pObject->IsArray() )
    {
        PdfArray::iterator it = pObject->GetArray().begin();

        while( it != pObject->GetArray().end() )
        {
            if( (*it).IsReference() )
            {
                (*it) = PdfReference( (*it).GetReference().ObjectNumber() + difference,
                                      (*it).GetReference().GenerationNumber() );

            }
            else if( (*it).IsDictionary() || 
                     (*it).IsArray() )
                FixObjectReferences( &(*it), difference );

            ++it;
        }
    }
    else if( pObject->IsReference() )
    {
        *pObject = PdfReference( pObject->GetReference().ObjectNumber() + difference,
                                 pObject->GetReference().GenerationNumber() );
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
        SetViewerPreference( PdfName( "NonFullScreenPageMode" ), PdfObject( *(GetCatalog()->MustGetIndirectKey( PdfName( "PageMode" ) )) ) );
    
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

PdfAcroForm* PdfDocument::GetAcroForm( bool bCreate, EPdfAcroFormDefaulAppearance eDefaultAppearance )
{
    PdfObject* pObj;

    if( !m_pAcroForms )
    {
        pObj = GetNamedObjectFromCatalog( "AcroForm" );
        if( !pObj ) 
        {
            if ( !bCreate )	return NULL;
            
            m_pAcroForms = new PdfAcroForm( this, eDefaultAppearance );
            this->GetCatalog()->GetDictionary().AddKey( "AcroForm", m_pAcroForms->GetObject()->Reference() );
        } else if ( pObj->GetDataType() != ePdfDataType_Dictionary ) {
            PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
        } else
            m_pAcroForms = new PdfAcroForm( this, pObj, eDefaultAppearance );
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

    pNames->AddValue( "EmbeddedFiles", rFileSpec.GetFilename(false), rFileSpec.GetObject()->Reference() );
}
    
PdfFileSpec* PdfDocument::GetAttachment( const PdfString & rName )
{
    PdfNamesTree* pNames = this->GetNamesTree();
    
    if( !pNames )
    {
        return NULL;
    }
    
    PdfObject* pObj = pNames->GetValue( "EmbeddedFiles", rName );
    
    if( !pObj )
    {
        return NULL;
    }
    
    return new PdfFileSpec(pObj);
}

void PdfDocument::SetInfo( PdfInfo* pInfo )
{
    delete m_pInfo;
    m_pInfo = pInfo;
}

void PdfDocument::SetTrailer( PdfObject* pObject ) 
{
    delete m_pTrailer;
    m_pTrailer = pObject;
    // Set owner so that GetIndirectKey will work
    m_pTrailer->SetOwner( &m_vecObjects );
}

};

