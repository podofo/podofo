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
     */
    virtual void Write( PdfOutputDevice* pDevice );

    /** Set a binary buffer as stream data, optionally taking ownership of the buffer.
     *
     *  \warning If takePossession is true, the stream must be allocated using
     *           malloc, as free() will be used to release it.
     *
     *  \param szBuffer buffer containing the stream data
     *  \param lLen length of the buffer
     *  \param takePossession does the stream now own this buffer...
     *  \returns ErrOk
     */
    virtual void Set( char* szBuffer, long lLen, bool takePossession = true );

    /** Append to the current stream contents. 
     *  \param pszString a buffer
     *  \param lLen length of the buffer
     *  \returns ErrOk on sucess
     */
    virtual void Append( const char* pszString, size_t lLen ); 

    /** Get the streams length
     *  \returns the length of the internal buffer
     */
    inline virtual unsigned long GetLength() const;

 private:
    PdfOutputDevice* m_pDevice;

    unsigned long    m_lLength;
    unsigned long    m_lOffset;

    PdfObject*       m_pLength;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned long PdfFileStream::GetLength() const
{
    return m_lLength;
}

};

#endif // _PDF_FILE_STREAM_H_
