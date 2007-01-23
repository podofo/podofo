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

#ifndef _PDF_XREF_H_
#define _PDF_XREF_H_

#include "PdfDefines.h"

#include "PdfReference.h"

namespace PoDoFo {

#define EMPTY_OBJECT_OFFSET   65535

class PdfOutputDevice;

/**
 * Creates an XRef table.
 *
 * This is an internal class of PoDoFo used by PdfWriter.
 */
class PdfXRef {
 protected:
    typedef struct TXRefItem {
        PdfReference reference;
        long         lOffset;

        bool operator<( const TXRefItem & rhs ) const
        {
            return this->reference < rhs.reference;
        }
    };
    
    typedef std::vector<TXRefItem>         TVecXRefItems;
    typedef TVecXRefItems::iterator        TIVecXRefItems;
    typedef TVecXRefItems::const_iterator  TCIVecXRefItems;

    typedef std::vector<PdfReference>      TVecReferences;
    typedef TVecReferences::iterator       TIVecReferences;
    typedef TVecReferences::const_iterator TCIVecReferences;

 public:
    /** Create a new XRef table
     */
    PdfXRef();

    /** Destruct the XRef table
     */
    virtual ~PdfXRef();

    /** Add an object to the XRef table.
     *  The object should have been written to an output device already.
     *  
     *  \param rRef reference of this object
     *  \param lOffset the offset where on the device the object was written
     *  \param bUsed specifies wether this is an used or free object.
     *               Set this value to true for all normal objects and to false
     *               for free object references.
     */
    void AddObject( const PdfReference & rRef, long lOffset, bool bUsed );

    /** Write the XRef table to an output device.
     * 
     *  \param pDevice an output device (usually a PDF file)
     *
     */
    virtual void Write( PdfOutputDevice* pDevice );

    /** Get the size of the XRef table.
     *  I.e. the highest object number + 1.
     *
     *  \returns the size of the xref table
     */
    unsigned int GetSize() const;

 protected:
    int GetItemCount( PdfXRef::TCIVecXRefItems it, PdfXRef::TCIVecReferences itFree ) const;

 protected:
    TVecXRefItems  m_vecXRef;
    TVecReferences m_vecFreeObjects;
};

};

#endif /* _PDF_XREF_H_ */
