/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#ifndef _PDF_FILTERS_PRIVATE_H_
#define _PDF_FILTERS_PRIVATE_H_

/**
 * \file PdfFiltersPrivate.h
 *
 * Provides implementations of various PDF stream filters.
 *
 * This is an internal header. It should not be included in podofo.h, and
 * should not be included directly by client applications. These filters should
 * only be accessed through the factory interface in PdfFilters.h .
 */

#include "PdfDefines.h"
#include "PdfDefinesPrivate.h"
#include "PdfFilter.h"
#include "PdfRefCountedBuffer.h"

#include <zlib.h>

#ifdef PODOFO_HAVE_JPEG_LIB
extern "C" {
#ifdef _WIN32		// Collision between win and jpeg-headers
#define XMD_H
#undef FAR
#endif
#include "jpeglib.h"
}
#endif // PODOFO_HAVE_JPEG_LIB

#ifdef PODOFO_HAVE_TIFF_LIB
extern "C" {
#include "tiffio.h"
#ifdef _WIN32		// Collision between tiff and jpeg-headers
#define XMD_H
#undef FAR
#endif
}
#endif // PODOFO_HAVE_TIFF_LIB


namespace PoDoFo {

#define PODOFO_FILTER_INTERNAL_BUFFER_SIZE 4096

class PdfPredictorDecoder;
class PdfOutputDevice;

/** The ascii hex filter.
 */
class PdfHexFilter : public PdfFilter {
 public:
    virtual ~PdfHexFilter() { }

    /** Check wether the encoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to encode data
     */
    inline virtual bool CanEncode() const; 

    /** Encode a block of data and write it to the PdfOutputStream
     *  specified by BeginEncodeImpl.
     *
     *  BeginEncodeImpl() has to be called before this function.
     *
     *  \param pBuffer pointer to a buffer with data to encode
     *  \param lLen length of data to encode.
     *
     *  Call EndEncodeImpl() after all data has been encoded
     *
     *
     *  \see BeginEncodeImpl
     *  \see EndEncodeImpl
     */
    virtual void EncodeBlockImpl( const char* pBuffer, pdf_long lLen );

    /** Check wether the decoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to decode data
     */
    inline virtual bool CanDecode() const; 

    /** Real implementation of `BeginDecode()'. NEVER call this method directly.
     *
     *  By default this function does nothing. If your filter needs to do setup for decoding,
     *  you should override this method.
     *
     *  PdfFilter ensures that a valid stream is available when this method is called, and
     *  that EndDecode() was called since the last BeginDecode()/DecodeBlock().
     *
     * \see BeginDecode */
    virtual void BeginDecodeImpl( const PdfDictionary* );

    /** Real implementation of `DecodeBlock()'. NEVER call this method directly.
     *
     *  You must override this method to decode the buffer passed by the caller.
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
     * \see DecodeBlock */
    virtual void DecodeBlockImpl( const char* pBuffer, pdf_long lLen );

    /** Real implementation of `EndDecode()'. NEVER call this method directly.
     *
     * By the time this method returns, all filtered data must be written to the stream
     * and the filter must be in a state where BeginDecode() can be safely called.
     *
     *  PdfFilter ensures that a valid stream is available when this method is
     *  called, and ensures that BeginDecodeImpl() has been called.
     *
     * \see EndDecode */
    virtual void EndDecodeImpl();

    /** GetType of this filter.
     *  \returns the GetType of this filter
     */
    inline virtual EPdfFilter GetType() const;

 private:
    char m_cDecodedByte;
    bool m_bLow;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfHexFilter::CanEncode() const
{
    return true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfHexFilter::CanDecode() const
{
    return true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfFilter PdfHexFilter::GetType() const
{
    return ePdfFilter_ASCIIHexDecode;
}

/** The Ascii85 filter.
 */
class PdfAscii85Filter : public PdfFilter {
 public:
    virtual ~PdfAscii85Filter() { }

    /** Check wether the encoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to encode data
     */
    inline virtual bool CanEncode() const; 

    /** Begin encoding data using this filter. Called by PdfFilter::BeginEncode.
     *
     *  \see EncodeBlockImpl
     *  \see EndEncodeImpl
     *  \see PdfFilter::BeginEncode
     */
    virtual void BeginEncodeImpl();

    /** Encode a block of data and write it to the PdfOutputStream
     *  specified by BeginEncodeImpl.
     *
     *  BeginEncodeImpl() has to be called before this function.
     *
     *  \param pBuffer pointer to a buffer with data to encode
     *  \param lLen length of data to encode.
     *
     *  Call EndEncodeImpl() after all data has been encoded
     *
     *
     *  \see BeginEncodeImpl
     *  \see EndEncodeImpl
     */
    virtual void EncodeBlockImpl( const char* pBuffer, pdf_long lLen );

    /**
     *  Finish encoding of data.
     *
     *  \see BeginEncodeImpl
     *  \see EncodeBlockImpl
     */
    virtual void EndEncodeImpl();

    /** Check wether the decoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to decode data
     */
    inline virtual bool CanDecode() const; 

    /** Real implementation of `BeginDecode()'. NEVER call this method directly.
     *
     *  By default this function does nothing. If your filter needs to do setup for decoding,
     *  you should override this method.
     *
     *  PdfFilter ensures that a valid stream is available when this method is called, and
     *  that EndDecode() was called since the last BeginDecode()/DecodeBlock().
     *
     * \see BeginDecode */
    virtual void BeginDecodeImpl( const PdfDictionary* );

    /** Real implementation of `DecodeBlock()'. NEVER call this method directly.
     *
     *  You must override this method to decode the buffer passed by the caller.
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
     * \see DecodeBlock */
    virtual void DecodeBlockImpl( const char* pBuffer, pdf_long lLen );

    /** Real implementation of `EndDecode()'. NEVER call this method directly.
     *
     * By the time this method returns, all filtered data must be written to the stream
     * and the filter must be in a state where BeginDecode() can be safely called.
     *
     *  PdfFilter ensures that a valid stream is available when this method is
     *  called, and ensures that BeginDecodeImpl() has been called.
     *
     * \see EndDecode */
    virtual void EndDecodeImpl();

    /** GetType of this filter.
     *  \returns the GetType of this filter
     */
    inline virtual EPdfFilter GetType() const;

 private:
    void EncodeTuple ( unsigned long tuple, int bytes );
    void WidePut( unsigned long tuple, int bytes ) const;

 private:
    int           m_count;
    unsigned long m_tuple;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfAscii85Filter::CanEncode() const
{
    return true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfAscii85Filter::CanDecode() const
{
    return true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfFilter PdfAscii85Filter::GetType() const
{
    return ePdfFilter_ASCII85Decode;
}

/** The flate filter.
 */
class PdfFlateFilter : public PdfFilter {
 public:
    PdfFlateFilter();
    virtual ~PdfFlateFilter();

    /** Check wether the encoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to encode data
     */
    inline virtual bool CanEncode() const; 

    /** Begin encoding data using this filter. Called by PdfFilter::BeginEncode.
     *
     *  \see EncodeBlockImpl
     *  \see EndEncodeImpl
     *  \see PdfFilter::BeginEncode
     */
    virtual void BeginEncodeImpl();

    /** Encode a block of data and write it to the PdfOutputStream
     *  specified by BeginEncodeImpl.
     *
     *  BeginEncodeImpl() has to be called before this function.
     *
     *  \param pBuffer pointer to a buffer with data to encode
     *  \param lLen length of data to encode.
     *
     *  Call EndEncodeImpl() after all data has been encoded
     *
     *
     *  \see BeginEncodeImpl
     *  \see EndEncodeImpl
     */
    virtual void EncodeBlockImpl( const char* pBuffer, pdf_long lLen );

    /**
     *  Finish encoding of data.
     *
     *  \see BeginEncodeImpl
     *  \see EncodeBlockImpl
     */
    virtual void EndEncodeImpl();

    /** Check wether the decoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to decode data
     */
    inline virtual bool CanDecode() const; 

    /** Real implementation of `BeginDecode()'. NEVER call this method directly.
     *
     *  By default this function does nothing. If your filter needs to do setup for decoding,
     *  you should override this method.
     *
     *  PdfFilter ensures that a valid stream is available when this method is called, and
     *  that EndDecode() was called since the last BeginDecode()/DecodeBlock().
     *
     *  \param pDecodeParms additional parameters for decoding data
     *
     * \see BeginDecode 
     */
    virtual void BeginDecodeImpl( const PdfDictionary* pDecodeParms );

    /** Real implementation of `DecodeBlock()'. NEVER call this method directly.
     *
     *  You must override this method to decode the buffer passed by the caller.
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
     * \see DecodeBlock */
    virtual void DecodeBlockImpl( const char* pBuffer, pdf_long lLen );

    /** Real implementation of `EndDecode()'. NEVER call this method directly.
     *
     * By the time this method returns, all filtered data must be written to the stream
     * and the filter must be in a state where BeginDecode() can be safely called.
     *
     *  PdfFilter ensures that a valid stream is available when this method is
     *  called, and ensures that BeginDecodeImpl() has been called.
     *
     * \see EndDecode */
    virtual void EndDecodeImpl();

    /** GetType of this filter.
     *  \returns the GetType of this filter
     */
    inline virtual EPdfFilter GetType() const;

 private:
    void EncodeBlockInternal( const char* pBuffer, pdf_long lLen, int nMode );

 private:
    unsigned char        m_buffer[PODOFO_FILTER_INTERNAL_BUFFER_SIZE];

    z_stream             m_stream;
    PdfPredictorDecoder* m_pPredictor;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfFlateFilter::CanEncode() const
{
    return true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfFlateFilter::CanDecode() const
{
    return true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfFilter PdfFlateFilter::GetType() const
{
    return ePdfFilter_FlateDecode;
}


/** The RLE filter.
 */
class PdfRLEFilter : public PdfFilter {
 public:
    virtual ~PdfRLEFilter() {}

    /** Check wether the encoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to encode data
     */
    inline virtual bool CanEncode() const; 

    virtual void BeginEncodeImpl();

    /** Encode a block of data and write it to the PdfOutputStream
     *  specified by BeginEncodeImpl.
     *
     *  BeginEncodeImpl() has to be called before this function.
     *
     *  \param pBuffer pointer to a buffer with data to encode
     *  \param lLen length of data to encode.
     *
     *  Call EndEncodeImpl() after all data has been encoded
     *
     *
     *  \see BeginEncodeImpl
     *  \see EndEncodeImpl
     */
    virtual void EncodeBlockImpl( const char* pBuffer, pdf_long lLen );

    /**
     *  Finish encoding of data.
     *
     *  \see BeginEncodeImpl
     *  \see EncodeBlockImpl
     */
    virtual void EndEncodeImpl();

    /** Check wether the decoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to decode data
     */
    inline virtual bool CanDecode() const; 

    /** Real implementation of `BeginDecode()'. NEVER call this method directly.
     *
     *  By default this function does nothing. If your filter needs to do setup for decoding,
     *  you should override this method.
     *
     *  PdfFilter ensures that a valid stream is available when this method is called, and
     *  that EndDecode() was called since the last BeginDecode()/DecodeBlock().
     *
     * \see BeginDecode */
    virtual void BeginDecodeImpl( const PdfDictionary* );

    /** Real implementation of `DecodeBlock()'. NEVER call this method directly.
     *
     *  You must override this method to decode the buffer passed by the caller.
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
     * \see DecodeBlock */
    virtual void DecodeBlockImpl( const char* pBuffer, pdf_long lLen );

    /** GetType of this filter.
     *  \returns the GetType of this filter
     */
    inline virtual EPdfFilter GetType() const;

 private:
    int m_nCodeLen;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfRLEFilter::CanEncode() const
{
    return false;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfRLEFilter::CanDecode() const
{
    return true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfFilter PdfRLEFilter::GetType() const
{
    return ePdfFilter_RunLengthDecode;
}

/** The LZW filter.
 */
class PdfLZWFilter : public PdfFilter {
    struct TLzwItem {
        std::vector<unsigned char> value;
    };
    
    typedef std::vector<TLzwItem>     TLzwTable;
    typedef TLzwTable::iterator       TILzwTable;
    typedef TLzwTable::const_iterator TCILzwTable;

 public:
    PdfLZWFilter();

    virtual ~PdfLZWFilter();

    /** Check wether the encoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to encode data
     */
    inline virtual bool CanEncode() const; 

    /** Begin encoding data using this filter. Called by PdfFilter::BeginEncode.
     *
     *  \see EncodeBlockImpl
     *  \see EndEncodeImpl
     *  \see PdfFilter::BeginEncode
     */
    virtual void BeginEncodeImpl();

    /** Encode a block of data and write it to the PdfOutputStream
     *  specified by BeginEncodeImpl.
     *
     *  BeginEncodeImpl() has to be called before this function.
     *
     *  \param pBuffer pointer to a buffer with data to encode
     *  \param lLen length of data to encode.
     *
     *  Call EndEncodeImpl() after all data has been encoded
     *
     *
     *  \see BeginEncodeImpl
     *  \see EndEncodeImpl
     */
    virtual void EncodeBlockImpl( const char* pBuffer, pdf_long lLen );

    /**
     *  Finish encoding of data.
     *
     *  \see BeginEncodeImpl
     *  \see EncodeBlockImpl
     */
    virtual void EndEncodeImpl();

    /** Check wether the decoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to decode data
     */
    inline virtual bool CanDecode() const; 

    /** Real implementation of `BeginDecode()'. NEVER call this method directly.
     *
     *  By default this function does nothing. If your filter needs to do setup for decoding,
     *  you should override this method.
     *
     *  PdfFilter ensures that a valid stream is available when this method is called, and
     *  that EndDecode() was called since the last BeginDecode()/DecodeBlock().
     *
     * \see BeginDecode */
    virtual void BeginDecodeImpl( const PdfDictionary* );

    /** Real implementation of `DecodeBlock()'. NEVER call this method directly.
     *
     *  You must override this method to decode the buffer passed by the caller.
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
     * \see DecodeBlock */
    virtual void DecodeBlockImpl( const char* pBuffer, pdf_long lLen );

    /** Real implementation of `EndDecode()'. NEVER call this method directly.
     *
     * By the time this method returns, all filtered data must be written to the stream
     * and the filter must be in a state where BeginDecode() can be safely called.
     *
     *  PdfFilter ensures that a valid stream is available when this method is
     *  called, and ensures that BeginDecodeImpl() has been called.
     *
     * \see EndDecode */
    virtual void EndDecodeImpl();

    /** GetType of this filter.
     *  \returns the GetType of this filter
     */
    inline virtual EPdfFilter GetType() const;

 private:
    /** Initialize an lzw table.
     */
    void InitTable();

 private:
    static const unsigned short s_masks[4];
    static const unsigned short s_clear;
    static const unsigned short s_eod;

    TLzwTable     m_table;

    unsigned int  m_mask;
    unsigned int  m_code_len;
    unsigned char m_character;

    bool          m_bFirst;

    PdfPredictorDecoder* m_pPredictor;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfLZWFilter::CanEncode() const
{
    return false;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfLZWFilter::CanDecode() const
{
    return true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfFilter PdfLZWFilter::GetType() const
{
    return ePdfFilter_LZWDecode;
}


#ifdef PODOFO_HAVE_JPEG_LIB

void PODOFO_API jpeg_memory_src (j_decompress_ptr cinfo, const JOCTET * buffer, size_t bufsize);

extern "C" {
void JPegErrorExit(j_common_ptr cinfo);

void JPegErrorOutput(j_common_ptr, int);
};

/** The DCT filter can decoded JPEG compressed data.
 *  
 *  This filter requires JPEG lib to be available
 */
class PdfDCTFilter : public PdfFilter {
 public:
    PdfDCTFilter();

    virtual ~PdfDCTFilter();

    /** Check wether the encoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to encode data
     */
    inline virtual bool CanEncode() const; 

    /** Begin encoding data using this filter. Called by PdfFilter::BeginEncode.
     *
     *  \see EncodeBlockImpl
     *  \see EndEncodeImpl
     *  \see PdfFilter::BeginEncode
     */
    virtual void BeginEncodeImpl();

    /** Encode a block of data and write it to the PdfOutputStream
     *  specified by BeginEncodeImpl.
     *
     *  BeginEncodeImpl() has to be called before this function.
     *
     *  \param pBuffer pointer to a buffer with data to encode
     *  \param lLen length of data to encode.
     *
     *  Call EndEncodeImpl() after all data has been encoded
     *
     *
     *  \see BeginEncodeImpl
     *  \see EndEncodeImpl
     */
    virtual void EncodeBlockImpl( const char* pBuffer, pdf_long lLen );

    /**
     *  Finish encoding of data.
     *
     *  \see BeginEncodeImpl
     *  \see EncodeBlockImpl
     */
    virtual void EndEncodeImpl();

    /** Check wether the decoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to decode data
     */
    inline virtual bool CanDecode() const; 

    /** Real implementation of `BeginDecode()'. NEVER call this method directly.
     *
     *  By default this function does nothing. If your filter needs to do setup for decoding,
     *  you should override this method.
     *
     *  PdfFilter ensures that a valid stream is available when this method is called, and
     *  that EndDecode() was called since the last BeginDecode()/DecodeBlock().
     *
     * \see BeginDecode */
    virtual void BeginDecodeImpl( const PdfDictionary* );

    /** Real implementation of `DecodeBlock()'. NEVER call this method directly.
     *
     *  You must override this method to decode the buffer passed by the caller.
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
     * \see DecodeBlock */
    virtual void DecodeBlockImpl( const char* pBuffer, pdf_long lLen );

    /** Real implementation of `EndDecode()'. NEVER call this method directly.
     *
     * By the time this method returns, all filtered data must be written to the stream
     * and the filter must be in a state where BeginDecode() can be safely called.
     *
     *  PdfFilter ensures that a valid stream is available when this method is
     *  called, and ensures that BeginDecodeImpl() has been called.
     *
     * \see EndDecode */
    virtual void EndDecodeImpl();

    /** GetType of this filter.
     *  \returns the GetType of this filter
     */
    inline virtual EPdfFilter GetType() const;

 private:
    struct jpeg_decompress_struct m_cinfo;
    struct jpeg_error_mgr         m_jerr;

    PdfRefCountedBuffer           m_buffer;
    PdfOutputDevice*              m_pDevice;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfDCTFilter::CanEncode() const
{
    return false;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfDCTFilter::CanDecode() const
{
    return true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfFilter PdfDCTFilter::GetType() const
{
    return ePdfFilter_DCTDecode;
}
#endif // PODOFO_HAVE_JPEG_LIB

#ifdef PODOFO_HAVE_TIFF_LIB

/** The CCITT filter can decoded CCITTFaxDecode compressed data.
 *  
 *  This filter requires TIFFlib to be available
 */
class PdfCCITTFilter : public PdfFilter {
 public:
    PdfCCITTFilter();

    virtual ~PdfCCITTFilter();

    /** Check wether the encoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to encode data
     */
    inline virtual bool CanEncode() const; 

    /** Begin encoding data using this filter. Called by PdfFilter::BeginEncode.
     *
     *  \see EncodeBlockImpl
     *  \see EndEncodeImpl
     *  \see PdfFilter::BeginEncode
     */
    virtual void BeginEncodeImpl();

    /** Encode a block of data and write it to the PdfOutputStream
     *  specified by BeginEncodeImpl.
     *
     *  BeginEncodeImpl() has to be called before this function.
     *
     *  \param pBuffer pointer to a buffer with data to encode
     *  \param lLen length of data to encode.
     *
     *  Call EndEncodeImpl() after all data has been encoded
     *
     *
     *  \see BeginEncodeImpl
     *  \see EndEncodeImpl
     */
    virtual void EncodeBlockImpl( const char* pBuffer, pdf_long lLen );

    /**
     *  Finish encoding of data.
     *
     *  \see BeginEncodeImpl
     *  \see EncodeBlockImpl
     */
    virtual void EndEncodeImpl();

    /** Check wether the decoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to decode data
     */
    inline virtual bool CanDecode() const; 

    /** Real implementation of `BeginDecode()'. NEVER call this method directly.
     *
     *  By default this function does nothing. If your filter needs to do setup for decoding,
     *  you should override this method.
     *
     *  PdfFilter ensures that a valid stream is available when this method is called, and
     *  that EndDecode() was called since the last BeginDecode()/DecodeBlock().
     *
     * \see BeginDecode */
    virtual void BeginDecodeImpl( const PdfDictionary* );

    /** Real implementation of `DecodeBlock()'. NEVER call this method directly.
     *
     *  You must override this method to decode the buffer passed by the caller.
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
     * \see DecodeBlock */
    virtual void DecodeBlockImpl( const char* pBuffer, pdf_long lLen );

    /** Real implementation of `EndDecode()'. NEVER call this method directly.
     *
     * By the time this method returns, all filtered data must be written to the stream
     * and the filter must be in a state where BeginDecode() can be safely called.
     *
     *  PdfFilter ensures that a valid stream is available when this method is
     *  called, and ensures that BeginDecodeImpl() has been called.
     *
     * \see EndDecode */
    virtual void EndDecodeImpl();

    /** GetType of this filter.
     *  \returns the GetType of this filter
     */
    inline virtual EPdfFilter GetType() const;

 private:
    TIFF* m_tiff;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfCCITTFilter::CanEncode() const
{
    return false;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfCCITTFilter::CanDecode() const
{
    return true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfFilter PdfCCITTFilter::GetType() const
{
    return ePdfFilter_CCITTFaxDecode;
}
#endif // PODOFO_HAVE_TIFF_LIB

};


#endif /* _PDF_FILTERS_PRIVATE_H_ */
