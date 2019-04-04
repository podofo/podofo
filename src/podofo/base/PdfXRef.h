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
    struct TXRefItem{
        TXRefItem( const PdfReference & rRef, const pdf_uint64 & off ) 
            : reference( rRef ), offset( off )
            {
            }

        PdfReference reference;
        pdf_uint64   offset;

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

    class PdfXRefBlock {
    public:
        PdfXRefBlock() 
            : m_nFirst( 0 ), m_nCount( 0 )
        {
        }

        PdfXRefBlock( const PdfXRefBlock & rhs )
            : m_nFirst( 0 ), m_nCount( 0 )
        {
            this->operator=( rhs );
        }
        
        bool InsertItem( const TXRefItem & rItem, bool bUsed );

        bool operator<( const PdfXRefBlock & rhs ) const
        {
            return m_nFirst < rhs.m_nFirst;
        }

        const PdfXRefBlock & operator=( const PdfXRefBlock & rhs )
        {
            m_nFirst  = rhs.m_nFirst;
            m_nCount  = rhs.m_nCount;
            
            items     = rhs.items;
            freeItems = rhs.freeItems;

            return *this;
        }

        pdf_objnum   m_nFirst;
        pdf_uint32   m_nCount;
        
        TVecXRefItems items;
        TVecReferences freeItems;
    };
    
    typedef std::vector<PdfXRefBlock>      TVecXRefBlock;
    typedef TVecXRefBlock::iterator        TIVecXRefBlock;
    typedef TVecXRefBlock::const_iterator  TCIVecXRefBlock;

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
     *  \param offset the offset where on the device the object was written
     *  \param bUsed specifies wether this is an used or free object.
     *               Set this value to true for all normal objects and to false
     *               for free object references.
     */
    void AddObject( const PdfReference & rRef, pdf_uint64 offset, bool bUsed );

    /** Write the XRef table to an output device.
     * 
     *  \param pDevice an output device (usually a PDF file)
     *
     */
    void Write( PdfOutputDevice* pDevice );

    /** Get the size of the XRef table.
     *  I.e. the highest object number + 1.
     *
     *  \returns the size of the xref table
     */
    pdf_uint32 GetSize() const;

    /**
     * \returns the offset in the file at which the XRef table
     *          starts after it was written
     */
    inline virtual pdf_uint64 GetOffset() const;

    /**
     * Mark as empty block.
     */
    void SetFirstEmptyBlock();

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
    const PdfReference* GetFirstFreeObject( PdfXRef::TCIVecXRefBlock itBlock, PdfXRef::TCIVecReferences itFree ) const;
    const PdfReference* GetNextFreeObject( PdfXRef::TCIVecXRefBlock itBlock, PdfXRef::TCIVecReferences itFree ) const;

    /** Merge all xref blocks that follow immediately after each other
     *  into a single block.
     *
     *  This results in slitely smaller PDF files which are easier to parse
     *  for other applications.
     */
    void MergeBlocks();

 private:
    pdf_uint64 m_offset;

 protected:
    TVecXRefBlock  m_vecBlocks;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline pdf_uint64 PdfXRef::GetOffset() const
{
    return m_offset;
}

};

#endif /* _PDF_XREF_H_ */
