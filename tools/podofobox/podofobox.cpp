/**
 * SPDX-FileCopyrightText: (C) 2010 Pierre Marchand <pierre@oep-h.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/podofo.h>

#include <string>
#include <iostream>

#include "boxsetter.h"

using namespace std;
using namespace PoDoFo;

void print_help()
{
    cerr << "Usage: podofobox [inputfile] [outpufile] [box] [left] [bottom] [width] [height]" << endl;
    cerr << "Box is one of media crop bleed trim art." << endl;
    cerr << "Give values * 100 as integers (avoid locale headaches with strtod)" << endl;
    cerr << endl << endl << "PoDoFo Version: " << PODOFO_VERSION_STRING << endl << endl;
}

int main(int argc, char* argv[])
{
    if (argc != 8)
    {
        print_help();
        exit(-1);
    }

    string input = argv[1];
    string output = argv[2];
    string box = argv[3];

    double left = double(atol(argv[4])) / 100.0;
    double bottom = double(atol(argv[5])) / 100.0;
    double width = double(atol(argv[6])) / 100.0;
    double height = double(atol(argv[7])) / 100.0;
    PdfRect rect(left, bottom, width, height);

    try
    {
        BoxSetter bs(input, output, box, rect);
    }
    catch (PdfError& e)
    {
        cerr << "Error: An error " << e.what() << " ocurred during processing the pdf file" << endl;
        e.PrintErrorMsg();
        return (int)e.GetCode();
    }

    return 0;
}
