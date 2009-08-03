#include "podofo.h"
#include "PdfContentsGraph.h"

#include <iostream>
#include <stack>
#include <algorithm>
#include <string>
#include <iomanip>
#include <cstdio>

using namespace std;
using namespace PoDoFo;

void usage()
{
    printf("Usage: pdfcontentgraph [-a] input_filename\n");
    printf("       -a   Process all pages of input, not just first\n");
}

int main( int argc, char* argv[] )
{
    bool all_pages = false;
    int firstPageNo = 0;
    string inputFileName;
    ++argv;
    --argc;
    while (argc)
    {
        if( argv[0][0] == '-' )
        {
            // Single character flag
            switch( argv[0][1] )
            {
                case 'a':
                    // Process all pages, not just first page
                    all_pages = true;
                    break;
                default:
                    usage();
                    return 1;
            }
        }
        else
        {
            // Input filename
            if (inputFileName.empty())
            {
                inputFileName = argv[0];
            }
            else
            {
                usage();
                return 1;
            }
        }
        ++argv;
        --argc;
    }

    if (inputFileName.empty())
    {
        usage();
        return 1;
    }

    try
    {
        PdfMemDocument doc( inputFileName.c_str() );
        if( !doc.GetPageCount() )
        {
            std::cerr << "This document contains no page!" << std::endl;
            return 1;
        }

        int toPage = all_pages ? doc.GetPageCount() : firstPageNo + 1 ;
        for ( int i = firstPageNo; i < toPage; ++i )
        {
            cout << "Processing page " << setw(6) << (i+1) << "..." << std::flush;
            PdfPage* page = doc.GetPage( i );
            PODOFO_RAISE_LOGIC_IF( !page, "Got null page pointer within valid page range" );

            PdfContentsTokenizer tokenizer( page );
            PdfContentsGraph grapher( tokenizer );

            cout << " - page ok" << endl;
        }
    }
    catch( const PdfError & e )
    {
        e.PrintErrorMsg();
        return e.GetError();
    }

    cout << endl;
    return 0;
}
