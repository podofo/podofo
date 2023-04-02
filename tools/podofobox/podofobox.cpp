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

void Main(const cspan<string_view>& args)
{
    if (args.size() != 8)
    {
        print_help();
        exit(-1);
    }

    string_view input = args[1];
    string_view output = args[2];
    string_view box = args[3];

    double left = double(atol(args[4].data())) / 100.0;
    double bottom = double(atol(args[5].data())) / 100.0;
    double width = double(atol(args[6].data())) / 100.0;
    double height = double(atol(args[7].data())) / 100.0;
    Rect rect(left, bottom, width, height);

    BoxSetter bs(input, output, box, rect);
}
