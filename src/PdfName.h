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

#ifndef _PDF_NAME_H_
#define _PDF_NAME_H_

#include "PdfDefines.h"
#include "PdfDataType.h"

namespace PoDoFo {

class PdfOutputDevice;

/** This class represents a PdfName.
 *  Whenever a key is required you have to use a PdfName object.
 *  
 *  PdfName are required as keys in PdfObject and PdfVariant objects.
 *
 *  PdfName may have a maximum length of 127 characters.
 *
 *  \see PdfObject \see PdfVariant
 */
class PdfName : public PdfDataType {
 public:
    /** Constructor to create NULL strings.
     *  use PdfName::KeyNull instead of this constructor
     */
    PdfName();

    /** Create a new PdfName object.
     *  \param sName the unescaped value of this name. Please specify
     *                 the name without the leading '/'.
     */
    PdfName( const std::string& sName );

    /** Create a new PdfName object.
     *  \param pszName the unescaped value of this name. Please specify
     *                 the name without the leading '/'.
     *                 Has to be a zero terminated string.
     */
    PdfName( const char* pszName );

    /** Create a new PdfName object.
     *  \param pszName the unescaped value of this name. Please specify
     *                 the name without the leading '/'.
     *  \param lLen    length of the name
     */
    PdfName( const char* pszName, long lLen );

    /** Create a new PdfName object from a string containing an escaped
     *  name string without the leading / .
     *
     *  \param name A string containing the escaped name
     *  \return A new PdfName
     */
    static PdfName FromEscaped( const std::string& sName );

    /** Create a new PdfName object from a string containing an escaped
     *  name string without the leading / .
     *  \param name A string containing the escaped name
     *  \return A new PdfName
     */
    static PdfName FromEscaped( const char * pszName, int ilength );

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
    PdfName( const PdfName & rhs );

    virtual ~PdfName();

    /** Write the name to an output device in PDF format.
     *  This is an overloaded member function.
     *
     *  \param pDevice write the object to this device
     */
    void Write( PdfOutputDevice* pDevice ) const;

    /** \returns the unescaped value of this name object
     *           without the leading slash
     */
    inline const std::string& GetName() const throw();

    /** \returns the unescaped length of this
     *           name object
     */
    inline size_t GetLength() const throw();

    /** Assign another name to this object
     *  \param rhs another PdfName object
     */
    const PdfName& operator=( const PdfName & rhs );

    /** compare to PdfName objects.
     *  \returns true if both PdfNames have the same value.
     */
    bool operator==( const PdfName & rhs ) const;

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
    bool operator==( const std::string& rhs ) const;

    /** compare two PdfName objects.
     *  \returns true if both PdfNames have different values.
     */
    inline bool operator!=( const PdfName & rhs ) const;

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
    bool operator<( const PdfName & rhs ) const;

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
const std::string & PdfName::GetName() const throw()
{
    return m_Data;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
size_t PdfName::GetLength() const throw()
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

};

#endif /* _PDF_NAME_H_ */
