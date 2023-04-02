/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Uncompress.h"

#include <cstdlib>
#include <cstdio>

using namespace std;
using namespace PoDoFo;

void print_help()
{
    printf("Usage: podofouncompress [inputfile] [outputfile]\n\n");
    printf("       This tool removes all compression from the PDF file.\n");
    printf("       It is useful for debugging errors in PDF files or analysing their structure.\n");
    printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

void Main(const cspan<string_view>& args)
{
    if (args.size() != 3)
    {
        print_help();
        exit(-1);
    }

    UnCompress unc;

    auto input = args[1];
    auto output = args[2];

    unc.Init(input, output);

    printf("%s was successfully uncompressed to: %s\n", input.data(), output.data());
}

