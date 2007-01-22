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

#ifndef _PDF_FILTERS_PRIVATE_H_
#define _PDF_FILTERS_PRIVATE_H_

#include "PdfDefines.h"
#include "PdfFilter.h"

namespace PoDoFo {

// TODO: explicit predictor handling
struct TFlatePredictorParams;

/** The ascii hex filter.
 */
class PdfHexFilter : public PdfFilter {
 public:
    virtual ~PdfHexFilter() {}

    /** Check wether the encoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to encode data
     */
    inline virtual bool CanEncode() const; 

    /** Encodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the encoded data
     *  \param plOutLen pointer to the length of the output buffer
     */
    virtual void Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const;

    /** Check wether the decoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to decode data
     */
    inline virtual bool CanDecode() const; 

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

    /** GetType of this filter.
     *  \returns the GetType of this filter
     */
    inline virtual EPdfFilter GetType() const;
};

bool PdfHexFilter::CanEncode() const
{
    return true;
}

bool PdfHexFilter::CanDecode() const
{
    return true;
}

EPdfFilter PdfHexFilter::GetType() const
{
    return ePdfFilter_ASCIIHexDecode;
}

/** The Ascii85 filter.
 */
class PdfAscii85Filter : public PdfFilter {
 public:
    virtual ~PdfAscii85Filter() {}

    /** Check wether the encoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to encode data
     */
    inline virtual bool CanEncode() const; 

    /** Encodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the encoded data
     *  \param plOutLen pointer to the length of the output buffer
     */
    virtual void Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const;

    /** Check wether the decoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to decode data
     */
    inline virtual bool CanDecode() const; 

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

    /** GetType of this filter.
     *  \returns the GetType of this filter
     */
    inline virtual EPdfFilter GetType() const;

 private:
    void Encode ( char* pBuffer, int* bufferPos, long lBufferLen, unsigned long tuple, int bytes ) const;
    void WidePut( char* pBuffer, int* bufferPos, long lBufferLen, unsigned long tuple, int bytes ) const;

    static unsigned long sPowers85[];
};

bool PdfAscii85Filter::CanEncode() const
{
    return true;
}

bool PdfAscii85Filter::CanDecode() const
{
    return true;
}

EPdfFilter PdfAscii85Filter::GetType() const
{
    return ePdfFilter_ASCII85Decode;
}

/** The flate filter.
 */
class PdfFlateFilter : public PdfFilter {
 public:
    virtual ~PdfFlateFilter() {}

    /** Check wether the encoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to encode data
     */
    inline virtual bool CanEncode() const; 

    /** Encodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the encoded data
     *  \param plOutLen pointer to the length of the output buffer
     */
    virtual void Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const;

    /** Check wether the decoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to decode data
     */
    inline virtual bool CanDecode() const; 

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

    /** GetType of this filter.
     *  \returns the GetType of this filter
     */
    inline virtual EPdfFilter GetType() const;

    // FIXME: DOM: Predictors should be implemented like filters!
    void RevertPredictor( const TFlatePredictorParams* pParams, const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const;
};

bool PdfFlateFilter::CanEncode() const
{
    return true;
}

bool PdfFlateFilter::CanDecode() const
{
    return true;
}

EPdfFilter PdfFlateFilter::GetType() const
{
    return ePdfFilter_FlateDecode;
}


/** The RLE filter.
 */
class PdfRLEFilter : public PdfFilter {
 public:
    virtual ~PdfRLEFilter() {}

    /** Check wether the encoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to encode data
     */
    inline virtual bool CanEncode() const; 

    /** Encodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the encoded data
     *  \param plOutLen pointer to the length of the output buffer
     */
    virtual void Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const;

    /** Check wether the decoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to decode data
     */
    inline virtual bool CanDecode() const; 

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

    /** GetType of this filter.
     *  \returns the GetType of this filter
     */
    inline virtual EPdfFilter GetType() const;
};

bool PdfRLEFilter::CanEncode() const
{
    return false;
}

bool PdfRLEFilter::CanDecode() const
{
    return true;
}

EPdfFilter PdfRLEFilter::GetType() const
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
    virtual ~PdfLZWFilter() {}

    /** Check wether the encoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to encode data
     */
    inline virtual bool CanEncode() const; 

    /** Encodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the encoded data
     *  \param plOutLen pointer to the length of the output buffer
     */
    virtual void Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const;

    /** Check wether the decoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to decode data
     */
    inline virtual bool CanDecode() const; 

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

    /** GetType of this filter.
     *  \returns the GetType of this filter
     */
    inline virtual EPdfFilter GetType() const;

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

bool PdfLZWFilter::CanEncode() const
{
    return false;
}

bool PdfLZWFilter::CanDecode() const
{
    return true;
}

EPdfFilter PdfLZWFilter::GetType() const
{
    return ePdfFilter_LZWDecode;
}

};


#endif /* _PDF_FILTERS_PRIVATE_H_ */
