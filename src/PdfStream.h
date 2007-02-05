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

#ifndef _PDF_STREAM_H_
#define _PDF_STREAM_H_

#include "PdfDefines.h"

#include "PdfDictionary.h"
#include "PdfRefCountedBuffer.h"

namespace PoDoFo {

typedef std::vector<EPdfFilter>            TVecFilters;
typedef TVecFilters::iterator              TIVecFilters;
typedef TVecFilters::const_iterator        TCIVecFilters;

class PdfName;
class PdfObject;

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
     */
    virtual void Write( PdfOutputDevice* pDevice ) = 0;

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
    virtual void Set( char* szBuffer, long lLen, bool takePossession = true ) = 0;

    /** Set a null-terminated char* buffer  as the streams contents.
     *
     *  The string will be copied into a newly allocated buffer.
     *  \param pszString a zero terminated string buffer containing only ASCII text data
     *  \returns ErrOk on sucess
     */
    inline void Set( const char* pszString );

    /** Append a binary buffer to the current stream contents.
     *  \param pszString a buffer
     *  \param lLen length of the buffer
     *  \returns ErrOk on sucess
     */
    virtual void Append( const char* pszString, size_t lLen ) = 0; 

    /** Append a null-terminated string to the current stream contents. 
     *
     *  \param pszString a zero terminated string buffer containing only ASCII text data
     *  \returns ErrOk on sucess
     */
    inline void Append( const char* pszString ); 

    /** Append to the current stream contents.
     *
     *  \param sString a std::string containing ASCII text data
     *  \returns ErrOk on sucess
     */
    inline void Append( const std::string& sString ); 

    /** Get the stream's length with all filters applied (eg if the stream is
     * Flate compressed, the length of the compressed data stream).
     *
     *  \returns the length of the internal buffer
     */
    virtual unsigned long GetLength() const = 0;

    /** Create a copy of a PdfStream object
     *  \param rhs the object to clone
     *  \returns a reference to this object
     */
    //const PdfStream & operator=( const PdfStream & rhs );

 protected:
    PdfObject*          m_pParent;
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

};

#endif // _PDF_STREAM_H_
