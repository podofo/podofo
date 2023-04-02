/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <cstdio>
#include <cstdlib>

#include "ImageConverter.h"
#include <podofo/podofo.h>
#include <podofo_config.h>

using namespace std;
using namespace PoDoFo;

static const char* s_formats[] = {
#ifdef PODOFO_HAVE_JPEG_LIB
        "JPEG",
#endif // PODOFO_HAVE_JPEG_LIB
#ifdef PODOFO_HAVE_PNG_LIB
        "PNG",
#endif // PODOFO_HAVE_PNG_LIB
#ifdef PODOFO_HAVE_TIFF_LIB
        "TIFF",
#endif // PODOFO_HAVE_TIFF_LIB
        nullptr,
};

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

    const char** formats = s_formats;
    while (*formats)
    {
        printf("\t%s\n", *formats);
        formats++;
    }
    printf("\n");
}

void Main(const cspan<string_view>& args)
{
    string_view outputFilename;
    if (args.size() < 3)
    {
        print_help();
        exit(-1);
    }

    outputFilename = args[1];
    printf("Output filename: %s\n", outputFilename.data());

    ImageConverter converter;
    converter.SetOutputFilename(outputFilename);
    for (unsigned i = 2; i < args.size(); i++)
    {
        string_view option = args[i];
        if (option == "-useimgsize")
        {
            converter.SetUseImageSize(true);
        }
        else
        {
            printf("Adding image: %s\n", args[i].data());
            converter.AddImage(args[i]);
        }
    }

    converter.Work();
    printf("Wrote PDF successfully: %s.\n", outputFilename.data());
}
