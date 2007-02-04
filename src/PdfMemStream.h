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

#ifndef _PDF_MEM_STREAM_H_
#define _PDF_MEM_STREAM_H_

#include "PdfDefines.h"

#include "PdfStream.h"
#include "PdfDictionary.h"
#include "PdfRefCountedBuffer.h"

namespace PoDoFo {

class PdfName;
class PdfObject;

/** A PDF stream can be appended to any PdfObject
 *  and can contain abitrary data.
 *
 *  A PDF memory stream is hold completely in memory.
 *
 *  Most of the time it will contain either drawing commands
 *  to draw onto a page or binary data like a font or an image.
 *
 *  A PdfMemStream is implicitly shared and can be copied very fast therefore.
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

    /** Create a copy of a PdfStream object
     *  \param rhs the object to clone
     */
    PdfMemStream( const PdfMemStream & rhs );

    ~PdfMemStream();

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
    void Append( const char* pszString, size_t lLen ); 

    /** Get a malloced buffer of the current stream.
     *  No filters will be applied to the buffer.
     *  The caller has to free the buffer.
     *  \param pBuffer pointer to the buffer
     *  \param lLen    pointer to the buffer length
     *  \returns ErrOk on success.
     */
    void GetCopy( char** pBuffer, long* lLen ) const;

    /** Get a malloced buffer of the current stream which has been
     *  filtered by all filters as specified in the dictioniries
     *  filter key. The caller has to free the buffer.
     *  \param pBuffer pointer to the buffer
     *  \param lLen    pointer to the buffer length
     *  \returns ErrOk on success.
     */
    void GetFilteredCopy( char** pBuffer, long* lLen ) const;

    /** Get a read-only handle to the currenct stream data.
     *  \returns a read-only handle to the streams data
     */
    inline const char* Get() const;

    /** Get the streams length
     *  \returns the length of the internal buffer
     *  \see Get()
     */
    inline unsigned long GetLength() const;

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
     *  \param rhs the object to clone
     *  \returns a reference to this object
     */
    const PdfStream & operator=( const PdfMemStream & rhs );

 private:
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

    /** Compress the current data using the FlateDecode(ZIP) algorithm
     *  Expects that all filters are setup correctly.
     *  \returns ErrOk on success
     */
    void FlateCompressStreamData();

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

 private:
    PdfRefCountedBuffer m_buffer;
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
unsigned long PdfMemStream::GetLength() const
{
    return m_buffer.GetSize();
}

};

#endif // _PDF_MEM_STREAM_H_
