/***************************************************************************
 *   Copyright (C) 2010 by Pierre Marchand   *
 *   pierre@oep-h.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "podofo.h"

#include <cstdlib>
#include <string>
#include <iostream>


#include "boxsetter.h"

void print_help()
{
	std::cerr<<"Usage: podofobox [inputfile] [outpufile] [box] [left] [bottom] [width] [height]\n";
	std::cerr<<"Box is one of media crop bleed trim art.\n";
	std::cerr<<"Give values * 100 as integers (avoid locale headaches with strtod).\n\n";
	std::cerr<<"\nPoDoFo Version: "<< PODOFO_VERSION_STRING <<"\n\n";
}

int main( int argc, char* argv[] )
{
	if( argc != 8 )
	{
		print_help();
		exit( -1 );
	}

	std::string input  = argv[1];
	std::string output = argv[2];
	std::string box = argv[3];

	double left = double(atol(argv[4])) /100.0;
	double bottom = double(atol(argv[5])) /100.0;
	double width = double(atol(argv[6])) /100.0;
	double height = double(atol(argv[7])) /100.0;
	PoDoFo::PdfRect rect( left , bottom, width, height );

	try
	{
		BoxSetter bs(input, output, box, rect);
	}
	catch( PoDoFo::PdfError & e )
	{
		std::cerr << "Error: An error "<< e.GetError() <<" ocurred during processing the pdf file\n";
		e.PrintErrorMsg();
		return e.GetError();
	}

	return 0;
}
