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

#ifndef _PDF_NAME_H_
#define _PDF_NAME_H_

#include "PdfDefines.h"
#include "PdfDataType.h"

namespace PoDoFo {

class PdfOutputDevice;
class PdfName;

//std::size_t hash_value(PdfName const& name);


/** This class represents a PdfName.
 *  Whenever a key is required you have to use a PdfName object.
 *  
 *  PdfName are required as keys in PdfObject and PdfVariant objects.
 *
 *  PdfName may have a maximum length of 127 characters.
 *
 *  \see PdfObject \see PdfVariant
 */
class PODOFO_API PdfName : public PdfDataType {
 public:

    /** Constructor to create NULL strings.
     *  use PdfName::KeyNull instead of this constructor
     */
    PdfName()
        : PdfDataType(), m_Data("")
    {
    }

    /** Create a new PdfName object.
     *  \param sName the unescaped value of this name. Please specify
     *                 the name without the leading '/'.
     */
    PdfName( const std::string& sName )
        : PdfDataType(), m_Data(sName)
    {
    }

    /** Create a new PdfName object.
     *  \param pszName the unescaped value of this name. Please specify
     *                 the name without the leading '/'.
     *                 Has to be a zero terminated string.
     */
    PdfName( const char* pszName )
        : PdfDataType()
    {
        if (pszName) m_Data.assign( pszName );
    }

    /** Create a new PdfName object.
     *  \param pszName the unescaped value of this name. Please specify
     *                 the name without the leading '/'.
     *  \param lLen    length of the name
     */
    PdfName( const char* pszName, long lLen )
        : PdfDataType()
    {
        if( pszName ) m_Data.assign( pszName, lLen );
    }

    /** Create a new PdfName object from a string containing an escaped
     *  name string without the leading / .
     *
     *  \param sName A string containing the escaped name
     *  \return A new PdfName
     */
    static PdfName FromEscaped( const std::string& sName );

    /** Create a new PdfName object from a string containing an escaped
     *  name string without the leading / .
     *  \param pszName A string containing the escaped name
     *  \param ilength length of the escaped string data. If a length
     *                 of 0 is passed, the string data is expected to 
     *                 be a zero terminated string.
     *  \return A new PdfName
     */
    static PdfName FromEscaped( const char * pszName, pdf_long ilength = 0 );

    /** \return an escaped representation of this name
     *          without the leading / .
     *
     *  There is no corresponding GetEscapedLength(), since
     *  generating the return value is somewhat expensive.
     */
    std::string GetEscapedName() const;

    /** Create a copy of an existing PdfName object.
     *  \param rhs another PdfName object
     */
    PdfName( const PdfName & rhs )
        : PdfDataType(), m_Data(rhs.m_Data)
    {
    }

    virtual ~PdfName();

    /** Write the name to an output device in PDF format.
     *  This is an overloaded member function.
     *
     *  \param pDevice write the object to this device
     *  \param eWriteMode additional options for writing this object
     *  \param pEncrypt an encryption object which is used to encrypt this object
     *                  or NULL to not encrypt this object     
     */
    void Write( PdfOutputDevice* pDevice, EPdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt = NULL) const;

    /** \returns the unescaped value of this name object
     *           without the leading slash
     */
    PODOFO_NOTHROW inline const std::string& GetName() const;

    /** \returns the unescaped length of this
     *           name object
     */
    PODOFO_NOTHROW inline size_t GetLength() const;

    /** Assign another name to this object
     *  \param rhs another PdfName object
     */
    PODOFO_NOTHROW inline const PdfName& operator=( const PdfName & rhs );

    /** compare to PdfName objects.
     *  \returns true if both PdfNames have the same value.
     */
    PODOFO_NOTHROW inline bool operator==( const PdfName & rhs ) const;

    /** overloaded operator for convinience
     *
     * The string argument is treated as an unescaped name.
     *
     *  \param rhs a name
     *  \returns true if this objects name is equal to pszName
     */
    bool operator==( const char* rhs ) const;

    /** overloaded operator for convinience
     *
     * The string argument is treated as an unescaped name.
     *
     *  \param rhs a name
     *  \returns true if this objects name is equal to pszName
     */
    PODOFO_NOTHROW inline bool operator==( const std::string& rhs ) const;

    /** compare two PdfName objects.
     *  \returns true if both PdfNames have different values.
     */
    PODOFO_NOTHROW inline bool operator!=( const PdfName & rhs ) const;

    /** overloaded operator for convinience
     *
     * The string argument is treated as an unescaped name.
     *
     *  \param rhs a name
     *  \returns true if this objects name is not equal to pszName
     */
    inline bool operator!=( const char* rhs ) const;

    /** compare two PdfName objects.
     *  Used for sorting in lists
     *  \returns true if this object is smaller than rhs
     */
    PODOFO_NOTHROW inline bool operator<( const PdfName & rhs ) const;

    static const PdfName KeyContents;
    static const PdfName KeyFlags;
    static const PdfName KeyLength;
    static const PdfName KeyNull;
    static const PdfName KeyRect;
    static const PdfName KeySize;
    static const PdfName KeySubtype;
    static const PdfName KeyType;
    static const PdfName KeyFilter;

 private:
    // The _unescaped_ name, without leading /
    std::string	m_Data;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
const std::string & PdfName::GetName() const
{
    return m_Data;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
size_t PdfName::GetLength() const
{
    return m_Data.length();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfName::operator!=( const PdfName & rhs ) const
{
    return !this->operator==( rhs );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfName::operator!=( const char* rhs ) const
{
    return !this->operator==( rhs );
}

bool PdfName::operator<( const PdfName & rhs ) const
{
    return m_Data < rhs.m_Data;
}

bool PdfName::operator==( const PdfName & rhs ) const
{
    return ( m_Data == rhs.m_Data );
}

bool PdfName::operator==( const std::string & rhs ) const
{
    return ( m_Data == rhs );
}

const PdfName& PdfName::operator=( const PdfName & rhs )
{
    m_Data = rhs.m_Data;
    return *this;
}


};

#endif /* _PDF_NAME_H_ */

