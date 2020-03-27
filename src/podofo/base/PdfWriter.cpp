/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#include "PdfWriter.h"

#include "PdfData.h"
#include "PdfDate.h"
#include "PdfDictionary.h"
//#include "PdfHintStream.h"
#include "PdfObject.h"
#include "PdfParser.h"
#include "PdfParserObject.h"
#include "PdfStream.h"
#include "PdfVariant.h"
#include "PdfXRef.h"
#include "PdfXRefStream.h"
#include "PdfDefinesPrivate.h"

#define PDF_MAGIC           "\xe2\xe3\xcf\xd3\n"
// 10 spaces
#define LINEARIZATION_PADDING "          " 

#include <iostream>
#include <stdlib.h>

namespace PoDoFo {

PdfWriter::PdfWriter( PdfParser* pParser )
    : m_bXRefStream( false ), m_pEncrypt( NULL ), 
      m_pEncryptObj( NULL ), 
      m_eWriteMode( ePdfWriteMode_Compact ),
      m_lPrevXRefOffset( 0 ),
      m_bIncrementalUpdate( false ),
      m_bLinearized( false ), m_lFirstInXRef( 0 ),
      m_lLinearizedOffset(0),
      m_lLinearizedLastOffset(0),
      m_lTrailerOffset(0)
{
    if( !(pParser && pParser->GetTrailer()) )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_eVersion     = pParser->GetPdfVersion();
    m_pTrailer     = new PdfObject( *(pParser->GetTrailer()) );
    m_vecObjects   = pParser->m_vecObjects;
}

PdfWriter::PdfWriter( PdfVecObjects* pVecObjects, const PdfObject* pTrailer )
    : m_bXRefStream( false ), m_pEncrypt( NULL ), 
      m_pEncryptObj( NULL ), 
      m_eWriteMode( ePdfWriteMode_Compact ),
      m_lPrevXRefOffset( 0 ),
      m_bIncrementalUpdate( false ),
      m_bLinearized( false ), m_lFirstInXRef( 0 ),
      m_lLinearizedOffset(0),
      m_lLinearizedLastOffset(0),
      m_lTrailerOffset(0)
{
    if( !pVecObjects || !pTrailer )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_eVersion     = ePdfVersion_Default;
    m_pTrailer     = new PdfObject( *pTrailer );
    m_vecObjects   = pVecObjects;
}

PdfWriter::PdfWriter( PdfVecObjects* pVecObjects )
    : m_bXRefStream( false ), m_pEncrypt( NULL ), 
      m_pEncryptObj( NULL ), 
      m_eWriteMode( ePdfWriteMode_Compact ),
      m_lPrevXRefOffset( 0 ),
      m_bIncrementalUpdate( false ),
      m_bLinearized( false ), m_lFirstInXRef( 0 ),
      m_lLinearizedOffset(0),
      m_lLinearizedLastOffset(0),
      m_lTrailerOffset(0)
{
    m_eVersion     = ePdfVersion_Default;
    m_pTrailer     = new PdfObject();
    m_vecObjects   = pVecObjects;
}

PdfWriter::~PdfWriter()
{
    delete m_pTrailer;
    delete m_pEncrypt;

    m_pTrailer     = NULL;
    m_vecObjects   = NULL;
}

void PdfWriter::Write( const char* pszFilename )
{
    PdfOutputDevice device( pszFilename );

    this->Write( &device );
}

#ifdef _WIN32
void PdfWriter::Write( const wchar_t* pszFilename )
{
    PdfOutputDevice device( pszFilename );

    this->Write( &device );
}
#endif // _WIN32

void PdfWriter::Write( PdfOutputDevice* pDevice )
{
    this->Write( pDevice, false );
}

void PdfWriter::Write( PdfOutputDevice* pDevice, bool bRewriteXRefTable )
{
    CreateFileIdentifier( m_identifier, m_pTrailer, &m_originalIdentifier );

    if( !pDevice )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // setup encrypt dictionary
    if( m_pEncrypt )
    {
        m_pEncrypt->GenerateEncryptionKey( m_identifier );

        // Add our own Encryption dictionary
        m_pEncryptObj = m_vecObjects->CreateObject();
        m_pEncrypt->CreateEncryptionDictionary( m_pEncryptObj->GetDictionary() );
    }

    if( m_bLinearized ) 
    {
        if( m_bIncrementalUpdate )
            PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Cannot write an incremental update as a linearized document." );

        this->WriteLinearized( pDevice );
    }
    else
    {
        PdfXRef* pXRef = m_bXRefStream ? new PdfXRefStream( m_vecObjects, this ) : new PdfXRef();

        try {
            if( !m_bIncrementalUpdate )
                WritePdfHeader( pDevice );

            WritePdfObjects( pDevice, *m_vecObjects, pXRef, bRewriteXRefTable );

            if( m_bIncrementalUpdate )
                pXRef->SetFirstEmptyBlock();

            pXRef->Write( pDevice );
            
            // XRef streams contain the trailer in the XRef
            if( !m_bXRefStream ) 
            {
                PdfObject  trailer;
                
                // if we have a dummy offset we write also a prev entry to the trailer
                FillTrailerObject( &trailer, pXRef->GetSize(), false );
                
                pDevice->Print("trailer\n");
                trailer.WriteObject( pDevice, m_eWriteMode, NULL ); // Do not encrypt the trailer dictionary!!!
            }
            
            pDevice->Print( "startxref\n%" PDF_FORMAT_UINT64 "\n%%%%EOF\n", pXRef->GetOffset() );
            delete pXRef;
        } catch( PdfError & e ) {
            // Make sure pXRef is always deleted
            delete pXRef;
            
            // P.Zent: Delete Encryption dictionary (cannot be reused)
            if(m_pEncryptObj) {
                m_vecObjects->RemoveObject(m_pEncryptObj->Reference());
                delete m_pEncryptObj;
            }
            
            e.AddToCallstack( __FILE__, __LINE__ );
            throw e;
        }
    }
    
    // P.Zent: Delete Encryption dictionary (cannot be reused)
    if(m_pEncryptObj) {
        m_vecObjects->RemoveObject(m_pEncryptObj->Reference());
        delete m_pEncryptObj;
    }
}

void PdfWriter::WriteUpdate( PdfOutputDevice* pDevice, PdfInputDevice* pSourceInputDevice, bool bRewriteXRefTable )
{
    if( !pDevice )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // make sure it's set that this is an incremental update
    m_bIncrementalUpdate = true;

    // the source device can be NULL, then the output device
    // is positioned at the end of the original file by the caller
    if( pSourceInputDevice )
    {
        // copy the original file content first
        unsigned int uBufferLen = 65535;
        char *pBuffer;

        while( pBuffer = reinterpret_cast<char *>( podofo_malloc( sizeof( char ) * uBufferLen) ), !pBuffer )
        {
            uBufferLen = uBufferLen / 2;
            if( !uBufferLen )
                break;
        }

        if( !pBuffer )
            PODOFO_RAISE_ERROR (ePdfError_OutOfMemory);

        try {
            pSourceInputDevice->Seek(0);

            while( !pSourceInputDevice->Eof() )
            {
                std::streamoff didRead;

                didRead = pSourceInputDevice->Read( pBuffer, uBufferLen );
                if( didRead > 0)
                    pDevice->Write( pBuffer, didRead );
            }

            podofo_free( pBuffer );
        } catch( PdfError & e ) {
            podofo_free( pBuffer );
            throw e;
        }
    }

    // then write the changes
    this->Write (pDevice, bRewriteXRefTable );
}

void PdfWriter::WriteLinearized( PdfOutputDevice* /* pDevice */ )
{
    PODOFO_RAISE_ERROR( ePdfError_NotImplemented );

    /*
    PdfObject*      pLinearize  = NULL;
    PdfPage*        pPage;
    PdfObject*      pLast;
    NonPublic::PdfHintStream*  pHint;
    PdfOutputDevice length;
    TVecXRefTable   vecXRef;
    TVecXRefOffset  vecXRefOffset;
    TIVecOffsets    it;

    // prepare the document for linearization
    this->FetchPagesTree();
    pPage = m_pPagesTree->GetPage( 0 );
    
    pLinearize = CreateLinearizationDictionary();
    pHint      = new NonPublic::PdfHintStream( m_vecObjects, m_pPagesTree );
    
    this->ReorderObjectsLinearized( pLinearize, pHint, pPage, &pLast );

    // The file is prepared for linearization,
    // so write it now.
    WritePdfHeader( pDevice );

    m_lLinearizedOffset = pDevice->GetLength();
    pLinearize->WriteObject( pDevice );

    // fill the XRef table with the objects
    {
        // Use nested scope and stack local for PdfOutputDevice
        // rather than using a temporary to stop gcc's whining.
        PdfOutputDevice o;
        WritePdfObjects(&o, m_vecLinearized, &vecXRef );
    }

    // prepend the linearization dictionary to the XRef table
    TXRefEntry entry;
    entry.lOffset     = m_lLinearizedOffset;
    entry.lGeneration = pLinearize->Reference().GenerationNumber();
    entry.cUsed       = 'n';
    
    vecXRef[0].nCount++;
    vecXRef[0].nFirst--; 
    vecXRef[0].vecOffsets.insert( vecXRef[0].vecOffsets.begin(), entry );

    // Calculate the length of the xref table
    WritePdfTableOfContents( &vecXRef, &length, &vecXRefOffset, true, false );
    
    it = vecXRef[0].vecOffsets.begin(); 
    it++; // skip the linearization dictionary, as it was written before the XRef table
          // and does already have a correct offset
    while( it != vecXRef[0].vecOffsets.end() )
    {
        (*it).lOffset += pDevice->GetLength() + length.GetLength();
        m_lLinearizedLastOffset = (*it).lOffset;
        ++it;
    }
    
    vecXRefOffset.clear();
    WritePdfTableOfContents( &vecXRef, pDevice, &vecXRefOffset, true, false );
    vecXRef.clear();

    WritePdfObjects( pDevice, m_vecLinearized, &vecXRef );
    vecXRef.clear();

    WritePdfObjects( pDevice, *m_vecObjects, &vecXRef );

    if( m_bXRefStream )
        WriteXRefStream( &vecXRef, pDevice );
    else
        WritePdfTableOfContents( &vecXRef, pDevice, &vecXRefOffset, false, m_bLinearized );

    this->FillLinearizationDictionary( pLinearize, pDevice, pPage, pLast, pHint, &vecXRefOffset );
    */
}

void PdfWriter::WritePdfHeader( PdfOutputDevice* pDevice )
{
    pDevice->Print( "%s\n%%%s", s_szPdfVersions[static_cast<int>(m_eVersion)], PDF_MAGIC );
}

void PdfWriter::WritePdfObjects( PdfOutputDevice* pDevice, const PdfVecObjects& vecObjects, PdfXRef* pXref, bool bRewriteXRefTable )
{
    TCIVecObjects itObjects, itObjectsEnd = vecObjects.end();

    for( itObjects = vecObjects.begin(); itObjects !=  itObjectsEnd; ++itObjects )
    {
        PdfObject *pObject = *itObjects;

	if( m_bIncrementalUpdate )
        {
            if( !pObject->IsDirty() )
            {
                bool canSkip = !bRewriteXRefTable;

                if( bRewriteXRefTable )
                {
                    const PdfParserObject *parserObject = dynamic_cast<const PdfParserObject *>(pObject);
                    // the reference looks like "0 0 R", while the object identifier like "0 0 obj", thus add two letters
                    int objRefLength = pObject->Reference().ToString().length() + 2;

                    // the offset points just after the "0 0 obj" string
                    if( parserObject && parserObject->GetOffset() - objRefLength > 0)
                    {
                        pXref->AddObject( pObject->Reference(), parserObject->GetOffset() - objRefLength, true );
                        canSkip = true;
                    }
                }

                if( canSkip )
                    continue;
            }
        }

        pXref->AddObject( pObject->Reference(), pDevice->Tell(), true );

        // Make sure that we do not encrypt the encryption dictionary!
        pObject->WriteObject( pDevice, m_eWriteMode, 
                              (pObject == m_pEncryptObj ? NULL : m_pEncrypt) );
    }

    TCIPdfReferenceList itFree, itFreeEnd = vecObjects.GetFreeObjects().end();
    for( itFree = vecObjects.GetFreeObjects().begin(); itFree != itFreeEnd; ++itFree )
    {
        pXref->AddObject( *itFree, 0, false );
    }
}

void PdfWriter::GetByteOffset( PdfObject* pObject, pdf_long* pulOffset )
{
    TCIVecObjects   it     = m_vecObjects->begin();
    PdfOutputDevice deviceHeader;

    if( !pObject || !pulOffset )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->WritePdfHeader( &deviceHeader );

    *pulOffset = deviceHeader.GetLength();

    while( it != m_vecObjects->end() )
    {
        if( (*it) == pObject )
            break;

        *pulOffset += (*it)->GetObjectLength( m_eWriteMode );
        ++it;
    }
}

void PdfWriter::WriteToBuffer( char** ppBuffer, pdf_long* pulLen )
{
    PdfOutputDevice device;

    if( !pulLen )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->Write( &device );

    *pulLen = device.GetLength();
    *ppBuffer = static_cast<char*>(podofo_calloc( *pulLen, sizeof(char) ));
    if( !*ppBuffer )
    {
        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
    }

    PdfOutputDevice memDevice( *ppBuffer, *pulLen );
    this->Write( &memDevice );
}

/*
PdfObject* PdfWriter::CreateLinearizationDictionary()
{
    PdfObject*       pLinearize        = m_vecObjects->CreateObject();

    // This will be overwritten later with valid data!
    PdfVariant place_holder( PdfData( LINEARIZATION_PADDING ) );

    PdfArray array;
    array.push_back( place_holder );
    array.push_back( place_holder );

    pLinearize->GetDictionary().AddKey( "Linearized", 1.0 );  // Version
    pLinearize->GetDictionary().AddKey( "L", place_holder );  // File length
    pLinearize->GetDictionary().AddKey( "H", array );         // Hint stream offset and length as PdfArray
    pLinearize->GetDictionary().AddKey( "E", place_holder );  // Offset of end of first page
    pLinearize->GetDictionary().AddKey( "N",                  // Number of pages in the document 
                                        static_cast<pdf_int64>(m_pPagesTree->GetTotalNumberOfPages()) );             
    pLinearize->GetDictionary().AddKey( "O", place_holder );  // Object number of the first page
    pLinearize->GetDictionary().AddKey( "T", place_holder );  // Offset of first entry in main cross reference table

    return pLinearize;
    }*/

/*
void PdfWriter::ReorderObjectsLinearized( PdfObject* pLinearize, NonPublic::PdfHintStream* pHint, PdfPage* pPage, PdfObject** ppLast ) 
{
    TPdfReferenceList   lstLinearizedGroup;
    TPdfReferenceSet    setLinearizedGroup;
    TCIPdfReferenceList it;
    TIVecObjects        itObjects;
    PdfObject*          pRoot;
    PdfObject*          pTmp = NULL;
    size_t        index, i;

    // get the dependend objects that are required to display
    // the first page. I.e. get all objects that have to be written
    // at the start of the file.
    // Add all depencies to lstLinearizedGroup
    m_vecObjects->GetObjectDependencies( pPage->GetObject(), &lstLinearizedGroup );

    // get the root dictionary, it has to be written at the top of the file too.
    pRoot = m_vecObjects->GetObject( m_pTrailer->GetDictionary().GetKey( "Root" )->GetReference() );
    // add the root dictionary
    lstLinearizedGroup.push_back( pRoot->Reference() );
    // add the first page itself
    lstLinearizedGroup.push_back( pPage->GetObject()->Reference() );

    // add several dependencies of the root dictionary
    this->FindCatalogDependencies( pRoot, "ViewerPreferences", &lstLinearizedGroup, true );
    this->FindCatalogDependencies( pRoot, "PageMode", &lstLinearizedGroup, true );
    this->FindCatalogDependencies( pRoot, "Threads", &lstLinearizedGroup, false );
    this->FindCatalogDependencies( pRoot, "OpenAction", &lstLinearizedGroup, true );
    this->FindCatalogDependencies( pRoot, "AcroForm", &lstLinearizedGroup, false );
    this->FindCatalogDependencies( pRoot, "Encrypt", &lstLinearizedGroup, true );

    // add the hint stream
    lstLinearizedGroup.push_back( pHint->GetObject()->Reference() );
    // add the linearization dictionary
    lstLinearizedGroup.push_back( pLinearize->Reference() );


    // move all objects which are required to display the first page
    // at the front of the vector of objects.
    // We only swap objects inside of the vector to avoid reallocations.
    // This is a fast operation therefore
    i  = m_vecObjects->GetSize()-1;
    it = lstLinearizedGroup.begin();

    while( it != lstLinearizedGroup.end() )
    {
        index = m_vecObjects->GetIndex( *it );

        if( index < i ) 
        {
            pTmp                   = (*m_vecObjects)[index];
            (*m_vecObjects)[index] = (*m_vecObjects)[i];
            (*m_vecObjects)[i]     = pTmp;
        }

        i--;
        ++it;
    }

    // Renumber all objects according to their position in the vector.
    // This is the slowest (only slow) operation when creating a 
    // linearized PDF file. Garbage collection goes along with this step.
    std::copy( lstLinearizedGroup.begin(), lstLinearizedGroup.end(), std::inserter(setLinearizedGroup, setLinearizedGroup.begin()) );
    m_vecObjects->RenumberObjects( m_pTrailer, &setLinearizedGroup );

    // reorder the objects in the file
    itObjects = m_vecObjects->begin();
    itObjects += m_vecObjects->GetSize() - setLinearizedGroup.size();
    m_vecObjects->RemoveObject( itObjects );

    while( itObjects != m_vecObjects->end() )
    {
        m_vecLinearized.push_back( *itObjects );
        // reset the owner
        (*itObjects)->SetOwner( m_vecObjects );
        m_vecObjects->RemoveObject( itObjects ); 
    }
    
    *ppLast = m_vecLinearized.GetBack();
    }*/

 /*
void PdfWriter::FindCatalogDependencies( PdfObject* pCatalog, const PdfName & rName, TPdfReferenceList* pList, bool bWithDependencies )
{
    if( pCatalog->GetDictionary().HasKey( rName ) && pCatalog->GetDictionary().GetKey( rName )->IsReference() )
    {
        if( bWithDependencies )
            m_vecObjects->GetObjectDependencies( pCatalog->GetIndirectKey( rName ), pList );
        else
            pList->push_back( pCatalog->GetIndirectKey( rName )->Reference() );
    }
    }*/

void PdfWriter::FillTrailerObject( PdfObject* pTrailer, pdf_long lSize, bool bOnlySizeKey ) const
{
    pTrailer->GetDictionary().AddKey( PdfName::KeySize, static_cast<pdf_int64>(lSize) );

    if( !bOnlySizeKey ) 
    {
        if( m_pTrailer->GetDictionary().HasKey( "Root" ) )
            pTrailer->GetDictionary().AddKey( "Root", m_pTrailer->GetDictionary().GetKey( "Root" ) );
        /*
          DominikS: It makes no sense to simple copy an encryption key
                    Either we have no encryption or we encrypt again by ourselves
        if( m_pTrailer->GetDictionary().HasKey( "Encrypt" ) )
            pTrailer->GetDictionary().AddKey( "Encrypt", m_pTrailer->GetDictionary().GetKey( "Encrypt" ) );
        */
        if( m_pTrailer->GetDictionary().HasKey( "Info" ) )
            pTrailer->GetDictionary().AddKey( "Info", m_pTrailer->GetDictionary().GetKey( "Info" ) );


        if( m_pEncryptObj ) 
            pTrailer->GetDictionary().AddKey( PdfName("Encrypt"), m_pEncryptObj->Reference() );

        PdfArray array;
        // The ID is the same unless the PDF was incrementally updated
        if( m_bIncrementalUpdate && m_originalIdentifier.IsValid() && m_originalIdentifier.GetLength() > 0 )
        {
            array.push_back( m_originalIdentifier );
        }
        else
        {
            array.push_back( m_identifier );
        }
        array.push_back( m_identifier );

        // finally add the key to the trailer dictionary
        pTrailer->GetDictionary().AddKey( "ID", array );

        if( m_lPrevXRefOffset > 0 )
        {
            PdfVariant value( m_lPrevXRefOffset );

            pTrailer->GetDictionary().AddKey( "Prev", value );
        }
    }
}

/*
void PdfWriter::FetchPagesTree() 
{
    if( !m_pPagesTree )
    {
        // try to find the pages tree
        PdfObject* pRoot = m_pTrailer->GetDictionary().GetKey( "Root" );

        if( !pRoot || !pRoot->IsReference() )
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
        }

        //printf("Fetching: %lu\n", pRoot->GetReference().ObjectNumber() );
        //printf("Size    : %i\n", static_cast<int>(m_vecObjects->GetSize()) );
        pRoot            = m_vecObjects->GetObject( pRoot->GetReference() );
        if( !pRoot ) 
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
        }

        m_pPagesTree     = new PdfPagesTree( pRoot->GetIndirectKey( "Pages" ) );
    }
}
*/

/*
void PdfWriter::FillLinearizationDictionary( PdfObject* pLinearize, PdfOutputDevice* pDevice, PdfPage* pPage, PdfObject* pLast, 
NonPublic::PdfHintStream* pHint, TVecXRefOffset* pVecXRefOffset )
{
    long            lFileSize        = pDevice->GetLength();
    PdfVariant      value( 0l );
    PdfArray        hints;
    PdfObject       trailer;

    value.SetPaddingLength( LINEARIZATION_PADDING );

    value.SetNumber( lFileSize );
    pLinearize->GetDictionary().AddKey( "L", value );
    value.SetNumber( pPage->GetObject()->Reference().ObjectNumber() );
    pLinearize->GetDictionary().AddKey( "O", value );
    value.SetNumber( m_lFirstInXRef );
    pLinearize->GetDictionary().AddKey( "T", value );
    value.SetNumber( m_lLinearizedLastOffset + pLast->GetObjectLength() );
    pLinearize->GetDictionary().AddKey( "E", value );

    value.SetNumber( m_lLinearizedOffset + pLinearize->GetObjectLength() );
    hints.push_back( value );
    value.SetNumber( pHint->GetObject()->GetObjectLength() );
    hints.push_back( value );
    pLinearize->GetDictionary().AddKey( "H", hints );

    pDevice->Seek( m_lLinearizedOffset );
    pLinearize->WriteObject( pDevice );
    pDevice->Seek( lFileSize );

    m_lPrevXRefOffset = pVecXRefOffset->back();
    FillTrailerObject( &trailer, pLast->Reference().ObjectNumber()+1, false );

    pDevice->Seek( m_lTrailerOffset );
    trailer.WriteObject( pDevice );
    pDevice->Seek( lFileSize );
}
*/

void PdfWriter::CreateFileIdentifier( PdfString & identifier, const PdfObject* pTrailer, PdfString* pOriginalIdentifier ) const
{
    PdfOutputDevice length;
    PdfObject*      pInfo;
    char*           pBuffer;
    bool            bOriginalIdentifierFound = false;
    
    if( pOriginalIdentifier && pTrailer->GetDictionary().HasKey( "ID" ))
    {
        const PdfObject* idObj = pTrailer->GetDictionary().GetKey("ID");
        // The PDF spec, section 7.5.5, implies that the ID may be
        // indirect as long as the PDF is not encrypted. Handle that
        // case.
        if ( idObj->IsReference() ) {
            idObj = m_vecObjects->MustGetObject( idObj->GetReference() );
        }

        TCIVariantList it = idObj->GetArray().begin();
        if( it != idObj->GetArray().end() &&
            it->GetDataType() == ePdfDataType_HexString )
        {
            PdfVariant var = (*it);
            *pOriginalIdentifier = var.GetString();
            bOriginalIdentifierFound = true;
        }
    }

    // create a dictionary with some unique information.
    // This dictionary is based on the PDF files information
    // dictionary if it exists.
    if( pTrailer->GetDictionary().HasKey("Info") )
    {
        const PdfReference & rRef = pTrailer->GetDictionary().GetKey( "Info" )->GetReference();
        const PdfObject* pObj = m_vecObjects->GetObject( rRef );

        if( pObj ) 
        {
            pInfo = new PdfObject( *pObj );
        }
        else
        {
            std::ostringstream oss;
            oss << "Error while retrieving info dictionary: " 
                << rRef.ObjectNumber() << " " 
                << rRef.GenerationNumber() << " R" << std::endl;
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, oss.str().c_str() );
        }
    }
    else 
    {
        PdfDate   date;
        PdfString dateString;

        date.ToString( dateString );

        pInfo = new PdfObject();
        pInfo->GetDictionary().AddKey( "CreationDate", dateString );
        pInfo->GetDictionary().AddKey( "Creator", PdfString("PoDoFo") );
        pInfo->GetDictionary().AddKey( "Producer", PdfString("PoDoFo") );
    }
    
    pInfo->GetDictionary().AddKey( "Location", PdfString("SOMEFILENAME") );

    pInfo->WriteObject( &length, m_eWriteMode, NULL );

    pBuffer = static_cast<char*>(podofo_calloc( length.GetLength(), sizeof(char) ));
    if( !pBuffer )
    {
        delete pInfo;
        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
    }

    PdfOutputDevice device( pBuffer, length.GetLength() );
    pInfo->WriteObject( &device, m_eWriteMode, NULL );

    // calculate the MD5 Sum
    identifier = PdfEncryptMD5Base::GetMD5String( reinterpret_cast<unsigned char*>(pBuffer),
                                           static_cast<unsigned int>(length.GetLength()) );
    podofo_free( pBuffer );

    delete pInfo;

    if( pOriginalIdentifier && !bOriginalIdentifierFound )
        *pOriginalIdentifier = identifier;
}

void PdfWriter::SetEncrypted( const PdfEncrypt & rEncrypt )
{
	delete m_pEncrypt;
	m_pEncrypt = PdfEncrypt::CreatePdfEncrypt( rEncrypt );
}

};

