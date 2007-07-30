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
 ***************************************************************************/

#include "PdfParser.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfInputDevice.h"
#include "PdfMemStream.h"
#include "PdfOutputDevice.h"
#include "PdfParserObject.h"
#include "PdfStream.h"
#include "PdfVariant.h"

#include <cstring>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <iostream>

using std::cerr;
using std::endl;
using std::flush;

#define PDF_MAGIC_LEN       8
#define PDF_XREF_ENTRY_SIZE 20
#define PDF_XREF_BUF        512


namespace PoDoFo {

PdfParser::PdfParser( PdfVecObjects* pVecObjects )
    : PdfTokenizer(), m_vecObjects( pVecObjects )
{
    this->Init();
}

PdfParser::PdfParser( PdfVecObjects* pVecObjects, const char* pszFilename, bool bLoadOnDemand )
    : PdfTokenizer(), m_vecObjects( pVecObjects )
{
    this->Init();
    this->ParseFile( pszFilename, bLoadOnDemand );
}

PdfParser::~PdfParser()
{
    Clear();
}

void PdfParser::Init() 
{
    m_bLoadOnDemand   = false;

    m_device          = PdfRefCountedInputDevice();
    m_pTrailer        = NULL;
    m_pLinearization  = NULL;
    m_pOffsets        = NULL;

    m_ePdfVersion     = ePdfVersion_Unknown;

    m_nXRefOffset     = 0;
    m_nFirstObject    = 0;
    m_nNumObjects     = 0;
    m_nXRefLinearizedOffset = 0;
}

void PdfParser::ParseFile( const char* pszFilename, bool bLoadOnDemand )
{
    if( !pszFilename || !pszFilename[0] )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    // make sure everything is clean
    Clear();

    m_bLoadOnDemand = bLoadOnDemand;

    m_device           = PdfRefCountedInputDevice( pszFilename, "rb" );
    if( !m_device.Device() )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, pszFilename );
    }

    if( !IsPdfFile() )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoPdfFile );
    }

    ReadDocumentStructure();
    try {
        ReadObjects();
    } catch( PdfError & e ) {
        e.AddToCallstack( __FILE__, __LINE__, "Unable to load objects from file." );
        throw e;
    }

    // Now sort the list of objects
    m_vecObjects->Sort();
}

void PdfParser::Clear()
{
    m_setObjectStreams.clear();

    if( m_pOffsets )
        free( m_pOffsets );

    m_device = PdfRefCountedInputDevice();

    delete m_pTrailer;
    delete m_pLinearization;

    this->Init();
}

void PdfParser::ReadDocumentStructure()
{
    HasLinearizationDict();
    
    // position at the end of the file to search the xref table.
    m_device.Device()->Seek( 0, std::ios_base::end );
    m_nFileSize = m_device.Device()->Tell();

    try {
        ReadXRef( &m_nXRefOffset );
    } catch( PdfError & e ) {
        e.AddToCallstack( __FILE__, __LINE__, "Unable to find startxref entry in file." );
        throw e;
    }

    try {
        ReadTrailer();
    } catch( PdfError & e ) {
        e.AddToCallstack( __FILE__, __LINE__, "Unable to find trailer in file." );
        throw e;
    }

    if( m_pLinearization )
    {
        try { 
            ReadXRefContents( m_nXRefOffset, true );
        } catch( PdfError & e ) {
            e.AddToCallstack( __FILE__, __LINE__, "Unable to skip xref dictionary." );
            throw e;
        }

        // another trailer directory is to follow right after this XRef section
        try {
            ReadNextTrailer();
        } catch( PdfError & e ) {
            if( e != ePdfError_NoTrailer )
                throw e;
        }
    }

    if( !m_pTrailer->IsDictionary() || !m_pTrailer->GetDictionary().HasKey( PdfName::KeySize ) )
    {
        PdfError::LogMessage( eLogSeverity_Error, "No /Size key was specified in the trailer directory." );
        PODOFO_RAISE_ERROR( ePdfError_InvalidTrailerSize );
    }

    m_nNumObjects = m_pTrailer->GetDictionary().GetKeyAsLong( PdfName::KeySize );

#ifdef PODOFO_VERBOSE_DEBUG
    PdfError::DebugMessage("Allocating for %i objects\n", m_nNumObjects );
#endif // PODOFO_VERBOSE_DEBUG

    m_pOffsets = static_cast<TXRefEntry*>(malloc( sizeof( TXRefEntry ) * (m_nNumObjects+1)  ));
    memset( m_pOffsets, 0, sizeof( TXRefEntry ) * (m_nNumObjects+1) );

#ifdef PODOFO_VERBOSE_DEBUG
    PdfError::DebugMessage("Linearized Offset: %i Pointer: %p\n", m_nXRefLinearizedOffset, m_pLinearization );
#endif // PODOFO_VERBOSE_DEBUG

    if( m_pLinearization )
    {
        try {
            ReadXRefContents( m_nXRefLinearizedOffset );
        } catch( PdfError & e ) {
            e.AddToCallstack( __FILE__, __LINE__, "Unable to read linearized XRef section." );
            throw e;
        }
    }

    try {
        ReadXRefContents( m_nXRefOffset );
    } catch( PdfError & e ) {
        e.AddToCallstack( __FILE__, __LINE__, "Unable to load xref entries." );
        throw e;
    }

    if( m_pTrailer->GetDictionary().HasKey( "Prev" ) )
    {
        try {
            ReadXRefContents( m_pTrailer->GetDictionary().GetKeyAsLong( "Prev", 0 ) ); 
        } catch( PdfError & e ) {
            e.AddToCallstack( __FILE__, __LINE__, "Unable to load /Prev xref entries." );
            throw e;
        }
    }
}

bool PdfParser::IsPdfFile()
{
    const char* szPdfMagicStart = "%PDF-";
    int i;

    if( m_device.Device()->Read( m_buffer.GetBuffer(), PDF_MAGIC_LEN ) != PDF_MAGIC_LEN )
        return false;

     if( strncmp( m_buffer.GetBuffer(), szPdfMagicStart, strlen( szPdfMagicStart ) ) != 0 )
        return false;
        
    // try to determine the excact PDF version of the file
    for( i=0;i<=MAX_PDF_VERSION_STRING_INDEX;i++ )
        if( strncmp( m_buffer.GetBuffer(), s_szPdfVersions[i], PDF_MAGIC_LEN ) == 0 )
        {
            m_ePdfVersion = static_cast<EPdfVersion>(i);
            break;
        }

    return true;
}

void PdfParser::HasLinearizationDict()
{
    if (m_pLinearization)
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic,
                "HasLinarizationDict() called twice on one object");
    }

    m_device.Device()->Seek( 0 );
    // look for a linearization dictionary in the first 1024 bytes
    if( m_device.Device()->Read( m_buffer.GetBuffer(), m_buffer.GetSize() ) != m_buffer.GetSize() )
    {
        // Clear the error state from the bad read
        m_device.Device()->Clear();
        return; // Ignore Error Code: ERROR_PDF_NO_TRAILER;
    }

    char * pszObj = strstr( m_buffer.GetBuffer(), "obj" );
    if( !pszObj )
        // strange that there is no obj in the first 1024 bytes,
        // but ignore it
        return;

    --pszObj; // *pszObj == 'o', so the while would fail without decrement
    while( *pszObj && (PdfTokenizer::IsWhitespace( *pszObj ) || (*pszObj >= '0' && *pszObj <= '9')) )
        --pszObj;

    m_pLinearization = new PdfParserObject( m_vecObjects, m_device, m_buffer, pszObj - m_buffer.GetBuffer() + 2 );

    try {
        static_cast<PdfParserObject*>(m_pLinearization)->ParseFile();
        if (! (m_pLinearization->IsDictionary() &&
               m_pLinearization->GetDictionary().HasKey( "Linearized" ) ) )
        {
            delete m_pLinearization;
            m_pLinearization = NULL;
            return;
        }
    } catch( PdfError & e ) {
        PdfError::LogMessage( eLogSeverity_Warning, e.what() );
        delete m_pLinearization;
        m_pLinearization = NULL;
        return;
    }
    
    long      lXRef      = -1;
    lXRef = m_pLinearization->GetDictionary().GetKeyAsLong( "T", lXRef );
    if( lXRef == -1 )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidLinearization );
    }

    // avoid moving to a negative file position here
    m_device.Device()->Seek( (lXRef-PDF_XREF_BUF > 0 ? lXRef-PDF_XREF_BUF : PDF_XREF_BUF) );
    m_nXRefLinearizedOffset = m_device.Device()->Tell();

    if( m_device.Device()->Read( m_buffer.GetBuffer(), PDF_XREF_BUF ) != PDF_XREF_BUF )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidLinearization );
    }

    m_buffer.GetBuffer()[PDF_XREF_BUF] = '\0';

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    const int XREF_LEN    = 4; // strlen( "xref" );
    int       i          = 0;
    char*     pszStart   = NULL;
    for( i = PDF_XREF_BUF - XREF_LEN; i >= 0; i-- )
        if( strncmp( m_buffer.GetBuffer()+i, "xref", XREF_LEN ) == 0 )
        {
            pszStart = m_buffer.GetBuffer()+i;
            break;
        }

    m_nXRefLinearizedOffset += i;
    
    if( !pszStart )
    {
        if( m_ePdfVersion < ePdfVersion_1_5 )
        {
            PdfError::LogMessage( eLogSeverity_Warning, 
                                  "Linearization dictionaries are only supported with PDF version 1.5. This is 1.%i. Trying to continue.\n", 
                                  static_cast<int>(m_ePdfVersion) );
            // PODOFO_RAISE_ERROR( ePdfError_InvalidLinearization );
        }

        {
            m_nXRefLinearizedOffset = lXRef;
            /*
            eCode = ReadXRefStreamContents();
            i     = 0;
            */
        }
    }
}

void PdfParser::MergeTrailer( const PdfObject* pTrailer )
{
    PdfVariant  cVar;

    if( !pTrailer || !m_pTrailer )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( pTrailer->GetDictionary().HasKey( PdfName::KeySize ) )
        m_pTrailer->GetDictionary().AddKey( PdfName::KeySize, *(pTrailer->GetDictionary().GetKey( PdfName::KeySize )) );

    if( pTrailer->GetDictionary().HasKey( "Root" ) )
        m_pTrailer->GetDictionary().AddKey( "Root", *(pTrailer->GetDictionary().GetKey( "Root" )) );

    if( pTrailer->GetDictionary().HasKey( "Encrypt" ) )
        m_pTrailer->GetDictionary().AddKey( "Encrypt", *(pTrailer->GetDictionary().GetKey( "Encrypt" )) );

    if( pTrailer->GetDictionary().HasKey( "Info" ) )
        m_pTrailer->GetDictionary().AddKey( "Info", *(pTrailer->GetDictionary().GetKey( "Info" )) );

    if( pTrailer->GetDictionary().HasKey( "ID" ) )
        m_pTrailer->GetDictionary().AddKey( "ID", *(pTrailer->GetDictionary().GetKey( "ID" )) );
}

void PdfParser::ReadNextTrailer()
{
    // ReadXRefcontents has read the first 't' from "trailer" so just check for "railer"
    if( this->IsNextToken( "trailer" ) )
    //if( strcmp( m_buffer.GetBuffer(), "railer" ) == 0 )
    {
        PdfParserObject trailer( m_vecObjects, m_device, m_buffer );
        try {
            trailer.ParseFile( true );
        } catch( PdfError & e ) {
            e.AddToCallstack( __FILE__, __LINE__, "The linearized trailer was found in the file, but contains errors." );
            throw e;
        }

        // now merge the information of this trailer with the main documents trailer
        MergeTrailer( &trailer );
        
        if( trailer.GetDictionary().HasKey( "Prev" ) )
        {
            try {
                ReadXRefContents( trailer.GetDictionary().GetKeyAsLong( "Prev", 0 ) );
            } catch( PdfError & e ) {
                e.AddToCallstack( __FILE__, __LINE__, "Unable to load /Prev xref entries." );
                throw e;
            }
        }
        else
        {
            PODOFO_RAISE_ERROR( ePdfError_NoTrailer );
        }
    }
}

void PdfParser::ReadTrailer()
{
    FindToken( "trailer", PDF_XREF_BUF );
    
    if( !this->IsNextToken( "trailer" ) ) 
    {
        if( m_ePdfVersion < ePdfVersion_1_5 )
        {
            PODOFO_RAISE_ERROR( ePdfError_NoTrailer );
        }
        else
        {
            // Since PDF 1.5 trailer information can also be found
            // in the crossreference stream object
            // and a trailer dictionary is not required
            m_device.Device()->Seek( m_nXRefOffset );

            m_pTrailer = new PdfParserObject( m_vecObjects, m_device, m_buffer );
            static_cast<PdfParserObject*>(m_pTrailer)->ParseFile( false );
            return;
        }
    }
    else 
    {
        m_pTrailer = new PdfParserObject( m_vecObjects, m_device, m_buffer );
        try {
            static_cast<PdfParserObject*>(m_pTrailer)->ParseFile( true );
        } catch( PdfError & e ) {
            e.AddToCallstack( __FILE__, __LINE__, "The trailer was found in the file, but contains errors." );
            throw e;
        }
#ifdef PODOFO_VERBOSE_DEBUG
        PdfError::DebugMessage("Size=%li\n", m_pTrailer->GetDictionary().GetKeyAsLong( PdfName::KeySize, 0 ) );
#endif // PODOFO_VERBOSE_DEBUG
    }
}

void PdfParser::ReadXRef( long* pXRefOffset )
{
    FindToken( "startxref", PDF_XREF_BUF );

    if( !this->IsNextToken( "startxref" ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    }
    
    *pXRefOffset = this->GetNextNumber();
}

void PdfParser::ReadXRefContents( long lOffset, bool bPositionAtEnd )
{
    long        nFirstObject = 0;
    long        nNumObjects  = 0;

    m_device.Device()->Seek( lOffset );
    
    //GetNextStringFromFile( );
    //if( strncmp( m_buffer.GetBuffer(), "xref", 4 ) != 0 )
    if( !this->IsNextToken( "xref" ) )
    {
        if( m_ePdfVersion < ePdfVersion_1_5 )
        {
            PODOFO_RAISE_ERROR( ePdfError_NoXRef );
        }
        else
        {
            ReadXRefStreamContents( lOffset, bPositionAtEnd );
            return;
        }
    }

    // read all xref subsections
    for( ;; )
    {
        try {
            //nFirstObject = GetNextNumberFromFile();
            //nNumObjects  = GetNextNumberFromFile();

            nFirstObject = this->GetNextNumber();
            nNumObjects  = this->GetNextNumber();

#ifdef PODOFO_VERBOSE_DEBUG
            PdfError::DebugMessage("Reading numbers: %i %i\n", nFirstObject, nNumObjects );
#endif // PODOFO_VERBOSE_DEBUG

            if( bPositionAtEnd )
            {
                m_device.Device()->Seek( nNumObjects* PDF_XREF_ENTRY_SIZE, std::ios_base::cur );
            }
            else
                ReadXRefSubsection( nFirstObject, nNumObjects );

        } catch( PdfError & e ) {
            if( e == ePdfError_NoNumber || e == ePdfError_InvalidXRef) 
                break;
            else 
            {
                e.AddToCallstack( __FILE__, __LINE__ );
                throw e;
            }
        }
    }

    try {
        ReadNextTrailer();
    } catch( PdfError & e ) {
        if( e != ePdfError_NoTrailer ) 
        {
            e.AddToCallstack( __FILE__, __LINE__ );
            throw e;
        }
    }
}

void PdfParser::ReadXRefSubsection( long & nFirstObject, long & nNumObjects )
{
    int         count        = 0;

#ifdef PODOFO_VERBOSE_DEBUG
    PdfError::DebugMessage("Reading XRef Section: %i with %i Objects\n", nFirstObject, nNumObjects );
#endif // PODOFO_VERBOSE_DEBUG 

    while( count < nNumObjects && m_device.Device()->Read( m_buffer.GetBuffer(), PDF_XREF_ENTRY_SIZE ) == PDF_XREF_ENTRY_SIZE )
    {
        m_buffer.GetBuffer()[PDF_XREF_ENTRY_SIZE] = '\0';

        int objID = nFirstObject + count;
        if( objID >= m_nNumObjects )
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidXRef );
        }
 
        if( !m_pOffsets[objID].bParsed )
        {
            m_pOffsets[objID].bParsed = true;
            sscanf( m_buffer.GetBuffer(), "%10ld %5ld %c \n", 
                    &(m_pOffsets[objID].lOffset), 
                    &(m_pOffsets[objID].lGeneration), &(m_pOffsets[objID].cUsed) );
       }

        ++count;
    }

    if( count != nNumObjects )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    }

    // now check if there is another xref section right before the next object.
    /*
    if( m_ppOffsets[nFirstObject]->cUsed == 'n' )
    {
        long lOffset = 0;

        PdfError::DebugMessage("Searching at: %i\n", m_ppOffsets[nFirstObject]->lOffset );
        SAFE_OP( fseek( m_hFile, m_ppOffsets[nFirstObject]->lOffset, SEEK_SET ) );
        eCode = ReadXRef( &lOffset );
        if( eCode )
            eCode == ErrOk;

        PdfError::DebugMessage("Found XRef at: %i\n", lOffset );
        SAFE_OP( ReadXRefContents( lOffset ) );
    }
    */
}

void PdfParser::ReadXRefStreamContents( long lOffset, bool bReadOnlyTrailer )
{
    char*       pBuffer;
    char*       pStart;
    long        lBufferLen;
    long        lSize     = 0;
    PdfVariant  vWArray;
    PdfObject*  pObj;

    long        nW[W_ARRAY_SIZE] = { 0, 0, 0 };
    int         i;

    m_device.Device()->Seek( lOffset );

    PdfParserObject xrefObject( m_vecObjects, m_device, m_buffer );
    xrefObject.ParseFile();


    if( !xrefObject.GetDictionary().HasKey( PdfName::KeyType ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    } 

    pObj = xrefObject.GetDictionary().GetKey( PdfName::KeyType );
    if( !pObj->IsName() || ( pObj->GetName() != "XRef" ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    } 

    if( !m_pTrailer )    
        m_pTrailer = new PdfParserObject( m_vecObjects, m_device, m_buffer );

    MergeTrailer( &xrefObject );

    if( bReadOnlyTrailer )
        return;

    if( !xrefObject.GetDictionary().HasKey( PdfName::KeySize ) || !xrefObject.GetDictionary().HasKey( "W" ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    }

    lSize   = xrefObject.GetDictionary().GetKeyAsLong( PdfName::KeySize, 0 );
    vWArray = *(xrefObject.GetDictionary().GetKey( "W" ));

    // The pdf reference states that W is always an array with 3 entries
    // all of them have to be integeres
    if( !vWArray.IsArray() || vWArray.GetArray().size() != 3 )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    }

    for( i=0;i<W_ARRAY_SIZE;i++ )
    {
        if( !vWArray.GetArray()[i].IsNumber() )
        {
            PODOFO_RAISE_ERROR( ePdfError_NoXRef );
        }

        nW[i] = vWArray.GetArray()[i].GetNumber();
    }

    std::vector<int> vecIndeces;
    // get the first object number in this crossref stream.
    // it is not required to have an index key though.
    if( xrefObject.GetDictionary().HasKey( "Index" ) )
    {
        // reuse vWArray!!
        vWArray = *(xrefObject.GetDictionary().GetKey( "Index" ));
        if( !vWArray.IsArray() )
        {
            PODOFO_RAISE_ERROR( ePdfError_NoXRef );
        }

        TCIVariantList it = vWArray.GetArray().begin();
        while ( it != vWArray.GetArray().end() )
        {
            vecIndeces.push_back( (*it).GetNumber() );
            ++it;
        }
    }
    else
    {
        vecIndeces.push_back( 0 );
        vecIndeces.push_back( lSize );
    }

    if( vecIndeces.size() % 2 )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    }

    if( !xrefObject.HasStreamToParse() )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    }

    xrefObject.GetStream()->GetFilteredCopy( &pBuffer, &lBufferLen );

    pStart        = pBuffer;
    int nCurIndex = 0;
    while( nCurIndex < vecIndeces.size() && pBuffer - pStart < lBufferLen )
    {
        long nFirstObj = vecIndeces[nCurIndex];
        long nCount    = vecIndeces[nCurIndex+1];

        PdfError::DebugMessage( "Reading Subrefsection: %li %li\n", nFirstObj, nCount );

        while( nCount-- && pBuffer - pStart < lBufferLen ) 
        {
            ReadXRefStreamEntry( pBuffer, lBufferLen, nW, nFirstObj++ );
            pBuffer += (nW[0] + nW[1] + nW[2]);
        }

        nCurIndex += 2;
    }
    free( pStart );

    if( xrefObject.GetDictionary().HasKey("Prev") )
    {
        lOffset = xrefObject.GetDictionary().GetKeyAsLong( "Prev", 0 );
        ReadXRefStreamContents( lOffset, bReadOnlyTrailer );
    }
}

void PdfParser::ReadXRefStreamEntry( char* pBuffer, long lLen, long lW[W_ARRAY_SIZE], int nObjNo )
{
    int              i, z;
    unsigned long    nData[W_ARRAY_SIZE];

    for( i=0;i<W_ARRAY_SIZE;i++ )
    {
        if( lW[i] > W_MAX_BYTES )
        {
            PdfError::LogMessage( eLogSeverity_Error, "The XRef stream dictionary has an entry in /W of size %i.\nThe maximum supported value is %i.\n", lW[i], W_MAX_BYTES );

            PODOFO_RAISE_ERROR( ePdfError_InvalidXRefStream );
        }
        
        nData[i] = 0;
        for( z=W_MAX_BYTES-lW[i];z<W_MAX_BYTES;z++ )
        {
            nData[i] = (nData[i] << 8) + static_cast<unsigned char>(*pBuffer);
            ++pBuffer;
        }
    }

    m_pOffsets[nObjNo].bParsed = true;

    switch( nData[0] ) // nData[0] contains the type information of this entry
    {
        case 0:
            // a free object
            m_pOffsets[nObjNo].lOffset     = nData[1];
            m_pOffsets[nObjNo].lGeneration = nData[2];
            m_pOffsets[nObjNo].cUsed       = 'f';
            break;
        case 1:
            // normal uncompressed object
            m_pOffsets[nObjNo].lOffset     = nData[1];
            m_pOffsets[nObjNo].lGeneration = nData[2];
            m_pOffsets[nObjNo].cUsed       = 'n';
            break;
        case 2:
            // object that is part of an object stream
            m_pOffsets[nObjNo].lOffset     = nData[2]; // index in the object stream
            m_pOffsets[nObjNo].lGeneration = nData[1]; // object number of the stream
            m_pOffsets[nObjNo].cUsed       = 's';      // mark as stream
            break;
        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidXRefType );
        }
    }
}

void PdfParser::ReadObjects()
{
    int              i          = 0;
    int              nLast      = 0;
    PdfParserObject* pObject    = NULL;

    m_vecObjects->Reserve( m_nNumObjects );

    for( ; i <= m_nNumObjects; i++ )
    {
        if( m_pOffsets[i].bParsed && m_pOffsets[i].cUsed == 'n' )
        {
            pObject = new PdfParserObject( m_vecObjects, m_device, m_buffer, m_pOffsets[i].lOffset );
            pObject->SetLoadOnDemand( m_bLoadOnDemand );
            try {
                pObject->ParseFile();
                nLast = pObject->Reference().ObjectNumber();

                // final pdf should not contain a linerization dictionary as it contents are invalid 
                // as we change some objects and the final xref table
                if( m_pLinearization && nLast == static_cast<int>(m_pLinearization->Reference().ObjectNumber()) )
                {
                    m_vecObjects->AddFreeObject( pObject->Reference() );
                    delete pObject;
                }
                else
                    m_vecObjects->push_back( pObject );
            } catch( PdfError & e ) {
                std::ostringstream oss;
                if( pObject )
                {
                    oss << "Error while loading object " << pObject->Reference().ObjectNumber() << " " 
                        << pObject->Reference().GenerationNumber() << std::endl;
                    delete pObject;
                }

                e.AddToCallstack( __FILE__, __LINE__, oss.str().c_str() );
                throw e;
            }
        }
        else if( m_pOffsets[i].bParsed && m_pOffsets[i].cUsed == 'f' && m_pOffsets[i].lOffset )
        {
            m_vecObjects->AddFreeObject( PdfReference( m_pOffsets[i].lOffset, 1 ) ); // TODO: do not hard code
        }
    }

    // all normal objects including object streams are available now,
    // we can parse the object streams safely now.
    //
    // Note that even if demand loading is enabled we still currently read all
    // objects from the stream into memory then free the stream.
    //
    for( i = 0; i < m_nNumObjects; i++ )
    {
        if( m_pOffsets[i].bParsed && m_pOffsets[i].cUsed == 's' ) // we have an object stream
        {
#if defined(PODOFO_VERBOSE_DEBUG)
            if (m_bLoadOnDemand) cerr << "Demand loading on, but can't demand-load found object stream." << endl;
#endif
            ReadObjectFromStream( m_pOffsets[i].lGeneration, m_pOffsets[i].lOffset );
        }
    }

    if( !m_bLoadOnDemand )
    {
        // Force loading of streams. We can't do this during the initial
        // run that populates m_vecObjects because a stream might have a /Length
        // key that references an object we haven't yet read. So we must do it here
        // in a second pass, or (if demand loading is enabled) defer it for later.
        for (TCIVecObjects itObjects = m_vecObjects->begin();
             itObjects != m_vecObjects->end();
             ++itObjects)
        {
            pObject = dynamic_cast<PdfParserObject*>(*itObjects);
            // only parse streams for objects that have not yet parsed
            // their streams
            if( pObject && pObject->HasStreamToParse() && !pObject->HasStream() )
                pObject->GetStream();
        }
    }
}

void PdfParser::ReadObjectFromStream( int nObjNo, int nIndex )
{
    // check if we already have read all objects
    // from this stream
    if( m_setObjectStreams.find( nObjNo ) != m_setObjectStreams.end() )
    {
        return;
    }
    else
        m_setObjectStreams.insert( nObjNo );

    // generation number of object streams is always 0
    PdfParserObject * const pStream = dynamic_cast<PdfParserObject*>(m_vecObjects->GetObject( PdfReference( nObjNo, 0 ) ) );
    if( !pStream )
    {
        std::ostringstream oss;
        oss << "Loading of object " << nObjNo << " 0 R failed!" << std::endl;

        PODOFO_RAISE_ERROR_INFO( ePdfError_NoObject, oss.str().c_str() );
    }
    
    long lNum   = pStream->GetDictionary().GetKeyAsLong( "N", 0 );
    long lFirst = pStream->GetDictionary().GetKeyAsLong( "First", 0 );
    
    char * pBuffer;
    long lBufferLen;

    pStream->GetStream()->GetFilteredCopy( &pBuffer, &lBufferLen );

    // the object stream is not needed anymore in the final PDF
    delete m_vecObjects->RemoveObject( pStream->Reference() );

    PdfRefCountedInputDevice device( pBuffer, lBufferLen );
    PdfTokenizer             tokenizer( device, m_buffer );
    PdfVariant               var;
    int                      i = 0;

    while( i < lNum )
    {
        const long lObj          = tokenizer.GetNextNumber();
        const long lOff          = tokenizer.GetNextNumber();
        const std::streamoff pos = device.Device()->Tell();

        // move to the position of the object in the stream
        device.Device()->Seek( lFirst + lOff );

        tokenizer.GetNextVariant( var );
        m_vecObjects->push_back( new PdfObject( PdfReference( lObj, 0 ), var ) );

        // move back to the position inside of the table of contents
        device.Device()->Seek( pos );

        ++i;
    }
    
    free( pBuffer );
}

const char* PdfParser::GetPdfVersionString() const
{
    return s_szPdfVersions[static_cast<int>(m_ePdfVersion)];
}

void PdfParser::FindToken( const char* pszToken, const long lRange )
{
    m_device.Device()->Seek( 0, std::ios_base::end );

    std::streamoff nFileSize = m_device.Device()->Tell();
    if (nFileSize == -1)
    {
        PODOFO_RAISE_ERROR_INFO(
                ePdfError_NoXRef,
                "Failed to seek to EOF when looking for xref");
    }

    long           lXRefBuf  = PDF_MIN( nFileSize, lRange );
    const int      nTokenLen = strlen( pszToken );
    int            i;

    m_device.Device()->Seek( -lXRefBuf, std::ios_base::cur );
    if( m_device.Device()->Read( m_buffer.GetBuffer(), lXRefBuf ) != lXRefBuf && !m_device.Device()->Eof() )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    }

    m_buffer.GetBuffer()[lXRefBuf] = '\0';

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    for( i = lXRefBuf - nTokenLen; i >= 0; i-- )
        if( strncmp( m_buffer.GetBuffer()+i, pszToken, nTokenLen ) == 0 )
        {
            break;
        }

    if( !i )
    {
        PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
    }

    m_device.Device()->Seek( (lXRefBuf-i)*-1, std::ios_base::end );
}


};



