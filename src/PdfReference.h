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

#ifndef _PDF_REFERENCE_H_
#define _PDF_REFERENCE_H_

#include "PdfDefines.h"

#include "PdfDataType.h"

namespace PoDoFo {

class PdfOutputDevice;

/**
 * A reference is a pointer to a object in the PDF file of the form
 * "4 0 R", where 4 is the object number and 0 is the generation number.
 * Every object in the PDF file can be indetified this way.
 *
 * This class is a indirect reference in a PDF file.
 */
class PdfReference : public PdfDataType {
 public:
    /**
     * Create a PdfReference with object number and generation number
     * initialized to 0.
     */
    PdfReference();

    /**
     * Create a PdfReference to an object with a given object and generation number.
     *
     * \param nObjectNo the object number
     * \param nGenerationNo the generation number
     */
    PdfReference( unsigned long nObjectNo, unsigned long nGenerationNo );

    /**
     * Create a copy of an existing PdfReference.
     * 
     * \param rhs the object to copy
     */
    PdfReference( const PdfReference & rhs );

    virtual ~PdfReference() {};

    /** Convert the reference to a string.
     *  \returns a string representation of the object.
     *
     *  \see PdfVariant::ToString
     */
    const std::string ToString() const;

   /**
     * Assigne the value of another object to this PdfReference.
     *
     * \param rhs the object to copy
     */
    const PdfReference & operator=( const PdfReference & rhs );

    /** Write the complete variant to an output device.
     *  This is an overloaded member function.
     *
     *  \param pDevice write the object to this device
     */
    void Write( PdfOutputDevice* pDevice ) const;

    /** 
     * Compare to PdfReference objects.
     * \returns true if both reference the same object
     */
    bool operator==( const PdfReference & rhs ) const;

    /** Set the object number of this object
     *  \param o the new object number
     */
    inline void SetObjectNumber( unsigned long o );

    /** Get the object number.
     *  \returns the object number of this PdfReference
     */
    inline unsigned long ObjectNumber() const;

    /** Set the generation number of this object
     *  \param g the new generation number
     */
    inline void SetGenerationNumber( unsigned long g );

    /** Get the generation number.
     *  \returns the generation number of this PdfReference
     */
    inline unsigned long GenerationNumber() const;

 private:
    unsigned long m_nObjectNo;
    unsigned long m_nGenerationNo;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfReference::SetObjectNumber( unsigned long o )
{
    m_nObjectNo = o;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned long PdfReference::ObjectNumber() const
{
    return m_nObjectNo;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfReference::SetGenerationNumber( unsigned long g )
{
    m_nGenerationNo = g;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned long PdfReference::GenerationNumber() const
{
    return m_nGenerationNo;
}

};

#endif // _PDF_REFERENCE_H_

