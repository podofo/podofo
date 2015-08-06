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

#ifndef _PDF_ARRAY_H_
#define _PDF_ARRAY_H_

#ifdef _WIN32
#ifdef _MSC_VER
// IC: VS2008 suppress dll warning
#pragma warning(disable: 4275)
#endif // _MSC_VER
#endif // _WIN32

#include "PdfDefines.h"
#include "PdfDataType.h"
#include "PdfObject.h"

namespace PoDoFo {

typedef std::vector<PdfObject> PdfArrayBaseClass;

/** This class represents a PdfArray
 *  Use it for all arrays that are written to a PDF file.
 *  
 *  A PdfArray can hold any PdfVariant.
 *
 *  \see PdfVariant
 */
class PODOFO_API PdfArray : private PdfArrayBaseClass, public PdfDataType {
 public:
    typedef PdfArrayBaseClass::iterator               iterator;
    typedef PdfArrayBaseClass::const_iterator         const_iterator;
    typedef PdfArrayBaseClass::reverse_iterator       reverse_iterator;
    typedef PdfArrayBaseClass::const_reverse_iterator const_reverse_iterator;

    /** Create an empty array 
     */
    PdfArray();

    /** Create an array and add one value to it.
     *  The value is copied.
     *
     *  \param var add this object to the array.
     */
    explicit PdfArray( const PdfObject & var );

    /** Deep copy an existing PdfArray
     *
     *  \param rhs the array to copy
     */
    PdfArray( const PdfArray & rhs );

    virtual ~PdfArray();

    /** assignment operator
     *
     *  \param rhs the array to assign
     */
    PdfArray& operator=(const PdfArray& rhs);

    /** 
     *  \returns the size of the array
     */
    inline size_t GetSize() const;

    /** Remove all elements from the array
     */
    inline void Clear();

    /** Write the array to an output device.
     *  This is an overloaded member function.
     *
     *  \param pDevice write the object to this device
     *  \param eWriteMode additional options for writing this object
     *  \param pEncrypt an encryption object which is used to encrypt this object
     *                  or NULL to not encrypt this object
     */
    virtual void Write( PdfOutputDevice* pDevice, EPdfWriteMode eWriteMode, 
                        const PdfEncrypt* pEncrypt = NULL ) const;

    /** Utility method to determine if the array contains
     *  contains any objects of ePdfDataType_String whose
     *  value is the passed string.
     *  \param cmpString the string to compare against
     *  \returns true if success, false if not
     */
    bool ContainsString( const std::string& cmpString ) const;
    
    /** Utility method to return the actual index in the
     *  array which contains an object of ePdfDataType_String whose
     *  value is the passed string.
     *  \param cmpString the string to compare against
     *  \returns true if success, false if not
     */
    size_t GetStringIndex( const std::string& cmpString ) const;

    /** Adds a PdfObject to the array
     *
     *  \param var add a PdfObject to the array
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    inline void push_back( const PdfObject & var );

    /** 
     *  \returns the size of the array
     */
    inline size_t size() const;

    /**
     *  \returns true if the array is empty.
     */
    inline bool empty() const;

    inline PdfObject & operator[](size_type __n);
    inline const PdfObject & operator[](size_type __n) const;

    /**
     * Resize the internal vector.
     * \param __n new size
     */
    inline void resize(size_t __n, value_type __x = PdfArrayBaseClass::value_type());
    
    /**
     *  Returns a read/write iterator that points to the first
     *  element in the array.  Iteration is done in ordinary
     *  element order.
     */
    inline iterator begin();

    /**
     *  Returns a read-only (constant) iterator that points to the
     *  first element in the array.  Iteration is done in ordinary
     *  element order.
     */
    inline const_iterator begin() const;

    /**
     *  Returns a read/write iterator that points one past the last
     *  element in the array.  Iteration is done in ordinary
     *  element order.
     */
    inline iterator end();

    /**
     *  Returns a read-only (constant) iterator that points one past
     *  the last element in the array.  Iteration is done in
     *  ordinary element order.
     */
    inline const_iterator end() const;

    /**
     *  Returns a read/write reverse iterator that points to the
     *  last element in the array.  Iteration is done in reverse
     *  element order.
     */
    inline reverse_iterator rbegin();

    /**
     *  Returns a read-only (constant) reverse iterator that points
     *  to the last element in the array.  Iteration is done in
     *  reverse element order.
     */
    inline const_reverse_iterator rbegin() const;

    /**
     *  Returns a read/write reverse iterator that points to one
     *  before the first element in the array.  Iteration is done
     *  in reverse element order.
     */
    inline reverse_iterator rend();

    /**
     *  Returns a read-only (constant) reverse iterator that points
     *  to one before the first element in the array.  Iteration
     *  is done in reverse element order.
     */
    inline const_reverse_iterator rend() const;

#if defined(_MSC_VER)  &&  _MSC_VER <= 1200    // workaround template-error in Visualstudio 6
    inline void insert(iterator __position, 
                       iterator __first,
                       iterator __last);
#else
    template<typename _InputIterator> 
        void insert(const iterator& __position, 
                    const _InputIterator& __first,
                    const _InputIterator& __last);
#endif

    inline PdfArray::iterator insert(const iterator& __position, const PdfObject & val );

    inline void erase( const iterator& pos );
    inline void erase( const iterator& first, const iterator& last );

    inline void reserve(size_type __n);

    /**
     *  \returns a read/write reference to the data at the first
     *           element of the array.
     */
    inline reference front();

    /**
     *  \returns a read-only (constant) reference to the data at the first
     *           element of the array.
     */
    inline const_reference front() const;

    /**
     *  \returns a read/write reference to the data at the last
     *           element of the array.
     */
    inline reference back();
      
    /**
     *  \returns a read-only (constant) reference to the data at the
     *           last element of the array.
     */
    inline const_reference back() const;

    inline bool operator==( const PdfArray & rhs ) const;
    inline bool operator!=( const PdfArray & rhs ) const;

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

 private:
    bool         m_bDirty; ///< Indicates if this object was modified after construction

};

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfArray::Clear() 
{
    AssertMutable();

    this->clear();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
size_t PdfArray::GetSize() const
{
    return this->size();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfArray::push_back( const PdfObject & var )
{
    AssertMutable();

    PdfArrayBaseClass::push_back( var );
    m_bDirty = true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
size_t PdfArray::size() const
{
    return PdfArrayBaseClass::size();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfArray::empty() const
{
    return PdfArrayBaseClass::empty();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfObject& PdfArray::operator[](size_type __n)
{
    AssertMutable();

    m_bDirty = true;
    return PdfArrayBaseClass::operator[](__n);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfObject& PdfArray::operator[](size_type __n) const
{
    return PdfArrayBaseClass::operator[](__n);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfArray::resize(size_t __n, value_type __x)
{
    PdfArrayBaseClass::resize(__n, __x);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::iterator PdfArray::begin()
{
    return PdfArrayBaseClass::begin();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::const_iterator PdfArray::begin() const
{
    return PdfArrayBaseClass::begin();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::iterator PdfArray::end()
{
    return PdfArrayBaseClass::end();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::const_iterator PdfArray::end() const
{
    return PdfArrayBaseClass::end();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::reverse_iterator PdfArray::rbegin()
{
    return PdfArrayBaseClass::rbegin();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::const_reverse_iterator PdfArray::rbegin() const
{
    return PdfArrayBaseClass::rbegin();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::reverse_iterator PdfArray::rend()
{
    return PdfArrayBaseClass::rend();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::const_reverse_iterator PdfArray::rend() const
{
    return PdfArrayBaseClass::rend();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200        // workaround template-error in Visualstudio 6
void PdfArray::insert(PdfArray::iterator __position, 
                      PdfArray::iterator __first,
                      PdfArray::iterator __last)
#else
template<typename _InputIterator>
void PdfArray::insert(const PdfArray::iterator& __position, 
                      const _InputIterator& __first,
                      const _InputIterator& __last)
#endif
{
    AssertMutable();

    PdfArrayBaseClass::insert( __position, __first, __last );
    m_bDirty = true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::iterator PdfArray::insert(const iterator& __position, const PdfObject & val )
{
    AssertMutable();

    m_bDirty = true;
    return PdfArrayBaseClass::insert( __position, val );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfArray::erase( const iterator& pos )
{
    AssertMutable();

    PdfArrayBaseClass::erase( pos );
    m_bDirty = true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfArray::erase( const iterator& first, const iterator& last )
{
    AssertMutable();

    PdfArrayBaseClass::erase( first, last );
    m_bDirty = true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfArray::reserve(size_type __n)
{
    PdfArrayBaseClass::reserve( __n );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfObject & PdfArray::front()
{
    return PdfArrayBaseClass::front();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfObject & PdfArray::front() const
{
    return PdfArrayBaseClass::front();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfObject & PdfArray::back()
{
    return PdfArrayBaseClass::back();
}
      
// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfObject & PdfArray::back() const
{
    return PdfArrayBaseClass::back();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfArray::operator==( const PdfArray & rhs ) const
{
    //TODO: This operator does not check for m_bDirty. Add comparison or add explanation why it should not be there
    return (static_cast< PdfArrayBaseClass >(*this) == static_cast< PdfArrayBaseClass >(rhs) );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfArray::operator!=( const PdfArray & rhs ) const
{
    //TODO: This operator does not check for m_bDirty. Add comparison or add explanation why it should not be there
    return (static_cast< PdfArrayBaseClass >(*this) != static_cast< PdfArrayBaseClass >(rhs) );
}

typedef PdfArray                 TVariantList;
typedef PdfArray::iterator       TIVariantList;
typedef PdfArray::const_iterator TCIVariantList;

};

#endif // _PDF_ARRAY_H_
