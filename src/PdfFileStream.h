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
 ***************************************************************************/

#ifndef _PDF_FILE_STREAM_H_
#define _PDF_FILE_STREAM_H_

#include "PdfDefines.h"

#include "PdfStream.h"

namespace PoDoFo {

class PdfOutputStream;

/** A PDF stream can be appended to any PdfObject
 *  and can contain arbitrary data.
 *
 *  Most of the time it will contain either drawing commands
 *  to draw onto a page or binary data like a font or an image.
 *
 *  A PdfFileStream writes all data directly to an output device
 *  without keeping it in memory.
 *  PdfFileStream is used automatically when creating PDF files
 *  using PdfImmediateWriter.
 *
 *  \see PdfVecObjects
 *  \see PdfStream
 *  \see PdfMemoryStream
 *  \see PdfFileStream
 */
class PODOFO_API PdfFileStream : public PdfStream {

 public:
    /** Create a new PdfFileStream object which has a parent PdfObject.
     *  The stream will be deleted along with the parent.
     *  This constructor will be called by PdfObject::Stream() for you.
     *
     *  \param pParent parent object
     *  \param pDevice output device
     */
    PdfFileStream( PdfObject* pParent, PdfOutputDevice* pDevice );

    virtual ~PdfFileStream();

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
     *  The caller has to free() the buffer.
     *
     *  This is currently not implemented for PdfFileStreams 
     *  and will raise an ePdfError_InternalLogic exception
     *
     *  \param pBuffer pointer to the buffer
     *  \param lLen    pointer to the buffer length
     *  \returns ErrOk on success.
     */
    virtual void GetCopy( char** pBuffer, long* lLen ) const;

    /** Get the streams length with all filters applied (eg the compressed
     *  length of a Flate compressed stream).
     *
     *  \returns the length of the stream with all filters applied
     */
    inline virtual unsigned long GetLength() const;

 protected:
    /** Required for the GetFilteredCopy implementation
     *  \returns a handle to the internal buffer
     */
    inline virtual const char* GetInternalBuffer() const;

    /** Required for the GetFilteredCopy implementation
     *  \returns the size of the internal buffer
     */
    inline virtual unsigned long GetInternalBufferSize() const;

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
    PdfOutputDevice* m_pDevice;
    PdfOutputStream* m_pStream;
    PdfOutputStream* m_pDeviceStream;

    unsigned long    m_lLenInitial;
    unsigned long    m_lLength;
    

    PdfObject*       m_pLength;

    PdfEncrypt*      m_pCurEncrypt;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned long PdfFileStream::GetLength() const
{
    return m_lLength;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const char* PdfFileStream::GetInternalBuffer() const
{
    return NULL;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned long PdfFileStream::GetInternalBufferSize() const
{
    return 0;
}

};

#endif // _PDF_FILE_STREAM_H_
