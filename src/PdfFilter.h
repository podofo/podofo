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

namespace PoDoFo {

/** 
 * This structur contains all necessary values
 * for a FlateDecode and LZWDecode Predictor.
 * These values are normally stored in the /DecodeParams
 * key of a PDF dictionary.
 */
struct TFlatePredictorParams {
    TFlatePredictorParams() {
        nPredictor   = 1;
        nColors      = 1;
        nBPC         = 8;
        nColumns     = 1;
        nEarlyChange = 1;
    };

    int nPredictor;
    int nColors;
    int nBPC;
    int nColumns;
    int nEarlyChange;
};

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
     *
     *  \returns ErrOk on success.
     */
    virtual PdfError Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) = 0;

    /** Decodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the decoded data
     *  \param plOutLen pointer to the length of the output buffer
     *
     *  \returns ErrOk on success.
     */
    virtual PdfError Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) = 0;

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
     *  The filter is allocated using new and the caller has to delete it.
     *
     *  \param eFilter the type of filter that should be created.
     *
     *  \returns a new PdfFilter allocated using new or NULL if no 
     *           filter is available for this type.
     */
    static PdfFilter* Create( const EPdfFilter eFilter );
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
     *
     *  \returns ErrOk on success.
     */
    virtual PdfError Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen );

    /** Decodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the decoded data
     *  \param plOutLen pointer to the length of the output buffer
     *
     *  \returns ErrOk on success.
     */
    virtual PdfError Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen );

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
     *
     *  \returns ErrOk on success.
     */
    virtual PdfError Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen );

    /** Decodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the decoded data
     *  \param plOutLen pointer to the length of the output buffer
     *
     *  \returns ErrOk on success.
     */
    virtual PdfError Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen );

    /** Type of this filter.
     *  \returns the type of this filter
     */
    inline virtual EPdfFilter type() const;

 private:
    PdfError Encode ( char* pBuffer, int* bufferPos, long lBufferLen, unsigned long tuple, int bytes );
    PdfError WidePut( char* pBuffer, int* bufferPos, long lBufferLen, unsigned long tuple, int bytes );

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
     *
     *  \returns ErrOk on success.
     */
    virtual PdfError Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen );

    /** Decodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the decoded data
     *  \param plOutLen pointer to the length of the output buffer
     *
     *  \returns ErrOk on success.
     */
    virtual PdfError Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen );

    /** Type of this filter.
     *  \returns the type of this filter
     */
    inline virtual EPdfFilter type() const;

    PdfError RevertPredictor( const TFlatePredictorParams* pParams, const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen );
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
     *
     *  \returns ErrOk on success.
     */
    virtual PdfError Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen );

    /** Decodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the decoded data
     *  \param plOutLen pointer to the length of the output buffer
     *
     *  \returns ErrOk on success.
     */
    virtual PdfError Decode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen );

    /** Type of this filter.
     *  \returns the type of this filter
     */
    inline virtual EPdfFilter type() const;
};

EPdfFilter PdfRLEFilter::type() const
{
    return ePdfFilter_RunLengthDecode;
}

};


#endif /* _PDF_FILTER_H_ */
