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
#include "PdfReference.h"
#include "PdfVariant.h"

namespace PoDoFo {

class PdfName;
class PdfObject;
class PdfOutputDevice;
class PdfStream;
class PdfString;

typedef std::map<PdfName,PdfVariant>       TKeyMap;
typedef TKeyMap::iterator                  TIKeyMap;
typedef TKeyMap::const_iterator            TCIKeyMap;

typedef std::map<PdfName,PdfObject*>       TObjKeyMap;
typedef TObjKeyMap::iterator               TIObjKeyMap;
typedef TObjKeyMap::const_iterator         TCIObjKeyMap;

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
class PdfObject {
 public:
    /** Construct a new PDF object.
     *  \param objectno the object number, maybe 0 if you call SetDirect( true ) later 
     *                  (direct objects do not have object numbers)
     *  \param generationno the generation number which is almost ever 0
     *  \param pszType if type is not null a key "/Type" will be added to the dictionary with
     *                 the value of type.
     *  \see SetDirect
     */
    PdfObject( unsigned int objectno, unsigned int generationno, const char* pszType = NULL );

    /** Creates a copy of an existing PdfObject
     *  All assosiated objects and streams will be copied along with the PdfObject
     *  \param rhs PdfObject to clone
     */
    PdfObject( const PdfObject & rhs );

    virtual ~PdfObject();

    /** Set wether this is an direct object or not.
     *  A direct PDF Object is part of another dictionary and has
     *  no own object and generation number.
     *  The default value for this property is false.
     *  \param direct if true this is set to be an direct object.
     *  \see IsDirect
     */
    inline void SetDirect( bool direct );

    /** Get if this is an direct object.
     *  \returns true if this is an direct object
     *  \see SetDirect
     */
    inline bool IsDirect() const;

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

    /** Add a key to the dictionary. It is parsed as a PdfVariant and added.
     *  \param identifier the key is identified by this name in the dictionary
     *  \param rValue the data of this key. Has to be a valid PDF datatype.
     *  \returns ErrOk on sucess
     */
    PdfError AddKey( const PdfName & identifier, const std::string & rValue );

    /** Add a key to the dictionary. It is parsed as a PdfVariant and added.
     *  \param identifier the key is identified by this name in the dictionary
     *  \param pszValue the data of this key. Has to be a valid PDF datatype.
     *  \returns ErrOk on sucess
     */
    PdfError AddKey( const PdfName & identifier, const char* pszValue );

    /** Add a key to the dictionary.
     *  \param identifier the key is identified by this name in the dictionary
     *  \param dValue the data of this key.
     *  \returns ErrOk on sucess
     */
    PdfError AddKey( const PdfName & identifier, double dValue );

    /** Add a key to the dictionary.
     *  \param identifier the key is identified by this name in the dictionary
     *  \param nValue the data of this key.
     *  \returns ErrOk on sucess
     */
    PdfError AddKey( const PdfName & identifier, long nValue );

    /** Add a key to the dictionary whose value is a PdfString
     *  \param identifier the key is identified by this name in the dictionary
     *  \param rValue the data of this key. 
     *  \returns ErrOk on sucess
     */
    PdfError AddKey( const PdfName & identifier, const PdfString & rValue );

    /** Add a key to the dictionary whose value is again a PdfName
     *  \param identifier the key is identified by this name in the dictionary
     *  \param rValue the data of this key. 
     *  \returns ErrOk on sucess
     */
    PdfError AddKey( const PdfName & identifier, const PdfName & rValue );

    /** Add a key to the dictionary whose value is a PdfReference
     *  \param identifier the key is identified by this name in the dictionary
     *  \param rValue the data of this key. 
     *  \returns ErrOk on sucess
     */
    PdfError AddKey( const PdfName & identifier, const PdfReference & rValue );

    /** Add a key to the dictionary. This is the fastest way to add a key
     *  as all other functions will have to parse the values given to them first.
     *  This is not necessary in this case
     *  \param identifier the key is identified by this name in the dictionary
     *  \param rVariant a variant object containing the data.
     *  \returns ErrOk on sucess
     */
    PdfError AddKey( const PdfName & identifier, const PdfVariant & rVariant );

    /** Add a key to the dictionary. This key has an internal PDF obect as value
     *  \param identifier the key is identified by this name in the dictionary
     *  \param pObj use this object as internal object. The object will get owned
     *         by this PdfObject. Be sure to never create internal objects with
     *         PdfWriter::CreateObject!
     */
    void AddKey( const PdfName & identifier, PdfObject* pObj );

    /** Remove a key from this object
     *  If the key does not exists, this function does nothing.
     *
     *  \param identifier the name of the key to delete
     * 
     *  \returns true if the key was found in the object and was removed
     *           if there was is no key with this name, false is returned.
     */
    bool RemoveKey( const PdfName & identifier );

    /** Remove a key from this object which has an associated internal object
     *  If the key does not exists, this function does nothing.
     *
     *  \param identifier the name of the key to delete
     * 
     *  \returns true if the key was found in the object and was removed
     *           if there was is no key with this name, false is returned.
     */
    bool RemoveObjectKey( const PdfName & identifier );

    /** Removes all keys from the dictionary
     */
    void ClearKeys();

    /** Removes all object keys from this dictionary
     */
    void ClearObjectKeys();

    /** Get a key from the object.
     *  \param key the key.
     *  \param sDefault a default value if the key is not found.
     *  \returns the value of the key as string
     */
    const PdfString & GetKeyValueString ( const PdfName & key, const PdfString & sDefault = PdfString::StringNull ) const;

    /** Get a key from the object.
     *  \param key the key.
     *  \param lDefault a default value if the key is not found.
     *  \returns the value of the key as long
     */
    long  GetKeyValueLong   ( const PdfName & key, long lDefault ) const;

    /** Get a key from the object.
     *  \param key the key.
     *  \param bDefault a default value if the key is not found.
     *  \returns the value of the key as bool
     */
    bool  GetKeyValueBool   ( const PdfName & key, bool bDefault ) const;

          /** Get the key data of an internal object.
           *  \param key look for the key named pszKey in the dictionary
           *  \returns the object or NULL if the object was not found.
           */
    PdfObject*  GetKeyValueObject( const PdfName & key ) const;

          /** Get the keys data inside of a variant object.
           *  \param key look for the key names pszKey in the dictionary
           *  \param rVariant the variant is returned in this object
           *  \returns ErrOk on sucess
           */
    PdfError     GetKeyValueVariant( const PdfName & key, PdfVariant & rVariant ) const;

          /** Allows to check if a dictionary contains a certain key.
           *  \param key look for the key named key.Name() in the dictionary
           *  \returns true if the key is part of the dictionary, otherwise false.
           */
          bool  HasKey            ( const PdfName & key  ) const;

          /** Allows to check if a dictionary contains a certain key which is an internal object.
           *  \param key look for the key named key in the dictionary
           *  \returns true if the key is part of the dictionary, otherwise false.
           */
          bool  HasObjectKey      ( const PdfName & key ) const;

    /** Sets the object to have only a single value 
     *  (instead of more dictionary keys), any existing keys will be
     *  cleared by calling this function.
     *  
     *  \param var a PdfVariant
     */
    void SetSingleValue( const PdfVariant & var );


    const PdfString & GetSingleValueString () const;
          long  GetSingleValueLong   () const;
          bool  GetSingleValueBool   () const;
    const PdfVariant & GetSingleValueVariant () const;


    /** Get read only access to all keys in the dictionary.
     *  You can iterate yourself over this map.
     *  \returns the internal key map.
     */
    inline const TKeyMap & GetKeys() const;

    /** Get read only access to all object keys in the dictionary.
     *  You can iterate yourself over this map.
     *  \returns the internal object key map.
     */
    inline const TObjKeyMap & GetObjectKeys() const;

    /** \returns true if this object has only a single value
     *           and is no dictionary.
     */
    inline bool HasSingleValue() const;

    /** Write the complete object to a file.
     *  \param pDevice write the object to this device
     *  \param keyStop if not KeyNull and a key == keyStop is found
     *                 writing will stop right before this key!
     *  \returns ErrOk on success
     */
    PdfError Write( PdfOutputDevice* pDevice, const PdfName & keyStop = PdfName::KeyNull );

    /** Get the length of the object in bytes if it was written to disk now.
     *  \param pulLength safe the length of the object in to this unsigned long
     *  \returns ErrOk on success
     */
    PdfError GetObjectLength( unsigned long* pulLength );

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

    /** Set the object number of this object.
     *  It is almost never necessary to use this call.
     *  It is only included for usage in the PdfParser.
     *  DO NOT USE IT!
     *
     *  \param nObjNo the new object number of this object
     */
    inline void SetObjectNumber( unsigned int nObjNo );

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
     *
     *  \returns ErrOk on sucess
     */
    PdfError FlateDecodeStream();

    /** Calculate the byte offset of the key pszKey from the start of the object
     *  if the object was written to disk at the moment of calling this function.
     *
     *  This function is very calculation intensive!
     *
     *  \param pszKey  key to calculate the byte offset
     *  \param pulOffset pointer to an unsigned long to save the offset
     *  \returns ErrOk on success
     */
    PdfError GetByteOffset( const char* pszKey, unsigned long* pulOffset );

 private:
    /** Initialize all private members with their default values
     */
    void Init();

    /** Clear all internal structures and free alocated memory
     */
    void Clear();

 protected:
    PdfReference m_reference;
    bool         m_bDirect;
    bool         m_bEmptyEntry;

    TKeyMap      m_mapKeys;
    TObjKeyMap   m_mapObjKeys;

    PdfVariant   m_singleValue;
    PdfStream*   m_pStream;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfObject::SetDirect( bool bDirect )
{
    m_bDirect = bDirect;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfObject::IsDirect() const 
{ 
    return m_bDirect; 
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
void PdfObject::SetObjectNumber( unsigned int nObjNo )
{
    m_reference.SetObjectNumber( nObjNo );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const TKeyMap & PdfObject::GetKeys() const
{
    return m_mapKeys;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const TObjKeyMap & PdfObject::GetObjectKeys() const
{
    return m_mapObjKeys;
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
bool PdfObject::HasSingleValue() const
{
    return !m_singleValue.IsEmpty();
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
    return (bool)m_pStream;
}

};

#endif // _PDF_OBJECT_H_

