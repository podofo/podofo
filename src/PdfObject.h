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
    /** Construct a new PDF object.
     *  \param objectno the object number
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

    /** Load all data of the object if load object on demand is enabled
     */
    inline virtual PdfError LoadOnDemand();

    /** Load the stream of the object if it has one and if loading on demand is enabled
     */
    inline virtual PdfError LoadStreamOnDemand();

 protected:
    /** Initialize all private members with their default values
     */
    void Init();

    /** Clear all internal structures and free alocated memory
     */
    void Clear();

    /**
     *  \returns ErrOk on sucess
     */
    inline virtual PdfError DelayedLoad() const;

    /**
     *  \returns ErrOk on sucess
     */
    inline PdfError DelayedStreamLoad() const;

 protected:
    PdfReference m_reference;
    bool         m_bEmptyEntry;
    
    bool         m_bLoadOnDemandDone;
    bool         m_bLoadStreamOnDemandDone;
    
    PdfStream*   m_pStream;
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
inline PdfError PdfObject::LoadOnDemand()
{
    m_bLoadOnDemandDone = true;
    return PdfError();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfError PdfObject::LoadStreamOnDemand()
{
    PdfError eCode = DelayedLoad();
    m_bLoadStreamOnDemandDone = true;
    return eCode;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfError PdfObject::DelayedLoad() const
{
    PdfError eCode;
    
    if( !m_bLoadOnDemandDone ) 
    {
        PdfObject* p = const_cast<PdfObject*>(this);
        SAFE_OP( p->LoadOnDemand() );
    }

    return eCode;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfError PdfObject::DelayedStreamLoad() const
{
    PdfError eCode;

    if( !m_bLoadOnDemandDone ) 
    {
        PdfObject* p = const_cast<PdfObject*>(this);
        SAFE_OP( p->LoadOnDemand() );
    }

    if( !m_bLoadStreamOnDemandDone ) 
    {
        PdfObject* p = const_cast<PdfObject*>(this);
        SAFE_OP( p->LoadStreamOnDemand() );
    }

    return eCode;
}

};

#endif // _PDF_OBJECT_H_

