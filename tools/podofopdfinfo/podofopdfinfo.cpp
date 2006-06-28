/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#include <iostream>
#include <PdfDefines.h>
#include "pdfinfo.h"

using namespace PoDoFo;

#ifdef _HAVE_CONFIG
#include <config.h>
#endif // _HAVE_CONFIG

void print_help()
{
  printf("Usage: podofopdfinfo [inputfile] \n\n");
  printf("       This tool displays information about the PDF file.\n");
}

int main( int argc, char* argv[] )
{
  PdfError eCode;

#if 1
  PdfError::EnableDebug( false );	// turn it off to better view the output from this app!
#endif

  if( argc != 2 )
  {
    print_help();
    exit( -1 );
  }

  char*    pszInput  = argv[1];
  std::string fName( pszInput );
  PdfInfo	myInfo( fName );

  std::cout << "Document Info for " << fName << std::endl;
  std::cout << "-------------------------------------------------------------------------------" << std::endl;
  myInfo.OutputDocumentInfo( std::cout );
  std::cout << std::endl;

  std::cout << "Classic Metadata" << std::endl;
  std::cout << "----------------" << std::endl;
  myInfo.OutputInfoDict( std::cout );
  std::cout << std::endl;

  std::cout << "Page Info" << std::endl;
  std::cout << "---------" << std::endl;
  myInfo.OutputPageInfo( std::cout );

  
  return eCode.Error();
}

