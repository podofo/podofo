
#include "podofo.h"
#include "../PdfTest.h"

#include <iostream>
#include <stack>
#include <algorithm>
#include <string>

using namespace std;
using namespace PoDoFo;

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

#if defined(HAVE_BOOST)
using namespace boost;
#include <boost/graph/depth_first_search.hpp>

#define PODOFO_FAIL(s) PODOFO_RAISE_ERROR_INFO( ePdfError_TestFailed, s )

/**
 * This visitor is invoked on the variant associated with each node entry
 * (arriving=true) and exit (arriving=false) to check that whatever the
 * tokenizer reads from the input stream matches what we expected to get based
 * on our graph.
 */
class NodeCheckVisitor
    : public boost::static_visitor<void>
{
    PdfContentsTokenizer * m_tok;
    mutable const char* m_pszToken;
    mutable PdfVariant m_var;
    mutable EPdfContentsType m_eType;
    bool m_arriving;
public:
    NodeCheckVisitor(PdfContentsTokenizer* tok, bool arriving) : m_tok(tok), m_arriving(arriving) { }
    void operator()(PdfContentsGraph::KWPair kp) const
    {
        // This is an internal node representing an opening/closing
        // operator pair for a context. We should get a keyword from the
        // tokenizer.
        m_tok->ReadNext( m_eType, m_pszToken, m_var );
        if ( m_eType != ePdfContentsType_Keyword )
            PODOFO_FAIL( "KWPair node: expected keyword from tokenizer, got variant" );
        const char* gotKw;
        if (m_arriving)
            gotKw = PdfContentsGraph::findKwById(kp.first).kwText;
        else
            gotKw = PdfContentsGraph::findKwById(kp.second).kwText;
        if ( strcmp(gotKw, m_pszToken) != 0 )
            PODOFO_FAIL( "KWPair node: Got wrong token" );
    }
    void operator()(PdfContentStreamKeyword op) const
    {
        if ( m_arriving || op == KW_RootNode ) return;
        m_tok->ReadNext( m_eType, m_pszToken, m_var );
        if ( m_eType != ePdfContentsType_Keyword )
            PODOFO_FAIL( "Keyword id node: expected keyword from tokenizer, got variant" );
        if ( strcmp(PdfContentsGraph::findKwById(op).kwText, m_pszToken) != 0 )
            PODOFO_FAIL( "Keyword id node: Got wrong token" );
    }
    void operator()(const string& str) const
    {
        if ( m_arriving ) return;
        m_tok->ReadNext( m_eType, m_pszToken, m_var );
        if ( m_eType != ePdfContentsType_Keyword )
            PODOFO_FAIL( "Keyword str node: expected keyword from tokenizer, got variant" );
        if ( str != m_pszToken )
            PODOFO_FAIL( "Keyword str node: Got wrong token" );
    }
    void operator()(const PdfVariant& var) const
    {
        if ( m_arriving ) return;
        m_tok->ReadNext( m_eType, m_pszToken, m_var );
        if ( m_eType != ePdfContentsType_Variant )
            PODOFO_FAIL( "Value node: expected variant from tokenizer, got keyword" );
        if ( !(var == m_var) )
            PODOFO_FAIL( "Value node: Value mismatch" );
    }
};

template<typename EV, bool Arriving>
class CheckVertexVisitor
{
    PdfContentsTokenizer* m_tok;
public:
    CheckVertexVisitor( PdfContentsTokenizer& tok ) : m_tok(&tok) { }
    typedef EV event_filter;
    void operator()(const PdfContentsGraph::Vertex & v,
                    const PdfContentsGraph::Graph & g)
    {
        boost::apply_visitor( NodeCheckVisitor(m_tok, Arriving), g[v] );
    }
};

void CheckGraph(PdfContentsTokenizer& tok, PdfContentsGraph& g)
{
    typedef pair<CheckVertexVisitor<on_discover_vertex,true>,CheckVertexVisitor<on_finish_vertex,false> > EVList;
    dfs_visitor<EVList> vis = make_dfs_visitor(
            EVList( CheckVertexVisitor<on_discover_vertex,true>(tok),
                    CheckVertexVisitor<on_finish_vertex,false>(tok) ) );
    depth_first_search( g.GetGraph(), visitor(vis));
}

void parse_page_graph( PdfMemDocument*, PdfPage* pPage )
{
    PdfContentsTokenizer tokenizer( pPage );
    PdfContentsGraph g( tokenizer );

    // Using another instance of the tokenizer, traverse the graph and
    // compare what we find in the graph at each node to what we get from the
    // tokenizer. If the graph read, construction, and traverse are correct the
    // results should be identical.
    PdfContentsTokenizer checkTokenizer( pPage );
    CheckGraph(checkTokenizer, g);
    // The above will either execute silently (a pass) or throw (a fail).

    // Write to stderr for test view
    g.WriteToStdErr();
}
#endif

void parse_page( PdfMemDocument*, PdfPage* pPage )
{
    PdfContentsTokenizer tokenizer( pPage );
    parse_contents( &tokenizer );
}

void usage()
{
    printf("Usage: ContentParser input_filename [g]\n");
}

int main( int argc, char* argv[] ) 
{
    if( argc < 2 || argc > 3 )
    {
        usage();
        return 1;
    }

    bool use_graph = false;
    if( argc == 3 )
    {
        if ( strcmp(argv[2],"g") == 0 )
        {
            use_graph = true;
        }
        else
        {
            usage();
            return 2;
        }
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
        
        if (!use_graph)
            parse_page( &doc, pFirst );
        else
        {
#if defined(HAVE_BOOST)
            parse_page_graph( &doc, pFirst );
#else
            std::cerr << "Can't use Boost::Graph output - not configured with Boost support" << std::endl;
            return 4;
#endif
        }
    }
    catch( const PdfError & e )
    {
        e.PrintErrorMsg();
        return e.GetError();
    }

    return 0;
}
