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

using std::cerr;
using std::endl;
using std::strtod;
using std::string;

struct _params {
    string executablePath;
    string inFilePath;
    string outFilePath;
    string planFilePath;
    double sheetMargin;
} params;

void usage()
{
     cerr << "Usage : " << params.executablePath << " input output plan sheetMargin" << endl;
     cerr << "\tinput is a PDF file" << endl;
     cerr << "\toutput will be a PDF file" << endl;
     cerr << "\tplan is an imposition plan file.\n\tFormat is simple, the first line indicates width and height of final page size (MediaBox), and follows a list of records of form :\n\tsourcePage destinationPage rotation(counterclokwise & with axis 0.0) Xcoordinate Ycoordinate" << endl;
     cerr << "\tsheetMargin is the size of the margins where cut marks will be drawn" << endl;
     cerr << "\nAll sizes are in point and user space as defined in PDF (origine is bottom left). " << endl;
}

int parseCommandLine(int argc, char* argv[])
{
      if(argc !=  5)
      {
          usage();
          return 1;
      }

      params.executablePath = argv[0];
      params.inFilePath = argv[1];
      params.outFilePath = argv[2];
      params.planFilePath = argv[3];

      char * endptr = 0;
      params.sheetMargin = strtod(argv[4], &endptr);
      if (!endptr)
      {
          cerr << "Cannot parse '" << argv[4] << "' as decimal for sheetMargin argument" << endl;
          usage();
          return 1;
      }
      return 0;
}

/**
 * Return values:
 *
 * 0 : success
 * 1 : bad command line arguments
 */
int main(int argc, char *argv[])
{
    int ret = parseCommandLine(argc, argv);
    if (ret)
        return ret;

    try {
        PdfTranslator *translator = new PdfTranslator(params.sheetMargin);
        translator->setSource(params.inFilePath);
        translator->setTarget(params.outFilePath);
        translator->loadPlan(params.planFilePath);
        translator->impose();
    }
    catch ( PoDoFo::PdfError & e )
    {
        e.PrintErrorMsg();
        return 3;
    }
    catch ( std::exception & e )
    {
        cerr << e.what() << endl;
    }

    return 0;
}

