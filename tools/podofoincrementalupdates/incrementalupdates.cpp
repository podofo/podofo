/**
 * SPDX-FileCopyrightText: (C) 2009 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/podofo.h>

#include <cstdlib>
#include <cstdio>

#include <podofo/private/PdfParser.h>

using namespace std;
using namespace PoDoFo;

void print_help()
{
    printf("Usage: podofoincrementalupdates [-e N out.pdf] file.pdf\n\n");
    printf("       This tool prints information of incremental updates to file.pdf.\n");
    printf("       By default the number of incremental updates will be printed.\n");
    printf("       -e N out.pdf\n");
    printf("       Extract the Nth update from file.pdf and write it to out.pdf.\n");
    printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

int get_info(const string_view& filepath)
{
    (void)filepath;
    int updateCount = 0;
    /* FIX-ME: Broken after PdfIndirectObjectList API review
    PdfIndirectObjectList objects;
    PdfParser parser(objects);

    FileStreamDevice input(filepath);
    parser.Parse(input);

    updateCount = parser.GetNumberOfIncrementalUpdates();

    printf("%s\t=\t%i\t(Number of incremental updates)\n", filepath.data(), updateCount);
    */
    return updateCount;
}

void extract(const string_view& filePath, int requestedNthUpdate, const string_view& outputFilePath)
{
    (void)filePath;
    (void)requestedNthUpdate;
    (void)outputFilePath;
    // TODO
    fprintf(stderr, "extraction is not implemented\n");
    exit(-2);
}

void Main(const cspan<string_view>& args)
{
    PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity::None);

    if (args.size() != 2 && args.size() != 5)
    {
        print_help();
        exit(-1);
    }

    string_view inputPath;
    string_view outputPath;
    int requestedNthUpdate = -1;

    if (args.size() == 2)
    {
        inputPath = args[1];
        get_info(inputPath);
    }
    else if (args.size() == 5)
    {
        requestedNthUpdate = strtol(args[2].data(), NULL, 10);
        outputPath = args[3];
        inputPath = args[4];
        extract(inputPath, requestedNthUpdate, outputPath);
    }
}
