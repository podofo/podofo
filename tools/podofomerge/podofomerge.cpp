/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <podofo/podofo.h>

#include <cstdlib>
#include <cstdio>

using namespace std;
using namespace PoDoFo;

void print_help()
{
    printf("Usage: podofomerge [inputfile1] [inputfile2] [outputfile]\n\n");
    printf("\nPoDoFo Version: %s\n\n", PODOFO_VERSION_STRING);
}

void merge(const char* input1Path, const char* input2Path, const char* outputPath)
{
    printf("Reading file: %s\n", input1Path);
    PdfMemDocument input1;
    input1.Load(input1Path);
    printf("Reading file: %s\n", input2Path);
    PdfMemDocument input2;
    input2.Load(input2Path);

    printf("Appending %i pages on a document with %i pages.\n", input2.GetPages().GetCount(), input1.GetPages().GetCount());
    input1.GetPages().AppendDocumentPages(input2);

    // we are going to bookmark the insertions
    // using destinations - also adding each as a NamedDest
    /*
      PdfDestination	p1Dest( input1.GetPage(0) );
    input1.AddNamedDestination( p1Dest, std::string("Input1") );
    PdfOutlines* bMarks = input1.GetOutlines();
    PdfOutlineItem*	bmRoot = bMarks->CreateRoot( "Merged Document" );
    PdfOutlineItem* child1 = bmRoot->CreateChild( pszInput1, p1Dest );
    PdfDestination	p2Dest( input1.GetPage(pgCount) );
    input1.AddNamedDestination( p2Dest, std::string("Input2") );
    child1->CreateNext( pszInput2, p2Dest );
    */

#ifdef TEST_FULL_SCREEN
    input1.GetCatalog().SetUseFullScreen();
#else
    input1.GetCatalog().SetPageMode(PdfPageMode::UseBookmarks);
    input1.GetCatalog().SetHideToolbar();
    input1.GetCatalog().SetPageLayout(PdfPageLayout::TwoColumnLeft);
#endif

    printf("Writing file: %s\n", outputPath);
    input1.Save(outputPath);
}

int main(int argc, char* argv[])
{
    char* input1Path;
    char* input2Path;
    char* outputPath;

    if (argc != 4)
    {
        print_help();
        exit(-1);
    }

    input1Path = argv[1];
    input2Path = argv[2];
    outputPath = argv[3];

    try
    {
        merge(input1Path, input2Path, outputPath);
    }
    catch (PdfError& e)
    {
        fprintf(stderr, "Error %i occurred!\n", (int)e.GetCode());
        e.PrintErrorMsg();
        return (int)e.GetCode();
    }

    return 0;
}
