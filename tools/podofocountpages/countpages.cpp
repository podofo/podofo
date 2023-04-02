/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/podofo.h>

#include <cstdlib>
#include <cstdio>

using namespace std;
using namespace PoDoFo;

void print_help()
{
    printf("Usage: podofocountpages [-s] [-t] file1.pdf ... \n\n");
    printf("       This tool counts the pages in a PDF file.\n");
    printf("       -s will enable the short format, which ommites\n");
    printf("          printing of the filename in the output.\n");
    printf("       -t print the total sum of all pages.\n");
    printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

int count_pages(const string_view filename, const bool& shortFormat)
{
    PdfMemDocument document;
    document.Load(filename);
    unsigned nPages = document.GetPages().GetCount();
    
    if (shortFormat)
        printf("%i\n", nPages);
    else
        printf("%s:\t%u\n", filename.data(), nPages);

    return nPages;
}

void Main(const cspan<string_view>& args)
{
    PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity::None);

    if (args.size() <= 1)
    {
        print_help();
        exit(-1);
    }

    bool total = false;
    bool shortFormat = false;
    int sum = 0;

    for (unsigned i = 1; i < args.size(); i++)
    {
        auto arg = args[i];

        if (arg == "-s")
        {
            shortFormat = true;
        }
        else if (arg == "-t")
        {
            total = true;
        }
        else
        {
            sum += count_pages(arg, shortFormat);
        }
    }

    if (total)
    {
        printf("Total:\t%i\n", sum);
    }
}
