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

#include "PdfParserObject.h"

#include "PdfParser.h"
#include "PdfStream.h"
#include "PdfVariant.h"

#define KEY_BUFFER 128

namespace PoDoFo {

using namespace std;

static const int s_nLenEndObj    = 6; // strlen("endobj");
static const int s_nLenStream    = 6; // strlen("stream");
static const int s_nLenEndStream = 9; // strlen("endstream");

PdfParserObject::PdfParserObject( FILE* hFile, char* szBuffer, long lBufferSize, long lOffset )
    : PdfObject( 0, 0, NULL), PdfParserBase( hFile, szBuffer, lBufferSize )
{
    Init();

    m_lOffset = lOffset == -1 ? ftell( m_hFile ) : lOffset;
}

PdfParserObject::PdfParserObject( char* szBuffer, long lBufferSize )
    : PdfObject( 0, 0, NULL), PdfParserBase( NULL, szBuffer, lBufferSize)
{
    Init();
    this->SetDirect( true );
}

PdfParserObject::~PdfParserObject()
{

}

void PdfParserObject::Init()
{
    m_bIsTrailer        = false;

    m_bLoadOnDemand     = false;
    m_lOffset           = -1;

    m_bStream           = false;
    m_lStreamOffset     = 0;

    PdfObject::Init();
}

PdfError PdfParserObject::ReadObjectNumber()
{
    PdfError  eCode;

    long      number;

    SAFE_OP( GetNextNumberFromFile( &number ) );
    m_reference.SetObjectNumber( number );
    SAFE_OP( GetNextNumberFromFile( &number ) );
    m_reference.SetGenerationNumber( number );
    
    SAFE_OP( GetNextStringFromFile( ) );
    if( strncmp( m_szBuffer, "obj", 3 ) != 0 )
    {
        RAISE_ERROR( ePdfError_NoObject );
    }
    
    return eCode;
}

PdfError PdfParserObject::ParseFile( PdfParser* pParser, bool bIsTrailer )
{
    PdfError     eCode;

    if( !m_hFile || !pParser )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_lOffset > -1 )
        fseek( m_hFile, m_lOffset, SEEK_SET );

    if( !bIsTrailer )
    {
        SAFE_OP( ReadObjectNumber() );
    }

    m_pParser    = pParser;
    m_lOffset    = ftell( m_hFile );
    m_bIsTrailer = bIsTrailer;

    if( !m_bLoadOnDemand )
        SAFE_OP( ParseFileComplete( m_bIsTrailer ) );

    return eCode;
}

PdfError PdfParserObject::ParseFileComplete( bool bIsTrailer )
{
    PdfError     eCode;

    int          c;
    int          counter         = 0;
    int          nObjCount       = 0;
    char*        szData          = m_szBuffer;
    long         lDataLen        = this->GetBufferSize();
    bool         bOwnBuffer      = false;
    bool         bStringMode     = false;
    bool         bHexStringMode  = false;
    bool         bIgnoreNextChar = false;    
    EPdfDataType eDataType       = ePdfDataType_Unknown;

    fseek( m_hFile, m_lOffset, SEEK_SET );

    // skip all whitespace
    while( (c = fgetc( m_hFile )) != EOF )
    {
        if( !IsWhitespace( c ) )
        {
            m_szBuffer[counter] = c;
            ++counter;
            break;
        }
    }

    SAFE_OP( GetDataType( c, &counter, &eDataType, NULL ) );
    while( (c = fgetc( m_hFile )) != EOF )
    {
        if( counter == lDataLen )
        {
            lDataLen = lDataLen << 1; // lDataLen *= 2

            if( bOwnBuffer )
                szData = (char*)realloc( szData, lDataLen * sizeof(char) );
            else
            {
                szData = (char*)malloc( lDataLen * sizeof(char) );
                memcpy( szData, m_szBuffer, lDataLen >> 1 );
                bOwnBuffer = true;
            }

            if( !szData )
            {
                RAISE_ERROR( ePdfError_OutOfMemory );
            }
        }

        szData[counter] = c;
        ++counter;

        if( eDataType == ePdfDataType_Array )
        {
            if( c == '[' )
                ++nObjCount;
            else if( c == ']' )
            {
                if( !nObjCount )
                    break;
                --nObjCount;
            }
        }
        else if( eDataType == ePdfDataType_String ) 
        {
            // we have to handle specialstrings like (Hallo\\) correctly
            // as well as (Hallo\))
            if( !bIgnoreNextChar && c == ')' )
                break;

            bIgnoreNextChar = ( c == '\\' && !bIgnoreNextChar);
        }
        else if( eDataType == ePdfDataType_HexString && c == '>' )
        {
            break;
        }
        else if( eDataType == ePdfDataType_Dictionary )
        {
            if( c == '<' && !bStringMode && !bHexStringMode )
            {
                szData[counter] = fgetc( m_hFile );            
                if( szData[counter] == '<' )
                    ++nObjCount;
                else 
                    bHexStringMode = true;
                ++counter;
            }
            else if( c == '>' && !bStringMode )
            {
                if( bHexStringMode )
                    bHexStringMode = false;
                else
                {
                    szData[counter] = fgetc( m_hFile );            
                    if( szData[counter] == '>' )
                    {
                        ++counter;
                        if( !nObjCount )
                            break;
                        --nObjCount;
                    }
                    else
                        ++counter;
                }
            }
            else if( c == '(' && !bStringMode ) // start string mode...
            {
                bStringMode = true;
            }
            else if( c == ')' && bStringMode )
            {
                // we have to handle specialstrings like (Hallo\\) correctly
                // as well as (Hallo\))

                // we have to increase every value by 1 as counter has been 
                // increased at the beginning of the loop
                if( counter >= 3 && 
                    szData[counter-2] == '\\' && szData[counter-3] == '\\' )
                    bStringMode = false; 
                else if( counter >= 2 && szData[counter-2] == '\\' )
                    bStringMode = true; 
                else
                    bStringMode = false; 
            }
        }
        else 
        {
            if( strncmp( (szData + counter - s_nLenEndObj), "endobj", s_nLenEndObj ) == 0 )
            {
                counter -= s_nLenEndObj;
                break;
            }
        }
    }
    szData[counter] = '\0';

    if( eDataType == ePdfDataType_Dictionary )
    {
        SAFE_OP( this->ParseDictionaryKeys( szData, counter ) );
    }
    else
    {
        PdfVariant var;
        SAFE_OP( var.Init( szData ) );
        this->SetSingleValue( var );
    }

    if( bOwnBuffer )
        free( szData );

    if( !bIsTrailer && eDataType != ePdfDataType_Unknown )
    {
        SAFE_OP( GetNextStringFromFile( ) );
        if( strncmp( m_szBuffer, "endobj", s_nLenEndObj ) == 0 )
            ; // nothing to do, just validate that the PDF is correct
        else if ( strncmp( m_szBuffer, "stream", s_nLenStream ) == 0 )
        {
            m_bStream = true;
            m_lStreamOffset = ftell( m_hFile );
        }
        else
        {        
            RAISE_ERROR( ePdfError_NoObject );
        }
    }

    return eCode;
}

PdfError PdfParserObject::ParseDictionaryKeys( char* szBuffer, long lBufferLen, long* plParsedLength )
{
    PdfError         eCode;
    string*          pCur    = NULL;
    string           sValue;
    char*            szInitial = szBuffer;
    PdfVariant       cVariant;
    PdfName          cName;
    long             lLen;
    long             lInternalLen;
    PdfParserObject* pObj;

    sValue.reserve( KEY_BUFFER );

/*
    while( *szBuffer && IsWhitespace( *szBuffer ) )
        ++szBuffer;

    if( *szBuffer == '>' && *(szBuffer+1) == '>' )
    {
        if( plParsedLength )
            *plParsedLength = szBuffer - szInitial;
        return eCode;
    }
*/
    // skip leading <<
    while( *szBuffer && *szBuffer == '<' )
        ++szBuffer;

    while( *szBuffer )
    {
        if( *szBuffer == '/' )
        {
            SAFE_OP_ADV( cVariant.Init( szBuffer, lBufferLen-(szBuffer-szInitial), &lLen ), "Parsing new key" );
            szBuffer+=lLen;

            if( cVariant.GetDataType() != ePdfDataType_Name )
            {
                RAISE_ERROR( ePdfError_NoObject );
            }
            
            cName = cVariant.GetName();

            while( *szBuffer && IsWhitespace( *szBuffer ) )
                ++szBuffer;

            SAFE_OP_ADV( cVariant.Init( szBuffer, lBufferLen-(szBuffer-szInitial), &lLen ), "Parsing new value" );
            szBuffer+=lLen;

            if( cVariant.GetDataType() == ePdfDataType_Dictionary )
            {
                this->AddKey( cName, new PdfObject( cVariant.GetDictionary() ) );
            }
            else
            {
                cVariant.ToString( sValue );
#ifdef _DEBUG
                printf("Key: (%s) Got Value: (%s) %i belongs to: %s\n", cName.Name(), sValue.c_str(), (int)cVariant.GetDataType(), this->Reference().ToString().c_str() );
#endif // _DEBUG
                this->AddKey( cName, cVariant );
            }
        }
        else if( *szBuffer == '>' )
        {
            ++szBuffer;
            if( *szBuffer == '>' )
                break;
        }
        else
            ++szBuffer;
    }

    if( plParsedLength )
        *plParsedLength = szBuffer - szInitial;

    return eCode;
}

#if 0
PdfError PdfParserObject::ParseValue( char** szBuffer, string & sKey, string & sValue  )
{
    PdfError         eCode;
    char*            szValue;
    PdfParserObject* pObj    = NULL;
    // strip leading whitespaces
    while( **szBuffer && IsWhitespace( **szBuffer ) ) 
        ++(*szBuffer);
                
    szValue = *szBuffer;

    if( *szValue == '/' )
    {
        ++(*szBuffer);
        while( **szBuffer && !IsWhitespace( **szBuffer ) && !IsDelimiter( **szBuffer ) )
            ++(*szBuffer);
    }
    else if( *szValue == '<' )
    {
        if( *(szValue+1) == '<' )
        {
            long lParsedLength;
            pObj = new PdfParserObject( this->GetBuffer(), this->GetBufferSize() );
            eCode = pObj->ParseDictionaryKeys( szValue, &lParsedLength );
            if( eCode != ePdfError_ErrOk )
            {
                delete pObj;
                return eCode;
            }

            char* tmp = (char*)malloc( lParsedLength * sizeof(char) + 1 );
            strncpy( tmp, *szBuffer, lParsedLength );
            tmp[lParsedLength] = '\0';
            *szBuffer += lParsedLength;
            
            free( tmp );
        }
        else
        {
            // it is just a hexadecimal encoded string
            // parse till the closing > is reached
            while( **szBuffer && **szBuffer != '>' )
                ++(*szBuffer);
            ++(*szBuffer);
        }
    }
    else if( *szValue == '(' )
    {
        bool bIgnoreNextChar = false;

        while( **szBuffer )
        {
            if( !bIgnoreNextChar && **szBuffer == ')' )
            {
                ++(*szBuffer);
                break;
            }

            bIgnoreNextChar = ( **szBuffer == '\\' && !bIgnoreNextChar);

            ++(*szBuffer);
        }
    }
    else if( *szValue == '[' )
    {
        // TODO: DS arrays inside of arrays

        // it is just an array
        // parse till the closing ] is reached
        while( **szBuffer && **szBuffer != ']' )
            ++(*szBuffer);
        ++(*szBuffer);
    }
    else
    {
        bool bInc = false;

        while( **szBuffer && **szBuffer != '/' && **szBuffer != '>' )
            ++(*szBuffer);

        // We are currently most likely on a separator
        // so skip before the separator and remove whitespaces there
        if( IsDelimiter( **szBuffer ) )
        {
            --(*szBuffer);
            bInc = true;
        }

        // strip trailing whitespaces
        while( IsWhitespace( **szBuffer ) ) 
            --(*szBuffer);

        if( bInc )
            ++(*szBuffer);
    }

    if( pObj )
    {
#ifdef _DEBUG
        printf("Object Key: (%s)\n", sKey.c_str() );
#endif // _DEBUG
        this->AddKey( sKey, pObj );
        pObj = NULL;
    }
    else
    {
        sValue.assign( szValue, *szBuffer - szValue );
#ifdef _DEBUG
        printf("Key: (%s) | (%s)\n", sKey.c_str(), sValue.c_str() );
#endif // _DEBUG
        this->AddKey( sKey, sValue );
    }

    sKey.clear();
    sValue.clear();

    --(*szBuffer); // reset buffer for caller...

    return eCode;
}
#endif // 0

PdfError PdfParserObject::ParseStream( const PdfVecObjects* pVecObjects )
{
    PdfError     eCode;
    long         lLen  = -1;
    char*        szBuf;
    int          c;
    PdfVariant   variant;
    PdfReference ref;

    if( !m_hFile || !pVecObjects )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( fseek( m_hFile, m_lStreamOffset, SEEK_SET ) != 0 )
    {
        RAISE_ERROR( ePdfError_InvalidStream );
    }

    // From the PDF Reference manual
    // The keyword stream that follows
    // the stream dictionary should be followed by an end-of-line marker consisting of
    // either a carriage return and a line feed or just a line feed, and not by a carriage re-
    // turn alone.
    c = fgetc( m_hFile );
    if( !IsWhitespace( c ) )
    {
        ungetc( c, m_hFile );
    } 
    else if( c == '\r' )
    {
        c = fgetc( m_hFile );
        if( c != '\n' )
        {   ungetc( '\r', m_hFile );
            ungetc( c, m_hFile );
        }
    }

    SAFE_OP( this->GetKeyValueVariant( PdfName::KeyLength, variant ) ); 
    if( variant.GetDataType() == ePdfDataType_Number )
    {
        SAFE_OP( variant.GetNumber( &lLen ) );
    }
    else if( variant.GetDataType() == ePdfDataType_Reference )
    {
        PdfObject* pObj;

        ref  = variant.GetReference();
        pObj = pVecObjects->GetObject( ref );
        if( !pObj )
        {
            RAISE_ERROR( ePdfError_InvalidHandle );
        }

        if( !pObj->HasSingleValue() )
        {
            RAISE_ERROR( ePdfError_InvalidStreamLength );
        }

        lLen = pObj->GetSingleValueLong();
        if( !lLen )
        {
            RAISE_ERROR( ePdfError_InvalidStreamLength );
        }

        // we do not use indirect references for the length of the document
        pObj->SetEmptyEntry( true );
    }
    else
    {
        RAISE_ERROR( ePdfError_InvalidStreamLength );
    }

    szBuf = (char*)malloc( lLen * sizeof(char) );
    if( !szBuf ) 
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    if( fread( szBuf, lLen, sizeof( char ), m_hFile ) != 1 )
    {
        RAISE_ERROR( ePdfError_InvalidStreamLength );
    }

    this->Stream()->Set( szBuf, lLen );

    /*
    SAFE_OP( GetNextStringFromFile( ) );
    if( strncmp( m_szBuffer, "endstream", s_nLenEndStream ) != 0 )
        return ERROR_PDF_MISSING_ENDSTREAM;
    */

    return eCode;
}

PdfError PdfParserObject::GetDataType( char c, int* counter, EPdfDataType* eDataType, bool* bType ) const
{
    PdfError eCode;

    // TODO: Allow for hexadecimal encoded strings: reference: p. 54
    switch( c )
    {
        case '[':
            if( eDataType )
                *eDataType = ePdfDataType_Array;

            if( bType )
                *bType = false;
            break;
        case '(':
            if( eDataType )
                *eDataType = ePdfDataType_String;


            if( bType )
                *bType = false;
            break;
        case '<':
            if( eDataType )
                *eDataType = ePdfDataType_HexString;
                    
            m_szBuffer[*counter] = fgetc( m_hFile );
            if( m_szBuffer[*counter] == '<' )
            {
                if( eDataType )
                    *eDataType = ePdfDataType_Dictionary;
            }
            ++(*counter);

            if( bType )
                *bType = false;
            break;
        default:
            if( bType )
                *bType = false;
            break;
    }
            
    return eCode;
}

PdfError PdfParserObject::LoadOnDemand( const PdfVecObjects* pVecObjects )
{
    PdfError eCode;

    if( m_bLoadOnDemand )
    {
        m_bLoadOnDemandDone = true;

        printf("-> Delayed Parsing Object %s\n", m_reference.ToString().c_str() );
        SAFE_OP( ParseFileComplete( m_bIsTrailer ) );

        printf("-> HasStreamToParse = %i HasStream() = %i\n", this->HasStreamToParse(), this->HasStream() );

        if( this->HasStreamToParse() && !this->HasStream() )
        {
            printf("-> Parsing Stream...\n");
            SAFE_OP_ADV( this->ParseStream( pVecObjects ), "Unable to parse the objects stream." );
        }
    }

    return eCode;
}

};
