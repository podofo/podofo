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
#include "PdfParserObject.h"
#include "PdfStream.h"
#include "PdfVariant.h"

#include <algorithm>
#include <cstring>
#include <cstdlib>

#define PDF_MAGIC_LEN       8
#define PDF_XREF_ENTRY_SIZE 20

namespace PoDoFo {

bool ObjectLittle( PdfObject* p1, PdfObject* p2 )
{
    return *p1 < *p2;
}

PdfParser::PdfParser( PdfVecObjects* pVecObjects )
    : PdfParserBase(), m_vecObjects( pVecObjects )
{
    this->Init();
}

PdfParser::PdfParser( PdfVecObjects* pVecObjects, const char* pszFilename, bool bLoadOnDemand )
    : PdfParserBase(), m_vecObjects( pVecObjects )
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

    m_file            = PdfRefCountedFile();
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
        RAISE_ERROR( ePdfError_FileNotFound );
    }

    // make sure everything is clean
    Clear();

    m_bLoadOnDemand = bLoadOnDemand;

    m_file           = PdfRefCountedFile( pszFilename, "rb" );
    if( !m_file.Handle() )
    {
        RAISE_ERROR( ePdfError_FileNotFound );
    }

    if( !IsPdfFile() )
    {
        RAISE_ERROR( ePdfError_NoPdfFile );
    }

    ReadDocumentStructure();
    try {
        ReadObjects();
    } catch( PdfError & e ) {
        e.AddToCallstack( __FILE__, __LINE__, "Unable to load objects from file." );
        throw e;
    }

    // Now sort the list of objects
    std::sort( m_vecObjects->begin(), m_vecObjects->end(), ObjectLittle );
}

void PdfParser::Clear()
{
    m_mapStreamCache.clear();

    if( m_pOffsets )
        free( m_pOffsets );

    m_file = PdfRefCountedFile();

    delete m_pTrailer;
    delete m_pLinearization;

    this->Init();
}

void PdfParser::ReadDocumentStructure()
{
    HasLinearizationDict();
    
    // position at the end of the file to search the xref table.
    fseek( m_file.Handle(), 0, SEEK_END );
    m_nFileSize = ftell( m_file.Handle() );

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
        RAISE_ERROR( ePdfError_InvalidTrailerSize );
    }

    m_nNumObjects = m_pTrailer->GetDictionary().GetKeyAsLong( PdfName::KeySize );

#ifdef _DEBUG
    PdfError::DebugMessage("Allocating for %i objects\n", m_nNumObjects );
#endif // _DEBUG

    m_pOffsets = (TXRefEntry*)malloc( sizeof( TXRefEntry ) * (m_nNumObjects+1)  );
    memset( m_pOffsets, 0, sizeof( TXRefEntry ) * m_nNumObjects );

#ifdef _DEBUG
    PdfError::DebugMessage("Linearized Offset: %i Pointer: %p\n", m_nXRefLinearizedOffset, m_pLinearization );
#endif // _DEBUG

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

    if( fread( m_buffer.Buffer(), PDF_MAGIC_LEN, sizeof(char), m_file.Handle() ) != 1 )
        return false;

     if( strncmp( m_buffer.Buffer(), szPdfMagicStart, strlen( szPdfMagicStart ) ) != 0 )
        return false;
        
    // try to determine the excact PDF version of the file
    for( i=0;i<=MAX_PDF_VERSION_STRING_INDEX;i++ )
        if( strncmp( m_buffer.Buffer(), s_szPdfVersions[i], PDF_MAGIC_LEN ) == 0 )
        {
            m_ePdfVersion = (EPdfVersion)i;
            break;
        }

    return true;
}

void PdfParser::HasLinearizationDict()
{
    int       i          = 0;
    char*     pszObj     = NULL;
    long      lXRef      = -1;
    const int XREF_LEN = 4; // strlen( "xref" );
    char*    pszStart   = NULL;

    fseek( m_file.Handle(), 0, SEEK_SET );
    // look for a linearization dictionary in the first 1024 bytes
    if( fread( m_buffer.Buffer(), m_buffer.Size(), sizeof(char), m_file.Handle() ) != 1 )
        return; // Ignore Error Code: ERROR_PDF_NO_TRAILER;

    pszObj = strstr( m_buffer.Buffer(), "obj" );
    if( !pszObj )
        // strange that there is no obj in the first 1024 bytes,
        // but ignore it
        return;

    --pszObj; // *pszObj == 'o', so the while would fail without decrement
    while( *pszObj && (IsWhitespace( *pszObj ) || (*pszObj >= '0' && *pszObj <= '9')) )
        --pszObj;

    fseek( m_file.Handle(), pszObj - m_buffer.Buffer() + 3, SEEK_SET );
    m_pLinearization = new PdfParserObject( m_vecObjects, m_file, m_buffer );

    try {
        static_cast<PdfParserObject*>(m_pLinearization)->ParseFile();

        if( !m_pLinearization->GetDictionary().HasKey( "Linearized" ) )
        {
            delete m_pLinearization;
            m_pLinearization = NULL;
            return;
        }
        
    } catch( PdfError & e ) {
		std::string	errStr( e.what() );	// avoid compiler warning and in case we need it...
        delete m_pLinearization;
        m_pLinearization = NULL;
        return;
    }
    
    lXRef = m_pLinearization->GetDictionary().GetKeyAsLong( "T", lXRef );
    if( lXRef == -1 )
    {
        RAISE_ERROR( ePdfError_InvalidLinearization );
    }

    // avoid moving to a negative file position here
    if( fseek( m_file.Handle(), (lXRef-PDF_XREF_BUF > 0 ? lXRef-PDF_XREF_BUF : PDF_XREF_BUF), SEEK_SET ) != 0 )
    {
        RAISE_ERROR( ePdfError_InvalidLinearization );
    }

    m_nXRefLinearizedOffset = ftell( m_file.Handle() );

    if( fread( m_buffer.Buffer(), PDF_XREF_BUF, sizeof(char), m_file.Handle() ) != 1 )
    {
        RAISE_ERROR( ePdfError_InvalidLinearization );
    }

    m_buffer.Buffer()[PDF_XREF_BUF] = '\0';

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    for( i = PDF_XREF_BUF - XREF_LEN; i >= 0; i-- )
        if( strncmp( m_buffer.Buffer()+i, "xref", XREF_LEN ) == 0 )
        {
            pszStart = m_buffer.Buffer()+i;
            break;
        }

    m_nXRefLinearizedOffset += i;
    
    if( !pszStart )
    {
        if( m_ePdfVersion < ePdfVersion_1_5 )
        {
            PdfError::LogMessage( eLogSeverity_Warning, 
                                  "Linearization dictionaries are only supported with PDF version 1.5. This is 1.%i. Trying to continue.\n", 
                                  (int)m_ePdfVersion );
            // RAISE_ERROR( ePdfError_InvalidLinearization );
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
        RAISE_ERROR( ePdfError_InvalidHandle );
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
    GetNextStringFromFile();
    // ReadXRefcontents has read the first 't' from "trailer" so just check for "railer"
    if( strcmp( m_buffer.Buffer(), "railer" ) == 0 )
    {
        PdfParserObject trailer( m_vecObjects, m_file, m_buffer );
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
            RAISE_ERROR( ePdfError_NoTrailer );
        }
    }
}

void PdfParser::ReadTrailer()
{
    char*      pszStart       = NULL;
    int        nSearchCnt     = 0;
    const int  nMaxSearch   = 5;
    int        nEof, i;
    const int  TRAILER_LEN  = 7; // strlen( "trailer" )
    long       lXRefBuf;

    fseek( m_file.Handle(), 0, SEEK_END );
    nEof = ftell( m_file.Handle() );
    lXRefBuf = PDF_MIN( nEof, PDF_XREF_BUF );

    do {
        // TODO: this can cause a problem if trailer is splitted beween to searches
        // e.g. the first search block starts with "ailer" whereas the second one
        // ends with "tr". trailer cannot be found in this case

        if( fseek( m_file.Handle(), -lXRefBuf, SEEK_CUR ) != 0 )
        {
            RAISE_ERROR( ePdfError_NoTrailer );
        }

        if( fread( m_buffer.Buffer(), lXRefBuf, sizeof(char), m_file.Handle() ) != 1 )
        {
            RAISE_ERROR( ePdfError_NoTrailer );
        }

        if( fseek( m_file.Handle(), -lXRefBuf, SEEK_CUR ) != 0 )
        {
            RAISE_ERROR( ePdfError_NoTrailer );
        }
        
        m_buffer.Buffer()[lXRefBuf] = '\0';
        // search backwards in the buffer in case the buffer contains null bytes
        // because it is right after a stream (can't use strstr for this reason)
        for( i = lXRefBuf - TRAILER_LEN; i >= 0; i-- )
            if( strncmp( m_buffer.Buffer()+i, "trailer", TRAILER_LEN ) == 0 )
            {
                pszStart = m_buffer.Buffer()+i;
                break;
            }

        ++nSearchCnt;
    } while( !pszStart && nSearchCnt < nMaxSearch );

    if( m_ePdfVersion < ePdfVersion_1_5 )
    {
        if( !pszStart )
        {
            RAISE_ERROR( ePdfError_NoTrailer );
        }
    }
    else
    {
        // Since PDF 1.5 trailer information can also be found
        // in the crossreference stream object
        // and a trailer dictionary is not required
        if( !pszStart )
            return;
    }

    if( fseek( m_file.Handle(), (pszStart - m_buffer.Buffer() + TRAILER_LEN), SEEK_CUR ) != 0 )
    {
        RAISE_ERROR( ePdfError_NoTrailer );
    }

    m_pTrailer = new PdfParserObject( m_vecObjects, m_file, m_buffer );
    try {
        static_cast<PdfParserObject*>(m_pTrailer)->ParseFile( true );
    } catch( PdfError & e ) {
        e.AddToCallstack( __FILE__, __LINE__, "The trailer was found in the file, but contains errors." );
        throw e;
    }
#ifdef _DEBUG
    PdfError::DebugMessage("Size=%li\n", m_pTrailer->GetDictionary().GetKeyAsLong( PdfName::KeySize, 0 ) );
#endif // _DEBUG
}

void PdfParser::ReadXRef( long* pXRefOffset )
{
    const int   STARTXREF_LEN = 9; // strlen( "startxref" )
    char*       pszStart = NULL;
    char*       pszEnd = NULL;
    int         i;
    long        lXRefBuf, lFileSize;

    lFileSize = ftell( m_file.Handle() );
    lXRefBuf = PDF_MIN( lFileSize, PDF_XREF_BUF );

    if( fseek( m_file.Handle(), -lXRefBuf, SEEK_CUR ) != 0 )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    }

    if( fread( m_buffer.Buffer(), lXRefBuf, sizeof(char), m_file.Handle() ) != 1 )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    }

    m_buffer.Buffer()[lXRefBuf] = '\0';

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    for( i = lXRefBuf - STARTXREF_LEN; i >= 0; i-- )
        if( strncmp( m_buffer.Buffer()+i, "startxref", STARTXREF_LEN ) == 0 )
        {
            pszStart = m_buffer.Buffer()+i;
            break;
        }
    
    if( !pszStart )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    }

    pszStart += STARTXREF_LEN;

    *pXRefOffset = strtol( ++pszStart, &pszEnd, 10 );

    if( pszStart == pszEnd )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    }

    while( IsWhitespace( *pszEnd ) )
        ++pszEnd;

    // just to make sure, compare pszEnd to %%EOF
    if( strncmp( pszEnd, "%%EOF", 5 ) != 0 )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    }
}

void PdfParser::ReadXRefContents( long lOffset, bool bPositionAtEnd )
{
    long        nFirstObject = 0;
    long        nNumObjects  = 0;

    if( fseek( m_file.Handle(), lOffset, SEEK_SET ) != 0 )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    }
    
    GetNextStringFromFile( );
    if( strncmp( m_buffer.Buffer(), "xref", 4 ) != 0 )
    {
        if( m_ePdfVersion < ePdfVersion_1_5 )
        {
            RAISE_ERROR( ePdfError_NoXRef );
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
            nFirstObject = GetNextNumberFromFile();
            nNumObjects  = GetNextNumberFromFile();

#ifdef _DEBUG
            PdfError::DebugMessage("Reading numbers: %i %i\n", nFirstObject, nNumObjects );
#endif // _DEBUG

            if( bPositionAtEnd )
            {
                fseek( m_file.Handle(), nNumObjects* PDF_XREF_ENTRY_SIZE, SEEK_CUR );
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
    };

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

#ifdef _DEBUG
    PdfError::DebugMessage("Reading XRef Section: %i with %i Objects\n", nFirstObject, nNumObjects );
#endif // _DEBUG 

    while( count < nNumObjects && fread( m_buffer.Buffer(), PDF_XREF_ENTRY_SIZE, sizeof(char), m_file.Handle() ) == 1 )
    {
        m_buffer.Buffer()[PDF_XREF_ENTRY_SIZE] = '\0';

        int objID = nFirstObject + count;
        if( objID >= m_nNumObjects )
        {
            RAISE_ERROR( ePdfError_InvalidXRef );
        }

        if( !m_pOffsets[objID].bParsed )
        {
            m_pOffsets[objID].bParsed = true;
            sscanf( m_buffer.Buffer(), "%10ld %5ld %c \n", 
                    &(m_pOffsets[objID].lOffset), 
                    &(m_pOffsets[objID].lGeneration), &(m_pOffsets[objID].cUsed) );
        }

        ++count;
    }

    if( count != nNumObjects )
    {
        RAISE_ERROR( ePdfError_NoXRef );
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
    int         count     = 0;
    long        nFirstObj = 0;
    char*       pBuffer;
    char*       pStart;
    long        lBufferLen;
    long        lSize     = 0;
    PdfVariant  vWArray;
    PdfObject*  pObj;

    long        nW[W_ARRAY_SIZE] = { 0, 0, 0 };
    int         i;

    if( fseek( m_file.Handle(), lOffset, SEEK_SET ) != 0 )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    }

    PdfParserObject xrefObject( m_vecObjects, m_file, m_buffer );
    xrefObject.ParseFile();


    if( !xrefObject.GetDictionary().HasKey( PdfName::KeyType ) )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    } 

    pObj = xrefObject.GetDictionary().GetKey( PdfName::KeyType );
    if( !pObj->IsName() || ( pObj->GetName().Name() != "XRef" ) )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    } 

    if( !m_pTrailer )    
        m_pTrailer = new PdfParserObject( m_vecObjects, m_file, m_buffer );

    MergeTrailer( &xrefObject );

    if( bReadOnlyTrailer )
        return;

    if( !xrefObject.GetDictionary().HasKey( PdfName::KeySize ) || !xrefObject.GetDictionary().HasKey( "W" ) )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    }

    lSize   = xrefObject.GetDictionary().GetKeyAsLong( PdfName::KeySize, 0 );
    vWArray = *(xrefObject.GetDictionary().GetKey( "W" ));

    // The pdf reference states that W is always an array with 3 entries
    // all of them have to be integeres
    if( !vWArray.IsArray() || vWArray.GetArray().size() != 3 )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    }

    for( i=0;i<W_ARRAY_SIZE;i++ )
    {
        if( !vWArray.GetArray()[i].IsNumber() )
        {
            RAISE_ERROR( ePdfError_NoXRef );
        }

        nW[i] = vWArray.GetArray()[i].GetNumber();
    }

    // get the first object number in this crossref stream.
    // it is not required to have an index key though.
    if( xrefObject.GetDictionary().HasKey( "Index" ) )
    {
        // reuse vWArray!!
        vWArray = *(xrefObject.GetDictionary().GetKey( "Index" ));
        if( !vWArray.IsArray() )
        {
            RAISE_ERROR( ePdfError_NoXRef );
        }

        if( !vWArray.GetArray()[0].IsNumber() )
        {
            RAISE_ERROR( ePdfError_NoXRef );
        }

        nFirstObj = vWArray.GetArray()[0].GetNumber();

        // TODO: fix this
        if( vWArray.GetArray().size() != 2 )
        {
            RAISE_ERROR( ePdfError_NoXRef );
        }
    }

    if( !xrefObject.HasStreamToParse() )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    }

    xrefObject.ParseStream();
    xrefObject.Stream()->GetFilteredCopy( &pBuffer, &lBufferLen );

    pStart = pBuffer;
    while( pBuffer - pStart < lBufferLen )
    {
        ReadXRefStreamEntry( pBuffer, lBufferLen, nW, nFirstObj + count++ );

        pBuffer += (nW[0] + nW[1] + nW[2]);
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

            RAISE_ERROR( ePdfError_InvalidXRefStream );
        }
        
        nData[i] = 0;
        for( z=W_MAX_BYTES-lW[i];z<W_MAX_BYTES;z++ )
        {
            nData[i] = (nData[i] << 8) + (unsigned char)*pBuffer;
            ++pBuffer;
        }
    }

    m_pOffsets[nObjNo].bParsed = true;

    switch( nData[0] ) // nData[0] contains the type information of this entry
    {
        case 0:
            // a free object
            m_pOffsets[nObjNo].lOffset     = 0;
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
            RAISE_ERROR( ePdfError_InvalidXRefType );
        }
    }
}

void PdfParser::ReadObjects()
{
    int              i          = 0;
    PdfParserObject* pObject    = NULL;
    TCIVecObjects    itObjects;

    m_vecObjects->reserve( m_nNumObjects );

    for( ; i < m_nNumObjects; i++ )
    {
        if( m_pOffsets[i].bParsed && m_pOffsets[i].cUsed == 'n' )
        {
            pObject = new PdfParserObject( m_vecObjects, m_file, m_buffer, m_pOffsets[i].lOffset );
            pObject->SetLoadOnDemand( m_bLoadOnDemand );
            try {
                pObject->ParseFile();
                
                // final pdf should not contain a linerization dictionary as it contents are invalid 
                // as we change some objects and the final xref table
                if( m_pLinearization && pObject->ObjectNumber() == m_pLinearization->ObjectNumber() )
                {
                    pObject->SetEmptyEntry( true );                    
                }
            } catch( PdfError & e ) {
                delete pObject;
                e.AddToCallstack( __FILE__, __LINE__ );
                throw e;
            }
            
            m_vecObjects->push_back( pObject );
        }
        else
        {
            /*
            // TODO: should not be necessary anymore, remove to save some ram
            
            // add an empty object to the vector
            // so that the XRef table we write out 
            // stays compatible to the object number
            // in the following objects.
            pObj = new PdfObject( i, 0, NULL );
            pObj->SetEmptyEntry( true );
            m_vecObjects.push_back( pObj );
            */
        }
    }

    // all normal objects including object streams are available now,
    // we can parse the object streams now savely
    for( i = 0; i < m_nNumObjects; i++ )
    {
        if( m_pOffsets[i].bParsed && m_pOffsets[i].cUsed == 's' ) // we have an object stream
            ReadObjectFromStream( m_pOffsets[i].lGeneration, m_pOffsets[i].lOffset );
    }

    m_mapStreamCache.clear();

    if( !m_bLoadOnDemand )
    {
        itObjects = m_vecObjects->begin();
        while( itObjects != m_vecObjects->end() )
        {
            pObject = dynamic_cast<PdfParserObject*>(*itObjects);
            // only parse streams for objects that have not yet parsed
            // their streams 
            if( pObject && pObject->HasStreamToParse() && !pObject->HasStream() )
            {
                pObject->ParseStream();
            }
            
            ++itObjects;
        }
        
    }
}

void PdfParser::ReadObjectFromStream( int nObjNo, int nIndex )
{
    PdfParserObject* pStream;
    PdfParserObject* pObj;
    PdfObject*       pTmp;
    char*            pBuffer;
    char*            pNumbers;
    long             lBufferLen;
    long             lFirst;
    long             lNum;
    long             lObj;
    long             lOff;
    int              i       = 0;

    // generation number of object streams is always 0
    pStream = dynamic_cast<PdfParserObject*>(m_vecObjects->GetObject( PdfReference( nObjNo, 0 ) ) );
    if( !pStream )
    {
        RAISE_ERROR( ePdfError_NoObject );
    }

    lNum   = pStream->GetDictionary().GetKeyAsLong( "N", 0 );
    lFirst = pStream->GetDictionary().GetKeyAsLong( "First", 0 );

    if( m_mapStreamCache.find( nObjNo ) == m_mapStreamCache.end() )
    {
        // the objects stream might not yet be parsed, make sure
        // that this is done now!
        if( pStream->HasStreamToParse() && !pStream->HasStream() )
        {
            pStream->ParseStream();
        }
        
        pStream->Stream()->GetFilteredCopy( &pBuffer, &lBufferLen );
        
        // the object stream is not needed anymore in the final PDF
        pStream->SetEmptyEntry( true );

        pNumbers = pBuffer;
        while( i < lNum )
        {
            lObj = strtol( pNumbers, &pNumbers, 10 );
            lOff = strtol( pNumbers, &pNumbers, 10 );
        
            pObj = new PdfParserObject( m_buffer );
            pObj->ParseDictionaryKeys( (char*)(pBuffer+lFirst+lOff), lBufferLen-lFirst-lOff, NULL );

            pObj->SetObjectNumber( lObj );

            // TODO: remove cache
            m_mapStreamCache[nObjNo][nIndex] = pObj;
            m_vecObjects->push_back( pObj );
            m_mapStreamCache[nObjNo][nIndex] = NULL;
            ++i;
        }
    }
    else
    {
        pTmp = m_mapStreamCache[nObjNo][nIndex];
        if( pTmp )
        {
            m_vecObjects->push_back( pTmp );
            // set to NULL afterwards to allow deletion of spare objects
            // in the desctructor
            m_mapStreamCache[nObjNo][nIndex] = NULL;
        }
    }
}

const char* PdfParser::GetPdfVersionString() const
{
    return s_szPdfVersions[(int)m_ePdfVersion];
}

};


