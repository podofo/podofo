/***************************************************************************
 *   Copyright (C) 2009 by Dominik Seichter                                *
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

#include <iostream>
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstdio>

#include "DeleteOperation.h"
#include "MoveOperation.h"

using namespace PoDoFo;

#ifdef _HAVE_CONFIG
#include <config.h>
#endif // _HAVE_CONFIG

class BadConversion : public std::runtime_error {
public:
    BadConversion(const std::string& s)
        : std::runtime_error(s)
    { }
};

void print_help()
{
  printf("Usage: podofopages [inputfile] [outputfile]\n");
  printf("Options:\n");
  printf("\t--delete NUMBER\n");
  printf("\tDeletes the page NUMBER (number is 0-based)\n"); 
  printf("\tThe page will not really be deleted from the PDF.\n");
  printf("\tIt is only removed from the so called pagestree and\n");
  printf("\ttherefore invisible. The content of the page can still\n");
  printf("\tbe retrieved from the document though.\n\n");
  printf("\t--move FROM TO\n");
  printf("\tMoves a page FROM TO in the document (FROM and TO are 0-based)\n\n"); 
  printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);  
}

void work(const char* pszInput, const char* pszOutput, std::vector<Operation*> & rvecOperations)
{
    std::cout << "Input file: " << pszInput << std::endl;
    std::cout << "Output file: " << pszOutput << std::endl;
    
    PdfMemDocument doc(pszInput);

    int total = rvecOperations.size();
    int i = 1;
    std::vector<Operation*>::iterator it = rvecOperations.begin();
    while( it != rvecOperations.end() ) 
    {
        std::string msg = (*it)->ToString();
        std::cout << "Operation " << i << " of " << total << ": " << msg;

        (*it)->Perform( doc );

        ++it;
        ++i;
    }

    std::cout << "Operations done. Writing PDF to disk." << std::endl;

    doc.Write( pszOutput );

    std::cout << "Done." << std::endl;
}

double convertToInt(const std::string& s)
{
    std::istringstream i(s);
    int x;
    if (!(i >> x))
        throw BadConversion("convertToInt(\"" + s + "\")");
    return x;
}

int main( int argc, char* argv[] )
{
  char* pszInput = NULL;
  char* pszOutput = NULL;

  if( argc < 3 )
  {
    print_help();
    exit( -1 );
  }

  // Fill operations vector
  std::vector<Operation*> vecOperations;
  for( int i=1; i < argc; i++ ) 
  {
      std::string argument = argv[i];
      if( argument == "--delete" || argument == "-delete" ) 
      {
          int page = static_cast<int>(convertToInt( std::string(argv[i+1]) ));
          vecOperations.push_back( new DeleteOperation( page ) );
          ++i;
      }
      else if( argument == "--move" || argument == "-move" ) 
      {
          int from = static_cast<int>(convertToInt( std::string(argv[i+1]) ));
          int to = static_cast<int>(convertToInt( std::string(argv[i+2]) ));
          vecOperations.push_back( new MoveOperation( from, to ) );
          ++i;
          ++i;          
      }
      else
      {
          if( pszInput == NULL ) 
          {
              pszInput = argv[i];
          }
          else if( pszOutput == NULL ) 
          {
              pszOutput = argv[i];
          }
          else
          {
              std::cerr << "Ignoring unknown argument: " << argument << std::endl;
          }
      }
  }

  if( !pszInput ) 
  {
      std::cerr << "Please specify an input file." << std::endl;
      exit( -2 );
  }

  if( !pszOutput ) 
  {
      std::cerr << "Please specify an output file." << std::endl;
      exit( -3 );
  }

  if( std::string(pszInput) == std::string(pszOutput) ) 
  {
      std::cerr << "Input and outpuf file must point to different files." << std::endl;
      exit( -4 );
  }

  try {
      work( pszInput, pszOutput, vecOperations );
  } catch( PdfError & e ) {
      std::cerr << "Error: An error " << e.GetError() << " ocurred." << std::endl;
      e.PrintErrorMsg();
      return e.GetError();
  }

  // Delete operations vectore
  std::vector<Operation*>::iterator it = vecOperations.begin();
  while( it != vecOperations.end() ) 
  {
      delete (*it);
      ++it;
  }

  return 0;
}

