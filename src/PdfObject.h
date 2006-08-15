/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#ifndef _PDF_OBJECT_H_
#define _PDF_OBJECT_H_

#include "PdfDefines.h"
#include "PdfName.h"
#include "PdfParser.h"
#include "PdfReference.h"
#include "PdfString.h"
#include "PdfVariant.h"

namespace PoDoFo {

class PdfObject;
class PdfOutputDevice;
class PdfStream;
class PdfVecObjects;

/**
 * This class represents a PDF Object into the memory
 * 
 * It allows to get or set key values. To check for existing keys
 * and to manipulate the optional stream which can be appended to 
 * the object.
 * It is uniquely identified by an object number and a gerneration number
 * which has to be passed to the constructor.
 *
 * The object can be written to a file easily using the Write() function.
 *
 * \see Write()
 */
class PdfObject : public PdfVariant {
 public:

    /** Create a PDF object with object and generation number -1
     *  and the value of being an empty PdfDictionary.
     */
    PdfObject();

    /** Construct a new PDF object of type PdfDictionary
     *  \param objectno the object number
     *  \param generationno the generation number which is almost ever 0
     *  \param pszType if type is not null a key "/Type" will be added to the dictionary with
     *                 the value of type.
     *  \see SetDirect
     */
    PdfObject( unsigned long objectno, unsigned long generationno, const char* pszType = NULL );

    /** Construct a new PDF object of type PdfArray
     *  \param objectno the object number
     *  \param generationno the generation number which is almost ever 0
     *  \param numElems the number of elements in the array to init it to
     *  \see SetDirect
     */
	PdfObject( unsigned long objectno, unsigned long generationno, int numElems );

    /** Construct a new PDF object.
     *  \param objectno the object number
     *  \param generationno the generation number which is almost ever 0
     *  \param rVariant the value of the PdfObject
     *  \see SetDirect
     */
    PdfObject( unsigned long objectno, unsigned long generationno, const PdfVariant & rVariant );

    /** Create a PDF object with object and generation number -1
     *  and the value of the passed variant.
     *
     *  \param var the value of the object
     */
    PdfObject( const PdfVariant & var );

    /** Construct a PdfObject with object and generation number -1
     *  and a bool as value.
     *
     *  \param b the boolean value of this PdfObject
     */
    PdfObject( bool b );

    /** Construct a PdfObject with object and generation number -1
     *  and a long as value.
     *
     *  \param l the long value of this PdfObject
     */
    PdfObject( long l );

    /** Construct a PdfObject with object and generation number -1
     *  and a double as value.
     *
     *  \param d the double value of this PdfObject
     */
    PdfObject( double d );

    /** Construct a PdfObject with object and generation number -1
     *  and a PdfString as value.
     *
     *  \param rsString the string value of this PdfObject
     */        
    PdfObject( const PdfString & rsString );

    /** Construct a PdfObject with object and generation number -1
     *  and a PdfName as value.
     *
     *  \param rName the value of this PdfObject
     */        
    PdfObject( const PdfName & rName );

    /** Construct a PdfObject with object and generation number -1
     *  and a PdfReference as value.
     *
     *  \param rRef the value of the this PdfObject
     */        
    PdfObject( const PdfReference & rRef );

    /** Construct a PdfObject with object and generation number -1
     *  and a PdfArray as value.
     *
     *  \param tList the value of the this PdfObject
     */        
    PdfObject( const PdfArray & tList );

    /** Construct a PdfObject with object and generation number -1
     *  and a PdfDictionary as value.
     *
     *  \param rDict the value of the this PdfObject
     */        
    PdfObject( const PdfDictionary & rDict );

    /** Creates a copy of an existing PdfObject
     *  All assosiated objects and streams will be copied along with the PdfObject
     *  \param rhs PdfObject to clone
     */
    PdfObject( const PdfObject & rhs );

    virtual ~PdfObject();

    /** Specifies wether this is an empty dictionary.
     *  i.e. the PdfWriter, should create an empty
     *  entry for this object in the XRef table.
     *  \param empty  true or false
     */
    inline void SetEmptyEntry( bool empty );

    /** Returns if this is object will be an empty
     *  xref entry when it is written to disc by the PDF writer
     *  \returns true or false
     */
    inline bool IsEmptyEntry() const;

    /** Get the keys value out of the dictionary. If the key is a reference, 
     *  the reference is resolved and the object pointed to by the reference is returned.
     *
     *
     *  \param key look for the key named key in the dictionary
     * 
     *  \returns the found value or NULL if the value is not in the 
     *           dictionary or if this object is no dictionary
     */
    PdfObject* GetIndirectKey( const PdfName & key );

    /** Write the complete object to a file.
     *  \param pDevice write the object to this device
     *  \param keyStop if not KeyNull and a key == keyStop is found
     *                 writing will stop right before this key!
     */
    void WriteObject( PdfOutputDevice* pDevice, const PdfName & keyStop = PdfName::KeyNull ) const;

    /** Get the length of the object in bytes if it was written to disk now.
     *  \returns  the length of the object
     */
     unsigned long GetObjectLength();

    /** Get a indirect reference to this object
     *  \returns a PdfReference pointing to this object.
     */
    inline const PdfReference & Reference() const;

    /** Get a handle to a PDF stream object
     *  If the PDF object does not have a stream,
     *  one will be created.
     *  \returns a PdfStream object
     */
    PdfStream* Stream();

    /** Get a handle to a const PDF stream object
     *  If the PDF object does not have a stream,
     *  null is returned
     *  \returns a PdfStream object or null
     */
    const PdfStream* Stream() const;

    /** Check if this object has a PdfStream object
     *  appended.
     * 
     *  \returns true if the object has a stream
     */
    inline bool HasStream() const;

    /** Get the object number of this object
     *  \returns the object number
     */
    inline unsigned int ObjectNumber() const;

    /** Get the generation number of this object
     *  \returns the generation number
     */
    inline unsigned int GenerationNumber() const;

    /** This operator is required for sorting a list of 
     *  PdfObjects. It compares the objectnumber. If objectnumbers
     *  are equal, the generation number is compared.
     */
    inline bool operator<( const PdfObject & rhs );

    /** Comperasion operator.
     *  Compares two PDF object only based on their object and generation number
     */
    inline bool operator==( const PdfObject & rhs );

    /** Set the parent of this object
     *  \param pVecObjects a vector of pdf objects
     */
    inline void SetParent( PdfVecObjects* pVecObjects );

    /** Get the parent of this object
     *  \return the parent of this object
     */
    inline PdfVecObjects* GetParent() const;

    /** Creates a copy of an existing PdfObject
     *  All assosiated objects and streams will be copied along with the PdfObject
     *  \param rhs PdfObject to clone
     *  \returns a reference to this object
     */
    const PdfObject & operator=( const PdfObject & rhs );

    /** This function compresses any currently set stream
     *  using the FlateDecode algorithm. JPEG compressed streams
     *  will not be compressed again using this function.
     *  Entries to the filter dictionary will be added if necessary.
     */
    void FlateDecodeStream();

    /** Calculate the byte offset of the key pszKey from the start of the object
     *  if the object was written to disk at the moment of calling this function.
     *
     *  This function is very calculation intensive!
     *
     *  \param pszKey  key to calculate the byte offset
     *  \returns the offset of the key 
     */
    unsigned long GetByteOffset( const char* pszKey );

    /** Load all data of the object if load object on demand is enabled
     */
    inline virtual void LoadOnDemand();

    /** Load the stream of the object if it has one and if loading on demand is enabled
     */
    inline virtual void LoadStreamOnDemand();

 protected:
    /** Initialize all private members with their default values
     *  \param bLoadOnDemandDone wether loading on demand is supported
     */
    void Init( bool bLoadOnDemandDone );

    /** Clear all internal structures and free alocated memory
     */
    void Clear();

    /**
     */
    inline virtual void DelayedLoad() const;

    /**
     */
    inline void DelayedStreamLoad() const;

 protected:
    PdfReference   m_reference;
    bool           m_bEmptyEntry;
    
    bool           m_bLoadOnDemandDone;
    bool           m_bLoadStreamOnDemandDone;
    
    PdfStream*     m_pStream;
    PdfVecObjects* m_pParent;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfObject::SetEmptyEntry( bool empty )
{
    m_bEmptyEntry = empty;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfObject::IsEmptyEntry() const
{
    return m_bEmptyEntry;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned int PdfObject::ObjectNumber() const
{ 
    return m_reference.ObjectNumber(); 
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned int PdfObject::GenerationNumber() const 
{ 
    return m_reference.GenerationNumber(); 
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfReference & PdfObject::Reference() const
{
    return m_reference;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfObject::SetParent( PdfVecObjects* pVecObjects )
{
    m_pParent = pVecObjects;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfVecObjects* PdfObject::GetParent() const
{
    return m_pParent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfObject::operator<( const PdfObject & rhs )
{
    if( ObjectNumber() == rhs.ObjectNumber() )
        return GenerationNumber() < rhs.GenerationNumber();
    else
        return ObjectNumber() < rhs.ObjectNumber();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfObject::operator==( const PdfObject & rhs )
{
    return (m_reference == rhs.m_reference);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfObject::HasStream() const
{
    DelayedStreamLoad();

    return ( m_pStream != NULL );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfObject::LoadOnDemand()
{
    m_bLoadOnDemandDone = true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfObject::LoadStreamOnDemand()
{
    DelayedLoad();
    m_bLoadStreamOnDemandDone = true;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfObject::DelayedLoad() const
{
    if( !m_bLoadOnDemandDone ) 
    {
        PdfObject* p = const_cast<PdfObject*>(this);
        p->LoadOnDemand();
    }
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfObject::DelayedStreamLoad() const
{
    if( !m_bLoadOnDemandDone ) 
    {
        PdfObject* p = const_cast<PdfObject*>(this);
        p->LoadOnDemand();
    }

    if( !m_bLoadStreamOnDemandDone ) 
    {
        PdfObject* p = const_cast<PdfObject*>(this);
        p->LoadStreamOnDemand();
    }
}

};

#endif // _PDF_OBJECT_H_

