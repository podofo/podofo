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

#ifndef _PDF_MEM_STREAM_H_
#define _PDF_MEM_STREAM_H_

#include "PdfDefines.h"

#include "PdfStream.h"
#include "PdfDictionary.h"
#include "PdfRefCountedBuffer.h"

namespace PoDoFo {

class PdfBufferOutputStream;
class PdfName;
class PdfObject;

/** A PDF stream can be appended to any PdfObject
 *  and can contain abitrary data.
 *
 *  A PDF memory stream is held completely in memory.
 *
 *  Most of the time it will contain either drawing commands
 *  to draw onto a page or binary data like a font or an image.
 *
 *  A PdfMemStream is implicitly shared and can therefore be copied very quickly.
 */
class PODOFO_API PdfMemStream : public PdfStream {
    friend class PdfVecObjects;

 public:

    /** Create a new PdfStream object which has a parent PdfObject.
     *  The stream will be deleted along with the parent.
     *  This constructor will be called by PdfObject::Stream() for you.
     *  \param pParent parent object
     */
    PdfMemStream( PdfObject* pParent );

    /** Create a shallow copy of a PdfStream object
     *
     *  \param rhs the object to clone
     */
    PdfMemStream( const PdfMemStream & rhs );

    ~PdfMemStream();

    /** Write the stream to an output device
     *  \param pDevice write to this outputdevice.
     *  \param pEncrypt encrypt stream data using this object
     */
    virtual void Write( PdfOutputDevice* pDevice, PdfEncrypt* pEncrypt = NULL );

    /** Get a malloced buffer of the current stream.
     *  No filters will be applied to the buffer, so
     *  if the stream is Flate compressed the compressed copy
     *  will be returned.
     *
     *  The caller has to podofo_free() the buffer.
     *
     *  \param pBuffer pointer to where the buffer's address will be stored
     *  \param lLen    pointer to the buffer length (output parameter)
     */
    virtual void GetCopy( char** pBuffer, pdf_long* lLen ) const;

    /** Get a copy of a the stream and write it to a PdfOutputStream
     *
     *  \param pStream data is written to this stream.
     */
    virtual void GetCopy( PdfOutputStream* pStream ) const;

    /** Get a read-only handle to the current stream data.
     *  The data will not be filtered before being returned, so (eg) calling
     *  Get() on a Flate compressed stream will return a pointer to the
     *  Flate-compressed buffer.
     *
     *  \warning Do not retain pointers to the stream's internal buffer,
     *           as it may be reallocated with any non-const operation.
     *
     *  \returns a read-only handle to the streams data
     */
    inline const char* Get() const;

    /** Get the stream's length. The length is that of the internal
     *  stream buffer, so (eg) for a Flate-compressed stream it will be
     *  the length of the compressed data.
     *
     *  \returns the length of the internal buffer
     *  \see Get()
     */
    virtual pdf_long GetLength() const;

    /** This function compresses any currently set stream
     *  using the FlateDecode(ZIP) algorithm. JPEG compressed streams
     *  will not be compressed again using this function.
     *  Entries to the filter dictionary will be added if necessary.
     */
    void FlateCompress();

    /** This method removes all filters from the stream
     */
    void Uncompress();

    /** Empty's the stream and set the streams buffer size to 0
     */
    void Empty();

    /** Create a copy of a PdfStream object
     *
     *  \param rhs the object to clone
     *  \returns a reference to this object
     */
    const PdfStream & operator=( const PdfStream & rhs );

 protected:
    /** Required for the GetFilteredCopy implementation
     *  \returns a handle to the internal buffer
     */
    inline virtual const char* GetInternalBuffer() const;

    /** Required for the GetFilteredCopy implementation
     *  \returns the size of the internal buffer
     */
    inline virtual pdf_long GetInternalBufferSize() const;

    /** Begin appending data to this stream.
     *  Clears the current stream contents.
     *
     *  \param vecFilters use this filters to encode any data written to the stream.
     */
    virtual void BeginAppendImpl( const TVecFilters & vecFilters );

    /** Append a binary buffer to the current stream contents.
     *
     *  \param pszString a buffer
     *  \param lLen length of the buffer
     *
     *  \see BeginAppend
     *  \see Append
     *  \see EndAppend
     */
    virtual void AppendImpl( const char* pszString, size_t lLen ); 

    /** Finish appending data to the stream
     */
    virtual void EndAppendImpl();

 private:
    /** Compress the current data using the FlateDecode (zlib) algorithm
     *  Expects that all filters are setup correctly.
     */
    void FlateCompressStreamData();


 private:
    PdfRefCountedBuffer    m_buffer;
    PdfOutputStream*       m_pStream;
    PdfBufferOutputStream* m_pBufferStream;

    pdf_long               m_lLength;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
const char* PdfMemStream::Get() const
{
    return m_buffer.GetBuffer();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const char* PdfMemStream::GetInternalBuffer() const
{
    return m_buffer.GetBuffer();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
pdf_long PdfMemStream::GetInternalBufferSize() const
{
    return m_lLength;
}

};

#endif // _PDF_MEM_STREAM_H_
