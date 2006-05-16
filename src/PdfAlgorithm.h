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

#ifndef _PDF_ALGORITHM_H_
#define _PDF_ALGORITHM_H_

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


/**
 * This class contains various simple algorithms which are used mostly
 * to implement the many PDF filters for streams and to implement
 * signatures.
 * All member functions of this class are declared so static. As a reason
 * the constructor was made private.
 */ 
class PdfAlgorithm {
 public:
    
    /** Deflates a buffer using zlib.
     *  If the buffer is to small, it will be free'd and a new 
     *  one will be allocated using malloc.
     *  \param pBuffer pointer to a buffer
     *  \param lLen pointer to the buffer length, the new buffer length will be
     *              returned in this variable
     *  \returns ErrOk on success.
     */
    static PdfError FlateDecodeBuffer( char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen );

    static PdfError FlateEncodeBuffer( char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen );

    /** Decode a hex encoded buffer.
     *  As the decoded buffer is always smaller than the encoded buffer
     *  all decoding can be done in the input buffer.
     *  \param pBuffer in: hexadecimal data out: the decoded data
     *  \param lLen the length of the input buffer. The length of the 
     *              decoded buffer is also returned in this value.
     *  \returns ErrOk on sucess.
     */
    static PdfError HexDecodeBuffer( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen );

    /** Hex encode a buffer
     *  A new buffer will be malloc'ed and returned.
     *
     *  \param pInBuffer pointer to a buffer.
     *  \param lInLen length of input the buffer
     *  \param ppOutBuffer a pointer to the newly malloced buffer is returned in this value
     *  \param plOutLen the length of the malloced buffer is returned in this variable
     *  \returns ErrOk on success.
     */
    static PdfError HexEncodeBuffer( const char* pInBuffer, long lInLen, char** ppOutBuffer, long *plOutLen );


    static PdfError RevertFlateDecodePredictor( const TFlatePredictorParams* pParams, char* pInBuffer, long lInLen, char** ppOutBuffer, long *plOutLen );

    static PdfError RunLengthDecodeBuffer( char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ); 

 private:
    /** Private constructor to avoid construction
     *  of this class which contains only static members.
     */
    PdfAlgorithm() {}
};

};

#endif // _PDF_ALGORITHM_H_
