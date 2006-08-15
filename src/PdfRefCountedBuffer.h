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
 ***************************************************************************/

#ifndef _PDF_REF_COUNTED_BUFFER_H_
#define _PDF_REF_COUNTED_BUFFER_H_

#include "PdfDefines.h"

namespace PoDoFo {

/** 
 * A reference counted buffer object
 * which is deleted as soon as the last
 * object having access to it is delteted.
 */
class PdfRefCountedBuffer {
 public:
    /** Created an empty reference counted file object
     *  The file handle will be initialize to NULL
     */
    PdfRefCountedBuffer();

    /** Create a new PdfRefCountedBuffer. 
     *  \param lSize buffer size
     */
    PdfRefCountedBuffer( long lSize );

    /** Copy an existing PdfRefCountedBuffer and increase
     *  the reference count
     *  \param rhs the PdfRefCountedBuffer to copy
     */
    PdfRefCountedBuffer( const PdfRefCountedBuffer & rhs );

    /** Decrease the reference count and delete the buffer
     *  if this is the last owner
     */
    ~PdfRefCountedBuffer();

    /** Get access to the buffer
     *  \returns the buffer
     */
    inline char* Buffer() const;

    /** Return the buffer size
     *  \returns the buffer size
     */
    inline long Size() const;

    /** Copy an existing PdfRefCountedBuffer and increase
     *  the reference count
     *  \param rhs the PdfRefCountedBuffer to copy
     *  \returns the copied object
     */
    const PdfRefCountedBuffer & operator=( const PdfRefCountedBuffer & rhs );

 private:
    /** Detach from the reference counted buffer
     */
    void Detach();

 private:
    typedef struct TRefCountedBuffer {
        char* m_pBuffer;
        long  m_lSize;
        long  m_lRefCount;
    };

    TRefCountedBuffer* m_pBuffer;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline char* PdfRefCountedBuffer::Buffer() const
{
    return m_pBuffer ? m_pBuffer->m_pBuffer : NULL;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline long PdfRefCountedBuffer::Size() const
{
    return m_pBuffer ? m_pBuffer->m_lSize : 0;
}

};

#endif // _PDF_REF_COUNTED_BUFFER_H_

