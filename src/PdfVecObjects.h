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

#ifndef _PDF_VEC_OBJECTS_H_
#define _PDF_VEC_OBJECTS_H_

#include "PdfDefines.h"
#include "PdfReference.h"

namespace PoDoFo {

class PdfObject;

/** A STL vector of PdfObjects. I.e. a list of PdfObject classes.
 *  The PdfParser will read the PdfFile into memory and create 
 *  a PdfVecObjects of all dictionaries found in the PDF file.
 * 
 *  The PdfWriter class contrary creates a PdfVecObjects internally
 *  and writes it to a PDF file later with an appropriate  table of 
 *  contents.
 *
 *  These class contains also advanced funtions for searching of PdfObject's
 *  in a PdfVecObject. 
 */
class PdfVecObjects : public std::vector<PdfObject*> {
    friend class PdfWriter;

 public:
    /** Default constuctor 
     */
    PdfVecObjects();

    /** Copy constructor
     *  \param rhs the object to copy
     */
    PdfVecObjects( const PdfVecObjects & rhs );

    virtual ~PdfVecObjects();

    /** Assign another PdfVecObjects to this PdfVecObjects
     *  \param rhs the object to copy
     *  \returns this object
     */
    const PdfVecObjects & operator=( const PdfVecObjects & rhs );

    /** Enable/disable auto deletion.
     *  \param bAutoDelete if true all objects will be deleted when the PdfVecObjects is 
     *         deleted.
     */
    inline void SetAutoDelete( bool bAutoDelete );

    /** 
     *  \returns if autodeletion is enabled and all objects will be deleted when the PdfVecObjects is 
     *           deleted.
     */
    inline bool AutoDelete() const;

    /** Finds the object with object no lObject and generation
     *  np lGeneration in m_vecOffsets and returns a pointer to it
     *  if it is found.
     *  \param ref the object to be found
     *  \returns the found object or NULL if no object was found.
     */
    PdfObject* GetObject( const PdfReference & ref ) const;

    /** Remove the object with the given object and generation number from the list
     *  of objects.
     *  The object is returned if it was found. Otherwise NULL is returned.
     *  The caller has to delte the object by hisself.
     *
     *  \param ref the object to be found
     *  \returns The removed object.
     */
    PdfObject* RemoveObject( const PdfReference & ref );

    /** Create a PdfObject of type T which must be a subclasss of PdfObject
     *  and it does not need a parameter for pszType.
     *  This function assigns the next free object number to the PdfObject
     *  and add is to the internal vector.
     *
     *  \returns a new PdfObject subclasss
     */
    template <class T> T* CreateObject();

    /** Creates a new object and inserts it into the vector.
     *  This function assigns the next free object number to the PdfObject.
     *
     *  \param pszType optionall value of the /Type key of the object
     *  \returns PdfObject pointer to the new PdfObject
     */
    PdfObject* CreateObject( const char* pszType = NULL );

 private:
    bool     m_bAutoDelete;
    size_t   m_nObjectCount;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfVecObjects::SetAutoDelete( bool bAutoDelete ) 
{
    m_bAutoDelete = bAutoDelete;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfVecObjects::AutoDelete() const
{
    return m_bAutoDelete;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
template <class T>
T* PdfVecObjects::CreateObject()
{
    T*         pTemplate = new T( m_nObjectCount++, 0 );
    PdfObject* pObject   = dynamic_cast<PdfObject*>(pTemplate);

    if( !pObject )
    {
        delete pTemplate;
        return NULL;
    }

    this->push_back( pObject );
    return pTemplate;
}

typedef PdfVecObjects                TVecObjects;
typedef TVecObjects::iterator        TIVecObjects;
typedef TVecObjects::const_iterator  TCIVecObjects;

};

#endif // _PDF_VEC_OBJECTS_H_
