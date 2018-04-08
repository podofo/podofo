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

#include "Uncompress.h"

#include <podofo.h>

#include <stdlib.h>
#include <cstdio>

using namespace PoDoFo;

#ifdef _HAVE_CONFIG
#include <config.h>
#endif // _HAVE_CONFIG

void print_help()
{
  printf("Usage: podofouncompress [inputfile] [outputfile]\n\n");
  printf("       This tool removes all compression from the PDF file.\n");
  printf("       It is useful for debugging errors in PDF files or analysing their structure.\n");
  printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

int main( int argc, char* argv[] )
{
  char*    pszInput;
  char*    pszOutput;

  UnCompress unc;


  if( argc != 3 )
  {
    print_help();
    exit( -1 );
  }

  pszInput  = argv[1];
  pszOutput = argv[2];

//  try {
      unc.Init( pszInput, pszOutput );
      /*
  } catch( PdfError & e ) {
      fprintf( stderr, "Error: An error %i ocurred during uncompressing the pdf file.\n", e.GetError() );
      e.PrintErrorMsg();
      return e.GetError();
  }
      */


  printf("%s was successfully uncompressed to: %s\n", pszInput, pszOutput );
  
  return 0;
}

