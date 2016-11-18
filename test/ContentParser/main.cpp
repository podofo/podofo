#include "podofo.h"
#include "../PdfTest.h"

#include <iostream>
#include <stack>
#include <algorithm>
#include <string>
#include <iomanip>
#include <cstdio>

using namespace std;
using namespace PoDoFo;

static bool print_output = false;

void parse_contents( PdfContentsTokenizer* pTokenizer )
{
    const char*      pszToken = NULL;
    PdfVariant       var;
    EPdfContentsType eType;
    std::string      str;

    int numKeywords = 0;
    int numVariants = 0;

    std::stack<PdfVariant> stack;

    while( pTokenizer->ReadNext( eType, pszToken, var ) )
    {
        if( eType == ePdfContentsType_Keyword )
        {
            ++numKeywords;
            if (print_output) std::cout << setw(12) << (numKeywords+numVariants)
                                        << " Keyword: " << pszToken << std::endl;

            // support 'l' and 'm' tokens
            if( strcmp( pszToken, "l" ) == 0 )
            {
                double dPosY = stack.top().GetReal();
                stack.pop();
                double dPosX = stack.top().GetReal();
                stack.pop();

                if(print_output) std::cout << string(12,' ') << " LineTo: " << dPosX << " " << dPosY << std::endl;
            }
            else if( strcmp( pszToken, "m" ) == 0 )
            {
                double dPosY = stack.top().GetReal();
                stack.pop();
                double dPosX = stack.top().GetReal();
                stack.pop();

                if(print_output) std::cout << string(12,' ') << " MoveTo: " << dPosX << " " << dPosY << std::endl;
            }
        }
        else if ( eType == ePdfContentsType_Variant )
        {
            ++numVariants;
            var.ToString( str );
            if(print_output) std::cout << setw(12) << (numKeywords+numVariants)
                                       << " Variant: " << str << std::endl;
            stack.push( var );
        }
        else if (eType == ePdfContentsType_ImageData)
        {
            if (print_output) {
                std::string d ( var.GetRawData().data() );
		std::cout << string(13, ' ') << "Inline image data: " << d.size() << " bytes. Hex follows." << std::hex << std::endl;
                std::cout << std::hex << std::setfill('0');
                for ( std::string::iterator i = d.begin(); i != d.end(); i ++) {
                    std::cout << std::setw(2) << (static_cast<unsigned short>(*i) & 0x00FF);
                }
		std::cout << std::dec << std::setfill(' ') << std::endl;
            }
        }
        else
        {
            // Impossible; type must be keyword or variant
            PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
        }
    }
    cout << ' ' << setw(12) << numKeywords << " keywords, " << setw(12) << numVariants << " variants";
}

void parse_page( PdfMemDocument*, PdfPage* pPage )
{
    PdfContentsTokenizer tokenizer( pPage );
    parse_contents( &tokenizer );
}

void usage()
{
    printf("Usage: ContentParser [-g] [-a] [-p] input_filename\n");
    printf("       -a   Process all pages of input, not just first\n");
    printf("       -p   Print parsed content stream to stdout\n");
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
                case 'p':
                    // Print output, rather than parsing & checking
                    // silently.
                    print_output = true;
                    break;
                case 'n':
                    // Page number request. Chars 2+ are page number int. Let's do
                    // this the quick and dirty way...
                    firstPageNo = atoi(argv[0]+2) - 1;
                    cerr << "Will process page: " << (firstPageNo+1) << endl;
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

            parse_page( &doc, page );

            cout << " - page ok" << endl;
        }
    }
    catch( PdfError & e )
    {
        e.PrintErrorMsg();
        return e.GetError();
    }

    cout << endl;
    return 0;
}
