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

    /** Set an encryption object which is used to encrypt
     *  all data written to this stream.
     *
     *  \param pEncrypt an encryption object or NULL if no encryption should be done
     */
    void SetEncrypted( PdfEncrypt* pEncrypt ); 

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
     *  This is currently not implemented for PdfFileStreams 
     *  and will raise an ePdfError_InternalLogic exception
     *
     *  \param pBuffer pointer to the buffer address (output parameter)
     *  \param lLen    pointer to the buffer length  (output parameter)
     */
    virtual void GetCopy( char** pBuffer, pdf_long* lLen ) const;

    /** Get a copy of a the stream and write it to a PdfOutputStream
     *
     *  \param pStream data is written to this stream.
     */
    virtual void GetCopy( PdfOutputStream* pStream ) const;

    /** Get the streams length with all filters applied (eg the compressed
     *  length of a Flate compressed stream).
     *
     *  \returns the length of the stream with all filters applied
     */
    inline virtual pdf_long GetLength() const;

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
    PdfOutputDevice* m_pDevice;
    PdfOutputStream* m_pStream;
    PdfOutputStream* m_pDeviceStream;
    PdfOutputStream* m_pEncryptStream;

    pdf_long    m_lLenInitial;
    pdf_long    m_lLength;
    

    PdfObject*       m_pLength;

    PdfEncrypt*      m_pCurEncrypt;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
pdf_long PdfFileStream::GetLength() const
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
pdf_long PdfFileStream::GetInternalBufferSize() const
{
    return 0;
}

};

#endif // _PDF_FILE_STREAM_H_
