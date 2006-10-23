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
class PODOFO_API PdfRefCountedBuffer {
 public:
    /** Created an empty reference counted buffer
     *  The buffer will be initialize to NULL
     */
    PdfRefCountedBuffer();

    /** Created an reference counted buffer and use an exiting buffer
     *  The buffer will be owned by this object.
     *
     *  \param pBuffer a pointer to an allocated buffer
     *  \param lSize   size of the allocated buffer
     *
     *  \see SetTakePossesion
     */
    PdfRefCountedBuffer( char* pBuffer, long lSize );

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
    
    /** Append to the current buffers contents. 
     *  If the buffer is referenced by another PdfRefCountedBuffer
     *  this object detaches from this buffer and allocates an own
     *  buffer. Otherwise the internal buffer is used and resized if
     *  necessary.
     *
     *  \param pszString a buffer
     *  \param lLen length of the buffer
     */
    void Append( const char* pszString, long lLen ); 

    /** Get access to the buffer
     *  \returns the buffer
     */
    inline char* GetBuffer() const;

    /** Return the buffer size
     *  \returns the buffer size
     */
    inline long GetSize() const;

    /** Copy an existing PdfRefCountedBuffer and increase
     *  the reference count
     *  \param rhs the PdfRefCountedBuffer to copy
     *  \returns the copied object
     */
    const PdfRefCountedBuffer & operator=( const PdfRefCountedBuffer & rhs );

    /** If the PdfRefCountedBuffer has no possesion on its buffer,
     *  it won't delete the buffer. By default the buffer is owned
     *  and deleted by the PdfRefCountedBuffer object.
     *
     *  \param bTakePossession if false the buffer will not be deleted.
     */
    inline void SetTakePossesion( bool bTakePossession );

    /** 
     * \returns true if the buffer is owned by the PdfRefCountedBuffer
     *               and is deleted along with it.
     */
    inline bool TakePossesion() const;

    /** Compare to buffers.
     *  \param rhs compare to this buffer
     *  \returns true if both buffers contain the same contents
     */
    bool operator==( const PdfRefCountedBuffer & rhs ) const;

 private:
    /** Detach from the reference counted buffer
     */
    void Detach();

 private:
    typedef struct TRefCountedBuffer {
        char* m_pBuffer;
        long  m_lSize;
        long  m_lInternalSize;
        long  m_lRefCount;
        bool  m_bPossesion;
    };

    TRefCountedBuffer* m_pBuffer;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline char* PdfRefCountedBuffer::GetBuffer() const
{
    return m_pBuffer ? m_pBuffer->m_pBuffer : NULL;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline long PdfRefCountedBuffer::GetSize() const
{
    return m_pBuffer ? m_pBuffer->m_lSize : 0;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfRefCountedBuffer::SetTakePossesion( bool bTakePossession )
{
    if( m_pBuffer )
        m_pBuffer->m_bPossesion = bTakePossession;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfRefCountedBuffer::TakePossesion() const
{
    return m_pBuffer ? m_pBuffer->m_bPossesion : false;
}

};

#endif // _PDF_REF_COUNTED_BUFFER_H_

