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
#include "PdfInputStream.h"
#include "PdfParser.h"
#include "PdfStream.h"
#include "PdfVariant.h"

#include <cassert>
#include <iostream>
#include <sstream>

#define KEY_BUFFER 128

namespace PoDoFo {

using namespace std;

static const int s_nLenEndObj    = 6; // strlen("endobj");
static const int s_nLenStream    = 6; // strlen("stream");
static const int s_nLenEndStream = 9; // strlen("endstream");

PdfParserObject::PdfParserObject( PdfVecObjects* pCreator, const PdfRefCountedInputDevice & rDevice, const PdfRefCountedBuffer & rBuffer, long lOffset )
    : PdfObject( PdfReference( 0, 0 ), static_cast<const char*>(NULL)), PdfTokenizer( rDevice, rBuffer )
{
    m_pOwner = pCreator;

    InitPdfParserObject();

    m_lOffset = lOffset == -1 ? m_device.Device()->Tell() : lOffset;
}

PdfParserObject::PdfParserObject( const PdfRefCountedBuffer & rBuffer )
    : PdfObject( PdfReference( 0, 0 ), static_cast<const char*>(NULL)), PdfTokenizer( PdfRefCountedInputDevice(), rBuffer )
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
    try {
        long obj = this->GetNextNumber();
        long gen = this->GetNextNumber();

        m_reference = PdfReference( obj, gen );
    } catch( PdfError & e ) {
        std::string errStr( e.what() );       // avoid compiler warning and in case we need it...
        PODOFO_RAISE_ERROR_INFO( ePdfError_NoObject, "Object and generation number cannot be read." );
    }
    
    if( !this->IsNextToken( "obj" ))
    {
        std::ostringstream oss;
        oss << "Error while reading object " << m_reference.ObjectNumber() << " " 
            << m_reference.GenerationNumber() << ": Next token is not 'obj'." << std::endl; 
        PODOFO_RAISE_ERROR_INFO( ePdfError_NoObject, oss.str().c_str() );
    }
}

void PdfParserObject::ParseFile( bool bIsTrailer )
{
    if( !m_device.Device() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_lOffset > -1 )
        m_device.Device()->Seek( m_lOffset );

    if( !bIsTrailer )
        ReadObjectNumber();

#if defined(PODOFO_VERBOSE_DEBUG)
    std::cerr << "Parsing object number: " << m_reference.ObjectNumber()
              << " " << m_reference.GenerationNumber() << " obj"
              << " " << m_lOffset << " offset"
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
            PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
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
    const char* pszToken;

    m_device.Device()->Seek( m_lOffset );
    this->GetNextVariant( *this );


    if( !bIsTrailer )
    {
        pszToken = this->GetNextToken();
        if( strncmp( pszToken, "endobj", s_nLenEndObj ) == 0 )
            ; // nothing to do, just validate that the PDF is correct
        // If it's a dictionary, it might have a stream, so check for that
        else if( this->IsDictionary() && strncmp( pszToken, "stream", s_nLenStream ) == 0 )
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
            PODOFO_RAISE_ERROR( ePdfError_NoObject );
        }
    }
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

    if( !m_device.Device() || !m_pOwner )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_device.Device()->Seek( m_lStreamOffset );

    // From the PDF Reference manual
    // The keyword stream that follows
    // the stream dictionary should be followed by an end-of-line marker consisting of
    // either a carriage return and a line feed or just a line feed, and not by a carriage re-
    // turn alone.
    c = m_device.Device()->Look();
    if( PdfTokenizer::IsWhitespace( c ) )
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
        pObj = m_pOwner->GetObject( pObj->GetReference() );
        if( !pObj )
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, "/Length key referenced indirect object that could not be loaded" );
        }

        if( !pObj->IsNumber() )
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidStreamLength, "/Length key for stream referenced non-number" );
        }

        lLen = pObj->GetNumber();
        if( !lLen )
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidStreamLength );
        }

        // we do not use indirect references for the length of the document
        // DS: Even though do not remove the length key,
        //     as 2 or more object might use the same object for key lengths.
        //     Deleting the length object of the first object will make
        //     all other objects non readable.
        //     If you want those length object to be removed,
        //     run the garbage collection of PdfVecObjects over your PDF.
        //delete m_pOwner->RemoveObject( pObj->Reference() );
    }
    else
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidStreamLength );
    }

    m_device.Device()->Seek( fLoc );	// reset it before reading!
    PdfDeviceInputStream reader( m_device.Device() );
    this->GetStream_NoDL()->SetRawData( &reader, lLen );

    /*
    SAFE_OP( GetNextStringFromFile( ) );
    if( strncmp( m_buffer.Buffer(), "endstream", s_nLenEndStream ) != 0 )
        return ERROR_PDF_MISSING_ENDSTREAM;
    */
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
            // TODO: track object ptr in error info so we don't have to do this memory-intensive
            // formatting here.
            std::ostringstream s;
            s << "Unable to parse the stream for object " << Reference().ObjectNumber() << ' '
              << Reference().GenerationNumber() << " obj .";
            e.AddToCallstack( __FILE__, __LINE__, s.str().c_str());
            throw e;
        }
    }

    // If we complete without throwing the stream will be flagged as loaded.
}

};
