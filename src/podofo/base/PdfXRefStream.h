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
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef _PDF_XREF_STREAM_H_
#define _PDF_XREF_STREAM_H_

#include "PdfDefines.h"

#include "PdfArray.h"
#include "PdfXRef.h"

namespace PoDoFo {

class PdfOutputDevice;
class PdfVecObjects;
class PdfWriter;

/**
 * Creates an XRef table that is a stream object.
 * Requires at least PDF 1.5. XRef streams are more
 * compact than normal XRef tables.
 *
 * This is an internal class of PoDoFo used by PdfWriter.
 */
class PdfXRefStream : public PdfXRef {
 public:
    /** Create a new XRef table
     *
     *  \param pParent a vector of PdfObject is required
     *                 to create a PdfObject for the XRef
     *  \param pWriter is needed to fill the trailer directory
     *                 correctly which is included into the XRef
     */
    PdfXRefStream( PdfVecObjects* pParent, PdfWriter* pWriter );

    /** Destruct the XRef table
     */
    virtual ~PdfXRefStream();

    /**
     * \returns the offset in the file at which the XRef table
     *          starts after it was written
     */
    inline virtual pdf_uint64 GetOffset() const;

 protected:
    /** Called at the start of writing the XRef table.
     *  This method can be overwritten in subclasses
     *  to write a general header for the XRef table.
     *
     *  @param pDevice the output device to which the XRef table 
     *                 should be written.
     */
    virtual void BeginWrite( PdfOutputDevice* pDevice );

    /** Begin an XRef subsection.
     *  All following calls of WriteXRefEntry belong to this XRef subsection.
     *
     *  @param pDevice the output device to which the XRef table 
     *                 should be written.
     *  @param nFirst the object number of the first object in this subsection
     *  @param nCount the number of entries in this subsection
     */
    virtual void WriteSubSection( PdfOutputDevice* pDevice, pdf_objnum nFirst, pdf_uint32 nCount );

    /** Write a single entry to the XRef table
     *  
     *  @param pDevice the output device to which the XRef table 
     *                 should be written.
     *  @param offset the offset of the object
     *  @param generation the generation number
     *  @param cMode the mode 'n' for object and 'f' for free objects
     *  @param objectNumber the object number of the currently written object if cMode = 'n' 
     *                       otherwise undefined
     */
    virtual void WriteXRefEntry( PdfOutputDevice* pDevice, pdf_uint64 offset, pdf_gennum generation, 
                                 char cMode, pdf_objnum objectNumber = 0 );

    /** Called at the end of writing the XRef table.
     *  Sub classes can overload this method to finish a XRef table.
     *
     *  @param pDevice the output device to which the XRef table 
     *                 should be written.
     */
    virtual void EndWrite( PdfOutputDevice* pDevice );

 private:
    PdfVecObjects* m_pParent;
    PdfWriter*     m_pWriter;
    PdfObject*     m_pObject;
    PdfArray       m_indeces;

    size_t         m_bufferLen; ///< The length of the internal buffer for one XRef entry
    pdf_uint64     m_offset;    ///< Offset of the XRefStream object
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline pdf_uint64 PdfXRefStream::GetOffset() const
{
    return m_offset;
}

};

#endif /* _PDF_XREF_H_ */
