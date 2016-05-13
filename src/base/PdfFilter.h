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

#ifndef _PDF_FILTER_H_
#define _PDF_FILTER_H_

#include <memory>
#include <cassert>

#include "PdfDefines.h"

#include "PdfInputStream.h"
#include "PdfOutputStream.h"

namespace PoDoFo {

class PdfDictionary;
class PdfName;
class PdfObject;
class PdfOutputStream;

typedef std::vector<EPdfFilter>            TVecFilters;
typedef TVecFilters::iterator              TIVecFilters;
typedef TVecFilters::const_iterator        TCIVecFilters;

/** Every filter in PoDoFo has to implement this interface.
 * 
 *  The two methods Encode() and Decode() have to be implemented 
 *  for every filter.
 *
 *  The output buffers are podofo_malloc()'ed in the functions and have
 *  to be podofo_free()'d by the caller.
 */
class PODOFO_API PdfFilter {
 public:
    /** Construct and initialize a new filter
     */
    PdfFilter();

    /** All classes with virtual functions need a virtual destructor
     */
    inline virtual ~PdfFilter();

    /** Check whether encoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to encode data
     */
    virtual bool CanEncode() const = 0; 

    /** Encodes a buffer using a filter. The buffer will podofo_malloc()'d and
     *  has to be podofo_free()'d by the caller.
     *
     *  This function uses BeginEncode()/EncodeBlock()/EndEncode()
     *  internally, so it's not safe to use when progressive encoding
     *  is in progress.
     *
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer receives pointer to the buffer of the encoded data
     *  \param plOutLen pointer to where to write the output buffer's length
     */
    void Encode( const char* pInBuffer, pdf_long lInLen, char** ppOutBuffer, pdf_long* plOutLen ) const;

    /** Begin progressively encoding data using this filter.
     *
     *  This method sets the filter's output stream and may
     *  perform other operations defined by particular filter
     *  implementations. It calls BeginEncodeImpl().
     *
     *  \param pOutput Encoded data will be written to this stream.
     *
     *  Call EncodeBlock() to encode blocks of data and use EndEncode()
     *  to finish the encoding process.
     *
     *  \see EncodeBlock
     *  \see EndEncode
     */
    inline void BeginEncode( PdfOutputStream* pOutput );

    /** Encode a block of data and write it to the PdfOutputStream
     *  specified by BeginEncode(). Ownership of the block is not taken
     *  and remains with the caller.
     *
     *  The filter implementation need not immediately process the buffer,
     *  and might internally buffer some or all of it. However, if it does
     *  this the buffer's contents will be copied, so it is guaranteed to be
     *  safe to free the passed buffer after this call returns.
     *
     *  This method is a wrapper around EncodeBlockImpl().
     *
     *  BeginEncode() must be called before this function.
     *
     *  \param pBuffer pointer to a buffer with data to encode
     *  \param lLen length of data to encode.
     *
     *  Call EndEncode() after all data has been encoded.
     *
     *  \see BeginEncode
     *  \see EndEncode
     */
    inline void EncodeBlock( const char* pBuffer, pdf_long lLen );

    /**
     *  Finish encoding of data and reset the stream's state.
     *
     *  \see BeginEncode
     *  \see EncodeBlock
     */
    inline void EndEncode();

    /** Check whether the decoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to decode data
     */
    virtual bool CanDecode() const = 0; 

    /** Decodes a buffer using a filter. The buffer will podofo_malloc()'d and
     *  has to be podofo_free()'d by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer receives pointer to the buffer of the decoded data
     *  \param plOutLen pointer to where to write the output buffer's length  
     *  \param pDecodeParms optional pointer to a decode-parameters dictionary
     *                      containing additional information to decode
     *                      the data. This pointer must be NULL if no
     *                      decode-parameters dictionary is available.
     */
    void Decode( const char* pInBuffer, pdf_long lInLen, char** ppOutBuffer, pdf_long* plOutLen, const PdfDictionary* pDecodeParms = NULL ) const;

    /** Begin progressively decoding data using this filter.
     *
     *  This method sets the filter's output stream and may
     *  perform other operations defined by particular filter
     *  implementations. It calls BeginDecodeImpl().
     *
     *  \param pOutput decoded data will be written to this stream
     *  \param pDecodeParms a dictionary containing additional information
     *                      for decoding
     *
     *  Call DecodeBlock() to decode blocks of data and use EndDecode()
     *  to finish the decoding process.
     *
     *  \see DecodeBlock
     *  \see EndDecode
     */
    inline void BeginDecode( PdfOutputStream* pOutput, const PdfDictionary* pDecodeParms = NULL );

    /** Decode a block of data and write it to the PdfOutputStream
     *  specified by BeginDecode(). Ownership of the block is not taken
     *  and remains with the caller.
     *
     *  The filter implementation need not immediately process the buffer,
     *  and might internally buffer some or all of it. However, if it does
     *  this the buffer's contents will be copied, so it is guaranteed to be
     *  safe to free the passed buffer after this call returns.
     *
     *  This method is a wrapper around DecodeBlockImpl().
     *
     *  BeginDecode() must be called before this function.
     *
     *  \param pBuffer pointer to a buffer with data to encode
     *  \param lLen length of data to encode.
     *
     *  Call EndDecode() after all data has been decoded.
     *
     *  \see BeginDecode
     *  \see EndDecode
     */
    inline void DecodeBlock( const char* pBuffer, pdf_long lLen );

    /**
     *  Finish decoding of data and reset the stream's state.
     *
     *  \see BeginDecode
     *  \see DecodeBlock
     */
    inline void EndDecode();

    /** Type of this filter.
     *  \returns the type of this filter
     */
    virtual EPdfFilter GetType() const = 0;

    PODOFO_NOTHROW inline PdfOutputStream* GetStream() const { return m_pOutputStream; }

 protected:
    /**
     * Indicate that the filter has failed, and will be non-functional
     * until BeginEncode() or BeginDecode() is next called. Call this
     * instead of EndEncode() or EndDecode if something went wrong.
     * It clears the stream output but otherwise does nothing.
     *
     * After this method is called further calls to EncodeBlock(),
     * DecodeBlock(), EndDecode() and EndEncode() before the next
     * BeginEncode() or BeginDecode() are guaranteed to throw
     * without calling their virtual implementations.
     */
    PODOFO_NOTHROW inline void FailEncodeDecode();

    /** Real implementation of BeginEncode(). NEVER call this method directly.
     *
     *  By default this function does nothing. If your filter needs to do setup
     *  for encoding, you should override this method.
     *
     *  PdfFilter ensures that a valid stream is available when this method is
     *  called, and that EndEncode() was called since the last BeginEncode()/
     *  EncodeBlock().
     * \see BeginEncode
     */
    virtual void BeginEncodeImpl( ) { }

    /** Real implementation of EncodeBlock(). NEVER call this method directly.
     *
     *  You must method-override it to encode the buffer passed by the caller.
     *
     *  You are not obliged to immediately process any or all of the data in
     *  the passed buffer, but you must ensure that you have processed it and
     *  written it out by the end of EndEncodeImpl(). You must copy the buffer
     *  if you're going to store it, as ownership is not transferred to the
     *  filter and the caller may free the buffer at any time.
     *
     *  PdfFilter ensures that a valid stream is available when this method is
     *  called, ensures that BeginEncode() has been called, and ensures that
     *  EndEncode() has not been called since the last BeginEncode().
     *
     * \see EncodeBlock
     */
    virtual void EncodeBlockImpl( const char* pBuffer, pdf_long lLen ) = 0;

    /** Real implementation of EndEncode(). NEVER call this method directly.
     *
     * By the time this method returns, all filtered data must be written to
     * the stream and the filter must be in a state where BeginEncode() can
     * be safely called.
     * PdfFilter ensures that a valid stream is available when this method is
     * called, and ensures that BeginEncodeImpl() has been called.
     *
     * \see EndEncode
     */
    virtual void EndEncodeImpl() { }

    /** Real implementation of BeginDecode(). NEVER call this method directly.
     *
     *  By default this function does nothing. If your filter needs to do setup
     *  for decoding, you should override this method.
     *
     *  PdfFilter ensures that a valid stream is available when this method is
     *  called, and that EndDecode() was called since the last BeginDecode()/
     *  DecodeBlock().
     * \see BeginDecode
     */
    virtual void BeginDecodeImpl( const PdfDictionary* ) { }

    /** Real implementation of DecodeBlock(). NEVER call this method directly.
     *
     *  You must method-override it to decode the buffer passed by the caller.
     *
     *  You are not obliged to immediately process any or all of the data in
     *  the passed buffer, but you must ensure that you have processed it and
     *  written it out by the end of EndDecodeImpl(). You must copy the buffer
     *  if you're going to store it, as ownership is not transferred to the
     *  filter and the caller may free the buffer at any time.
     *
     *  PdfFilter ensures that a valid stream is available when this method is
     *  called, ensures that BeginDecode() has been called, and ensures that
     *  EndDecode() has not been called since the last BeginDecode().
     *
     * \see DecodeBlock
     */
    virtual void DecodeBlockImpl( const char* pBuffer, pdf_long lLen ) = 0;

    /** Real implementation of EndDecode(). NEVER call this method directly.
     *
     *  By the time this method returns, all filtered data must be written to
     *  the stream and the filter must be in a state where BeginDecode() can be
     *  safely called.
     *  PdfFilter ensures that a valid stream is available when this method is
     *  called, and ensures that BeginDecodeImpl() has been called.
     *
     * \see EndDecode
     */
    virtual void EndDecodeImpl() { }

 private:
    PdfOutputStream* m_pOutputStream;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfFilter::BeginEncode( PdfOutputStream* pOutput )
{
    PODOFO_RAISE_LOGIC_IF( m_pOutputStream, "BeginEncode() on failed filter or without EndEncode()" );
    m_pOutputStream = pOutput;

	try {
		BeginEncodeImpl();
	} catch( const PdfError & e ) {
		// Clean up and close stream
		this->FailEncodeDecode();
		throw e;
	}
}
 
// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfFilter::EncodeBlock( const char* pBuffer, pdf_long lLen )
{
    PODOFO_RAISE_LOGIC_IF( !m_pOutputStream, "EncodeBlock() without BeginEncode() or on failed filter" );

	try {
		EncodeBlockImpl(pBuffer, lLen);
	} catch( const PdfError & e ) {
		// Clean up and close stream
		this->FailEncodeDecode();
		throw e;
	}
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfFilter::EndEncode()
{
    PODOFO_RAISE_LOGIC_IF( !m_pOutputStream, "EndEncode() without BeginEncode() or on failed filter" );

	try {
		EndEncodeImpl();
	} catch( const PdfError & e ) {
		// Clean up and close stream
		this->FailEncodeDecode();
		throw e;
	}    

    m_pOutputStream->Close();
    m_pOutputStream = NULL;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfFilter::BeginDecode( PdfOutputStream* pOutput, const PdfDictionary* pDecodeParms )
{
    PODOFO_RAISE_LOGIC_IF( m_pOutputStream, "BeginDecode() on failed filter or without EndDecode()" );
    m_pOutputStream = pOutput;

	try {
		BeginDecodeImpl( pDecodeParms );
	} catch( const PdfError & e ) {
		// Clean up and close stream
		this->FailEncodeDecode();
		throw e;
	}    
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfFilter::DecodeBlock( const char* pBuffer, pdf_long lLen )
{
    PODOFO_RAISE_LOGIC_IF( !m_pOutputStream, "DecodeBlock() without BeginDecode() or on failed filter" )

	try {
		DecodeBlockImpl(pBuffer, lLen);
	} catch( const PdfError & e ) {
		// Clean up and close stream
		this->FailEncodeDecode();
		throw e;
	}    
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfFilter::EndDecode()
{
    PODOFO_RAISE_LOGIC_IF( !m_pOutputStream, "EndDecode() without BeginDecode() or on failed filter" )

	try {
	    EndDecodeImpl();
	} catch( const PdfError & e ) {
		// Clean up and close stream
		this->FailEncodeDecode();
		throw e;
	}    

    if( m_pOutputStream ) 
    {
        m_pOutputStream->Close();
        m_pOutputStream = NULL;
    }
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfFilter::FailEncodeDecode()
{
    if ( m_pOutputStream != NULL ) // OC 19.08.2010 BugFix: Sometimes FailEncodeDecode() is called twice
        m_pOutputStream->Close();
    m_pOutputStream = NULL;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfFilter::~PdfFilter()
{
    // Whoops! Didn't call EndEncode() before destroying the filter!
    // Note that we can't do this for the user, since EndEncode() might
    // throw and we can't safely have that in a dtor. That also means
    // we can't throw here, but must abort.
    assert(!m_pOutputStream);
}


/** A factory to create a filter object for a filter type (as GetType() gives)
 *  from the EPdfFilter enum. 
 *  All filters should be created using this factory.
 */
class PODOFO_API PdfFilterFactory {
 public:
    /** Create a filter from an enum.
     *
     *  Ownership is transferred to the caller, who should let the auto_ptr
     *  the filter is returned in take care of freeing it when they're done
     *  with it.
     *
     *  \param eFilter return value of GetType() for filter to be created
     *
     *  \returns a new PdfFilter allocated using new, or NULL if no
     *           filter is available for this type.
     */
    static std::auto_ptr<PdfFilter> Create( const EPdfFilter eFilter );

    /** Create a PdfOutputStream that applies a list of filters 
     *  on all data written to it.
     *
     *  \param filters a list of filters
     *  \param pStream write all data to this PdfOutputStream after it has been
     *         encoded
     *  \returns a new PdfOutputStream that has to be deleted by the caller.
     *
     *  \see PdfFilterFactory::CreateFilterList
     */
    static PdfOutputStream* CreateEncodeStream( const TVecFilters & filters, PdfOutputStream* pStream );

    /** Create a PdfOutputStream that applies a list of filters 
     *  on all data written to it.
     *
     *  \param filters a list of filters
     *  \param pStream write all data to this PdfOutputStream
     *         after it has been decoded.
     *  \param pDictionary pointer to a dictionary that might
     *         contain additional parameters for stream decoding.
     *         This method will look for a key named DecodeParms
     *         in this dictionary and pass the information found
     *         in that dictionary to the filters.
     *  \returns a new PdfOutputStream that has to be deleted by the caller.
     *
     *  \see PdfFilterFactory::CreateFilterList
     */
    static PdfOutputStream* CreateDecodeStream( const TVecFilters & filters, PdfOutputStream* pStream, 
                                                const PdfDictionary* pDictionary = NULL );

    /** Converts a filter name to the corresponding enum
     *  \param name of the filter without leading
     *  \param bSupportShortNames The PDF Reference supports several
     *         short names for filters (e.g. AHx for AsciiHexDecode), if true
     *         support for these short names will be enabled. 
     *         This is often used in inline images.
     *  \returns the filter as enum
     */
    static EPdfFilter FilterNameToType( const PdfName & name, bool bSupportShortNames = true );

    /** Converts a filter type enum to the corresponding PdfName
     *  \param eFilter a filter type
     *  \returns the filter as name
     */
    static const char* FilterTypeToName( EPdfFilter eFilter );

    /** The passed PdfObject has to be a dictionary with a Filters key,
     *  a (possibly empty) array of filter names or a filter name.
     *
     *  \param pObject must define a filter or list of filters (can be
     *         empty, although then you should use TVecFilters' default)
     *
     *  \returns a list of filters
     */
    static TVecFilters CreateFilterList( const PdfObject* pObject );

 private:
    // prohibit instantiation of all-methods-static factory from outside
    PdfFilterFactory();
};


};

#endif /* _PDF_FILTER_H_ */
