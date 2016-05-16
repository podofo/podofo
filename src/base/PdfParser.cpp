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

#include "PdfParser.h"

#include "PdfArray.h"
#include "PdfDefinesPrivate.h"
#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfInputDevice.h"
#include "PdfMemStream.h"
#include "PdfObjectStreamParserObject.h"
#include "PdfOutputDevice.h"
#include "PdfParserObject.h"
#include "PdfStream.h"
#include "PdfVariant.h"
#include "PdfXRefStreamParserObject.h"

#include <cstring>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <limits>

using std::cerr;
using std::endl;
using std::flush;

#define PDF_MAGIC_LEN       8
#define PDF_XREF_ENTRY_SIZE 20
#define PDF_XREF_BUF        512

#if defined( PTRDIFF_MAX )
#define PDF_LONG_MAX PTRDIFF_MAX
#else
// only old compilers don't define PTRDIFF_MAX (all 32-bit only?)
#define PDF_LONG_MAX INT_MAX
#endif

namespace PoDoFo {

long PdfParser::s_nMaxObjects = std::numeric_limits<long>::max();

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

#ifdef _WIN32
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200    // not for MS Visual Studio 6
#else
PdfParser::PdfParser( PdfVecObjects* pVecObjects, const wchar_t* pszFilename, bool bLoadOnDemand )
    : PdfTokenizer(), m_vecObjects( pVecObjects )
{
    this->Init();
    this->ParseFile( pszFilename, bLoadOnDemand );
}
#endif
#endif // _WIN32

PdfParser::PdfParser( PdfVecObjects* pVecObjects, const char* pBuffer, long lLen, bool bLoadOnDemand )
    : PdfTokenizer(), m_vecObjects( pVecObjects )
{
    this->Init();
    this->ParseFile( pBuffer, lLen, bLoadOnDemand );
}

PdfParser::PdfParser( PdfVecObjects* pVecObjects, const PdfRefCountedInputDevice & rDevice, 
                      bool bLoadOnDemand )
    : PdfTokenizer(), m_vecObjects( pVecObjects )
{
    this->Init();

    if( !rDevice.Device() )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, "Cannot create PdfRefCountedInputDevice." );
    }

    this->ParseFile( rDevice, bLoadOnDemand );
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
    m_offsets.clear();

    m_pEncrypt        = NULL;

    m_ePdfVersion     = ePdfVersion_Default;

    m_nXRefOffset     = 0;
    m_nFirstObject    = 0;
    m_nNumObjects     = 0;
    m_nXRefLinearizedOffset = 0;
    m_lLastEOFOffset  = 0;

    m_bStrictParsing  = false;
    m_bIgnoreBrokenObjects = false;
    m_nIncrementalUpdates = 0;
    m_nReadNextTrailerLevel = 0;
}

void PdfParser::ParseFile( const char* pszFilename, bool bLoadOnDemand )
{
    if( !pszFilename || !pszFilename[0] )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    PdfRefCountedInputDevice device( pszFilename, "rb" );
    if( !device.Device() )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, pszFilename );
    }

    this->ParseFile( device, bLoadOnDemand );
}

#ifdef _WIN32
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200			// not for MS Visual Studio 6
#else
void PdfParser::ParseFile( const wchar_t* pszFilename, bool bLoadOnDemand )
{
    if( !pszFilename || !pszFilename[0] )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    PdfRefCountedInputDevice device( pszFilename, "rb" );
    if( !device.Device() )
    {
		PdfError e( ePdfError_FileNotFound, __FILE__, __LINE__ );
		e.SetErrorInformation( pszFilename );
		throw e;
	}

    this->ParseFile( device, bLoadOnDemand );
}
#endif
#endif // _WIN32

void PdfParser::ParseFile( const char* pBuffer, long lLen, bool bLoadOnDemand )
{
    if( !pBuffer || !lLen )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    PdfRefCountedInputDevice device( pBuffer, lLen );
    if( !device.Device() )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, "Cannot create PdfParser from buffer." );
    }

    this->ParseFile( device, bLoadOnDemand );
}

void PdfParser::ParseFile( const PdfRefCountedInputDevice & rDevice, bool bLoadOnDemand )
{
    Clear();

    m_device = rDevice;

    m_bLoadOnDemand = bLoadOnDemand;

    try {
        if( !IsPdfFile() )
        {
            PODOFO_RAISE_ERROR( ePdfError_NoPdfFile );
        }
    
        ReadDocumentStructure();
        ReadObjects();
    } catch( PdfError & e ) {
        if( e.GetError() == ePdfError_InvalidPassword ) 
        {
            // Do not clean up, expect user to call ParseFile again
            throw e;
        }

        // If this is being called from a constructor then the
        // destructor will not be called.
        // Clean up here  
        Clear();
        e.AddToCallstack( __FILE__, __LINE__, "Unable to load objects from file." );
        throw e;
    }
}


void PdfParser::Clear()
{
    m_setObjectStreams.clear();
    m_offsets.clear();

    m_device = PdfRefCountedInputDevice();

    delete m_pTrailer;
    m_pTrailer = NULL;

    delete m_pLinearization;
    m_pLinearization = NULL;

    delete m_pEncrypt;
    m_pEncrypt = NULL;

    this->Init();
}

void PdfParser::ReadDocumentStructure()
{
// Ulrich Arnold 8.9.2009, deactivated because of problems during reading xref's
    // HasLinearizationDict();
    
    // position at the end of the file to search the xref table.
    m_device.Device()->Seek( 0, std::ios_base::end );
    m_nFileSize = m_device.Device()->Tell();

// James McGill 18.02.2011, validate the eof marker and when not in strict mode accept garbage after it
    try {
        CheckEOFMarker();
    } catch( PdfError & e ) {
        e.AddToCallstack( __FILE__, __LINE__, "EOF marker could not be found." );
        throw e;
    }

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

    if( m_pTrailer->IsDictionary() && m_pTrailer->GetDictionary().HasKey( PdfName::KeySize ) )
    {
        m_nNumObjects = static_cast<long>(m_pTrailer->GetDictionary().GetKeyAsLong( PdfName::KeySize ));
    }
    else
    {
        PdfError::LogMessage( eLogSeverity_Warning, "PDF Standard Violation: No /Size key was specified in the trailer directory. Will attempt to recover." );
        // Treat the xref size as unknown, and expand the xref dynamically as we read it.
        m_nNumObjects = 0;
    }

    // allow caller to specify a max object count to avoid very slow load times on large documents
    if (s_nMaxObjects != std::numeric_limits<long>::max()
        && m_nNumObjects > s_nMaxObjects)
        PODOFO_RAISE_ERROR_INFO( ePdfError_ValueOutOfRange,  "m_nNumObjects is greater than m_nMaxObjects." );

    if (m_nNumObjects > 0)
        m_offsets.resize(m_nNumObjects);

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
    {
        if( strncmp( m_buffer.GetBuffer(), s_szPdfVersions[i], PDF_MAGIC_LEN ) == 0 )
        {
            m_ePdfVersion = static_cast<EPdfVersion>(i);
            break;
        }
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

    // The linearization dictionary must be in the first 1024 
    // bytes of the PDF, our buffer might be larger so.
    // Therefore read only the first 1024 byte.
    // Normally we should jump to the end of the file, to determine
    // it's filesize and read the min(1024, filesize) to not fail
    // on smaller files, but jumping to the end is against the idea
    // of linearized PDF. Therefore just check if we read anything.
    const std::streamoff MAX_READ = 1024;
    PdfRefCountedBuffer linearizeBuffer( MAX_READ );

    std::streamoff size = m_device.Device()->Read( linearizeBuffer.GetBuffer(), 
                                                   linearizeBuffer.GetSize() );
    // Only fail if we read nothing, to allow files smaller than MAX_READ
    if( static_cast<size_t>(size) <= 0 )
    {
        // Clear the error state from the bad read
        m_device.Device()->Clear();
        return; // Ignore Error Code: ERROR_PDF_NO_TRAILER;
    }

    //begin L.K
    //char * pszObj = strstr( m_buffer.GetBuffer(), "obj" );
    char * pszObj = strstr( linearizeBuffer.GetBuffer(), "obj" );
    //end L.K
    if( !pszObj )
        // strange that there is no obj in the first 1024 bytes,
        // but ignore it
        return;
    
    --pszObj; // *pszObj == 'o', so the while would fail without decrement
    while( *pszObj && (PdfTokenizer::IsWhitespace( *pszObj ) || (*pszObj >= '0' && *pszObj <= '9')) )
        --pszObj;

    m_pLinearization = new PdfParserObject( m_vecObjects, m_device, linearizeBuffer,
                                            pszObj - linearizeBuffer.GetBuffer() + 2 );

    try {
        // Do not care for encryption here, as the linearization dictionary does not contain strings or streams
        // ... hint streams do, but we do not load the hintstream.
        static_cast<PdfParserObject*>(m_pLinearization)->ParseFile( NULL );
        if (! (m_pLinearization->IsDictionary() &&
               m_pLinearization->GetDictionary().HasKey( "Linearized" ) ) )
        {
            delete m_pLinearization;
            m_pLinearization = NULL;
            return;
        }
    } catch( PdfError & e ) {
        PdfError::LogMessage( eLogSeverity_Warning, e.ErrorName(e.GetError()) );
        delete m_pLinearization;
        m_pLinearization = NULL;
        return;
    }
    
    pdf_int64      lXRef      = -1;
    lXRef = m_pLinearization->GetDictionary().GetKeyAsLong( "T", lXRef );
    if( lXRef == -1 )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidLinearization );
    }

    // avoid moving to a negative file position here
    m_device.Device()->Seek( (static_cast<pdf_long>(lXRef-PDF_XREF_BUF) > 0 ? static_cast<pdf_long>(lXRef-PDF_XREF_BUF) : PDF_XREF_BUF) );
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
            m_nXRefLinearizedOffset = static_cast<pdf_long>(lXRef);
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

    // Only update keys, if not already present                   
    if( pTrailer->GetDictionary().HasKey( PdfName::KeySize )      
        && !m_pTrailer->GetDictionary().HasKey( PdfName::KeySize ) )
        m_pTrailer->GetDictionary().AddKey( PdfName::KeySize, *(pTrailer->GetDictionary().GetKey( PdfName::KeySize )) );
                                                                                                                         
    if( pTrailer->GetDictionary().HasKey( "Root" )                                                                      
        && !m_pTrailer->GetDictionary().HasKey( "Root" ))                                                               
        m_pTrailer->GetDictionary().AddKey( "Root", *(pTrailer->GetDictionary().GetKey( "Root" )) );                    
                                                                                                                         
    if( pTrailer->GetDictionary().HasKey( "Encrypt" )                                                                   
        && !m_pTrailer->GetDictionary().HasKey( "Encrypt" ) )                                                                                                                                            
        m_pTrailer->GetDictionary().AddKey( "Encrypt", *(pTrailer->GetDictionary().GetKey( "Encrypt" )) );                                                                                               
                                                                                                                                                                                                          
    if( pTrailer->GetDictionary().HasKey( "Info" )                                                                                                                                                       
        && !m_pTrailer->GetDictionary().HasKey( "Info" ) )                                                                                                                                               
        m_pTrailer->GetDictionary().AddKey( "Info", *(pTrailer->GetDictionary().GetKey( "Info" )) );                                                                                                     
                                                                                                                                                                                                          
    if( pTrailer->GetDictionary().HasKey( "ID" )                                                                                                                                                         
        && !m_pTrailer->GetDictionary().HasKey( "ID" ) )                                                                                                                                                 
        m_pTrailer->GetDictionary().AddKey( "ID", *(pTrailer->GetDictionary().GetKey( "ID" )) );        
}

void PdfParser::ReadNextTrailer()
{
    // be careful changing this limit - overflow limits depend on the OS, linker settings, and how much stack space compiler allocates
    // 500 limit prevents overflow on Win7 with VC++ 2005 with default linker stack size (1000 caused overflow with same compiler/OS)
    const int maxReadNextTrailerLevel = 500;
    
    ++m_nReadNextTrailerLevel;
    
    if ( m_nReadNextTrailerLevel > maxReadNextTrailerLevel )
    {
        // avoid stack overflow on documents that have circular cross references in trailer
        PODOFO_RAISE_ERROR( ePdfError_InvalidXRef );
    }

    // ReadXRefcontents has read the first 't' from "trailer" so just check for "railer"
    if( this->IsNextToken( "trailer" ) )
    //if( strcmp( m_buffer.GetBuffer(), "railer" ) == 0 )
    {
        PdfParserObject trailer( m_vecObjects, m_device, m_buffer );
        try {
            // Ignore the encryption in the trailer as the trailer may not be encrypted
            trailer.ParseFile( NULL, true );
        } catch( PdfError & e ) {
            e.AddToCallstack( __FILE__, __LINE__, "The linearized trailer was found in the file, but contains errors." );
            throw e;
        }

        // now merge the information of this trailer with the main documents trailer
        MergeTrailer( &trailer );
        
        if( trailer.GetDictionary().HasKey( "XRefStm" ) )
        {
            // Whenever we read a XRefStm key, 
            // we know that the file was updated.
            if( !trailer.GetDictionary().HasKey( "Prev" ) )
                m_nIncrementalUpdates++;

            try {
                ReadXRefStreamContents( static_cast<pdf_long>(trailer.GetDictionary().GetKeyAsLong( "XRefStm", 0 )), false );
            } catch( PdfError & e ) {
                e.AddToCallstack( __FILE__, __LINE__, "Unable to load /XRefStm xref stream." );
                throw e;
            }
        }

        if( trailer.GetDictionary().HasKey( "Prev" ) )
        {
            // Whenever we read a Prev key, 
            // we know that the file was updated.
            m_nIncrementalUpdates++;

            try {
                ReadXRefContents( static_cast<pdf_long>(trailer.GetDictionary().GetKeyAsLong( "Prev", 0 )) );
            } catch( PdfError & e ) {
                e.AddToCallstack( __FILE__, __LINE__, "Unable to load /Prev xref entries." );
                throw e;
            }
        }
    }
    else // OC 13.08.2010 BugFix: else belongs to IsNextToken( "trailer" ) and not to HasKey( "Prev" )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoTrailer );
    }

    --m_nReadNextTrailerLevel;
}

void PdfParser::ReadTrailer()
{
    FindToken( "trailer", PDF_XREF_BUF );
    
    if( !this->IsNextToken( "trailer" ) ) 
    {
//      if( m_ePdfVersion < ePdfVersion_1_5 )
//		Ulrich Arnold 19.10.2009, found linearized 1.3-pdf's with trailer-info in xref-stream
        if( m_ePdfVersion < ePdfVersion_1_3 )
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
            static_cast<PdfParserObject*>(m_pTrailer)->ParseFile( NULL, false );
            return;
        }
    }
    else 
    {
        m_pTrailer = new PdfParserObject( m_vecObjects, m_device, m_buffer );
        try {
            // Ignore the encryption in the trailer as the trailer may not be encrypted
            static_cast<PdfParserObject*>(m_pTrailer)->ParseFile( NULL, true );
        } catch( PdfError & e ) {
            e.AddToCallstack( __FILE__, __LINE__, "The trailer was found in the file, but contains errors." );
            throw e;
        }
#ifdef PODOFO_VERBOSE_DEBUG
        PdfError::DebugMessage("Size=%li\n", m_pTrailer->GetDictionary().GetKeyAsLong( PdfName::KeySize, 0 ) );
#endif // PODOFO_VERBOSE_DEBUG
    }
}

void PdfParser::ReadXRef( pdf_long* pXRefOffset )
{
    FindToken( "startxref", PDF_XREF_BUF );

    if( !this->IsNextToken( "startxref" ) )
    {
		// Could be non-standard startref
		if(!m_bStrictParsing) {
			FindToken( "startref", PDF_XREF_BUF );
			if( !this->IsNextToken( "startref" ) )
			{
				PODOFO_RAISE_ERROR( ePdfError_NoXRef );
			}
		} else 
		{
			PODOFO_RAISE_ERROR( ePdfError_NoXRef );
		}
    }
    *pXRefOffset = this->GetNextNumber();
}

void PdfParser::ReadXRefContents( pdf_long lOffset, bool bPositionAtEnd )
{
    pdf_int64 nFirstObject = 0;
    pdf_int64 nNumObjects  = 0;

    size_t curPosition = m_device.Device()->Tell();
    m_device.Device()->Seek(0,std::ios_base::end);
    std::streamoff fileSize = m_device.Device()->Tell();
    m_device.Device()->Seek(curPosition,std::ios_base::beg);

    if (lOffset > fileSize)
    { 
        // Invalid "startxref" Peter Petrov 23 December 2008
		 // ignore returned value and get offset from the device
        ReadXRef( &lOffset );
        lOffset = m_device.Device()->Tell();
        // TODO: hard coded value "4"
        m_buffer.Resize(PDF_XREF_BUF*4);
        FindToken2("xref", PDF_XREF_BUF*4,lOffset);
        m_buffer.Resize(PDF_XREF_BUF);
        lOffset = m_device.Device()->Tell();
        m_nXRefOffset = lOffset;
    }
    else
    {
        m_device.Device()->Seek( lOffset );
    }
    
    if( !this->IsNextToken( "xref" ) )
    {
//      if( m_ePdfVersion < ePdfVersion_1_5 )
//		Ulrich Arnold 19.10.2009, found linearized 1.3-pdf's with trailer-info in xref-stream
        if( m_ePdfVersion < ePdfVersion_1_3 )
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
    // OC 13.08.2010: Avoid exception to terminate endless loop
    for( int nXrefSection = 0; ; ++nXrefSection )
    {
        try {

            // OC 13.08.2010: Avoid exception to terminate endless loop
            if ( nXrefSection > 0 )
            {
                // something like PeekNextToken()
                EPdfTokenType eType;
                const char* pszRead;
                bool gotToken = this->GetNextToken( pszRead, &eType );
                if( gotToken )
                {
                    this->QuequeToken( pszRead, eType );
                    if ( strcmp( "trailer", pszRead ) == 0 )
                        break;
                }
            }

            nFirstObject = this->GetNextNumber();
            nNumObjects  = this->GetNextNumber();

#ifdef PODOFO_VERBOSE_DEBUG
            PdfError::DebugMessage("Reading numbers: %" PDF_FORMAT_INT64 " %" PDF_FORMAT_INT64 "\n", nFirstObject, nNumObjects );
#endif // PODOFO_VERBOSE_DEBUG

            if( bPositionAtEnd )
            {
#ifdef _WIN32
				m_device.Device()->Seek( static_cast<std::streamoff>(nNumObjects* PDF_XREF_ENTRY_SIZE), std::ios_base::cur );
#else
                m_device.Device()->Seek( nNumObjects* PDF_XREF_ENTRY_SIZE, std::ios_base::cur );
#endif // _WIN32
			}
            else
            {
                ReadXRefSubsection( nFirstObject, nNumObjects );
            }
        } catch( PdfError & e ) {
            if( e == ePdfError_NoNumber || e == ePdfError_InvalidXRef || e == ePdfError_UnexpectedEOF ) 
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

void PdfParser::ReadXRefSubsection( pdf_int64 & nFirstObject, pdf_int64 & nNumObjects )
{
    int count = 0;

#ifdef PODOFO_VERBOSE_DEBUG
    PdfError::DebugMessage("Reading XRef Section: %" PDF_FORMAT_INT64 " with %" PDF_FORMAT_INT64 " Objects.\n", nFirstObject, nNumObjects );
#endif // PODOFO_VERBOSE_DEBUG 

    if ( nFirstObject + nNumObjects > m_nNumObjects )
    {
        // Total number of xref entries to read is greater than the /Size
        // specified in the trailer if any. That's an error unless we're trying
        // to recover from a missing /Size entry.
		PdfError::LogMessage( eLogSeverity_Warning,
			      "There are more objects (%" PDF_FORMAT_INT64 ") in this XRef table than "
			      "specified in the size key of the trailer directory (%" PDF_FORMAT_INT64 ")!\n",
			      nFirstObject + nNumObjects, m_nNumObjects );

#ifdef _WIN32
		m_nNumObjects = static_cast<long>(nFirstObject + nNumObjects);
		m_offsets.resize(static_cast<long>(nFirstObject+nNumObjects));
#else
		m_nNumObjects = nFirstObject + nNumObjects;
		m_offsets.resize(nFirstObject+nNumObjects);
#endif // _WIN32
	}

    // consume all whitespaces
    int charcode;
    while( this->IsWhitespace((charcode = m_device.Device()->Look())) )
    {
        m_device.Device()->GetChar();
    }

    while( count < nNumObjects && m_device.Device()->Read( m_buffer.GetBuffer(), PDF_XREF_ENTRY_SIZE ) == PDF_XREF_ENTRY_SIZE )
    {
        char empty1;
        char empty2;
        m_buffer.GetBuffer()[PDF_XREF_ENTRY_SIZE] = '\0';

#ifdef _WIN32
		const int objID = static_cast<int>(nFirstObject+count);
#else
		const int objID = nFirstObject+count;
#endif // _WIN32

        if( static_cast<size_t>(objID) < m_offsets.size() && !m_offsets[objID].bParsed )
        {
            // don't scan directly into m_offsets since TXRefEntry structure member sizes change between platforms and compilers
            //
            //  pdf_long lOffset; // pdf_long is ptrdiff_t: 64 bits on Mac64, Linux64, Win64; 32 bits on Mac32, Linux32, Win32
            //  long lGeneration; // 64 bits on Mac64, Linux64; 32 bits on Win64, Mac32, Linux32, Win32
            //  char cUsed;       // always 8 bits
            //
            pdf_int64 llOffset = 0;
            pdf_int64 llGeneration = 0;
            char cUsed = 0;
            
            // XRefEntry is defined in PDF spec section 7.5.4 Cross-Reference Table as
            // nnnnnnnnnn ggggg n eol
            // nnnnnnnnnn is 10-digit offset number with max value 9999999999 (bigger than 2**32 = 4GB)
            // ggggg is a 5-digit generation number with max value 99999 (smaller than 2**17)
            int read = sscanf( m_buffer.GetBuffer(), "%10" PDF_FORMAT_INT64 " %5" PDF_FORMAT_INT64 " %c%c%c",
                              &llOffset, &llGeneration, &cUsed, &empty1, &empty2 );
            
            if ( read != 5 )
            {
                // part of XrefEntry is missing, or i/o error
                PODOFO_RAISE_ERROR( ePdfError_InvalidXRef );
            }
            
            if ( llOffset > PDF_LONG_MAX )
            {
                // pdf_long max size is PTRDIFF_MAX, so throw error if llOffset too big
                PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange ); 
            }
            
            m_offsets[objID].lOffset = static_cast<pdf_long>(llOffset);
            m_offsets[objID].lGeneration = static_cast<long>(llGeneration);
            m_offsets[objID].cUsed = cUsed;
            m_offsets[objID].bParsed = true;
       }

        ++count;
    }

    if( count != nNumObjects )
    {
        PdfError::LogMessage( eLogSeverity_Warning, "Count of readobject is %i. Expected %" PDF_FORMAT_INT64 ".\n", count, nNumObjects );
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    }

}

void PdfParser::ReadXRefStreamContents( pdf_long lOffset, bool bReadOnlyTrailer )
{
    m_device.Device()->Seek( lOffset );

    PdfXRefStreamParserObject xrefObject( m_vecObjects, m_device, m_buffer, &m_offsets );
    xrefObject.Parse();

    if( !m_pTrailer )
        m_pTrailer = new PdfParserObject( m_vecObjects, m_device, m_buffer );

    MergeTrailer( &xrefObject );

    if( bReadOnlyTrailer )
        return;

    xrefObject.ReadXRefTable();

    // Check for a previous XRefStm or xref table
    if(xrefObject.HasPrevious()) 
    {
        try {
            m_nIncrementalUpdates++;

            // PDFs that have been through multiple PDF tools may have a mix of xref tables (ISO 32000-1 7.5.4) 
            // and XRefStm streams (ISO 32000-1 7.5.8.1) and in the Prev chain, 
            // so call ReadXRefContents (which deals with both) instead of ReadXRefStreamContents 
            ReadXRefContents( xrefObject.GetPreviousOffset(), bReadOnlyTrailer );
        } catch(PdfError &e) {
            /* Be forgiving, the error happens when an entry in XRef stream points
               to a wrong place (offset) in the PDF file. */
            if( e != ePdfError_NoNumber )
            {
                e.AddToCallstack( __FILE__, __LINE__ );
                throw e;
            }
        }
    }
}

bool PdfParser::QuickEncryptedCheck( const char* pszFilename ) 
{
    bool bEncryptStatus   = false;
    bool bOldLoadOnDemand = m_bLoadOnDemand;
    Init();
    Clear();

   
    m_bLoadOnDemand   = true; // maybe will be quicker if true?

    if( !pszFilename || !pszFilename[0] )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_device = PdfRefCountedInputDevice( pszFilename, "rb" );
    if( !m_device.Device() )
    {
        //PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, pszFilename );
        // If we can not open PDF file
        // then file does not exist
        return false;
    }

    if( !IsPdfFile() )
    {
        //PODOFO_RAISE_ERROR( ePdfError_NoPdfFile );
        return false;
    }

    ReadDocumentStructure();
    try {

        m_vecObjects->Reserve( m_nNumObjects );

        // Check for encryption and make sure that the encryption object
        // is loaded before all other objects
        const PdfObject * encObj = m_pTrailer->GetDictionary().GetKey( PdfName("Encrypt") );
        if( encObj && ! encObj->IsNull() ) 
        {
            bEncryptStatus = true;
        }
    } catch( PdfError & e ) {
        m_bLoadOnDemand = bOldLoadOnDemand; // Restore load on demand behaviour
        e.AddToCallstack( __FILE__, __LINE__, "Unable to load objects from file." );
        throw e;
    }

    m_bLoadOnDemand = bOldLoadOnDemand; // Restore load on demand behaviour

    return bEncryptStatus;
}

void PdfParser::ReadObjects()
{
    int              i          = 0;
    PdfParserObject* pObject    = NULL;

    m_vecObjects->Reserve( m_nNumObjects );

    // Check for encryption and make sure that the encryption object
    // is loaded before all other objects
    PdfObject* pEncrypt = m_pTrailer->GetDictionary().GetKey( PdfName("Encrypt") );
    if( pEncrypt && !pEncrypt->IsNull() )
    {
#ifdef PODOFO_VERBOSE_DEBUG
        PdfError::DebugMessage("The PDF file is encrypted.\n" );
#endif // PODOFO_VERBOSE_DEBUG

        if( pEncrypt->IsReference() ) 
        {
            i = pEncrypt->GetReference().ObjectNumber();
            pObject = new PdfParserObject( m_vecObjects, m_device, m_buffer, m_offsets[i].lOffset );
            pObject->SetLoadOnDemand( false ); // Never load this on demand, as we will use it immediately
            try {
                pObject->ParseFile( NULL ); // The encryption dictionary is not encrypted :)
                // Never add the encryption dictionary to m_vecObjects
                // we create a new one, if we need it for writing
                // m_vecObjects->push_back( pObject );
                m_offsets[i].bParsed = false;
                m_pEncrypt = PdfEncrypt::CreatePdfEncrypt( pObject );
                delete pObject;
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
        else if( pEncrypt->IsDictionary() ) 
        {
            m_pEncrypt = PdfEncrypt::CreatePdfEncrypt( pEncrypt );
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidEncryptionDict, 
                                     "The encryption entry in the trailer is neither an object nor a reference." ); 
        }

        // Generate encryption keys
        // Set user password, try first with an empty password
        bool bAuthenticate = m_pEncrypt->Authenticate( "", this->GetDocumentId() );
#ifdef PODOFO_VERBOSE_DEBUG
        PdfError::DebugMessage("Authentication with empty password: %i.\n", bAuthenticate );
#endif // PODOFO_VERBOSE_DEBUG
        if( !bAuthenticate ) 
        {
            // authentication failed so we need a password from the user.
            // The user can set the password using PdfParser::SetPassword
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidPassword, "A password is required to read this PDF file.");
        }

    }

    ReadObjectsInternal();
}

void PdfParser::ReadObjectsInternal() 
{
    int              i            = 0;
    int              nLast        = 0;
    PdfParserObject* pObject      = NULL;

    // Read objects
    for( i=0; i < m_nNumObjects; i++ )
    {
#ifdef PODOFO_VERBOSE_DEBUG
		std::cerr << "ReadObjectsInteral\t" << i << " "
			<< (m_offsets[i].bParsed ? "parsed" : "unparsed") << " "
			<< m_offsets[i].cUsed << " "
			<< m_offsets[i].lOffset << " "
			<< m_offsets[i].lGeneration << std::endl;
#endif
        if( m_offsets[i].bParsed && m_offsets[i].cUsed == 'n' && m_offsets[i].lOffset > 0 )
        {
            //printf("Reading object %i 0 R from %li\n", i, m_offsets[i].lOffset );
            
            pObject = new PdfParserObject( m_vecObjects, m_device, m_buffer, m_offsets[i].lOffset );
            pObject->SetLoadOnDemand( m_bLoadOnDemand );
            try {
				pObject->ParseFile( m_pEncrypt );
				if (m_pEncrypt && pObject->IsDictionary()) {
					PdfObject* pObjType = pObject->GetDictionary().GetKey( PdfName::KeyType );
					if( pObjType && pObjType->IsName() && pObjType->GetName() == "XRef" ) {
						// XRef is never encrypted
						delete pObject;
						pObject = new PdfParserObject( m_vecObjects, m_device, m_buffer, m_offsets[i].lOffset );
						pObject->SetLoadOnDemand( m_bLoadOnDemand );
						pObject->ParseFile( NULL );
					}
				}
                nLast = pObject->Reference().ObjectNumber();

                /*
                if( i != pObject->Reference().ObjectNumber() ) 
                {
                    printf("Expected %i got %i\n", i, pObject->Reference().ObjectNumber());
                }
                if( pObject->Reference().ObjectNumber() != i ) 
                {
                    printf("EXPECTED: %i got %i\n", i, pObject->Reference().ObjectNumber() );
                    abort();
                }
                */

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
                    oss << "Error while loading object " << pObject->Reference().ObjectNumber() 
                        << " " << pObject->Reference().GenerationNumber() 
                        << " Offset = " << m_offsets[i].lOffset
                        << " Index = " << i << std::endl;
                    delete pObject;
                }

                if( m_bIgnoreBrokenObjects ) 
                {
                    PdfError::LogMessage( eLogSeverity_Error, oss.str().c_str() );
                    m_vecObjects->AddFreeObject( PdfReference( i, 0 ) );
                }
                else
                {
                    e.AddToCallstack( __FILE__, __LINE__, oss.str().c_str() );
                    throw e;
                }
            }
        }
        else if( m_offsets[i].bParsed && m_offsets[i].cUsed == 'n' && (m_offsets[i].lOffset == 0)  )
        {
            // There are broken PDFs which add objects with 'n' 
            // and 0 offset and 0 generation number
            // to the xref table instead of using free objects
            // treating them as free objects
            if( m_bStrictParsing ) 
            {
                PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidXRef,
                                         "Found object with 0 offset which should be 'f' instead of 'n'." );
            }
            else
            {
                PdfError::LogMessage( eLogSeverity_Warning, 
                                      "Treating object %i 0 R as a free object." );
                m_vecObjects->AddFreeObject( PdfReference( i, PODOFO_LL_LITERAL(1) ) );
            }
        }
// Ulrich Arnold 30.7.2009: the linked free list in the xref section is not always correct in pdf's
//							(especially Illustrator) but Acrobat still accepts them. I've seen XRefs 
//							where some object-numbers are alltogether missing and multiple XRefs where 
//							the link list is broken.
//							Because PdfVecObjects relies on a unbroken range, fill the free list more
//							robustly from all places which are either free or unparsed
//      else if( m_offsets[i].bParsed && m_offsets[i].cUsed == 'f' && m_offsets[i].lOffset )
//      {
//          m_vecObjects->AddFreeObject( PdfReference( static_cast<int>(m_offsets[i].lOffset), PODOFO_LL_LITERAL(1) ) ); // TODO: do not hard code
//      }
        else if( (!m_offsets[i].bParsed || m_offsets[i].cUsed == 'f') && i != 0 )
        {
			m_vecObjects->AddFreeObject( PdfReference( static_cast<int>(i), PODOFO_LL_LITERAL(1) ) ); // TODO: do not hard code generation number
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
        if( m_offsets[i].bParsed && m_offsets[i].cUsed == 's' ) // we have an object stream
        {
#if defined(PODOFO_VERBOSE_DEBUG)
            if (m_bLoadOnDemand) cerr << "Demand loading on, but can't demand-load from object stream." << endl;
#endif
            ReadObjectFromStream( static_cast<int>(m_offsets[i].lGeneration), 
                                  static_cast<int>(m_offsets[i].lOffset) );
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


    // Now sort the list of objects
    m_vecObjects->Sort();

    UpdateDocumentVersion();
}

void PdfParser::SetPassword( const std::string & sPassword )
{
    if( !m_pEncrypt ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "Cannot set password for unencrypted PDF." );
    } 

    bool bAuthenticate = m_pEncrypt->Authenticate( sPassword, this->GetDocumentId() ); 
    if( !bAuthenticate ) 
    {
#ifdef PODOFO_VERBOSE_DEBUG
        PdfError::DebugMessage("Authentication with user password failed\n" );
#endif // PODOFO_VERBOSE_DEBUG
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidPassword, "Authentication with user specified password failed.");
    }
    
    ReadObjectsInternal();
}

void PdfParser::ReadObjectFromStream( int nObjNo, int )
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
    PdfParserObject* pStream = dynamic_cast<PdfParserObject*>(m_vecObjects->GetObject( PdfReference( nObjNo, 0 ) ) );
    if( !pStream )
    {
        std::ostringstream oss;
        oss << "Loading of object " << nObjNo << " 0 R failed!" << std::endl;

        PODOFO_RAISE_ERROR_INFO( ePdfError_NoObject, oss.str().c_str() );
    }
    

	PdfObjectStreamParserObject::ObjectIdList list;
    for( int i = 0; i < m_nNumObjects; i++ ) 
    {
        if( m_offsets[i].bParsed && m_offsets[i].cUsed == 's' &&
			m_offsets[i].lGeneration == nObjNo) 
        {
            list.push_back(static_cast<pdf_int64>(i));
		}
	}
    
    PdfObjectStreamParserObject pParserObject( pStream, m_vecObjects, m_buffer, m_pEncrypt );
    pParserObject.Parse( list );
}

const char* PdfParser::GetPdfVersionString() const
{
    return s_szPdfVersions[static_cast<int>(m_ePdfVersion)];
}

void PdfParser::FindToken( const char* pszToken, const long lRange )
{
// James McGill 18.02.2011, offset read position to the EOF marker if it is not the last thing in the file
    m_device.Device()->Seek( -m_lLastEOFOffset, std::ios_base::end );

    std::streamoff nFileSize = m_device.Device()->Tell();
    if (nFileSize == -1)
    {
        PODOFO_RAISE_ERROR_INFO(
                ePdfError_NoXRef,
                "Failed to seek to EOF when looking for xref");
    }

    pdf_long lXRefBuf  = PDF_MIN( static_cast<pdf_long>(nFileSize), static_cast<pdf_long>(lRange) );
    size_t   nTokenLen = strlen( pszToken );

    m_device.Device()->Seek( -lXRefBuf, std::ios_base::cur );
    if( m_device.Device()->Read( m_buffer.GetBuffer(), lXRefBuf ) != lXRefBuf && !m_device.Device()->Eof() )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    }

    m_buffer.GetBuffer()[lXRefBuf] = '\0';

    int i; // Do not make this unsigned, this will cause infinte loops in files without trailer
 
    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    for( i = lXRefBuf - nTokenLen; i >= 0; i-- )
    {
        if( strncmp( m_buffer.GetBuffer()+i, pszToken, nTokenLen ) == 0 )
        {
            break;
        }
    }

    if( !i )
    {
        PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
    }

// James McGill 18.02.2011, offset read position to the EOF marker if it is not the last thing in the file
    m_device.Device()->Seek( ((lXRefBuf-i)*-1)-m_lLastEOFOffset, std::ios_base::end );
}

// Peter Petrov 23 December 2008
void PdfParser::FindToken2( const char* pszToken, const long lRange, size_t searchEnd )
{
    m_device.Device()->Seek( searchEnd, std::ios_base::beg );

    std::streamoff nFileSize = m_device.Device()->Tell();
    if (nFileSize == -1)
    {
        PODOFO_RAISE_ERROR_INFO(
                ePdfError_NoXRef,
                "Failed to seek to EOF when looking for xref");
    }

    pdf_long      lXRefBuf  = PDF_MIN( static_cast<pdf_long>(nFileSize), static_cast<pdf_long>(lRange) );
    size_t        nTokenLen = strlen( pszToken );

    m_device.Device()->Seek( -lXRefBuf, std::ios_base::cur );
    if( m_device.Device()->Read( m_buffer.GetBuffer(), lXRefBuf ) != lXRefBuf && !m_device.Device()->Eof() )
    {
        PODOFO_RAISE_ERROR( ePdfError_NoXRef );
    }

    m_buffer.GetBuffer()[lXRefBuf] = '\0';

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    int i; // Do not use an unsigned variable here
    for( i = lXRefBuf - nTokenLen; i >= 0; i-- )
        if( strncmp( m_buffer.GetBuffer()+i, pszToken, nTokenLen ) == 0 )
        {
            break;
        }

    if( !i )
    {
        PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
    }

    m_device.Device()->Seek( searchEnd + (lXRefBuf-i)*-1, std::ios_base::beg );
}

const PdfString & PdfParser::GetDocumentId() 
{
    if( !m_pTrailer->GetDictionary().HasKey( PdfName("ID") ) )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidEncryptionDict, "No document ID found in trailer.");
    }

    return m_pTrailer->GetDictionary().GetKey( PdfName("ID") )->GetArray()[0].GetString();
}

void PdfParser::UpdateDocumentVersion()
{
    if( m_pTrailer->IsDictionary() && m_pTrailer->GetDictionary().HasKey( PdfName("Root") ) )
    {
        PdfObject* pCatalog = m_pTrailer->GetDictionary().GetKey( PdfName("Root") );
        if( pCatalog->IsReference() ) 
        {
            pCatalog = m_vecObjects->GetObject( pCatalog->GetReference() );
        }

        if( pCatalog
            && pCatalog->IsDictionary() 
            && pCatalog->GetDictionary().HasKey( PdfName("Version" ) ) ) 
        {
            PdfObject* pVersion = pCatalog->GetDictionary().GetKey( PdfName( "Version" ) );
            for(int i=0;i<=MAX_PDF_VERSION_STRING_INDEX;i++)
            {
                if(pVersion->GetName().GetName() == s_szPdfVersionNums[i])
                {
                    PdfError::LogMessage( eLogSeverity_Information,
                                          "Updating version from %s to %s\n", 
                                          s_szPdfVersionNums[static_cast<int>(m_ePdfVersion)],
                                          s_szPdfVersionNums[i] );
                    m_ePdfVersion = static_cast<EPdfVersion>(i);
                    break;
                }
            }
        }
    }
    
}

void PdfParser::CheckEOFMarker()
{
    // Check for the existence of the EOF marker
    m_lLastEOFOffset = 0;
    const char* pszEOFToken = "%%EOF";
    const size_t nEOFTokenLen = 5;
    char pszBuff[nEOFTokenLen+1];

    m_device.Device()->Seek(-static_cast<int>(nEOFTokenLen), std::ios_base::end );
    if( IsStrictParsing() )
    {
        // For strict mode EOF marker must be at the very end of the file
        if( static_cast<size_t>(m_device.Device()->Read( pszBuff, nEOFTokenLen )) != nEOFTokenLen
            && !m_device.Device()->Eof() )
            PODOFO_RAISE_ERROR( ePdfError_NoEOFToken );

        if (strncmp( pszBuff, pszEOFToken, nEOFTokenLen) != 0)
            PODOFO_RAISE_ERROR( ePdfError_NoEOFToken );
    }
    else
    {
        // Search for the Marker from the end of the file
        pdf_long lCurrentPos =  m_device.Device()->Tell();
        bool bFound = false;
        while (lCurrentPos>=0)
        {
            m_device.Device()->Seek( lCurrentPos, std::ios_base::beg );
            if( static_cast<size_t>(m_device.Device()->Read( pszBuff, nEOFTokenLen )) != nEOFTokenLen 
                && !m_device.Device()->Eof() )
            {
                PODOFO_RAISE_ERROR( ePdfError_NoEOFToken );
            }

            if (strncmp( pszBuff, pszEOFToken, nEOFTokenLen) == 0)
            {
                bFound = true;
                break;
            }
            --lCurrentPos;
        }

        // Try and deal with garbage by offsetting the buffer reads in PdfParser from now on
        if (bFound)
            m_lLastEOFOffset = (m_nFileSize - (m_device.Device()->Tell()-1)) + nEOFTokenLen;
        else
            PODOFO_RAISE_ERROR( ePdfError_NoEOFToken );
    }
}

bool PdfParser::HasXRefStream()
{
   m_device.Device()->Tell();
   m_device.Device()->Seek( m_nXRefOffset );
   
   if( !this->IsNextToken( "xref" ) )  {
        //      if( m_ePdfVersion < ePdfVersion_1_5 )
        //		Ulrich Arnold 19.10.2009, found linearized 1.3-pdf's with trailer-info in xref-stream
       if( m_ePdfVersion < ePdfVersion_1_3 )  {
           return false;
       } else {
           return true;
       }
   }

   return false;
}


};



