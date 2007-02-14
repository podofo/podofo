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

void function1()
{
    PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
}

void function2()
{
    try {
        function1();
    } catch( PdfError & e ) {
        e.AddToCallstack( __FILE__, __LINE__ );
        throw e;
    }
}

void function3()
{
    try {
        function2();
    } catch( PdfError & e ) {
        e.AddToCallstack( __FILE__, __LINE__ );
        throw e;
    }
}

int main()
{
    printf("Creating an error callstack.\n");
    try {
        function3();
    } catch( PdfError & e ) {
        e.PrintErrorMsg();
    }

    return 0;
}
