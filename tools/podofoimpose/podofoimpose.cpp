/***************************************************************************
 *   Copyright (C) 2007 by Pierre Marchand   *
 *   pierre@moulindetouvois.com   *
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

#include "pdftranslator.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <cstdio>

using std::cerr;
using std::endl;
using std::strtod;
using std::string;

struct _params
{
	string executablePath;
	string inFilePath;
	string outFilePath;
	string planFilePath;
	PoDoFo::Impose::PlanReader planReader;
} params;

void usage()
{
	cerr << "Usage : " << params.executablePath << " Input Output Plan [Interpretor]" << endl;
	cerr << "***" << endl;
	cerr << "\tInput is a PDF file or a file which contains a list of PDF file paths" << endl<< endl;
	cerr << "\tOutput will be a PDF file" << endl<< endl;
	cerr << "\tPlan is an imposition plan file" <<endl<< endl;
	cerr << "\t[Interpretor] Can be \"native\" (default value) or \"lua\""<< endl<< endl;
    cerr << "PoDoFo Version: " << PODOFO_VERSION_STRING << endl << endl;
}

int parseCommandLine ( int argc, char* argv[] )
{
	params.executablePath = argv[0];

	if ( argc <  4 )
	{
		usage();
		return 1;
	}

	params.inFilePath = argv[1];
	params.outFilePath = argv[2];
	params.planFilePath = argv[3];
	params.planReader = PoDoFo::Impose::Legacy;
	if ( argc >= 5 )
	{
		std::string native ( "native" );
		std::string lua ( "lua" );
		std::string interpretor ( argv[4] );

		if ( !interpretor.compare ( native ) )
			params.planReader = PoDoFo::Impose::Legacy;
		else if ( !interpretor.compare ( lua ) )
			params.planReader = PoDoFo::Impose::Lua;
	}

	return 0;
}

/**
 * Return values:
 *
 * 0 : success
 * 1 : bad command line arguments
 */
int main ( int argc, char *argv[] )
{
#if 0
	PoDoFo::PdfError::EnableDebug ( false );
	PoDoFo::PdfError::EnableLogging ( false );
#endif
	int ret = parseCommandLine ( argc, argv );
	if ( ret )
		return ret;

	std::cerr<<"Source : "<<params.inFilePath<<std::endl;
	std::cerr<<"Target : "<<params.outFilePath<<std::endl;
	std::cerr<<"Plan   : "<<params.planFilePath<<std::endl;


	try
	{
		PoDoFo::Impose::PdfTranslator *translator = new  PoDoFo::Impose::PdfTranslator;

		translator->setSource ( params.inFilePath );
		translator->setTarget ( params.outFilePath );
		translator->loadPlan ( params.planFilePath, params.planReader );

		translator->impose();
	}
	catch ( PoDoFo::PdfError & e )
	{
		e.GetCallstack();
		e.PrintErrorMsg();
		return 3;
	}
	catch ( std::exception & e )
	{
		cerr << e.what() << endl;
	}

	return 0;
}

