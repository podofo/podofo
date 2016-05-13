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

#ifndef _PDF_STREAM_H_
#define _PDF_STREAM_H_

#include "PdfDefines.h"

#include "PdfDictionary.h"
#include "PdfFilter.h"
#include "PdfRefCountedBuffer.h"

#include <string.h>

namespace PoDoFo {

class PdfInputStream;
class PdfName;
class PdfObject;
class PdfOutputStream;

/** A PDF stream can be appended to any PdfObject
 *  and can contain arbitrary data.
 *
 *  Most of the time it will contain either drawing commands
 *  to draw onto a page or binary data like a font or an image.
 *
 *  You have to use a concrete implementation of a stream,
 *  which can be retrieved from a StreamFactory.
 *  \see PdfVecObjects
 *  \see PdfMemoryStream
 *  \see PdfFileStream
 */
class PODOFO_API PdfStream {

 public:
    /** Create a new PdfStream object which has a parent PdfObject.
     *  The stream will be deleted along with the parent.
     *  This constructor will be called by PdfObject::Stream() for you.
     *  \param pParent parent object
     */
    PdfStream( PdfObject* pParent );

    virtual ~PdfStream();

    /** Write the stream to an output device
     *  \param pDevice write to this outputdevice.
     *  \param pEncrypt encrypt stream data using this object
     */
    virtual void Write( PdfOutputDevice* pDevice, PdfEncrypt* pEncrypt = NULL ) = 0;

    /** Set a binary buffer as stream data.
     *
     * Use PdfFilterFactory::CreateFilterList() if you want to use the contents
     * of the stream dictionary's existing filter key.
     *
     *  \param szBuffer buffer containing the stream data
     *  \param lLen length of the buffer
     *  \param vecFilters a list of filters to use when appending data
     */
    void Set( const char* szBuffer, pdf_long lLen, const TVecFilters & vecFilters );

    /** Set a binary buffer as stream data.
     *  All data will be Flate-encoded.
     *
     *  \param szBuffer buffer containing the stream data
     *  \param lLen length of the buffer
     */
    void Set( const char* szBuffer, pdf_long lLen );

    /** Set a binary buffer whose contents are read from a PdfInputStream
     *  All data will be Flate-encoded.
     * 
     *  \param pStream read stream contents from this PdfInputStream
     */
    void Set( PdfInputStream* pStream );

    /** Set a binary buffer whose contents are read from a PdfInputStream
     * 
     * Use PdfFilterFactory::CreateFilterList() if you want to use the contents
     * of the stream dictionary's existing filter key.
     *
     *  \param pStream read stream contents from this PdfInputStream
     *  \param vecFilters a list of filters to use when appending data
     */
    void Set( PdfInputStream* pStream, const TVecFilters & vecFilters );

    /** Set a null-terminated char* buffer as the stream's contents.
     *
     *  The string will be copied into a newly allocated buffer.
     *  \param pszString a zero terminated string buffer containing only
     *         ASCII text data
     */
    inline void Set( const char* pszString );

    /** Sets raw data for this stream which is read from an input stream.
     *  This method does neither encode nor decode the read data.
     *  The filters of the object are not modified and the data is expected
     *  encoded as stated by the /Filters key in the stream's object.
     *
     *  \param pStream read data from this input stream
     *  \param lLen    read exactly lLen bytes from the input stream,
     *                 if lLen = -1 read until the end of the input stream
     *                 was reached.
     */
    void SetRawData( PdfInputStream* pStream, pdf_long lLen = -1 );

    /** Start appending data to this stream.
     *
     *  This method has to be called before any of the append methods.
     *  All appended data will be Flate-encoded.
     *
     *  \param bClearExisting if true any existing stream contents will
     *         be cleared.
     *
     *  \see Append
     *  \see EndAppend
     */
    void BeginAppend( bool bClearExisting = true );

    /** Start appending data to this stream.
     *  This method has to be called before any of the append methods.
     *
     * Use PdfFilterFactory::CreateFilterList() if you want to use the contents
     * of the stream dictionary's existing filter key.
     *
     *  \param vecFilters a list of filters to use when appending data
     *  \param bClearExisting if true any existing stream contents will
               be cleared.
     *  \param bDeleteFilters if true existing filter keys are deleted if an
     *         empty list of filters is passed (required for SetRawData())
     *
     *  \see Append
     *  \see EndAppend
     */
    void BeginAppend( const TVecFilters & vecFilters, bool bClearExisting = true, bool bDeleteFilters = true );

    /** Append a binary buffer to the current stream contents.
     *
     *  Make sure BeginAppend() has been called before.
     *
     *  \param pszString a buffer
     *  \param lLen length of the buffer
     *
     *  \see BeginAppend
     *  \see EndAppend
     */
    inline void Append( const char* pszString, size_t lLen ); 

    /** Append a null-terminated string to the current stream contents. 
     *
     *  Make sure BeginAppend() has been called before.
     *
     *  \param pszString a zero-terminated string buffer containing only
     *         ASCII text data
     *
     *  \see BeginAppend
     *  \see EndAppend
     */
    inline void Append( const char* pszString ); 

    /** Append to the current stream contents.
     *
     *  Make sure BeginAppend() has been called before.
     *
     *  \param sString a std::string containing only ASCII text data
     *
     *  \see BeginAppend
     *  \see EndAppend
     */
    inline void Append( const std::string& sString ); 

    /** Finish appending data to this stream.
     *  BeginAppend() has to be called before this method.
     *
     *  \see BeginAppend
     *  \see Append
     */
    void EndAppend();

    /**
     * \returns true if code is between BeginAppend()
     *          and EndAppend() at the moment, i.e. it
     *          is safe to call EndAppend() now.
     *
     *  \see BeginAppend
     *  \see Append
     */
    inline bool IsAppending() const;

    /** Get the stream's length with all filters applied (e.g. if the stream is
     * Flate-compressed, the length of the compressed data stream).
     *
     *  \returns the length of the internal buffer
     */
    virtual pdf_long GetLength() const = 0;

    /** Get a malloc()'d buffer of the current stream.
     *  No filters will be applied to the buffer, so
     *  if the stream is Flate-compressed the compressed copy
     *  will be returned.
     *
     *  The caller has to podofo_free() the buffer.
     *
     *  \param pBuffer pointer to the buffer
     *  \param lLen    pointer to the buffer length
     */
    virtual void GetCopy( char** pBuffer, pdf_long* lLen ) const = 0;

    /** Get a copy of a the stream and write it to a PdfOutputStream
     *
     *  \param pStream data is written to this stream.
     */
    virtual void GetCopy( PdfOutputStream* pStream ) const = 0;

    /** Get a malloc()'d buffer of the current stream which has been
     *  filtered by all filters as specified in the dictionary's
     *  /Filter key. For example, if the stream is Flate-compressed,
     *  the buffer returned from this method will have been decompressed.
     *
     *  The caller has to podofo_free() the buffer.
     *
     *  \param pBuffer pointer to the buffer
     *  \param lLen    pointer to the buffer length
     */
    void GetFilteredCopy( char** pBuffer, pdf_long* lLen ) const;

    /** Get a filtered copy of a the stream and write it to a PdfOutputStream
     *  
     *  \param pStream filtered data is written to this stream.
     */
    void GetFilteredCopy( PdfOutputStream* pStream ) const;
    
    /** Create a copy of a PdfStream object
     *  \param rhs the object to clone
     *  \returns a reference to this object
     */
    const PdfStream & operator=( const PdfStream & rhs );

 protected:
    /** Required for the GetFilteredCopy() implementation
     *  \returns a handle to the internal buffer
     */
    virtual const char* GetInternalBuffer() const = 0;

    /** Required for the GetFilteredCopy() implementation
     *  \returns the size of the internal buffer
     */
    virtual pdf_long GetInternalBufferSize() const = 0;

    /** Begin appending data to this stream.
     *  Clears the current stream contents.
     *
     * Use PdfFilterFactory::CreateFilterList() if you want to use the contents
     * of the stream dictionary's existing filter key.
     *
     *  \param vecFilters use these filters to encode any data written
     *         to the stream.
     */
    virtual void BeginAppendImpl( const TVecFilters & vecFilters ) = 0;

    /** Append a binary buffer to the current stream contents.
     *
     *  \param pszString a buffer
     *  \param lLen length of the buffer
     *
     *  \see BeginAppend
     *  \see Append
     *  \see EndAppend
     */
    virtual void AppendImpl( const char* pszString, size_t lLen ) = 0; 

    /** Finish appending data to the stream
     */
    virtual void EndAppendImpl() = 0;

 protected:
    PdfObject*          m_pParent;

    bool                m_bAppend;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStream::Set( const char* pszString )
{
    if( pszString ) 
        Set( const_cast<char*>(pszString), strlen( pszString ) );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStream::Append( const char* pszString, size_t lLen )
{
    PODOFO_RAISE_LOGIC_IF( !m_bAppend, "Append() failed because BeginAppend() was not yet called!" );

    this->AppendImpl( pszString, lLen );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStream::Append( const char* pszString )
{
    if( pszString )
        Append( pszString, strlen( pszString ) );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfStream::Append( const std::string& sString ) 
{
    Append( sString.c_str(), sString.length() );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfStream::IsAppending() const
{
    return m_bAppend;
}

};

#endif // _PDF_STREAM_H_
