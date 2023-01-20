/**
 * SPDX-FileCopyrightText: (C) 2009 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/podofo.h>

#include <cstdlib>
#include <cstdio>

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
    int updateCount = 0;

    PdfIndirectObjectList objects;
    PdfParser parser(objects);

    FileStreamDevice input(filepath);
    parser.Parse(input);

    updateCount = parser.GetNumberOfIncrementalUpdates();

    printf("%s\t=\t%i\t(Number of incremental updates)\n", filepath.data(), updateCount);

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

int main(int argc, char* argv[])
{
    PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity::None);

    if (argc != 2 && argc != 5)
    {
        print_help();
        exit(-1);
    }

    try
    {
        const char* inputPath;
        const char* outputPath;
        int requestedNthUpdate = -1;

        if (argc == 2)
        {
            inputPath = argv[1];
            get_info(inputPath);
        }
        else if (argc == 5)
        {
            requestedNthUpdate = strtol(argv[2], NULL, 10);
            outputPath = argv[3];
            inputPath = argv[4];
            extract(inputPath, requestedNthUpdate, outputPath);
        }


    }
    catch (PdfError& e)
    {
        fprintf(stderr, "Error: An error %i ocurred during counting pages in the pdf file.\n", (int)e.GetCode());
        e.PrintErrorMsg();
        return (int)e.GetCode();
    }

    return 0;
}
