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

#include <stdio.h>
#include <stdlib.h>

#ifdef _HAVE_CONFIG
#include <config.h>
#endif // _HAVE_CONFIG

#include "ImageConverter.h"
#include <podofo.h>

void print_help()
{
  printf("Usage: podofoimg2pdf [output.pdf] [-useimgsize] [image1 image2 image3 ...]\n\n");
  printf("Options:\n");
  printf(" -useimgsize    Use the imagesize as page size, instead of A4\n");
  printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
  printf("\n");
  printf("This tool will combine any number of images into a single PDF.\n");
  printf("This is useful to create a document from scanned images.\n");
  printf("Large pages will be scaled to fit the page and imags smaller\n");
  printf("than the defined page size, will be centered.\n");
  printf("\n");
  printf("Supported image formats:\n");

  const char** ppszFormats = PoDoFo::PdfImage::GetSupportedFormats();
  while( *ppszFormats ) 
  {
      printf("\t%s\n", *ppszFormats );
      ++ppszFormats;
  }
  printf("\n");
}

int main( int argc, char* argv[] )
{
  char*    pszOutput;

  if( argc < 3 )
  {
    print_help();
    exit( -1 );
  }

  pszOutput = argv[1];
  printf("Output filename: %s\n", pszOutput);
  
  ImageConverter converter;
  converter.SetOutputFilename( pszOutput );
  for( int i=2;i<argc;i++ ) 
  {
      std::string sOption = argv[i];
      if( sOption == "-useimgsize" ) 
      {
          converter.SetUseImageSize( true );
      }
      else 
      {
          printf("Adding image: %s\n", argv[i]);
          converter.AddImage( argv[i] );
      }
  }
  
  try {
      converter.Work();
  } catch( PoDoFo::PdfError & e ) {
      fprintf( stderr, "Error: An error %i ocurred during processing the pdf file.\n", e.GetError() );
      e.PrintErrorMsg();
      return e.GetError();
  }

  printf("Wrote PDF successfully: %s.\n", pszOutput );
  
  return 0;
}
