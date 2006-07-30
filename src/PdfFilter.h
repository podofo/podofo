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

#ifndef _PDF_FILTER_H_
#define _PDF_FILTER_H_

#include "PdfDefines.h"
#include <map>

namespace PoDoFo {

class PdfDictionary;
struct TFlatePredictorParams;

/** The interface that every PdfFilter has to implement.
 *  The two methods Encode and Decode have to be implemented 
 *  for every filter.
 *
 *  The output buffers are malloc'ed in the functions and have
 *  to be free'd by the caller.
 */
class PdfFilter {
 public:
    /** Encodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the encoded data
     *  \param plOutLen pointer to the length of the output buffer
     */
    virtual void Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const = 0;

    /** Decodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the decoded data
     *  \param plOutLen pointer to the length of the output buffer
     *  \param pDecodeParms optional pointer to an decode parameters dictionary
     *                      containing additional information to decode the data.
     *                      This pointer must be NULL if no decode parameter dictionary
     *                      is available.
     */
    virtual void Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen, const PdfDictionary* pDecodeParms = NULL ) const = 0;

    /** Type of this filter.
     *  \returns the type of this filter
     */
    virtual EPdfFilter type() const = 0;
};

/** A factory to create a filter object for a filter type from the EPdfFilter enum.
 */
class PdfFilterFactory {
 public:
    /** Create a filter from an enum. 
     *  The filter is cached and may not be delted!
     *
     *  \param eFilter the type of filter that should be created.
     *
     *  \returns a new PdfFilter allocated using new or NULL if no 
     *           filter is available for this type.
     */
    static const PdfFilter* Create( const EPdfFilter eFilter );

 private:
    /** static cache of filter objects
     */
    static std::map<EPdfFilter,PdfFilter*> s_mapFilters;
};

/** The ascii hex filter.
 */
class PdfHexFilter : public PdfFilter {
 public:
    /** Encodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the encoded data
     *  \param plOutLen pointer to the length of the output buffer
     */
    virtual void Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const;

    /** Decodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the decoded data
     *  \param plOutLen pointer to the length of the output buffer
     *  \param pDecodeParms optional pointer to an decode parameters dictionary
     *                      containing additional information to decode the data.
     *                      This pointer must be NULL if no decode parameter dictionary
     *                      is available.
     */
    virtual void Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen, const PdfDictionary* pDecodeParms = NULL ) const;

    /** Type of this filter.
     *  \returns the type of this filter
     */
    inline virtual EPdfFilter type() const;
};

EPdfFilter PdfHexFilter::type() const
{
    return ePdfFilter_ASCIIHexDecode;
}

/** The Ascii85 filter.
 */
class PdfAscii85Filter : public PdfFilter {
 public:
    /** Encodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the encoded data
     *  \param plOutLen pointer to the length of the output buffer
     */
    virtual void Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const;

    /** Decodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the decoded data
     *  \param plOutLen pointer to the length of the output buffer
     *  \param pDecodeParms optional pointer to an decode parameters dictionary
     *                      containing additional information to decode the data.
     *                      This pointer must be NULL if no decode parameter dictionary
     *                      is available.
     */
    virtual void Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen, const PdfDictionary* pDecodeParms = NULL ) const;

    /** Type of this filter.
     *  \returns the type of this filter
     */
    inline virtual EPdfFilter type() const;

 private:
    void Encode ( char* pBuffer, int* bufferPos, long lBufferLen, unsigned long tuple, int bytes ) const;
    void WidePut( char* pBuffer, int* bufferPos, long lBufferLen, unsigned long tuple, int bytes ) const;

    static unsigned long sPowers85[];
};

EPdfFilter PdfAscii85Filter::type() const
{
    return ePdfFilter_ASCII85Decode;
}

/** The flate filter.
 */
class PdfFlateFilter : public PdfFilter {
 public:
    /** Encodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the encoded data
     *  \param plOutLen pointer to the length of the output buffer
     */
    virtual void Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const;

    /** Decodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the decoded data
     *  \param plOutLen pointer to the length of the output buffer
     *  \param pDecodeParms optional pointer to an decode parameters dictionary
     *                      containing additional information to decode the data.
     *                      This pointer must be NULL if no decode parameter dictionary
     *                      is available.
     */
    virtual void Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen, const PdfDictionary* pDecodeParms = NULL ) const;

    /** Type of this filter.
     *  \returns the type of this filter
     */
    inline virtual EPdfFilter type() const;

    void RevertPredictor( const TFlatePredictorParams* pParams, const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const;
};

EPdfFilter PdfFlateFilter::type() const
{
    return ePdfFilter_FlateDecode;
}

/** The RLE filter.
 */
class PdfRLEFilter : public PdfFilter {
 public:
    /** Encodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the encoded data
     *  \param plOutLen pointer to the length of the output buffer
     */
    virtual void Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const;

    /** Decodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the decoded data
     *  \param plOutLen pointer to the length of the output buffer
     *  \param pDecodeParms optional pointer to an decode parameters dictionary
     *                      containing additional information to decode the data.
     *                      This pointer must be NULL if no decode parameter dictionary
     *                      is available.
     */
    virtual void Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen, const PdfDictionary* pDecodeParms = NULL ) const;

    /** Type of this filter.
     *  \returns the type of this filter
     */
    inline virtual EPdfFilter type() const;
};

EPdfFilter PdfRLEFilter::type() const
{
    return ePdfFilter_RunLengthDecode;
}


struct TLzwItem {
    std::vector<unsigned char> value;
};

typedef std::vector<TLzwItem>     TLzwTable;
typedef TLzwTable::iterator       TILzwTable;
typedef TLzwTable::const_iterator TCILzwTable;

/** The LZW filter.
 */
class PdfLZWFilter : public PdfFilter {
 public:
    /** Encodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the encoded data
     *  \param plOutLen pointer to the length of the output buffer
     */
    virtual void Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const;

    /** Decodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the decoded data
     *  \param plOutLen pointer to the length of the output buffer
     *  \param pDecodeParms optional pointer to an decode parameters dictionary
     *                      containing additional information to decode the data.
     *                      This pointer must be NULL if no decode parameter dictionary
     *                      is available.
     */
    virtual void Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen, const PdfDictionary* pDecodeParms = NULL ) const;

    /** Type of this filter.
     *  \returns the type of this filter
     */
    inline virtual EPdfFilter type() const;

 private:
    /** Initialize an lzw table.
     *  \param pTable the lzw table to initialize
     */
    void InitTable( TLzwTable* pTable ) const;

 private:
    static const unsigned short s_masks[4];
    static const unsigned short s_clear;
    static const unsigned short s_eod;
};

EPdfFilter PdfLZWFilter::type() const
{
    return ePdfFilter_LZWDecode;
}

};


#endif /* _PDF_FILTER_H_ */
