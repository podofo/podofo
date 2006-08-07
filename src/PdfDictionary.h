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

#ifndef _PDF_DICTIONARY_H_
#define _PDF_DICTIONARY_H_

#include "PdfDefines.h"
#include "PdfDataType.h"

#include "PdfName.h"
#include "PdfObject.h"

namespace PoDoFo {

typedef std::map<PdfName,PdfObject*>      TKeyMap;
typedef TKeyMap::iterator                 TIKeyMap;
typedef TKeyMap::const_iterator           TCIKeyMap;

class PdfOutputDevice;

class PdfDictionary : public PdfDataType {
 public:
    /** Create a new, empty dictionary
     */
    PdfDictionary();

    /** Copy a dictionary
     *  \param rhs the PdfDictionary to copy
     */
    PdfDictionary( const PdfDictionary & rhs );

    /** Destructor
     */
    virtual ~PdfDictionary();

    /** Asignment operator.
     *  Asign another PdfDictionary to this dictionary.
     *  \param rhs the PdfDictionary to copy.
     *
     *  \return this PdfDictionary
     */
    const PdfDictionary & operator=( const PdfDictionary & rhs );

    /** Removes all keys from the dictionary
     */
    void Clear();

    /** Add a key to the dictionary. 
     *
     *  \param identifier the key is identified by this name in the dictionary
     *  \param rObject a variant object containing the data.
     */
    void AddKey( const PdfName & identifier, const PdfObject & rObject );

    /** Add a key to the dictionary. 
     *  This is an overloaded member function.
     *
     *  \param identifier the key is identified by this name in the dictionary
     *  \param rObject a variant object containing the data.
     */
    void AddKey( const PdfName & identifier, const PdfObject* pObject );

    /** Get the keys value out of the dictionary.
     *
     *  \param key look for the key names pszKey in the dictionary
     * 
     *  \returns the found value of an empty PdfVariant if the key was not found
     */
    const PdfObject* GetKey( const PdfName & key ) const;

    /** Get the keys value out of the dictionary.
     *  This is an overloaded member function.
     *
     *  \param key look for the key named key in the dictionary
     * 
     *  \returns the found value of an empty PdfVariant if the key was not found
     */
    PdfObject* GetKey( const PdfName & key );

    long GetKeyAsLong( const PdfName & key, long lDefault = 0 ) const;

    bool GetKeyAsBool( const PdfName & key, bool bDefault = false ) const;

    PdfName GetKeyAsName( const PdfName & key ) const;

    /** Allows to check if a dictionary contains a certain key.
     *  \param key look for the key named key.Name() in the dictionary
     *
     *  \returns true if the key is part of the dictionary, otherwise false.
     */
    bool  HasKey( const PdfName & key  ) const;

    /** Remove a key from this dictionary.
     *  If the key does not exists, this function does nothing.
     *
     *  \param identifier the name of the key to delete
     * 
     *  \returns true if the key was found in the object and was removed
     *           if there was is no key with this name, false is returned.
     */
    bool RemoveKey( const PdfName & identifier );

    /** Write the complete dictionary to a file.
     *  \param pDevice write the object to this device
     *  \returns ErrOk on success
     */
    inline void Write( PdfOutputDevice* pDevice ) const;

    /** Write the complete dictionary to a file.
     *  \param pDevice write the object to this device
     *  \param keyStop if not KeyNull and a key == keyStop is found
     *                 writing will stop right before this key!
     *  \returns ErrOk on success
     */
    void Write( PdfOutputDevice* pDevice, const PdfName & keyStop = PdfName::KeyNull ) const;

    /** Get access to the internal map of keys.
     *  \returns all keys of this dictionary
     */
    inline const TKeyMap & GetKeys() const;

 private:
    TKeyMap      m_mapKeys;
};

typedef std::vector<PdfDictionary*>      TVecDictionaries;
typedef TVecDictionaries::iterator       TIVecDictionaries;
typedef TVecDictionaries::const_iterator TCIVecDictionaries;

// -----------------------------------------------------
// 
// -----------------------------------------------------
const TKeyMap & PdfDictionary::GetKeys() const
{
    return m_mapKeys;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfDictionary::Write( PdfOutputDevice* pDevice ) const
{
    return this->Write( pDevice, PdfName::KeyNull );
}

};

#endif // _PDF_DICTIONARY_H_
