/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _PDF_CONTENTS_TOKENIZER_H_
#define _PDF_CONTENTS_TOKENIZER_H_

#include "PdfDefines.h"
#include "PdfTokenizer.h"
#include "PdfVariant.h"

#include <list>

namespace PoDoFo {

class PdfDocument;
class PdfCanvas;
class PdfObject;

/** An enum describing the type of a read token
 */
enum EPdfContentsType {
    ePdfContentsType_Keyword, ///< A keyword is a PDF function
    ePdfContentsType_Variant  ///< A variant is usually a parameter to a keyword 
};

/** This class is a parser for content streams in PDF documents.
 *
 *  The parsed content stream can be used and modified in various ways.
 *
 *  This class is currently work in progress and subject to change!
 */
class PODOFO_API PdfContentsTokenizer : public PdfTokenizer {
public:

    /** Construct a PdfContentsTokenizer from an existing buffer.
     *  Usually a stream from a PdfPage.
     *
     *  \param pBuffer pointer to a buffer
     *  \param lLen length of the buffer
     */
    PdfContentsTokenizer( const char* pBuffer, long lLen )
        : PoDoFo::PdfTokenizer( pBuffer, lLen )
    {
    }

    /** Construct a PdfContentsTokenizer from a PdfCanvas 
     *  (i.e. PdfPage or a PdfXObject).
     *
     *  This is more convinient as you do not have
     *  to care about buffers yourself.
     *
     *  \param pCanvas an object that hold a PDF contents stream
     */
    PdfContentsTokenizer( PdfCanvas* pCanvas );

    virtual ~PdfContentsTokenizer() { }
    
    /** Read the next keyword or variant, returning true and setting reType if something was read.
     *  Either rpszKeyword or rVariant, but never both, have defined and usable values on
     *  true return, with which being controlled by the value of eType.
     *
     *  If EOF is encountered, returns false and leaves eType, pszKeyword and
     *  rVariant undefined.
     *
     *  \param[out] reType will be set to either keyword or variant if true is returned. Undefined
     *              if false is returned.
     *
     *  \param[out] rpszKeyword if pType is set to ePdfContentsType_Keyword this will point to the keyword,
     *              otherwise the value is undefined. If set, the value points to memory owned by the
     *              PdfContentsTokenizer and must not be freed. The value is invalidated when ReadNext
     *              is next called or when the PdfContentsTokenizer is destroyed.
     *        
     *  \param[out] rVariant if pType is set to ePdfContentsType_Variant this will be set to the read variant,
     *              otherwise the value is undefined.
     *
     */
    bool ReadNext( EPdfContentsType& reType, const char*& rpszKeyword, PoDoFo::PdfVariant & rVariant );

 private:
    /** Set another objects stream as the current stream for parsing
     *
     *  \param pObject use the stream of this object for parsing
     */
    void SetCurrentContentsStream( PdfObject* pObject );

 private:
    PdfRefCountedBuffer       m_curBuffer;    ///< A copy of the current contents stream
    std::list<PdfObject*>     m_lstContents;  ///< A list containing pointers to all contents objects 
};

};

#endif // _PDF_CONTENTS_TOKENIZER_H_

