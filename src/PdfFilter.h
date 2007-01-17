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

class PdfDictionary;

/** Every filter in PoDoFo has to implement this interface.
 * 
 *  The two methods Encode and Decode have to be implemented 
 *  for every filter.
 *
 *  The output buffers are malloc'ed in the functions and have
 *  to be free'd by the caller.
 */
class PODOFO_API PdfFilter {
 public:
    /** All classes with virtual functions need a virtual destructor
     */
    virtual ~PdfFilter() {};

    /** Check wether the encoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to encode data
     */
    virtual bool CanEncode() const = 0; 

    /** Encodes a buffer using a filter. The buffer will malloc'ed and
     *  has to be free'd by the caller.
     *  
     *  \param pInBuffer input buffer
     *  \param lInLen    length of the input buffer
     *  \param ppOutBuffer pointer to the buffer of the encoded data
     *  \param plOutLen pointer to the length of the output buffer
     */
    virtual void Encode( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const = 0;

    /*
    virtual void StartEncode() = 0;
    virutal void EncodeBlock( const char* pInBuffer, long lInLen, char** ppOutBuffer, long* plOutLen ) const = 0;
    virtual void EndEncode() = 0;
    */

    /** Check wether the decoding is implemented for this filter.
     * 
     *  \returns true if the filter is able to decode data
     */
    virtual bool CanDecode() const = 0; 

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
    virtual EPdfFilter GetType() const = 0;
};

/** A factory to create a filter object for a filter GetType from the EPdfFilter enum.
 * 
 *  All filters should be created using this factory which does also caching of filter
 *  instances.
 */
class PODOFO_API PdfFilterFactory {
 public:
    /** Create a filter from an enum. 
     *  The filter is cached and may not be delted!
     *
     *  \param eFilter the GetType of filter that should be created.
     *
     *  \returns a new PdfFilter allocated using new or NULL if no 
     *           filter is available for this GetType.
     */
    static const PdfFilter* Create( const EPdfFilter eFilter );

 private:
    /** static cache of filter objects
     */
    static std::map<EPdfFilter,PdfFilter*> s_mapFilters;
};

};

#endif /* _PDF_FILTER_H_ */
