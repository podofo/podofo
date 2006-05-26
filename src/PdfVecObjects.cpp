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

#include "PdfVecObjects.h"

#include "PdfObject.h"

#include <algorithm>

namespace PoDoFo {

class ObjectsComperator { 
public:
    ObjectsComperator( long lObject, long lGeneration )
        {
            m_lObject     = lObject;
            m_lGeneration = lGeneration;
        }
    
    bool operator()(const PdfObject* p1) const { 
        return (p1->ObjectNumber() == m_lObject && p1->GenerationNumber() == m_lGeneration );
    }
private:
    long m_lObject;
    long m_lGeneration;
};

PdfObject* PdfVecObjects::GetObject( long lObject, long lGeneration ) const
{
    TCIVecObjects it;

    it = std::find_if( this->begin(), this->end(), ObjectsComperator( lObject, lGeneration ) );
    
    if( it != this->end() )
        return (*it);

    return NULL;
}

PdfObject* PdfVecObjects::RemoveObject( long lObject, long lGeneration )
{
    TIVecObjects it;
    PdfObject*   pObj;

    it = std::find_if( this->begin(), this->end(), ObjectsComperator( lObject, lGeneration ) );
    
    if( it != this->end() )
    {
        pObj = *it;
        this->erase( it );
        return pObj;
    }

    return NULL;
}



};

