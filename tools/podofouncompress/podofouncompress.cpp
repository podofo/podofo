/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Uncompress.h"

#include <cstdlib>
#include <cstdio>

using namespace PoDoFo;

void print_help()
{
    printf("Usage: podofouncompress [inputfile] [outputfile]\n\n");
    printf("       This tool removes all compression from the PDF file.\n");
    printf("       It is useful for debugging errors in PDF files or analysing their structure.\n");
    printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

int main(int argc, char* argv[])
{
    char* input;
    char* output;

    UnCompress unc;

    if (argc != 3)
    {
        print_help();
        exit(-1);
    }

    input = argv[1];
    output = argv[2];

    try
    {
        unc.Init(input, output);
    }
    catch (PdfError& e)
    {
        fprintf(stderr, "Error: An error %i ocurred during uncompressing the pdf file.\n", (int)e.GetCode());
        e.PrintErrorMsg();
        return (int)e.GetCode();
    }

    printf("%s was successfully uncompressed to: %s\n", input, output);

    return 0;
}

