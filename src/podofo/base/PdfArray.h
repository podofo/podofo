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
#include "PdfOwnedDataType.h"
#include "PdfObject.h"

namespace PoDoFo {

/** This class represents a PdfArray
 *  Use it for all arrays that are written to a PDF file.
 *  
 *  A PdfArray can hold any PdfVariant.
 *
 *  \see PdfVariant
 */
class PODOFO_API PdfArray : public PdfOwnedDataType {
 public:
    typedef size_t                                          size_type;
    typedef PdfObject                                       value_type;
    typedef value_type &                                    reference;
    typedef const value_type &                              const_reference;
    typedef std::vector<value_type>::iterator               iterator;
    typedef std::vector<value_type>::const_iterator         const_iterator;
    typedef std::vector<value_type>::reverse_iterator       reverse_iterator;
    typedef std::vector<value_type>::const_reverse_iterator const_reverse_iterator;

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

    /** Get the object at the given index out of the array.
     *
     * Lookup in the indirect objects as well, if the shallow object was a reference.
     * The returned value is a pointer to the internal object in the dictionary
     * so it MUST not be deleted.
     *
     *  \param idx
     *  \returns pointer to the found value. NULL if the index was out of the boundaries
     */
    inline const PdfObject * FindAt( size_type idx ) const;
    inline PdfObject * FindAt( size_type idx );

    /** Adds a PdfObject to the array
     *
     *  \param var add a PdfObject to the array
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    inline void push_back( const PdfObject & var );

    /** Remove all elements from the array
     */
    void clear();

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
     * \param count new size
     * \param value refernce value
     */
    void resize( size_t count, value_type val = value_type() );
    
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

    iterator insert( const iterator &pos, const PdfObject &val );

    void erase( const iterator& pos );
    void erase( const iterator& first, const iterator& last );

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

 protected:
     void SetOwner( PdfObject* pOwner );

 private:
    PdfObject * findAt(size_type idx) const;

 private:
    bool         m_bDirty; ///< Indicates if this object was modified after construction
    std::vector<PdfObject> m_objects;
};

// -----------------------------------------------------
//
// -----------------------------------------------------
inline const PdfObject * PdfArray::FindAt( size_type idx ) const
{
    return findAt( idx );
}

// -----------------------------------------------------
//
// -----------------------------------------------------
inline PdfObject * PdfArray::FindAt( size_type idx )
{
    return findAt( idx );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
size_t PdfArray::GetSize() const
{
    return m_objects.size();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfArray::push_back( const PdfObject & var )
{
    insert( end(), var );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfArray::Clear()
{
    clear();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
size_t PdfArray::size() const
{
    return m_objects.size();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfArray::empty() const
{
    return m_objects.empty();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfObject& PdfArray::operator[](size_type __n)
{
    AssertMutable();

    return m_objects[__n];
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfObject& PdfArray::operator[](size_type __n) const
{
    return m_objects[__n];
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::iterator PdfArray::begin()
{
    return m_objects.begin();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::const_iterator PdfArray::begin() const
{
    return m_objects.begin();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::iterator PdfArray::end()
{
    return m_objects.end();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::const_iterator PdfArray::end() const
{
    return m_objects.end();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::reverse_iterator PdfArray::rbegin()
{
    return m_objects.rbegin();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::const_reverse_iterator PdfArray::rbegin() const
{
    return m_objects.rbegin();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::reverse_iterator PdfArray::rend()
{
    return m_objects.rend();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfArray::const_reverse_iterator PdfArray::rend() const
{
    return m_objects.rend();
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

    PdfVecObjects *pOwner = GetObjectOwner();
    iterator it1 = __first;
    iterator it2 = __position;
    for ( ; it1 != __last; it1++, it2++ )
    {
        it2 = m_objects.insert( it2, *it1 );
        if ( pOwner != NULL )
            it2->SetOwner( pOwner );
    }

    m_bDirty = true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfArray::reserve( size_type __n )
{
    AssertMutable();

    m_objects.reserve( __n );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfObject & PdfArray::front()
{
    return m_objects.front();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfObject & PdfArray::front() const
{
    return m_objects.front();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfObject & PdfArray::back()
{
    return m_objects.back();
}
      
// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfObject & PdfArray::back() const
{
    return m_objects.back();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfArray::operator==( const PdfArray & rhs ) const
{
    //TODO: This operator does not check for m_bDirty. Add comparison or add explanation why it should not be there
    return m_objects == rhs.m_objects;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfArray::operator!=( const PdfArray & rhs ) const
{
    //TODO: This operator does not check for m_bDirty. Add comparison or add explanation why it should not be there
    return m_objects != rhs.m_objects;
}

typedef PdfArray                 TVariantList;
typedef PdfArray::iterator       TIVariantList;
typedef PdfArray::const_iterator TCIVariantList;

};

#endif // _PDF_ARRAY_H_
