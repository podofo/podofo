/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
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

#include "colorchanger.h"
#include "dummyconverter.h"
#include "grayscaleconverter.h"
#include "luaconverter.h"

static void print_help()
{
	std::cerr << "Usage: podofocolor [converter] [inputfile] [outpufile]\n";
    std::cerr << "\t[converter] can be one of: dummy|grayscale|lua [planfile]\n";
	std::cerr << "\tpodofocolor is a tool to change all colors in a PDF file based on a predefined or Lua description.\n";
	std::cerr << "\nPoDoFo Version: "<< PODOFO_VERSION_STRING <<"\n\n";
}

/**
 * @return a converter implementation or NULL if unknown
 */
static IConverter* ConverterForName( const std::string & converter, const std::string & lua )
{
    IConverter* pConverter = NULL;
    if( converter == "dummy" ) 
    {
        pConverter = new DummyConverter();
    }
    else if( converter == "grayscale" )
    {
        pConverter = new GrayscaleConverter();
    }
    else if( converter == "lua" )
    {
        pConverter = new LuaConverter( lua );
    }

    return pConverter;
}

int main( int argc, char* argv[] )
{
	if( !(argc == 4 || argc == 5) )
	{
		print_help();
		exit( -1 );
	}

    std::string converter = argv[1];
	std::string input   = argv[2];
	std::string output = argv[3];
    std::string lua;
    
    if( argc == 4 && converter != "lua" )
    {
        input = argv[2];
        output = argv[3];
    }
    else if( argc == 5 && converter == "lua" )
    {
        lua = argv[2];
        input = argv[3];
        output = argv[4];
    }
    else
    {
        print_help();
        exit( -3 );
    }

    IConverter* pConverter = ConverterForName( converter, lua );
    if( !pConverter ) 
    {
        std::cerr << "Aborting! Unknown converter: " << converter << std::endl;
        print_help();
        exit( -2 );
    }
    
	try
	{
        ColorChanger cc(pConverter, input, output);
        cc.start();
    }
	catch( PoDoFo::PdfError & e )
	{
		std::cerr << "Error: An error "<< e.GetError() <<" ocurred during processing the pdf file\n";
		e.PrintErrorMsg();
		return e.GetError();
	}

    delete pConverter;
	return 0;
}
