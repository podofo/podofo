/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#include <podofo.h>

#include <stdlib.h>
#include <cstdio>

using namespace PoDoFo;

#ifdef _HAVE_CONFIG
#include <config.h>
#endif // _HAVE_CONFIG

void print_help()
{
  printf("Usage: podofomerge [inputfile1] [inputfile2] [outputfile]\n\n");
  printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

void merge( const char* pszInput1, const char* pszInput2, const char* pszOutput )
{
    printf("Reading file: %s\n", pszInput1 );
    PdfMemDocument input1( pszInput1 );
    printf("Reading file: %s\n", pszInput2 );
    PdfMemDocument input2( pszInput2 );

// #define TEST_ONLY_SOME_PAGES
#ifdef TEST_ONLY_SOME_PAGES
    input1.InsertPages( input2, 1, 2 );
#else
    printf("Appending %i pages on a document with %i pages.\n", input2.GetPageCount(), input1.GetPageCount() );
    input1.Append( input2 );
#endif

    // we are going to bookmark the insertions
    // using destinations - also adding each as a NamedDest
    /*
      PdfDestination	p1Dest( input1.GetPage(0) );
    input1.AddNamedDestination( p1Dest, std::string("Input1") );
    PdfOutlines* bMarks = input1.GetOutlines();
    PdfOutlineItem*	bmRoot = bMarks->CreateRoot( "Merged Document" );
    PdfOutlineItem* child1 = bmRoot->CreateChild( pszInput1, p1Dest );
    PdfDestination	p2Dest( input1.GetPage(pgCount) );
    input1.AddNamedDestination( p2Dest, std::string("Input2") );
    child1->CreateNext( pszInput2, p2Dest );
    */
    
#ifdef TEST_FULL_SCREEN
    input1.SetUseFullScreen();
#else
    input1.SetPageMode( ePdfPageModeUseBookmarks );
    input1.SetHideToolbar();
    input1.SetPageLayout( ePdfPageLayoutTwoColumnLeft );
#endif

    printf("Writing file: %s\n", pszOutput );
    input1.Write( pszOutput );
}

int main( int argc, char* argv[] )
{
  char*   pszInput1;
  char*   pszInput2;
  char*   pszOutput;

  if( argc != 4 )
  {
    print_help();
    exit( -1 );
  }

  pszInput1 = argv[1];
  pszInput2 = argv[2];
  pszOutput = argv[3];

  try {
        merge( pszInput1, pszInput2, pszOutput );
  } catch( PdfError & e ) {
      fprintf( stderr, "Error %i occurred!\n", e.GetError() );
      e.PrintErrorMsg();
      return e.GetError();
  }

  return 0;
}

