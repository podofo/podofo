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

#ifndef _PDF_DATA_H_
#define _PDF_DATA_H_

#include "PdfDefines.h"

#include "PdfDataType.h"

namespace PoDoFo {

class PdfOutputDevice;

/** A datatype that allows to write abitrary data
 *  to a PDF file. 
 *  The user of this class has to ensure that the data
 *  written to the PDF file using this class is valid data
 *  for a PDF file!
 *
 *  This class is used in PoDoFo to pad PdfVariants.
 *
 */
class PODOFO_API PdfData : public PdfDataType {
 public:
    /**
     * Create a new PdfData object with valid PdfData
     *
     * \param pszData has to be a valid value in a PDF file.
     *                It will be written directly to the PDF file.
     */
    PdfData( const char* pszData )
        : PdfDataType(), m_sData( pszData ) 
        {
        }

    /** Copy an existing PdfData 
     *  \param rhs another PdfData to copy
     */
    PdfData( const PdfData & rhs )
        : PdfDataType()
        {
            this->operator=( rhs );
        }

    /** Write the complete datatype to a file.
     *  \param pDevice write the object to this device
     */
    void Write( PdfOutputDevice* pDevice, const PdfEncrypt* pEncrypt = NULL ) const;

    /** Copy an existing PdfData 
     *  \param rhs another PdfData to copy
     *  \returns this object
     */
    inline const PdfData & operator=( const PdfData & rhs );

 private:
    std::string m_sData;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfData & PdfData::operator=( const PdfData & rhs )
{
    m_sData = rhs.m_sData;
    return (*this);
}

}; // namespace PoDoFo

#endif /* _PDF_DATATYPE_H_ */

