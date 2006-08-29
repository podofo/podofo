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

#include "PdfDestination.h"

#include "PdfAction.h"
#include "PdfDictionary.h"
#include "PdfPage.h"

namespace PoDoFo {

PdfDestination::PdfDestination()
    : m_bIsAction( false ),
      m_bIsEmpty( true )
{
}

PdfDestination::PdfDestination( const PdfPage* pPage )
    : m_bIsAction( false ),
      m_bIsEmpty( false )
{
    m_array.push_back( pPage->Object()->Reference() );
    m_array.push_back( PdfName( "Fit" ) );
}

PdfDestination::PdfDestination( const PdfAction* pAction )
    : m_bIsAction( true ),
      m_bIsEmpty( false )
{
    m_action = pAction->Object()->Reference();
}

PdfDestination::PdfDestination( const PdfDestination & rhs )
{
    this->operator=( rhs );
}

const PdfDestination & PdfDestination::operator=( const PdfDestination & rhs )
{
    m_bIsAction = rhs.m_bIsAction;
    m_action    = rhs.m_action;
    m_array     = rhs.m_array;

    return *this;
}

void PdfDestination::AddToDictionary( PdfDictionary & dictionary ) const
{
    dictionary.RemoveKey( "Dest" );
    dictionary.RemoveKey( "A" );

    if( !m_bIsEmpty ) 
    {
        if( m_bIsAction ) 
            dictionary.AddKey( "A", m_action );
        else
            dictionary.AddKey( "Dest", m_array );
    }
}

};
