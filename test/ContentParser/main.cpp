
#include "podofo.h"
#include "../PdfTest.h"

#include <iostream>
#include <stack>
#include <algorithm>

using namespace PoDoFo;

typedef std::pair< std::string, std::vector<PdfVariant> > Operation;
typedef std::vector<Operation> OperationList;

bool GetOperation( PdfContentsTokenizer* pTokenizer, Operation& op )
{
    PdfVariant var;
    EPdfContentsType eType;
    const char* pszToken = NULL;

    op.second.clear();

    while ( pTokenizer->ReadNext( eType, pszToken, var ) )
    {
        if( eType == ePdfContentsType_Keyword )
        {
            // A keyword terminates the operation
            op.first = pszToken;
            return true;
        }
        else
        {
            // Push another operand and keep scanning
            op.second.push_back( var );
        }
    }
    // If we get here, we hit EOF without reading a keyword.
    return false;
}

void ParseContentStreamInto( PdfContentsTokenizer* pTokenizer, OperationList& ops ) 
{
    bool got_op = true;
    while( got_op )
    {
        Operation op;
        got_op = GetOperation(pTokenizer, op);
        if (got_op)
            ops.push_back(op);
    }
}

void parse_contents( PdfContentsTokenizer* pTokenizer ) 
{
    const char*      pszToken = NULL;
    PdfVariant       var;
    EPdfContentsType eType;
    std::string      str;

    std::stack<PdfVariant> stack;
    std::cout << std::endl << "Parsing a page:" << std::endl;

    while( pTokenizer->ReadNext( eType, pszToken, var ) )
    {
        if( eType == ePdfContentsType_Keyword )
        {
            std::cout << "Keyword: " << pszToken << std::endl;

            // support 'l' and 'm' tokens
            if( strcmp( pszToken, "l" ) == 0 ) 
            {
                double dPosY = stack.top().GetReal();
                stack.pop();
                double dPosX = stack.top().GetReal();
                stack.pop();

                std::cout << "LineTo: " << dPosX << " " << dPosY << std::endl;
            }
            else if( strcmp( pszToken, "m" ) == 0 ) 
            {
                double dPosY = stack.top().GetReal();
                stack.pop();
                double dPosX = stack.top().GetReal();
                stack.pop();

                std::cout << "MoveTo: " << dPosX << " " << dPosY << std::endl;
            }
        }
        else if ( eType == ePdfContentsType_Variant )
        {
            var.ToString( str );
            std::cout << "Variant: " << str << std::endl;
            stack.push( var );
        }
        else
        {
            // Impossible; type must be keyword or variant
            PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
        }
    }
    std::cout << "EOF" << std::endl;
}


void parse_page( PdfMemDocument*, PdfPage* pPage )
{
    std::cerr << "Reading content stream for " << pPage->GetObject()->Reference().ToString() << std::endl;
    // Run a demo parser over the stream
    try 
    {
        PdfContentsTokenizer tokenizer( pPage );
        parse_contents( &tokenizer );
    } 
    catch( const PdfError & e )
    {
        throw e;
    }

    // Group the stream into a list of operators with associated operands.
    try 
    {
        PdfContentsTokenizer tokenizer( pPage );
        OperationList ops;
        ParseContentStreamInto( &tokenizer, ops );
        std::cerr << "Read " << ops.size() << " operators." << std::endl;
    } 
    catch( const PdfError & e )
    {
        throw e;
    }

    std::cerr << "Done reading content stream" << std::endl;
}

int main( int argc, char* argv[] ) 
{
    if( argc != 2 )
    {
        printf("Usage: ContentParser [input_filename]\n");
        return 0;
    }

    try 
    {
        PdfMemDocument doc( argv[1] );
        if( !doc.GetPageCount() )
        {
            std::cerr << "This document contains no page!" << std::endl;
            return 1;
        }

        PdfPage* pFirst = doc.GetPage( 0 );
        
        parse_page( &doc, pFirst );
    } 
    catch( const PdfError & e )
    {
        e.PrintErrorMsg();
        return e.GetError();
    }

    return 0;
}
