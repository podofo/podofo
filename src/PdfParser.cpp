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

PdfParser::PdfParser()
    : PdfParserBase()
{
    m_bLoadOnDemand   = false;

    m_hFile           = NULL;
    m_pTrailer        = NULL;
    m_pLinearization  = NULL;
    m_ppOffsets       = NULL;

    m_ePdfVersion     = ePdfVersion_Unknown;

    m_nXRefOffset     = 0;
    m_nFirstObject    = 0;
    m_nNumObjects     = 0;
    m_nXRefLinearizedOffset = 0;
}

PdfParser::~PdfParser()
{
    Clear();
}

PdfError PdfParser::Init( const char* pszFilename, bool bLoadOnDemand )
{
    PdfError eCode;

    if( !pszFilename || !pszFilename[0] )
    {
        RAISE_ERROR( ePdfError_FileNotFound );
    }

    // make sure everything is clean
    Clear();

    m_bLoadOnDemand = bLoadOnDemand;

    m_hFile           = fopen( pszFilename, "rb" );
    if( !m_hFile )
    {
        RAISE_ERROR( ePdfError_FileNotFound );
    }

    if( !IsPdfFile() )
    {
        RAISE_ERROR( ePdfError_NoPdfFile );
    }

    SAFE_OP( ReadDocumentStructure() );
    SAFE_OP_ADV( ReadObjects(),      "Unable to load objects from file." );

    // Now sort the list of objects
    std::sort( m_vecObjects.begin(), m_vecObjects.end(), ObjectLittle );

    return eCode;
}

void PdfParser::Clear()
{
    int          i         = 0;
    TIVecObjects itObjects = m_vecObjects.begin();
    TIMapObjects itMapObjects;
    TIMapObjectStreamCache itCache = m_mapStreamCache.begin();

    while( itObjects != m_vecObjects.end() )
    {
        delete (*itObjects);
        ++itObjects;
    }

    while( itCache != m_mapStreamCache.end() )
    {
        itMapObjects = (*itCache).second.begin();

        while( itMapObjects != (*itCache).second.end() )
        {
            if( (*itMapObjects).second )
                delete (*itMapObjects).second;

            ++itMapObjects;
        }

        ++itCache;
    }

    m_vecObjects.clear();
    m_mapStreamCache.clear();

    if( m_ppOffsets )
    {
        for( ; i < m_nNumObjects; i++ )
            free( m_ppOffsets[i] );
        free( m_ppOffsets );
    }

    if( m_hFile )
        fclose( m_hFile );

    delete m_pTrailer;
    delete m_pLinearization;

    m_pTrailer        = NULL;
    m_pLinearization  = NULL;
    m_ppOffsets       = NULL;

    m_ePdfVersion     = ePdfVersion_Unknown;

    m_nXRefOffset     = 0;
    m_nFirstObject    = 0;
    m_nNumObjects     = 0;
    m_nXRefLinearizedOffset = 0;
}

PdfError PdfParser::ReadDocumentStructure()
{
    PdfError eCode;

    SAFE_OP( IsLinearized() );
    
    // position at the end of the file to search the xref table.
    fseek( m_hFile, 0, SEEK_END );

    SAFE_OP_ADV( ReadXRef( &m_nXRefOffset ), "Unable to find startxref entry in file." );
    SAFE_OP_ADV( ReadTrailer(),              "Unable to find trailer in file." );

    if( m_pLinearization )
    {
        SAFE_OP_ADV( ReadXRefContents( m_nXRefOffset, true ), "Unable to skip xref dictionary." );
        // another trailer directory is to follow right after this XRef section
        SAFE_OP( GetNextStringFromFile() );

        // ReadXRefcontents has read the first 't' from "trailer" so just check for "railer"
        if( strcmp( m_szBuffer, "railer" ) == 0 )
        {
            PdfParserObject pTrailer( this, m_hFile, this->GetBuffer(), this->GetBufferSize() );
            SAFE_OP_ADV( pTrailer.ParseFile( true ), "The linearized trailer was found in the file, but contains errors." );

            // now merge the information of this trailer with the main documents trailer
            SAFE_OP( MergeTrailer( &pTrailer ) );
        }
    }

    m_nNumObjects = m_pTrailer->GetKeyValueLong( PdfName::KeySize, -1 );
    if( m_nNumObjects == -1 )
    {
        PdfError::LogMessage( eLogSeverity_Error, "No /Size key was specified in the trailer directory." );
        RAISE_ERROR( ePdfError_InvalidTrailerSize );
    }

#ifdef _DEBUG
    printf("Allocating for %i objects\n", m_nNumObjects );
#endif // _DEBUG

    m_ppOffsets = (TXRefEntry**)malloc( sizeof( TXRefEntry* ) * (m_nNumObjects+1)  );
    memset( m_ppOffsets, 0, sizeof( TXRefEntry* ) * m_nNumObjects );

#ifdef _DEBUG
        printf("Linearized Offset: %i Pointer: %p\n", m_nXRefLinearizedOffset, m_pLinearization );
#endif // _DEBUG

    if( m_pLinearization )
    {
        SAFE_OP_ADV( ReadXRefContents( m_nXRefLinearizedOffset ), "Unable to read linearized XRef section." );
    }

    SAFE_OP_ADV( ReadXRefContents( m_nXRefOffset ), "Unable to load xref entries." );

    if( m_pTrailer->HasKey( "Prev" ) )
    {
        SAFE_OP_ADV( ReadXRefContents( m_pTrailer->GetKeyValueLong( "Prev", 0 ) ), "Unable to load /Prev xref entries." );
    }

    return eCode;
}

bool PdfParser::IsPdfFile()
{
    const char* szPdfMagicStart = "%PDF-";
    int i;

    if( fread( m_szBuffer, PDF_MAGIC_LEN, sizeof(char), m_hFile ) != 1 )
        return false;

     if( strncmp( m_szBuffer, szPdfMagicStart, strlen( szPdfMagicStart ) ) != 0 )
        return false;
        
    // try to determine the excact PDF version of the file
    for( i=0;i<=MAX_PDF_VERSION_STRING_INDEX;i++ )
        if( strncmp( m_szBuffer, s_szPdfVersions[i], PDF_MAGIC_LEN ) == 0 )
        {
            m_ePdfVersion = (EPdfVersion)i;
            break;
        }

    return true;
}

PdfError PdfParser::IsLinearized()
{
    PdfError  eCode;
    int       i          = 0;
    char*     pszObj     = NULL;
    long      lXRef      = -1;
    const int XREF_LEN = 4; // strlen( "xref" );
    char*    pszStart   = NULL;

    fseek( m_hFile, 0, SEEK_SET );
    // look for a linearization dictionary in the first 1024 bytes
    if( fread( m_szBuffer, PDF_BUFFER, sizeof(char), m_hFile ) != 1 )
        return ePdfError_ErrOk; // Ignore Error Code: ERROR_PDF_NO_TRAILER;

    pszObj = strstr( m_szBuffer, "obj" );
    if( !pszObj )
        // strange that there is no obj in the first 1024 bytes,
        // but ignore it
        return ePdfError_ErrOk;

    --pszObj; // *pszObj == 'o', so the while would fail without decrement
    while( *pszObj && (IsWhitespace( *pszObj ) || (*pszObj >= '0' && *pszObj <= '9')) )
        --pszObj;

    fseek( m_hFile, pszObj - m_szBuffer + 3, SEEK_SET );
    m_pLinearization = new PdfParserObject( this, m_hFile, this->GetBuffer(), this->GetBufferSize() );
    eCode = static_cast<PdfParserObject*>(m_pLinearization)->ParseFile();

    if( eCode.IsError() || !m_pLinearization->HasKey( "Linearized" ) )
    {
        delete m_pLinearization;
        m_pLinearization = NULL;
        return ePdfError_ErrOk;
    }
    
    lXRef = m_pLinearization->GetKeyValueLong( "T", lXRef );
    if( lXRef == -1 )
    {
        RAISE_ERROR( ePdfError_InvalidLinearization );
    }

    // avoid moving to a negative file position here
    if( fseek( m_hFile, (lXRef-PDF_XREF_BUF > 0 ? lXRef-PDF_XREF_BUF : PDF_XREF_BUF), SEEK_SET ) != 0 )
    {
        RAISE_ERROR( ePdfError_InvalidLinearization );
    }

    m_nXRefLinearizedOffset = ftell( m_hFile );

    if( fread( m_szBuffer, PDF_XREF_BUF, sizeof(char), m_hFile ) != 1 )
    {
        RAISE_ERROR( ePdfError_InvalidLinearization );
    }

    m_szBuffer[PDF_XREF_BUF] = '\0';

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    for( i = PDF_XREF_BUF - XREF_LEN; i >= 0; i-- )
        if( strncmp( m_szBuffer+i, "xref", XREF_LEN ) == 0 )
        {
            pszStart = m_szBuffer+i;
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

    return eCode;
}

PdfError PdfParser::MergeTrailer( const PdfObject* pTrailer )
{
    PdfError    eCode;
    PdfVariant  cVar;

    if( !pTrailer || !m_pTrailer )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( pTrailer->HasKey( PdfName::KeySize ) )
    {
        pTrailer->GetKeyValueVariant( PdfName::KeySize, cVar );
        m_pTrailer->AddKey( PdfName::KeySize, cVar );
    }

    if( pTrailer->HasKey( "Root" ) )
    {
        pTrailer->GetKeyValueVariant( "Root", cVar );
        m_pTrailer->AddKey( "Root", cVar );
    }

    if( pTrailer->HasKey( "Encrypt" ) )
    {
        pTrailer->GetKeyValueVariant( "Encrypt", cVar );
        m_pTrailer->AddKey( "Encrypt", cVar );
    }

    if( pTrailer->HasKey( "Info" ) )
    {
        pTrailer->GetKeyValueVariant( "Info", cVar );
        m_pTrailer->AddKey( "Info", cVar );
    }

    if( pTrailer->HasKey( "ID" ) )
    {            
        pTrailer->GetKeyValueVariant( "ID", cVar );
        m_pTrailer->AddKey( "ID", cVar );
    }

    return eCode;
}

PdfError PdfParser::ReadTrailer()
{
    PdfError   eCode;
    char*      pszStart       = NULL;
    int        nSearchCnt     = 0;
    const int  nMaxSearch   = 5;
    int        nEof, i;
    const int  TRAILER_LEN  = 7; // strlen( "trailer" )
    long       lXRefBuf;

    fseek( m_hFile, 0, SEEK_END );
    nEof = ftell( m_hFile );
    lXRefBuf = PDF_MIN( nEof, PDF_XREF_BUF );

    do {
        // TODO: this can cause a problem if trailer is splitted beween to searches
        // e.g. the first search block starts with "ailer" whereas the second one
        // ends with "tr". trailer cannot be found in this case

        if( fseek( m_hFile, -lXRefBuf, SEEK_CUR ) != 0 )
        {
            RAISE_ERROR( ePdfError_NoTrailer );
        }

        if( fread( m_szBuffer, lXRefBuf, sizeof(char), m_hFile ) != 1 )
        {
            RAISE_ERROR( ePdfError_NoTrailer );
        }

        if( fseek( m_hFile, -lXRefBuf, SEEK_CUR ) != 0 )
        {
            RAISE_ERROR( ePdfError_NoTrailer );
        }
        
        m_szBuffer[lXRefBuf] = '\0';
        // search backwards in the buffer in case the buffer contains null bytes
        // because it is right after a stream (can't use strstr for this reason)
        for( i = lXRefBuf - TRAILER_LEN; i >= 0; i-- )
            if( strncmp( m_szBuffer+i, "trailer", TRAILER_LEN ) == 0 )
            {
                pszStart = m_szBuffer+i;
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
            return ePdfError_ErrOk;
    }

    if( fseek( m_hFile, (pszStart - m_szBuffer + TRAILER_LEN), SEEK_CUR ) != 0 )
    {
        RAISE_ERROR( ePdfError_NoTrailer );
    }

    m_pTrailer = new PdfParserObject( this, m_hFile, this->GetBuffer(), this->GetBufferSize() );
    SAFE_OP_ADV( static_cast<PdfParserObject*>(m_pTrailer)->ParseFile( true ), "The trailer was found in the file, but contains errors." );
#ifdef _DEBUG
    printf("Size=%li\n", m_pTrailer->GetKeyValueLong( PdfName::KeySize, 0 ) );
#endif // _DEBUG

    return eCode;
}

PdfError PdfParser::ReadXRef( long* pXRefOffset )
{
    PdfError eCode;

    const int   STARTXREF_LEN = 9; // strlen( "startxref" )
    char*       pszStart = NULL;
    char*       pszEnd = NULL;
    int         i;
    long        lXRefBuf, lFileSize;

    lFileSize = ftell( m_hFile );
    lXRefBuf = PDF_MIN( lFileSize, PDF_XREF_BUF );

    if( fseek( m_hFile, -lXRefBuf, SEEK_CUR ) != 0 )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    }

    if( fread( m_szBuffer, lXRefBuf, sizeof(char), m_hFile ) != 1 )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    }

    m_szBuffer[lXRefBuf] = '\0';

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    for( i = lXRefBuf - STARTXREF_LEN; i >= 0; i-- )
        if( strncmp( m_szBuffer+i, "startxref", STARTXREF_LEN ) == 0 )
        {
            pszStart = m_szBuffer+i;
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

    return eCode;
}

PdfError PdfParser::ReadXRefContents( long lOffset, bool bPositionAtEnd )
{
    PdfError    eCode;
    long        nFirstObject = 0;
    long        nNumObjects  = 0;

    if( fseek( m_hFile, lOffset, SEEK_SET ) != 0 )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    }
    
    SAFE_OP( GetNextStringFromFile( ) );

    if( strncmp( m_szBuffer, "xref", 4 ) != 0 )
    {
        if( m_ePdfVersion < ePdfVersion_1_5 )
        {
            RAISE_ERROR( ePdfError_NoXRef );
        }
        else
        {
            return ReadXRefStreamContents( lOffset, bPositionAtEnd );
        }
    }

    // read all xref subsections
    do {
        eCode = GetNextNumberFromFile( &nFirstObject );
        if( eCode == ePdfError_NoNumber )
            return ePdfError_ErrOk;

        eCode = GetNextNumberFromFile( &nNumObjects );
        if( eCode == ePdfError_NoNumber )
            return ePdfError_ErrOk;

#ifdef _DEBUG
        printf("Reading numbers: %i %i\n", nFirstObject, nNumObjects );
#endif // _DEBUG

        if( bPositionAtEnd && !eCode.IsError() )
        {
            fseek( m_hFile, nNumObjects* PDF_XREF_ENTRY_SIZE, SEEK_CUR );
        }
        else
            eCode = ReadXRefSubsection( nFirstObject, nNumObjects );
    } while( !eCode.IsError() );
    
    return eCode;
}

PdfError PdfParser::ReadXRefSubsection( long & nFirstObject, long & nNumObjects )
{
    PdfError    eCode;
    int         count        = 0;

#ifdef _DEBUG
    printf("Reading XRef Section: %i with %i Objects\n", nFirstObject, nNumObjects );
#endif // _DEBUG 

    while( count < nNumObjects && fread( m_szBuffer, PDF_XREF_ENTRY_SIZE, sizeof(char), m_hFile ) == 1 )
    {
        m_szBuffer[PDF_XREF_ENTRY_SIZE] = '\0';

        if( nFirstObject + count >= m_nNumObjects )
        {
            RAISE_ERROR( ePdfError_InvalidXRef );
        }

        if( !m_ppOffsets[nFirstObject + count] )
        {
            m_ppOffsets[nFirstObject + count] = (TXRefEntry*)malloc( sizeof( TXRefEntry ) );
            sscanf( m_szBuffer, "%10ld %5ld %c \n", 
                    &(m_ppOffsets[nFirstObject + count]->lOffset), 
                    &(m_ppOffsets[nFirstObject + count]->lGeneration), &(m_ppOffsets[nFirstObject + count]->cUsed) );
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

        printf("Searching at: %i\n", m_ppOffsets[nFirstObject]->lOffset );
        SAFE_OP( fseek( m_hFile, m_ppOffsets[nFirstObject]->lOffset, SEEK_SET ) );
        eCode = ReadXRef( &lOffset );
        if( eCode )
            eCode == ErrOk;

        printf("Found XRef at: %i\n", lOffset );
        SAFE_OP( ReadXRefContents( lOffset ) );
    }
    */

    return eCode;
}

PdfError PdfParser::ReadXRefStreamContents( long lOffset, bool bReadOnlyTrailer )
{
    PdfError    eCode;
    int         count     = 0;
    long        nFirstObj = 0;
    char*       pBuffer;
    char*       pStart;
    long        lBufferLen;
    long        lSize     = 0;
    PdfVariant  vWArray;
    PdfVariant  xrefType;

    long        nW[W_ARRAY_SIZE] = { 0, 0, 0 };
    int         i;

    if( fseek( m_hFile, lOffset, SEEK_SET ) != 0 )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    }

    PdfParserObject xrefObject( this, m_hFile, m_szBuffer, this->GetBufferSize() );
    SAFE_OP( xrefObject.ParseFile() );


    SAFE_OP( xrefObject.GetKeyValueVariant( PdfName::KeyType, xrefType ) );
    if( xrefType.GetDataType() != ePdfDataType_Name || strcmp( xrefType.GetName().Name(), "XRef" ) != 0 )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    } 

    if( !m_pTrailer )    
        m_pTrailer = new PdfParserObject( this, m_hFile, this->GetBuffer(), this->GetBufferSize() );

    SAFE_OP( MergeTrailer( &xrefObject ) );

    if( bReadOnlyTrailer )
        return ePdfError_ErrOk;

    if( !xrefObject.HasKey( PdfName::KeySize ) || !xrefObject.HasKey( "W" ) )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    }

    lSize  = xrefObject.GetKeyValueLong    ( PdfName::KeySize, 0    );
    SAFE_OP( xrefObject.GetKeyValueVariant ( "W", vWArray ) );

    // The pdf reference states that W is always an array with 3 entries
    // all of them have to be integeres
    if( vWArray.GetDataType() != ePdfDataType_Array || vWArray.GetArray().size() != 3 )
    {
        RAISE_ERROR( ePdfError_NoXRef );
    }

    for( i=0;i<W_ARRAY_SIZE;i++ )
    {
        if( vWArray.GetArray()[i].GetDataType() != ePdfDataType_Number )
        {
            RAISE_ERROR( ePdfError_NoXRef );
        }

        vWArray.GetArray()[i].GetNumber( &(nW[i]) );
    }

    // get the first object number in this crossref stream.
    // it is not required to have an index key though.
    if( xrefObject.HasKey( "Index" ) )
    {
        // reuse vWArray!!
        SAFE_OP( xrefObject.GetKeyValueVariant ( "Index", vWArray ) );
        if( vWArray.GetDataType() != ePdfDataType_Array )
        {
            RAISE_ERROR( ePdfError_NoXRef );
        }

        if( vWArray.GetArray()[0].GetDataType() != ePdfDataType_Number )
        {
            RAISE_ERROR( ePdfError_NoXRef );
        }

        vWArray.GetArray()[0].GetNumber( &nFirstObj );
        std::string str;
        vWArray.ToString( str );
        
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

    SAFE_OP( xrefObject.ParseStream() );
    SAFE_OP( xrefObject.Stream()->GetFilteredCopy( &pBuffer, &lBufferLen ) );

    pStart = pBuffer;
    while( pBuffer - pStart < lBufferLen )
    {
        SAFE_OP( ReadXRefStreamEntry( pBuffer, lBufferLen, nW, nFirstObj + count++ ) );

        pBuffer += (nW[0] + nW[1] + nW[2]);
    }
    free( pStart );

    if( xrefObject.HasKey("Prev") )
    {
        lOffset = xrefObject.GetKeyValueLong( "Prev", 0 );
        SAFE_OP( ReadXRefStreamContents( lOffset, bReadOnlyTrailer ) );
    }

    return eCode;
}

PdfError PdfParser::ReadXRefStreamEntry( char* pBuffer, long lLen, long lW[W_ARRAY_SIZE], int nObjNo )
{

/**
expected result:
00 000000 00
 02 00CDBF 00
 01 00CE45 00

*/
    PdfError         eCode;
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

    if( !m_ppOffsets[nObjNo] )
        m_ppOffsets[nObjNo] = (TXRefEntry*)malloc( sizeof( TXRefEntry ) );

    switch( nData[0] ) // nData[0] contains the type information of this entry
    {
        case 0:
            // a free object
            m_ppOffsets[nObjNo]->lOffset     = 0;
            m_ppOffsets[nObjNo]->lGeneration = nData[2];
            m_ppOffsets[nObjNo]->cUsed       = 'f';
            break;
        case 1:
            // normal uncompressed object
            m_ppOffsets[nObjNo]->lOffset     = nData[1];
            m_ppOffsets[nObjNo]->lGeneration = nData[2];
            m_ppOffsets[nObjNo]->cUsed       = 'n';
            break;
        case 2:
            // object that is part of an object stream
            m_ppOffsets[nObjNo]->lOffset     = nData[2]; // index in the object stream
            m_ppOffsets[nObjNo]->lGeneration = nData[1]; // object number of the stream
            m_ppOffsets[nObjNo]->cUsed       = 's';      // mark as stream
            break;
        default:
        {
            RAISE_ERROR( ePdfError_InvalidXRefType );
        }
    }

    return eCode;
}

PdfError PdfParser::ReadObjects()
{
    PdfError         eCode;
    int              i          = 0;
    PdfParserObject* pObject    = NULL;
    PdfObject*       pObj       = NULL;
    TCIVecObjects    itObjects;

    m_vecObjects.reserve( m_nNumObjects );

    for( ; i < m_nNumObjects; i++ )
    {
        if( m_ppOffsets[i] )
        {
            if( m_ppOffsets[i]->cUsed == 'n'  )
            {
                pObject = new PdfParserObject( this, m_hFile, m_szBuffer, this->GetBufferSize(), m_ppOffsets[i]->lOffset );
                pObject->SetLoadOnDemand( m_bLoadOnDemand );
                eCode = pObject->ParseFile();

                // final pdf should not contain a linerization dictionary as it contents are invalid 
                // as we change some objects and the final xref table
                if( m_pLinearization && pObject->ObjectNumber() == m_pLinearization->ObjectNumber() )
                {
                    pObject->SetEmptyEntry( true );                    
                }

                if( eCode.IsError() )
                {
                    delete pObject;
                    return eCode;
                }

                m_vecObjects.push_back( pObject );
            }
        }
        else
        {
            // TODO: should not be necessary anymore, remove to save some ram
            
            // add an empty object to the vector
            // so that the XRef table we write out 
            // stays compatible to the object number
            // in the following objects.
            pObj = new PdfObject( i, 0, NULL );
            pObj->SetEmptyEntry( true );
            m_vecObjects.push_back( pObj );
        }
    }

    // all normal objects including object streams are available now,
    // we can parse the object streams now savely
    for( i = 0; i < m_nNumObjects; i++ )
    {
        if( m_ppOffsets[i] && m_ppOffsets[i]->cUsed == 's' ) // we have an object stream
        {
            SAFE_OP( ReadObjectFromStream( m_ppOffsets[i]->lGeneration, m_ppOffsets[i]->lOffset ) );
        }
    }

    m_mapStreamCache.clear();

    // Now let all objects read their associate streams
	// NOTE: this is BAD IDEA to do upfront for PDFs with LOTS of images or other streams!
    if( !m_bLoadOnDemand )
    {
        itObjects = m_vecObjects.begin();
        while( itObjects != m_vecObjects.end() )
        {
            pObject = dynamic_cast<PdfParserObject*>(*itObjects);
            // only parse streams for objects that have not yet parsed
            // their streams 
            if( pObject && pObject->HasStreamToParse() && !pObject->HasStream() )
            {
                SAFE_OP_ADV( pObject->ParseStream(), "Unable to parse the objects stream." );
            }
            
            ++itObjects;
        }
        
    }
    
    return eCode;
}

PdfError PdfParser::ReadObjectFromStream( int nObjNo, int nIndex )
{
    PdfError         eCode;
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
    pStream = dynamic_cast<PdfParserObject*>(m_vecObjects.GetObject( PdfReference( nObjNo, 0 ) ) );
    if( !pStream )
    {
        RAISE_ERROR( ePdfError_NoObject );
    }

    lNum   = pStream->GetKeyValueLong( "N", 0 );
    lFirst = pStream->GetKeyValueLong( "First", 0 );

    if( m_mapStreamCache.find( nObjNo ) == m_mapStreamCache.end() )
    {
        // the objects stream might not yet be parsed, make sure
        // that this is done now!
        if( pStream->HasStreamToParse() && !pStream->HasStream() )
        {
            SAFE_OP_ADV( pStream->ParseStream(), "Unable to parse the objects stream." );
        }
        
        SAFE_OP( pStream->Stream()->GetFilteredCopy( &pBuffer, &lBufferLen ) );
        
        // the object stream is not needed anymore in the final PDF
        pStream->SetEmptyEntry( true );

        pNumbers = pBuffer;
        while( i < lNum )
        {
            lObj = strtol( pNumbers, &pNumbers, 10 );
            lOff = strtol( pNumbers, &pNumbers, 10 );
        
            pObj = new PdfParserObject( this->GetBuffer(), this->GetBufferSize() );
            SAFE_OP( pObj->ParseDictionaryKeys( (char*)(pBuffer+lFirst+lOff), lBufferLen-lFirst-lOff, NULL ) );

            pObj->SetObjectNumber( lObj );
            pObj->SetDirect( false );

            // TODO: remove cache
            m_mapStreamCache[nObjNo][nIndex] = pObj;
            m_vecObjects.push_back( pObj );
            m_mapStreamCache[nObjNo][nIndex] = NULL;
            ++i;
        }
    }
    else
    {
        pTmp = m_mapStreamCache[nObjNo][nIndex];
        if( pTmp )
        {
            m_vecObjects.push_back( pTmp );
            // set to NULL afterwards to allow deletion of spare objects
            // in the desctructor
            m_mapStreamCache[nObjNo][nIndex] = NULL;
        }
    }

    return eCode;
}

const char* PdfParser::GetPdfVersionString() const
{
    return s_szPdfVersions[(int)m_ePdfVersion];
}

};


