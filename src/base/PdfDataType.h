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

#ifndef _PDF_DATATYPE_H_
#define _PDF_DATATYPE_H_

#include "PdfDefines.h"

namespace PoDoFo {

class PdfEncrypt;
class PdfOutputDevice;

/** An interface for all PDF datatype classes.
 *
 *  
 *  \see PdfName \see PdfArray \see PdfReference 
 *  \see PdfVariant \see PdfDictionary \see PdfString
 */
class PODOFO_API PdfDataType {

 protected:
    /** Create a new PdfDataType.
     *  Can only be called by subclasses
     */
    PdfDataType();

 public:
    virtual ~PdfDataType();

    /** Write the complete datatype to a file.
     *  \param pDevice write the object to this device
     *  \param eWriteMode additional options for writing this object
     *  \param pEncrypt an encryption object which is used to encrypt this object
     *                  or NULL to not encrypt this object
     */
    virtual void Write( PdfOutputDevice* pDevice, EPdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt = NULL ) const = 0;

    /** The dirty flag is set if this variant
     *  has been modified after construction.
     *  
     *  Usually the dirty flag is also set
     *  if you call any non-const member function
     *  as we cannot determine if you actually changed 
     *  something or not.
     *
     *  \returns true if the value is dirty and has been 
     *                modified since construction
     */
    virtual bool IsDirty() const;

    /** Sets the dirty flag of this PdfVariant
     *
     *  \param bDirty true if this PdfVariant has been
     *                modified from the outside
     *
     *  \see IsDirty
     */
    virtual void SetDirty( bool bDirty );

    /**
     * Sets this object to immutable,
     * so that no keys can be edited or changed.
     *
     * @param bImmutable if true set the object to be immutable
     *
     * This is used by PdfImmediateWriter and PdfStreamedDocument so 
     * that no keys can be added to an object after setting stream data on it.
     *
     */
    inline void SetImmutable(bool bImmutable);

    /**
     * Retrieve if an object is immutable.
     *
     * This is used by PdfImmediateWriter and PdfStreamedDocument so 
     * that no keys can be added to an object after setting stream data on it.
     *
     * @returns true if the object is immutable
     */
    inline bool GetImmutable() const;

protected:
    /**
     *  Will throw an exception if called on an immutable object,
     *  so this should be called before actually changing a value!
     * 
     */
    inline void AssertMutable() const;

private:
    bool m_bImmutable;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfDataType::SetImmutable(bool bImmutable)
{
    m_bImmutable = bImmutable;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfDataType::GetImmutable() const 
{
    return m_bImmutable;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfDataType::AssertMutable() const
{
    if(m_bImmutable) 
    {
        throw PdfError( ePdfError_ChangeOnImmutable );
    }
}

}; // namespace PoDoFo

#endif /* _PDF_DATATYPE_H_ */

