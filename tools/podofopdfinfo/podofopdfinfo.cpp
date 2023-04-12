/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <iostream>
#include "pdfinfo.h"

#include <cstdlib>
#include <cstdio>

using namespace std;
using namespace PoDoFo;

void print_help()
{
    printf("Usage: podofopdfinfo [DCPON] [inputfile] \n\n");
    printf("       This tool displays information about the PDF file\n");
    printf("       according to format instruction (if not provided, displays all).\n");
    printf("       D displays Document Info.\n");
    printf("       C displays Classic Metadata.\n");
    printf("       P displays Page Info.\n");
    printf("       O displays Outlines.\n");
    printf("       N displays Names.\n");
    printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

struct Format
{
    bool document; // D
    bool classic; // C
    bool pages; // P
    bool outlines; // O
    bool names; // N
    Format() :document(true), classic(true), pages(true), outlines(true), names(true) {}
};

Format ParseFormat(const string_view& fs)
{
    Format ret;

    if (fs.find('D') == string_view::npos)
        ret.document = false;

    if (fs.find('C') == string_view::npos)
        ret.classic = false;

    if (fs.find('P') == string_view::npos)
        ret.pages = false;

    if (fs.find('O') == string_view::npos)
        ret.outlines = false;

    if (fs.find('N') == string_view::npos)
        ret.names = false;

    return ret;
}

void Main(const cspan<string_view>& args)
{
#if 1
    PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity::None);	// turn it off to better view the output from this app!
#endif

    if (args.size() < 2 || args.size() > 3)
    {
        print_help();
        exit(-1);
    }

    string_view input;
    Format format;
    string filepath;

    if (args.size() == 2)
    {
        input = args[1];
    }
    else if (args.size() == 3)
    {
        input = args[2];
        format = ParseFormat(args[1]);
    }

    if (!input.empty())
        filepath = input;
    //else leave empty

    PdfInfoHelper info(filepath);

    if (format.document)
    {
        cout << "Document Info" << endl;
        cout << "-------------" << endl;
        cout << "\tFile: " << filepath << endl;
        info.OutputDocumentInfo(cout);
        cout << endl;
    }

    if (format.classic)
    {
        cout << "Classic Metadata" << endl;
        cout << "----------------" << endl;
        info.OutputInfoDict(cout);
        cout << endl;
    }

    if (format.pages)
    {
        cout << "Page Info" << endl;
        cout << "---------" << endl;
        info.OutputPageInfo(cout);
    }

    if (format.outlines)
    {
        cout << "Outlines" << endl;
        cout << "--------" << endl;
        info.OutputOutlines(cout);
    }

    if (format.names)
    {
        cout << "Names" << endl;
        cout << "-----" << endl;
        info.OutputNames(cout);
    }
}
