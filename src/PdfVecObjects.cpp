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
    ObjectsComperator( const PdfReference & ref )
        : m_ref( ref )
        {
        }
    
    bool operator()(const PdfObject* p1) const { 
        return (p1->Reference() == m_ref );
    }
private:
    const PdfReference m_ref;
};

PdfVecObjects::PdfVecObjects()
    : m_bAutoDelete( false ), m_nObjectCount( 1 )
{
}

PdfVecObjects::PdfVecObjects( const PdfVecObjects & rhs )
{
    this->operator=( rhs );
}

PdfVecObjects::~PdfVecObjects()
{
    if( m_bAutoDelete ) 
    {
        TIVecObjects it = this->begin();
        while( it != this->end() )
        {
            delete *it;
            ++it;
        }
    }
}

const PdfVecObjects & PdfVecObjects::operator=( const PdfVecObjects & rhs )
{
    TIVecObjects it;
    std::vector<PdfObject*>::operator=( rhs );

    m_bAutoDelete  = rhs.m_bAutoDelete;
    m_nObjectCount = rhs.m_nObjectCount;

    it = this->begin();
    while( it != this->end() )
    {
        (*it)->SetParent( this );
        ++it;
    }

    return *this;
}

PdfObject* PdfVecObjects::GetObject( const PdfReference & ref ) const
{
    TCIVecObjects it;

    it = std::find_if( this->begin(), this->end(), ObjectsComperator( ref ) );
    
    if( it != this->end() )
        return (*it);

    return NULL;
}

PdfObject* PdfVecObjects::RemoveObject( const PdfReference & ref )
{
    TIVecObjects it;
    PdfObject*   pObj;

    it = std::find_if( this->begin(), this->end(), ObjectsComperator( ref ) );
    
    if( it != this->end() )
    {
        pObj = *it;
        this->erase( it );
        return pObj;
    }

    return NULL;
}

PdfObject* PdfVecObjects::CreateObject( const char* pszType )
{
    PdfObject* pObj = new PdfObject( m_nObjectCount, 0, pszType );

    this->push_back( pObj );

    return pObj;
}

PdfObject* PdfVecObjects::CreateObject( int numElems )
{
	PdfObject* pObj = new PdfObject( m_nObjectCount, 0, numElems );

	this->push_back( pObj );

	return pObj;
}

PdfObject* PdfVecObjects::CreateObject( const PdfVariant & rVariant )
{
    PdfObject* pObj = new PdfObject( m_nObjectCount, 0, rVariant );

    this->push_back( pObj );

    return pObj;
}

void PdfVecObjects::push_back( PdfObject* pObj )
{
    ++m_nObjectCount;

    pObj->SetParent( this );
    std::vector<PdfObject*>::push_back( pObj );
}

};

