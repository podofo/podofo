/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ImageExtractor.h"

#include <cstdio>
#include <cstdlib>

using namespace PoDoFo;

void print_help()
{
    printf("Usage: podofoimgextract [inputfile] [outputdirectory]\n\n");
    printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

int main(int argc, char* argv[])
{
    char* input;
    char* output;

    ImageExtractor extractor;

    if (argc != 3)
    {
        print_help();
        exit(-1);
    }

    input = argv[1];
    output = argv[2];

    try
    {
        extractor.Init(input, output);
    }
    catch (PdfError& e)
    {
        fprintf(stderr, "Error: An error %i ocurred during processing the pdf file.\n", (int)e.GetCode());
        e.PrintErrorMsg();
        return (int)e.GetCode();
    }

    unsigned imageCount = extractor.GetNumImagesExtracted();
    printf("Extracted %u images successfully from the PDF file.\n", imageCount);
    return 0;
}
