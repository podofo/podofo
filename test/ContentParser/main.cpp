#include "podofo.h"
#include "../PdfTest.h"
#include "ContentParser.h"
#include <iostream>
#include <stack>
#include <algorithm>

#if defined(HAVE_BOOST)
//#include <boost/graph.hpp>
#endif

using namespace PoDoFo;

#if defined(HAVE_BOOST)
void parse_contents_graph( PdfContentsTokenizer* pTokenizer ) 
{
    const char*      pszToken = NULL;
    PdfVariant       var;

    std::cout << std::endl << "Parsing a page to build a graph:" << std::endl;

#if 0
    try 
    {
        while( true )
        {
            EPdfContentsType eType;
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
                string str;
                var.ToString( str );
                std::cout << "Variant: " << str << std::endl;
                stack.push( var );
            }
        }
    }
    catch( const PdfError & e )
    {
        if( e.GetError() == ePdfError_UnexpectedEOF )
            return; // done with the stream
        else 
            throw e;
    }
#endif
}
#endif // defined(HAVE_BOOST)

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
        std::cerr << "Remaining stack:" << std::endl;
        while (stack.size())
        {
            std::string s;
            stack.top().ToString(s);
            std::cerr << s << std::endl;
            stack.pop();
        }
    }
    catch( const PdfError & e )
    {
        if( e.GetError() == ePdfError_UnexpectedEOF )
            return; // done with the stream
        else 
            throw e;
    }
}

void ParseContentsObject( PdfObject * pContentsObject )
{
	std::cerr << "Reading content stream for " << pContentsObject->Reference().ToString() << std::endl;

    PdfStream* pStream = pContentsObject->GetStream();
    char*      pBuffer;
    long       lLen;
    pStream->GetFilteredCopy( &pBuffer, &lLen );
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
