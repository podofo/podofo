#include "podofo.h"
#include "../PdfTest.h"
#include "ContentParser.h"
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
    try
    {
        while (true)
        {
            pTokenizer->ReadNext( &eType, &pszToken, var );
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
    }
    catch( const PdfError & e )
    {
        if( e.GetError() == ePdfError_UnexpectedEOF && op.second.size() == 0 )
        {
            // EOF when no operands have been read is OK, so we'll just return.
            return false;
        }
        else 
        {
            // something else went wrong
            throw e;
        }
    }
    assert(false);  // Unreachable
    return false;
}

void ParseContentStreamInto( PdfContentsTokenizer* pTokenizer, OperationList& ops ) 
{
    const char*      pszToken = NULL;
    PdfVariant       var;

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

    try 
    {
        while( true )
        {
            pTokenizer->ReadNext( &eType, &pszToken, var );
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
            else
            {
                var.ToString( str );
                std::cout << "Variant: " << str << std::endl;
                stack.push( var );
            }
        }
    }
    catch( const PdfError & e )
    {
        if( e.GetError() == ePdfError_UnexpectedEOF ) { /* Done with the stream */ }
        else { throw e; }
    }
}

void ParseContentsObject( PdfObject * pContentsObject )
{
    std::cerr << "Reading content stream for " << pContentsObject->Reference().ToString() << std::endl;

    PdfStream* pStream = pContentsObject->GetStream();
    char*      pBuffer;
    long       lLen;
    pStream->GetFilteredCopy( &pBuffer, &lLen );
    // Run a demo parser over the stream
    try 
    {
        PdfContentsTokenizer tokenizer( pBuffer, lLen );
        parse_contents( &tokenizer );
    } 
    catch( const PdfError & e )
    {
        free( pBuffer );
        throw e;
    }
    // Group the stream into a list of operators with associated operands.
    try 
    {
        PdfContentsTokenizer tokenizer( pBuffer, lLen );
        OperationList ops;
        ParseContentStreamInto( &tokenizer, ops );
        std::cerr << "Read " << ops.size() << " operators." << std::endl;
    } 
    catch( const PdfError & e )
    {
        free( pBuffer );
        throw e;
    }
    free( pBuffer );
    std::cerr << "Done reading content stream" << std::endl;
}

void parse_page( PdfMemDocument* doc, PdfPage* pPage )
{
    PdfObject* pContents = pPage->GetContents();
    if( pContents && pContents->IsArray()  ) 
    {
        PdfArray& a ( pContents->GetArray() );
        for ( PdfArray::iterator it = a.begin(); it != a.end() ; ++it )
        {
            if ( !(*it).IsReference() )
            {
                PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "/Contents array contained non-references" );
            }
            PdfObject* pContentsObj = doc->GetObjects().GetObject( (*it).GetReference() );
            ParseContentsObject(pContentsObj);
        }
    }
    else if ( pContents && pContents->HasStream() )
    {
        ParseContentsObject(pContents);
    }
    else
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "Page /Contents not stream or array of streams" );
    }
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
