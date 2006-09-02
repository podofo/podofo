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

#include "PdfDictionary.h"
#include "PdfParser.h"
#include "PdfStream.h"
#include "PdfVariant.h"

#define KEY_BUFFER 128

namespace PoDoFo {

using namespace std;

static const int s_nLenEndObj    = 6; // strlen("endobj");
static const int s_nLenStream    = 6; // strlen("stream");
static const int s_nLenEndStream = 9; // strlen("endstream");

PdfParserObject::PdfParserObject( PdfVecObjects* pParent, const PdfRefCountedFile & rFile, const PdfRefCountedBuffer & rBuffer, long lOffset )
    : PdfObject( 0, 0, (const char*)NULL), PdfParserBase( rFile, rBuffer )
{
    m_pParent = pParent;

    Init();

    m_lOffset = lOffset == -1 ? ftell( m_file.Handle() ) : lOffset;
}

PdfParserObject::PdfParserObject( const PdfRefCountedBuffer & rBuffer )
    : PdfObject( 0, 0, (const char*)NULL), PdfParserBase( PdfRefCountedFile(), rBuffer )
{
    Init();
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

    PdfObject::Init( false );
}

void PdfParserObject::ReadObjectNumber()
{
    long number;

    try {
        number = GetNextNumberFromFile();
        m_reference.SetObjectNumber( number );
        number = GetNextNumberFromFile();
        m_reference.SetGenerationNumber( number );
    } catch( PdfError & e ) {
		std::string	errStr( e.what() );	// avoid compiler warning and in case we need it...
        RAISE_ERROR_INFO( ePdfError_NoObject, "object and generation number cannot be read." );
    }

    GetNextStringFromFile( );
    if( strncmp( m_buffer.Buffer(), "obj", 3 ) != 0 )
    {
        RAISE_ERROR( ePdfError_NoObject );
    }
}

void PdfParserObject::ParseFile( bool bIsTrailer )
{
    if( !m_file.Handle() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_lOffset > -1 )
        fseek( m_file.Handle(), m_lOffset, SEEK_SET );

    if( !bIsTrailer )
        ReadObjectNumber();

    m_lOffset    = ftell( m_file.Handle() );
    m_bIsTrailer = bIsTrailer;

    if( !m_bLoadOnDemand )
    {
        ParseFileComplete( m_bIsTrailer );

        m_bLoadOnDemandDone       = true;
        m_bLoadStreamOnDemandDone = true;
    }
}

void PdfParserObject::ParseFileComplete( bool bIsTrailer )
{
    int          c;
    int          counter         = 0;
    int          nObjCount       = 0;
    char*        szData          = m_buffer.Buffer();
    long         lDataLen        = this->GetBufferSize();
    bool         bOwnBuffer      = false;
    bool         bStringMode     = false;
    bool         bHexStringMode  = false;
    bool         bIgnoreNextChar = false;    
    EPdfDataType eDataType       = ePdfDataType_Unknown;

    fseek( m_file.Handle(), m_lOffset, SEEK_SET );

//#error " TODO: PdfParserBase::SkipWhiteSpace();"

    // skip all whitespace
    while( (c = fgetc( m_file.Handle() )) != EOF )
    {
        if( !IsWhitespace( c ) )
        {
            m_buffer.Buffer()[counter] = c;
            ++counter;
            break;
        }
    }

    GetDataType( c, &counter, &eDataType, NULL );
    while( (c = fgetc( m_file.Handle() )) != EOF )
    {
        if( counter == lDataLen )
        {
            lDataLen = lDataLen << 1; // lDataLen *= 2

            if( bOwnBuffer )
                szData = (char*)realloc( szData, lDataLen * sizeof(char) );
            else
            {
                szData = (char*)malloc( lDataLen * sizeof(char) );
                memcpy( szData, m_buffer.Buffer(), lDataLen >> 1 );
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
                szData[counter] = fgetc( m_file.Handle() );            
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
                    szData[counter] = fgetc( m_file.Handle() );            
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
        this->ParseDictionaryKeys( szData, counter );
    }
    else
    {
        this->Parse( szData, counter );
    }

    if( bOwnBuffer )
        free( szData );

    if( !bIsTrailer && eDataType != ePdfDataType_Unknown )
    {
        GetNextStringFromFile( );
        if( strncmp( m_buffer.Buffer(), "endobj", s_nLenEndObj ) == 0 )
            ; // nothing to do, just validate that the PDF is correct
        else if ( strncmp( m_buffer.Buffer(), "stream", s_nLenStream ) == 0 )
        {
            m_bStream = true;
            m_lStreamOffset = ftell( m_file.Handle() );	// NOTE: whitespace after "stream" handle in stream parser!

            // Most of the code relies on PdfObjects that are dictionaries
            // to have the datatype ePdfDataType_Dictionary and not Stream.
            // Please use PdfObject::HasStream to check wether it has a stream.
            //
            // Commenting this out is right now easier than fixing all code to check
            // either for ePdfDataType_Stream or ePdfDataType_Dictionary
            //
	    //eDataType = ePdfDataType_Stream;	// reset the object type to stream!
        }
        else
        {        
            RAISE_ERROR( ePdfError_NoObject );
        }
    }
}

void PdfParserObject::ParseDictionaryKeys( char* szBuffer, long lBufferLen, long* plParsedLength )
{
    string           sValue;
    char*            szInitial = szBuffer;
    PdfVariant       cVariant;
    PdfName          cName;
    long             lLen;

    sValue.reserve( KEY_BUFFER );

    // skip leading <<
    while( *szBuffer && *szBuffer == '<' )
        ++szBuffer;

    while( *szBuffer )
    {
        if( *szBuffer == '/' )
        {
            try {
                cVariant.Parse( szBuffer, lBufferLen-(szBuffer-szInitial), &lLen );
            } catch( PdfError & e ) {
                e.AddToCallstack( __FILE__, __LINE__, "Parsing dictionary key" );
                throw e;
            }

            szBuffer+=lLen;

            if( cVariant.GetDataType() != ePdfDataType_Name )
            {
                RAISE_ERROR( ePdfError_NoObject );
            }
            
            cName = cVariant.GetName();

            while( *szBuffer && IsWhitespace( *szBuffer ) )
                ++szBuffer;

            try {
                cVariant.Parse( szBuffer, lBufferLen-(szBuffer-szInitial), &lLen );
            } catch( PdfError & e ) {
                e.AddToCallstack( __FILE__, __LINE__, "Parsing dictionary value" );
                throw e;
            }

            szBuffer+=lLen;

#ifdef _DEBUG
            cVariant.ToString( sValue );
            PdfError::DebugMessage("Key: (%s) Got Value: (%s) %i belongs to: %s\n", cName.Name().c_str(), sValue.c_str(), (int)cVariant.GetDataType(), this->Reference().ToString().c_str() );

#endif // _DEBUG
            this->GetDictionary().AddKey( cName, cVariant );
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
}

void PdfParserObject::ParseStream()
{
    long         lLen  = -1;
    char*        szBuf;
    int          c;
    PdfReference ref;

    if( !m_file.Handle() || !m_pParent )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( fseek( m_file.Handle(), m_lStreamOffset, SEEK_SET ) != 0 )
    {
        RAISE_ERROR( ePdfError_InvalidStream );
    }

    // From the PDF Reference manual
    // The keyword stream that follows
    // the stream dictionary should be followed by an end-of-line marker consisting of
    // either a carriage return and a line feed or just a line feed, and not by a carriage re-
    // turn alone.
    c = fgetc( m_file.Handle() );
    if( !IsWhitespace( c ) )
    {
        ungetc( c, m_file.Handle() );
    } 
    else if( c == '\r' )
    {
        c = fgetc( m_file.Handle() );
        if( c != '\n' )
        {   ungetc( '\r', m_file.Handle() );
            ungetc( c, m_file.Handle() );
        }
    }
    
    long fLoc = ftell( m_file.Handle() );	// we need to save this, since loading the Length key could disturb it!

    PdfObject* pObj = this->GetDictionary().GetKey( PdfName::KeyLength );  
    if( pObj && pObj->IsNumber() )
    {
        lLen = pObj->GetNumber();   
    }
    else if( pObj && pObj->IsReference() )
    {
        pObj = m_pParent->GetObject( pObj->GetReference() );
        if( !pObj )
        {
            RAISE_ERROR( ePdfError_InvalidHandle );
        }

        if( !pObj->IsNumber() )
        {
            RAISE_ERROR( ePdfError_InvalidStreamLength );
        }

        lLen = pObj->GetNumber();
        if( !lLen )
        {
            RAISE_ERROR( ePdfError_InvalidStreamLength );
        }

        // we do not use indirect references for the length of the document
        delete m_pParent->RemoveObject( pObj->Reference() );
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

    fseek( m_file.Handle(), fLoc, SEEK_SET );	// reset it before reading!
    size_t frRet = fread( szBuf, lLen, sizeof( char ), m_file.Handle() );
    if( frRet != 1 )
    {
        RAISE_ERROR( ePdfError_InvalidStreamLength );
    }

    this->Stream()->Set( szBuf, lLen );

    /*
    SAFE_OP( GetNextStringFromFile( ) );
    if( strncmp( m_buffer.Buffer(), "endstream", s_nLenEndStream ) != 0 )
        return ERROR_PDF_MISSING_ENDSTREAM;
    */
}

void PdfParserObject::GetDataType( char c, int* counter, EPdfDataType* eDataType, bool* bType ) const
{
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
                    
            m_buffer.Buffer()[*counter] = fgetc( m_file.Handle() );
            if( m_buffer.Buffer()[*counter] == '<' )
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
}

void PdfParserObject::LoadOnDemand()
{
    if( m_bLoadOnDemand && !m_bLoadOnDemandDone )
    {
        m_bLoadOnDemandDone = true;

        PdfError::DebugMessage( "Loading on Demand: %s\n", this->Reference().ToString().c_str() );
        ParseFileComplete( m_bIsTrailer );
    }
}

void PdfParserObject::LoadStreamOnDemand()
{
    if( m_bLoadOnDemand && !m_bLoadStreamOnDemandDone )
    {
        if( !m_bLoadOnDemandDone )
        {
            LoadOnDemand();
        }

        m_bLoadStreamOnDemandDone = true;

        if( this->HasStreamToParse() && !this->HasStream() )
        {
            try {
                this->ParseStream();
            } catch( PdfError & e ) {
                e.AddToCallstack( __FILE__, __LINE__, "Unable to parse the objects stream." );
                throw e;
            }
        }
    }
}

};
