/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
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

#include "PdfFontMetricsBase14.h"

namespace PoDoFo {

void PdfFontMetricsBase14::GetBoundingBox( PdfArray & array ) const
{
		array.Clear();
		array.push_back( PdfVariant( bbox.left * 1000.0 / units_per_EM ) );
		array.push_back( PdfVariant( bbox.bottom  * 1000.0 / units_per_EM ) );
		array.push_back( PdfVariant( bbox.right  * 1000.0 / units_per_EM ) );
		array.push_back( PdfVariant( bbox.top  * 1000.0 / units_per_EM ) );

		return ;
}

void PdfFontMetricsBase14::GetWidthArray( PdfVariant & var, unsigned int nFirst, unsigned int nLast ) const
{
    unsigned int  i;
    PdfArray  list;
 
    for( i=nFirst;i<=nLast;i++ )
    {
		list.push_back( PdfVariant(  double(widths_table[i].width)  ) );
    }

    var = PdfVariant( list );

}

};
