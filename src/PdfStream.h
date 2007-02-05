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

    /** Get a malloced buffer of the current stream.
     *  No filters will be applied to the buffer, so
     *  if the stream is Flate compressed the compressed copy
     *  will be returned.
     *
     *  The caller has to free() the buffer.
     *
     *  \param pBuffer pointer to the buffer
     *  \param lLen    pointer to the buffer length
     *  \returns ErrOk on success.
     */
    virtual void GetCopy( char** pBuffer, long* lLen ) const = 0;

    /** Get a malloced buffer of the current stream which has been
     *  filtered by all filters as specified in the dictionary's
     *  /Filter key. For example, if the stream is Flate compressed,
     *  the buffer returned from this method will have been decompressed.
     *
     *  The caller has to free() the buffer.
     *
     *  \param pBuffer pointer to the buffer
     *  \param lLen    pointer to the buffer length
     *  \returns ErrOk on success.
     */
    void GetFilteredCopy( char** pBuffer, long* lLen ) const;

    /** Create a copy of a PdfStream object
     *  \param rhs the object to clone
     *  \returns a reference to this object
     */
    //const PdfStream & operator=( const PdfStream & rhs );

 protected:
    /** Required for the GetFilteredCopy implementation
     *  \returns a handle to the internal buffer
     */
    virtual const char* GetInternalBuffer() const = 0;

    /** Required for the GetFilteredCopy implementation
     *  \returns the size of the internal buffer
     */
    virtual unsigned long GetInternalBufferSize() const = 0;

    /** Reads the /Filters key from the current directory
     *  and adds every found filter to the vector.
     *  \param vecFilters add all filters to this vector
     *  \returns ErrOk on success
     */
    void FillFilterList( TVecFilters & vecFilters ) const;

    /** Converts a filter name to the corresponding enum
     *  \param name of the filter without leading
     *  \returns the filter as enum
     */
    static EPdfFilter FilterNameToType( const PdfName & name );

    /** Get a list of extra decode parameters for this dictionary.
     *  The list contains copies of the objects and has to be deleted by the caller! 
     *  \returns ErrOk on success
     */
    void GetDecodeParms( TVecDictionaries* pParams ) const;

    /** Set a list of extra decode parameters for this dictionary. Replace any old
     *  decode paramaters with this.
     *
     *  This function may change pParams->SetAutoDelete!
     *
     *  \param pParams a list of decode parameter dictioniers, may contain null pointers
     *  \returns ErrOk on success
     */
    void SetDecodeParms( TVecDictionaries* pParams );

    /** Deletes all dictionaries in the vector
     *  \param pParams delete all dictionaries in this vector
     */
    void FreeDecodeParms( TVecDictionaries* pParams ) const;

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
