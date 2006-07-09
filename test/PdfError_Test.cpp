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

#include "PdfTest.h"

#include "PdfString.h"

using namespace PoDoFo;

PdfError function1()
{
    PdfError eCode;

    RAISE_ERROR( ePdfError_InvalidHandle );

    return eCode;
}

PdfError function2()
{
    PdfError eCode;

    SAFE_OP( function1() );

    return eCode;
}

PdfError function3()
{
    PdfError eCode;

    SAFE_OP( function2() );

    return eCode;
}

int main( int argc, char* argv[] ) 
{
    PdfError eCode;

    printf("Creating an error callstack.\n");
    eCode = function3();

    if( eCode.IsError() )
    {
        eCode.PrintErrorMsg();
    }

    return 0;
}
