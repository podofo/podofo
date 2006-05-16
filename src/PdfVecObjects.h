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

namespace PoDoFo {

class PdfObject;

class PdfVecObjects : public std::vector<PdfObject*> {
 public:

    /** Finds the object with object no lObject and generation
     *  np lGeneration in m_vecOffsets and returns a pointer to it
     *  if it is found.
     *  \param lObject the object number of the object to be found
     *  \param lGeneration the object number of the object to be found
     *  \returns the found object or NULL if no object was found.
     */
    PdfObject* GetObject( long lObject, long lGeneration ) const;
};

typedef PdfVecObjects                TVecObjects;
typedef TVecObjects::iterator        TIVecObjects;
typedef TVecObjects::const_iterator  TCIVecObjects;

};

#endif // _PDF_VEC_OBJECTS_H_
