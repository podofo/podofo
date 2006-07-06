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

#include "PdfArray.h"

#include "PdfOutputDevice.h"

namespace PoDoFo {

PdfArray::PdfArray()
{
}

PdfArray::PdfArray( const PdfObject & var ) 
{
    this->push_back( var );
}

PdfArray::PdfArray( const PdfArray & rhs )
{
    this->operator=( rhs );
}

PdfError PdfArray::Write( PdfOutputDevice* pDevice ) const
{
    PdfError                 eCode;
    PdfArray::const_iterator it = this->begin();

    SAFE_OP( pDevice->Print( "[ " ) );
    while( it != this->end() )
    {
        SAFE_OP( (*it).Write( pDevice ) );
        SAFE_OP( pDevice->Print( " " ) );

        ++it;
    }

    SAFE_OP( pDevice->Print( "]" ) );

    return eCode;
}

};
