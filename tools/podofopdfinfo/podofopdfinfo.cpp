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

Format ParseFormat(const string& fs)
{
    Format ret;

    if (fs.find('D') == string::npos)
        ret.document = false;

    if (fs.find('C') == string::npos)
        ret.classic = false;

    if (fs.find('P') == string::npos)
        ret.pages = false;

    if (fs.find('O') == string::npos)
        ret.outlines = false;

    if (fs.find('N') == string::npos)
        ret.names = false;

    return ret;
}

int main(int argc, char* argv[])
{
#if 1
    PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity::None);	// turn it off to better view the output from this app!
#endif

    if ((argc < 2) || (argc > 3))
    {
        print_help();
        return (-1);
    }

    char* input = 0;
    Format format;
    string filepath;

    if (argc == 2)
    {
        input = argv[1];
    }
    else if (argc == 3)
    {
        input = argv[2];
        format = ParseFormat(string(argv[1]));
    }

    if (input != nullptr)
    {
        filepath = input;
    }
    //else leave empty

    try
    {
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
    catch (PdfError& e)
    {
        fprintf(stderr, "Error: An error %i ocurred during uncompressing the pdf file.\n", (int)e.GetCode());
        e.PrintErrorMsg();
        return (int)e.GetCode();
    }

    //   cerr << "All information written successfully.\n" << endl << endl;

    return 0;
}
