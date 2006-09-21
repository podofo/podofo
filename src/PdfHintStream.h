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

#ifndef _PDF_HINT_STREAM_H_
#define _PDF_HINT_STREAM_H_

#include "PdfDefines.h"
#include "PdfElement.h"
#include "PdfWriter.h"

namespace PoDoFo {

class PdfPagesTree;

class PdfHintStream : public PdfElement {
 public:
    PdfHintStream( PdfVecObjects* pParent, PdfPagesTree* pPagesTree );
    ~PdfHintStream();

    /** Create the hint stream 
     *  \param pXRef pointer to a valid XREF table structure
     */
    void Create( TVecXRefTable* pXRef );

    /** Write a pdf_uint16 to the stream in big endian format.
     *  \param val the value to write to the stream
     */
    void WriteUInt16( pdf_uint16 val );

    /** Write a pdf_uint32 to the stream in big endian format.
     *  \param val the value to write to the stream
     */
    void WriteUInt32( pdf_uint32 );

 private:
    void CreatePageHintTable( TVecXRefTable* pXRef );
    void CreateSharedObjectHintTable();
 
 private:
    PdfPagesTree* m_pPagesTree;

    bool          m_bLittleEndian;
};

};

#endif /* _PDF_HINT_STREAM_H_ */
