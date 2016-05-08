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

#include "PdfMemDocument.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfImmediateWriter.h"
#include "base/PdfObject.h"
#include "base/PdfParserObject.h"
#include "base/PdfStream.h"
#include "base/PdfVecObjects.h"

#include "PdfAcroForm.h"
#include "PdfDestination.h"
#include "PdfFileSpec.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfInfo.h"
#include "PdfNamesTree.h"
#include "PdfOutlines.h"
#include "PdfPage.h"
#include "PdfPagesTree.h"

using namespace std;

namespace PoDoFo {

PdfMemDocument::PdfMemDocument()
    : PdfDocument(), m_pEncrypt( NULL ), m_pParser( NULL )
{
    m_eVersion    = ePdfVersion_Default;
    m_eWriteMode  = ePdfWriteMode_Default;
    m_bLinearized = false;
}

PdfMemDocument::PdfMemDocument(bool bOnlyTrailer)
    : PdfDocument(bOnlyTrailer), m_pEncrypt( NULL ), m_pParser( NULL )
{
    m_eVersion    = ePdfVersion_Default;
    m_eWriteMode  = ePdfWriteMode_Default;
    m_bLinearized = false;
}

PdfMemDocument::PdfMemDocument( const char* pszFilename )
    : PdfDocument(), m_pEncrypt( NULL ), m_pParser( NULL )
{
    this->Load( pszFilename );
}

#ifdef _WIN32
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200    // not for MS Visual Studio 6
#else
PdfMemDocument::PdfMemDocument( const wchar_t* pszFilename )
    : PdfDocument(), m_pEncrypt( NULL ), m_pParser( NULL )
{
    this->Load( pszFilename );
}
#endif
#endif // _WIN32

PdfMemDocument::~PdfMemDocument()
{
    this->Clear();
}

void PdfMemDocument::Clear() 
{
    if( m_pEncrypt ) 
    {
        delete m_pEncrypt;
        m_pEncrypt = NULL;
    }

    if( m_pParser ) 
    {
        delete m_pParser;
        m_pParser = NULL;
    }

    m_eWriteMode  = ePdfWriteMode_Default;
    PdfDocument::Clear();
}

void PdfMemDocument::InitFromParser( PdfParser* pParser )
{
    m_eVersion     = pParser->GetPdfVersion();
    m_bLinearized  = pParser->IsLinearized();

    PdfObject* pTrailer = new PdfObject( *(pParser->GetTrailer()) );
    this->SetTrailer ( pTrailer ); // Set immediately as trailer
                                   // so that pTrailer has an owner
                                   // and GetIndirectKey will work

    if(PdfError::DebugEnabled())
    {
        // OC 17.08.2010: Avoid using cout here:
     // PdfOutputDevice debug( &(std::cout) );
     // pTrailer->Write( &debug );
     // debug.Write("\n", 1); // OC 17.08.2010: Append Linefeed
        PdfRefCountedBuffer buf;
        PdfOutputDevice debug( &buf );
        pTrailer->Write( &debug, m_eWriteMode );
        debug.Write("\n", 1); // OC 17.08.2010: Append Linefeed
        size_t siz = buf.GetSize();
        char*  ptr = buf.GetBuffer();
        PdfError::LogMessage(eLogSeverity_Information, "%.*s", siz, ptr);
    }

    PdfObject* pCatalog = pTrailer->GetIndirectKey( "Root" );
    if( !pCatalog )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_NoObject, "Catalog object not found!" );
    }


    PdfObject* pInfo = pTrailer->GetIndirectKey( "Info" );
    PdfInfo*   pInfoObj;
    if( !pInfo ) 
    {
        pInfoObj = new PdfInfo( PdfDocument::GetObjects() );
        pTrailer->GetDictionary().AddKey( "Info", pInfoObj->GetObject()->Reference() );
    }
    else 
        pInfoObj = new PdfInfo( pInfo );

    if( pParser->GetEncrypted() ) 
    {
        // All PdfParserObjects have a pointer to the PdfEncrypt obkect
        // So we have to take ownership of it.
        delete m_pEncrypt;
        m_pEncrypt = pParser->TakeEncrypt();
    }

    this->SetCatalog ( pCatalog );
    this->SetInfo    ( pInfoObj );
}

void PdfMemDocument::Load( const char* pszFilename )
{
    this->Clear();

    // Call parse file instead of using the constructor
    // so that m_pParser is initialized for encrypted documents
    m_pParser = new PdfParser( PdfDocument::GetObjects() );
    m_pParser->ParseFile( pszFilename, true );
    InitFromParser( m_pParser );
    InitPagesTree();

    // Delete the temporary pdfparser object.
    // It is only set to m_pParser so that SetPassword can work
    delete m_pParser;
    m_pParser = NULL;
}

#ifdef _WIN32
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200    // not for MS Visual Studio 6
#else
void PdfMemDocument::Load( const wchar_t* pszFilename )
{
    this->Clear();

    // Call parse file instead of using the constructor
    // so that m_pParser is initialized for encrypted documents
    m_pParser = new PdfParser( PdfDocument::GetObjects() );
    m_pParser->ParseFile( pszFilename, true );
    InitFromParser( m_pParser );
    InitPagesTree();

    // Delete the temporary pdfparser object.
    // It is only set to m_pParser so that SetPassword can work
    delete m_pParser;
    m_pParser = NULL;
}
#endif
#endif // _WIN32

void PdfMemDocument::Load( const char* pBuffer, long lLen )
{
    this->Clear();

    // Call parse file instead of using the constructor
    // so that m_pParser is initialized for encrypted documents
    m_pParser = new PdfParser( PdfDocument::GetObjects() );
    m_pParser->ParseFile( pBuffer, lLen, true );
    InitFromParser( m_pParser );
    InitPagesTree();

    // Delete the temporary pdfparser object.
    // It is only set to m_pParser so that SetPassword can work
    delete m_pParser;
    m_pParser = NULL;
}

void PdfMemDocument::Load( const PdfRefCountedInputDevice & rDevice )
{
    this->Clear();

    // Call parse file instead of using the constructor
    // so that m_pParser is initialized for encrypted documents
    m_pParser = new PdfParser( PdfDocument::GetObjects() );
    m_pParser->ParseFile( rDevice, true );
    InitFromParser( m_pParser );
    InitPagesTree();

    // Delete the temporary pdfparser object.
    // It is only set to m_pParser so that SetPassword can work
    delete m_pParser;
    m_pParser = NULL;
}
    
/** Add a vendor-specific extension to the current PDF version.
 *  \param ns  namespace of the extension
 *  \param level  level of the extension
 */
void PdfMemDocument::AddPdfExtension( const char* ns, pdf_int64 level ) {
    
    if (!this->HasPdfExtension(ns, level)) {
        
        PdfObject* pExtensions = this->GetCatalog()->GetIndirectKey("Extensions");
        PdfDictionary newExtension;
        
        newExtension.AddKey("BaseVersion", PdfName(s_szPdfVersionNums[m_eVersion]));
        newExtension.AddKey("ExtensionLevel", PdfVariant(level));
        
        if (pExtensions && pExtensions->IsDictionary()) {
            
            pExtensions->GetDictionary().AddKey(ns, newExtension);
            
        } else {
            
            PdfDictionary extensions;
            extensions.AddKey(ns, newExtension);
            this->GetCatalog()->GetDictionary().AddKey("Extensions", extensions);
        }
    }
}

/** Checks whether the documents is tagged to imlpement a vendor-specific
 *  extension to the current PDF version.
 *  \param ns  namespace of the extension
 *  \param level  level of the extension
 */
bool PdfMemDocument::HasPdfExtension( const char* ns, pdf_int64 level ) const {
    
    PdfObject* pExtensions = this->GetCatalog()->GetIndirectKey("Extensions");
    
    if (pExtensions) {
        
        PdfObject* pExtension = pExtensions->GetIndirectKey(ns);
        
        if (pExtension) {
            
            PdfObject* pLevel = pExtension->GetIndirectKey("ExtensionLevel");
            
            if (pLevel && pLevel->IsNumber() && pLevel->GetNumber() == level)
                return true;
        }
    }
    
    return false;
}

/** Return the list of all vendor-specific extensions to the current PDF version.
 *  \param ns  namespace of the extension
 *  \param level  level of the extension
 */
std::vector<PdfExtension> PdfMemDocument::GetPdfExtensions() const {
    
    std::vector<PdfExtension> result;
    
    PdfObject* pExtensions = this->GetCatalog()->GetIndirectKey("Extensions");

    if (pExtensions) {

        // Loop through all declared extensions
        for (TKeyMap::const_iterator it = pExtensions->GetDictionary().GetKeys().begin(); it != pExtensions->GetDictionary().GetKeys().end(); ++it) {

            PdfObject *bv = it->second->GetIndirectKey("BaseVersion");
            PdfObject *el = it->second->GetIndirectKey("ExtensionLevel");
            
            if (bv && el && bv->IsName() && el->IsNumber()) {

                // Convert BaseVersion name to EPdfVersion
                for(int i=0; i<=MAX_PDF_VERSION_STRING_INDEX; i++) {
                    if(bv->GetName().GetName() == s_szPdfVersionNums[i]) {
                        result.push_back(PdfExtension(it->first.GetName().c_str(), static_cast<EPdfVersion>(i), el->GetNumber()));
                    }
                }
            }
        }
    }
    
    return result;
}
    

    
/** Remove a vendor-specific extension to the current PDF version.
 *  \param ns  namespace of the extension
 *  \param level  level of the extension
 */
void PdfMemDocument::RemovePdfExtension( const char* ns, pdf_int64 level ) {
    
    if (this->HasPdfExtension(ns, level))
        this->GetCatalog()->GetIndirectKey("Extensions")->GetDictionary().RemoveKey("ns");
}

void PdfMemDocument::SetPassword( const std::string & sPassword )
{
    PODOFO_RAISE_LOGIC_IF( !m_pParser, "SetPassword called without reading a PDF file." );

    m_pParser->SetPassword( sPassword );
    InitFromParser( m_pParser );
    InitPagesTree();

    delete m_pParser;
    m_pParser = NULL;
}

void PdfMemDocument::Write( const char* pszFilename )
{
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
	// makes sure pending subset-fonts are embedded
	m_fontCache.EmbedSubsetFonts();

    PdfOutputDevice device( pszFilename );

    this->Write( &device );
}

#ifdef _WIN32
void PdfMemDocument::Write( const wchar_t* pszFilename )
{
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
    PdfOutputDevice device( pszFilename );

    this->Write( &device );
}
#endif // _WIN32

void PdfMemDocument::Write( PdfOutputDevice* pDevice ) 
{
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

    PdfWriter writer( &(this->GetObjects()), this->GetTrailer() );
    writer.SetPdfVersion( this->GetPdfVersion() );
    writer.SetWriteMode( m_eWriteMode );

    if( m_pEncrypt ) 
        writer.SetEncrypted( *m_pEncrypt );

    writer.Write( pDevice );    
}

PdfObject* PdfMemDocument::GetNamedObjectFromCatalog( const char* pszName ) const 
{
    return this->GetCatalog()->GetIndirectKey( PdfName( pszName ) );
}

void PdfMemDocument::DeletePages( int inFirstPage, int inNumPages )
{
    for( int i = 0 ; i < inNumPages ; i++ )
    {
        this->GetPagesTree()->DeletePage( inFirstPage ) ;
    }
}

const PdfMemDocument & PdfMemDocument::InsertPages( const PdfMemDocument & rDoc, int inFirstPage, int inNumPages )
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

void PdfMemDocument::SetEncrypted( const std::string & userPassword, const std::string & ownerPassword, 
                                   int protection, PdfEncrypt::EPdfEncryptAlgorithm eAlgorithm,
                                   PdfEncrypt::EPdfKeyLength eKeyLength )
{
    delete m_pEncrypt;
	m_pEncrypt = PdfEncrypt::CreatePdfEncrypt( userPassword, ownerPassword, protection, eAlgorithm, eKeyLength );
}

void PdfMemDocument::SetEncrypted( const PdfEncrypt & pEncrypt )
{
    delete m_pEncrypt;
    m_pEncrypt = PdfEncrypt::CreatePdfEncrypt( pEncrypt );
}

PdfFont* PdfMemDocument::GetFont( PdfObject* pObject )
{
    return m_fontCache.GetFont( pObject );
}

void PdfMemDocument::FreeObjectMemory( const PdfReference & rRef, bool bForce )
{
    FreeObjectMemory( this->GetObjects().GetObject( rRef ), bForce );
}

void PdfMemDocument::FreeObjectMemory( PdfObject* pObj, bool bForce )
{
    if( !pObj ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    PdfParserObject* pParserObject = dynamic_cast<PdfParserObject*>(pObj);
    if( !pParserObject ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, 
                                 "FreeObjectMemory works only on classes of type PdfParserObject." );
    }

    pParserObject->FreeObjectMemory( bForce );
}

};

