/**
 * SPDX-FileCopyrightText: (C) 2008 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */


#include <cstdlib>
#include <cstdio>

#include <podofo/podofo.h>

using namespace std;
using namespace PoDoFo;

void print_help()
{
    printf("Usage: podofotxtextract [inputfile]\n\n");
    printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

void Main(const cspan<string_view>& args)
{
    string_view input;
    if (args.size() != 2)
    {
        print_help();
        exit(-1);
    }

    input = args[1];

    PdfMemDocument doc;
    doc.Load(input);
    auto& pages = doc.GetPages();
    for (unsigned i = 0; i < pages.GetCount(); i++)
    {
        auto& page = pages.GetPageAt(i);

        vector<PdfTextEntry> entries;
        page.ExtractTextTo(entries);

        for (auto& entry : entries)
            printf("(%.3f,%.3f) %s \n", entry.X, entry.Y, entry.Text.data());
    }
}
