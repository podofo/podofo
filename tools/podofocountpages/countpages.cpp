/**
 * SPDX-FileCopyrightText: (C) 2010 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/podofo.h>

#include <cstdlib>
#include <cstdio>

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

int count_pages(const char* filename, const bool& shortFormat)
{
    PdfMemDocument document;
    document.Load(filename);
    int nPages = document.GetPages().GetCount();

    if (shortFormat)
        printf("%i\n", nPages);
    else
        printf("%s:\t%i\n", filename, nPages);

    return nPages;
}

int main(int argc, char* argv[])
{
    PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity::None);

    if (argc <= 1)
    {
        print_help();
        exit(-1);
    }


    try
    {
        bool total = false;
        bool shortFormat = false;
        int sum = 0;

        for (int i = 1; i < argc; i++)
        {
            const char* arg = argv[i];

            if (strcmp(arg, "-s") == 0)
            {
                shortFormat = true;
            }
            else if (strcmp(arg, "-t") == 0)
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
    catch (PdfError& e)
    {
        fprintf(stderr, "Error: An error %i ocurred during counting pages in the pdf file.\n", (int)e.GetCode());
        e.PrintErrorMsg();
        return (int)e.GetCode();
    }

    return 0;
}
