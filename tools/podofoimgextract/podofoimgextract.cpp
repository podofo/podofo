/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ImageExtractor.h"

#include <cstdio>
#include <cstdlib>

using namespace std;
using namespace PoDoFo;

void print_help()
{
    printf("Usage: podofoimgextract [inputfile] [outputdirectory]\n\n");
    printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

void Main(const cspan<string_view>& args)
{
    ImageExtractor extractor;

    if (args.size() != 3)
    {
        print_help();
        exit(-1);
    }

    auto input = args[1];
    auto output = args[2];

    extractor.Init(input, output);

    unsigned imageCount = extractor.GetNumImagesExtracted();
    printf("Extracted %u images successfully from the PDF file.\n", imageCount);
}
