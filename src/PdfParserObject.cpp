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
#include "PdfInputDevice.h"
#include "PdfParser.h"
#include "PdfStream.h"
#include "PdfVariant.h"

#include <cassert>
#include <iostream>

#define KEY_BUFFER 128

namespace PoDoFo {

using namespace std;

static const int s_nLenEndObj    = 6; // strlen("endobj");
static const int s_nLenStream    = 6; // strlen("stream");
static const int s_nLenEndStream = 9; // strlen("endstream");

PdfParserObject::PdfParserObject( PdfVecObjects* pParent, const PdfRefCountedInputDevice & rDevice, const PdfRefCountedBuffer & rBuffer, long lOffset )
    : PdfObject( PdfReference( 0, 0 ), static_cast<const char*>(NULL)), PdfParserBase( rDevice, rBuffer )
{
    m_pParent = pParent;

    InitPdfParserObject();

    m_lOffset = lOffset == -1 ? m_device.Device()->Tell() : lOffset;
}

PdfParserObject::PdfParserObject( const PdfRefCountedBuffer & rBuffer )
    : PdfObject( PdfReference( 0, 0 ), static_cast<const char*>(NULL)), PdfParserBase( PdfRefCountedInputDevice(), rBuffer )
{
    InitPdfParserObject();
}

PdfParserObject::~PdfParserObject()
{

}

void PdfParserObject::InitPdfParserObject()
{
    m_bIsTrailer        = false;

    // Whether or not demand loading is disabled we still don't load
    // anything in the ctor. This just controls whether ::ParseFile(...)
    // forces an immediate demand load, or lets it genuinely happen
    // on demand.
    m_bLoadOnDemand     = false;

    // We rely heavily on the demand loading infrastructure whether or not
    // we *actually* delay loading.
    EnableDelayedLoading();
    EnableDelayedStreamLoading();

    m_lOffset           = -1;

    m_bStream           = false;
    m_lStreamOffset     = 0;
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
        std::string errStr( e.what() );       // avoid compiler warning and in case we need it...
        RAISE_ERROR_INFO( ePdfError_NoObject, "object and generation number cannot be read." );
    }

    GetNextStringFromFile( );
    if( strncmp( m_buffer.GetBuffer(), "obj", 3 ) != 0 )
    {
        RAISE_ERROR( ePdfError_NoObject );
    }
}

void PdfParserObject::ParseFile( bool bIsTrailer )
{
    if( !m_device.Device() )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_lOffset > -1 )
        m_device.Device()->Seek( m_lOffset );

    if( !bIsTrailer )
        ReadObjectNumber();

#if defined(PODOFO_VERBOSE_DEBUG)
    std::cerr << "Parsing object number: " << m_reference.ObjectNumber()
              << " " << m_reference.GenerationNumber() << " obj"
              << " (DL: " << ( m_bLoadOnDemand ? "on" : "off" ) << ")"
              << endl;
#endif // PODOFO_VERBOSE_DEBUG

    m_lOffset    = m_device.Device()->Tell();
    m_bIsTrailer = bIsTrailer;

    if( !m_bLoadOnDemand )
    {
        // Force immediate loading of the object.  We need to do this through
        // the deferred loading machinery to avoid getting the object into an
        // inconsistent state.
        // We can't do a full DelayedStreamLoad() because the stream might use
        // an indirect /Length or /Length1 key that hasn't been read yet.
        DelayedLoad();

        // TODO: support immediate loading of the stream here too. For that, we need
        // to be able to trigger the reading of not-yet-parsed indirect objects
        // such as might appear in a /Length key with an indirect reference.

#if defined(PODOFO_EXTRA_CHECKS)
        // Sanity check - the variant base must be fully loaded now
        if (!DelayedLoadDone() )
        {
            // We don't know what went wrong, but the internal state is
            // broken or the API rules aren't being followed and we
            // can't carry on.
            RAISE_ERROR( ePdfError_InternalLogic );
        }
#endif // PODOF_EXTRA_CHECKS
    }
}

// Only called via the demand loading mechanism
// Be very careful to avoid recursive demand loads via PdfVariant
// or PdfObject method calls here.
void PdfParserObject::ParseFileComplete( bool bIsTrailer )
{
#if defined(PODOFO_EXTRA_CHECKS)
    assert(DelayedLoadInProgress());
    assert(!DelayedLoadDone());
#endif
    int          c;
    int          counter         = 0;
    int          nObjCount       = 0;
    char*        szData          = m_buffer.GetBuffer();
    long         lDataLen        = this->GetBufferSize();
    bool         bOwnBuffer      = false;
    bool         bStringMode     = false;
    bool         bHexStringMode  = false;
    bool         bIgnoreNextChar = false;    
    EPdfDataType eDataType       = ePdfDataType_Unknown;

    m_device.Device()->Seek( m_lOffset );

//#error " TODO: PdfParserBase::SkipWhiteSpace();"

    // skip all whitespace
    while( (c = m_device.Device()->GetChar()) != EOF )
    {
        if( !PdfParserBase::IsWhitespace( c ) )
        {
            m_buffer.GetBuffer()[counter] = c;
            ++counter;
            break;
        }
    }

    DetermineDataType( c, counter, eDataType );
    while( (c = m_device.Device()->GetChar()) != EOF )
    {
        if( counter == lDataLen )
        {
            lDataLen = lDataLen << 1; // lDataLen *= 2

            if( bOwnBuffer )
                szData = static_cast<char*>(realloc( szData, lDataLen * sizeof(char) ));
            else
            {
                szData = static_cast<char*>(malloc( lDataLen * sizeof(char) ));
                memcpy( szData, m_buffer.GetBuffer(), lDataLen >> 1 );
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
                szData[counter] = m_device.Device()->GetChar();
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
                    szData[counter] = m_device.Device()->GetChar();
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
            if( (counter >= s_nLenEndObj) && strncmp( (szData + counter - s_nLenEndObj), "endobj", s_nLenEndObj ) == 0 )
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
        if( strncmp( m_buffer.GetBuffer(), "endobj", s_nLenEndObj ) == 0 )
            ; // nothing to do, just validate that the PDF is correct
        // If it's a dictionary, it might have a stream, so check for that
        else if ( eDataType == ePdfDataType_Dictionary && strncmp( m_buffer.GetBuffer(), "stream", s_nLenStream ) == 0 )
        {
            m_bStream = true;
            m_lStreamOffset = m_device.Device()->Tell(); // NOTE: whitespace after "stream" handle in stream parser!

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

// This method may be called during delayed loading or directly.
void PdfParserObject::ParseDictionaryKeys( const char* szBuffer, long lBufferLen, long* plParsedLength )
{
    const char *     szInitial = szBuffer;
    PdfVariant       cVariant;
    long             lLen;

    // FIXME: Testing for *szBuffer=0 when buffer need not be null-terminated
    // skip leading << if present
    while( *szBuffer && *szBuffer == '<' )
        ++szBuffer;

    // We can't assume the buffer is 0-terminated, so check length
    while( (szBuffer-szInitial) < lBufferLen && *szBuffer )
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
            
            const PdfName cName(cVariant.GetName());

            while( *szBuffer && PdfParserBase::IsWhitespace( *szBuffer ) )
                ++szBuffer;

            try {
                cVariant.Parse( szBuffer, lBufferLen-(szBuffer-szInitial), &lLen );
            } catch( PdfError & e ) {
                e.AddToCallstack( __FILE__, __LINE__, "Parsing dictionary value" );
                throw e;
            }

            szBuffer+=lLen;

#ifdef PODOFO_VERBOSE_DEBUG
            string           sValue;
            sValue.reserve( KEY_BUFFER );
            cVariant.ToString( sValue );
            PdfError::DebugMessage("Key: (%s) Got Value: (%s) %i belongs to: %s\n", cName.GetName().c_str(), sValue.c_str(), (int)cVariant.GetDataType(), this->Reference().ToString().c_str() );

#endif // PODOFO_VERBOSE_DEBUG
            this->GetDictionary_NoDL().AddKey( cName, cVariant );
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

// Only called during delayed loading. Must be careful to avoid
// triggering recursive delay loading due to use of accessors of
// PdfVariant or PdfObject.
void PdfParserObject::ParseStream()
{
#if defined(PODOFO_EXTRA_CHECKS)
    assert(DelayedLoadDone());
    assert(DelayedStreamLoadInProgress());
    assert(!DelayedStreamLoadDone());
#endif

    long         lLen  = -1;
    int          c;

    if( !m_device.Device() || !m_pParent )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_device.Device()->Seek( m_lStreamOffset );

    // From the PDF Reference manual
    // The keyword stream that follows
    // the stream dictionary should be followed by an end-of-line marker consisting of
    // either a carriage return and a line feed or just a line feed, and not by a carriage re-
    // turn alone.
    c = m_device.Device()->Look();
    if( PdfParserBase::IsWhitespace( c ) )
    {
        c = m_device.Device()->GetChar();

        if( c == '\r' )
        {
            c = m_device.Device()->Look();
            if( c == '\n' )
            {
                c = m_device.Device()->GetChar();
            }
        }
    } 
    
    long fLoc = m_device.Device()->Tell();	// we need to save this, since loading the Length key could disturb it!

    PdfObject* pObj = this->GetDictionary_NoDL().GetKey( PdfName::KeyLength );  
    if( pObj && pObj->IsNumber() )
    {
        lLen = pObj->GetNumber();   
    }
    else if( pObj && pObj->IsReference() )
    {
        pObj = m_pParent->GetObject( pObj->GetReference() );
        if( !pObj )
        {
            RAISE_ERROR_INFO( ePdfError_InvalidHandle, "/Length key referenced indirect object that could not be loaded" );
        }

        if( !pObj->IsNumber() )
        {
            RAISE_ERROR_INFO( ePdfError_InvalidStreamLength, "/Length key for stream referenced non-number" );
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

    char * szBuf = static_cast<char*>(malloc( lLen * sizeof(char) ));
    if( !szBuf ) 
    {
        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    m_device.Device()->Seek( fLoc );	// reset it before reading!
    if( m_device.Device()->Read( szBuf, lLen ) != lLen )
    {
        RAISE_ERROR( ePdfError_InvalidStreamLength );
    }

    this->GetStream_NoDL()->Set( szBuf, lLen );

    /*
    SAFE_OP( GetNextStringFromFile( ) );
    if( strncmp( m_buffer.Buffer(), "endstream", s_nLenEndStream ) != 0 )
        return ERROR_PDF_MISSING_ENDSTREAM;
    */
}

// Called as part of demand loading process. Be careful not to trigger
// a recursive demand load via PdfObject or PdfVariant calls.
void PdfParserObject::DetermineDataType( char c, int & counter, EPdfDataType & eDataType ) const
{
#if defined(PODOFO_EXTRA_CHECKS)
    assert(DelayedLoadInProgress());
    assert(!DelayedLoadDone());
#endif
    switch( c )
    {
        case '[':
            eDataType = ePdfDataType_Array;
            break;
        case '(':
            eDataType = ePdfDataType_String;
            break;
        case '<':
            eDataType = ePdfDataType_HexString;

            m_buffer.GetBuffer()[counter] = m_device.Device()->GetChar();
            if( m_buffer.GetBuffer()[counter] == '<' )
                eDataType = ePdfDataType_Dictionary;
            ++counter;
            break;
        default:
            break;
    }
}

void PdfParserObject::DelayedLoadImpl()
{
#if defined(PODOFO_EXTRA_CHECKS)
    // DelayedLoadImpl() should only ever be called via DelayedLoad(),
    // which ensures that it is never called repeatedly.
    assert(!DelayedLoadDone());
    assert(DelayedLoadInProgress());
#endif

    ParseFileComplete( m_bIsTrailer );

    // If we complete without throwing DelayedLoadDone will be set
    // for us.
}

void PdfParserObject::DelayedStreamLoadImpl()
{
#if defined(PODOFO_EXTRA_CHECKS)
    // DelayedLoad() must've been called, either directly earlier
    // or via DelayedStreamLoad. DelayedLoad() will throw if the load
    // failed, so if we're being called this condition must be true.
    assert(DelayedLoadDone());

    // Similarly, we should not be being called unless the stream isn't
    // already loaded.
    assert(!DelayedStreamLoadDone());
    assert(DelayedStreamLoadInProgress());
#endif

    // Note: we can't use HasStream() here because it'll call DelayedStreamLoad()
    // causing a nasty loop. test m_pStream directly instead.
    if( this->HasStreamToParse() && !m_pStream )
    {
        try {
            this->ParseStream();
        } catch( PdfError & e ) {
            e.AddToCallstack( __FILE__, __LINE__, "Unable to parse the objects' stream." );
            throw e;
        }
    }

    // If we complete without throwing the stream will be flagged as loaded.
}

};
