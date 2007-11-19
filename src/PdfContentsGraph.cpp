#include "PdfContentsGraph.h"

#if defined(HAVE_BOOST)

#include "PdfContentsTokenizer.h"
#include "PdfOutputDevice.h"
#include "PdfOutputStream.h"
#include "PdfInputStream.h"

#include <cassert>
#include <string>
#include <iostream>
#include <map>
#include <stack>

#include <boost/graph/depth_first_search.hpp>
#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace PoDoFo;

namespace {

//
// The KWType enumeration is used to identify whether a given keyword should be expected to
// open or close a new scope (think q/Q pairs) or whether it's just a plain unscoped operator.
//
enum KWType
{
    KT_Undefined = 0, // Only used for sentinel
    KT_Standalone,    // Keyword doesn't open or close a scope. Must have opposite id = KW_Undefined
    KT_Opening,       // Keyword opens a new scope. Must have valid opposite id.
    KT_Closing        // Keyword closes an open scope. Must have valid opposite id.
};

//
// KWInfo describes a single PDF keyword's characteristics. See kwInfo[] .
//
struct KWInfo {
    // Keyword type ( ends scope, begins scope, or scope neutral )
    KWType kt;
    // Keyword ID (enum)
    PdfContentStreamKeyword kw;
    // ID enum of opposing keyword, eg KW_EndQ if kw = KW_StartQ
    PdfContentStreamKeyword kwOpposite;
    // null-terminated keyword text
    char kwText[6];
};

//
// This static structure describes the content stream keywords PoDoFo knows about.
// Anything unrecognised will be assumed to be standalone keyword that doesn't open
// or close a scope.
//
static const KWInfo kwInfo[] = {
    { KT_Standalone, KW_MoveTo, KW_Undefined, "m" },
    { KT_Standalone, KW_LineTo, KW_Undefined, "l" },
    { KT_Opening,    KW_StartQ, KW_EndQ,      "q" },
    { KT_Closing,    KW_EndQ,   KW_StartQ,    "Q" },
    // Sentinel
    { KT_Undefined, KW_Undefined, KW_Undefined, "\0" }
};

//
// This value is returned when an unknown keyword is encountered.
//
static const KWInfo kwInfoUnknown = { KT_Standalone, KW_Unknown, KW_Undefined, "\0" };

// This function populates kwNameMap at startup, permitting use to look up KWInfo structures
// by keyword string value.
map<string,const KWInfo*> generateKWNameMap()
{
    map<string,const KWInfo*> m;
    const KWInfo* ki = &(kwInfo[0]);
    do {
        m.insert( pair<string,const KWInfo*>(ki->kwText,ki) );
        ki ++;
    } while ( ki->kt != KT_Undefined );
    return m;
}

// This function populates kwIdMap at startup, permitting use to look up KWInfo structures
// by keyword enum value.
map<PdfContentStreamKeyword,const KWInfo*> generateKWIdMap()
{
    map<PdfContentStreamKeyword,const KWInfo*> m;
    const KWInfo* ki = &(kwInfo[0]);
    do {
        m.insert( pair<PdfContentStreamKeyword,const KWInfo*>(ki->kw,ki) );
        ki ++;
    } while ( ki->kt != KT_Undefined );
    return m;
}

// Mapping table from keyword string value to KWInfo
static const map<string,const KWInfo*> kwNameMap = generateKWNameMap();
// Mapping table from keyword enum value to KWInfo
static const map<PdfContentStreamKeyword,const KWInfo*> kwIdMap = generateKWIdMap();

// Look up a keyword string and return a reference to the associated keyword info struct
// If the keyword string is not known, return kwInfoUnknown .
const KWInfo& findKwByName(const string & kwText)
{
    static const map<string,const KWInfo*>::const_iterator itEnd = kwNameMap.end();
    map<string,const KWInfo*>::const_iterator it = kwNameMap.find(kwText);
    if (it == itEnd)
        return kwInfoUnknown;
    else
        return *((*it).second);
}

// Look up an operator code and return the associated keyword string. All defined enums MUST
// exist in kwIdMap .
const KWInfo& findKwById(PdfContentStreamKeyword kw)
{
    static const map<PdfContentStreamKeyword,const KWInfo*>::const_iterator itEnd = kwIdMap.end();
    map<PdfContentStreamKeyword,const KWInfo*>::const_iterator it = kwIdMap.find(kw);
    assert( it != itEnd );
    return *((*it).second);
}

// A boost::variant visitor that prints the value of the variant to a PdfOutputStream
// It's somewhat clumsy and inefficient since PdfVariant can't write straight to a stream,
// we incur a virtual function call for writing the newline, etc.
//
// The first parameter is the output stream to write to. The second indicates whether
// the node is being arrived at initially (true) or just about to be left after visiting all children (false),
// IOW true is "white", false is "black".
struct PrintVariantVisitor
    : public static_visitor<>
{
    PdfOutputStream * const m_os;
    mutable string s;
    mutable bool arriving;

    PrintVariantVisitor(PdfOutputStream* os, bool arriving) : static_visitor<>(), m_os(os), s(), arriving(arriving) { }
    PrintVariantVisitor(const PrintVariantVisitor& rhs) : static_visitor<>(), m_os(rhs.m_os), s(), arriving(arriving) { }
    void operator()(PdfContentsGraph::KWPair kp) const
    {
        if (arriving)
        {
            arriving = false;
            (*this)(kp.first);
        }
        else
            (*this)(kp.second);
    }
    void operator()(PdfContentStreamKeyword op) const
    {
        if ( arriving || op == KW_RootNode ) return;
        m_os->Write( findKwById(op).kwText );
        m_os->Write( "\n", 1 );
    }
    void operator()(const string& s) const
    {
        if ( arriving ) return;
        m_os->Write( s.data(), s.size() );
        m_os->Write( "\n", 1 );
    }
    void operator()(const PdfVariant& var) const
    {
        if ( arriving ) return;
        var.ToString(s);
        m_os->Write(s);
        m_os->Write( "\n", 1 );
    }

};

// boost graph depth_first_search visitor that invokes PrintVariantVisitor on each
// node's variant value.
//
// These two classes could be replaced by a simpler class that built on dfs_visitor()
// but this works well enough for now.
//
struct PrintDepartingVertexVisitor
{
    PdfOutputStream * const m_os;
    PrintDepartingVertexVisitor(PdfOutputStream* os) : m_os(os) { }
    PrintDepartingVertexVisitor(const PrintDepartingVertexVisitor& rhs) : m_os(rhs.m_os) { }
    typedef boost::on_finish_vertex event_filter;
    void operator()(const boost::graph_traits<PdfContentsGraph::Graph>::vertex_descriptor & v,
                    const PdfContentsGraph::Graph & g)
    {
        boost::apply_visitor( PrintVariantVisitor(m_os, false), g[v] );
    }
};
struct PrintArrivingVertexVisitor
{
    PdfOutputStream * const m_os;
    PrintArrivingVertexVisitor(PdfOutputStream* os) : m_os(os) { }
    PrintArrivingVertexVisitor(const PrintArrivingVertexVisitor& rhs) : m_os(rhs.m_os) { }
    typedef boost::on_discover_vertex event_filter;
    void operator()(const boost::graph_traits<PdfContentsGraph::Graph>::vertex_descriptor & v,
                    const PdfContentsGraph::Graph & g)
    {
        boost::apply_visitor( PrintVariantVisitor(m_os, true), g[v] );
    }
};

} // end anon namespace

namespace PoDoFo {

PdfContentsGraph::PdfContentsGraph()
    : m_graph()
{
    // Init the root node, leaving an otherwise empty graph.
    Vertex v = add_vertex(m_graph);
    m_graph[v] = KW_RootNode;
}

PdfContentsGraph::PdfContentsGraph( PdfContentsTokenizer & contentsTokenizer )
    : m_graph()
{
    EPdfContentsType t;
    const char * kwText;
    PdfVariant var;
    bool readToken;

    // Set up the node stack and initialize the root node
    stack<Vertex> parentage;
    parentage.push( add_vertex(m_graph) );
    m_graph[parentage.top()] = KW_RootNode;

    // This flag controls whether the vertex on the top of the parentage stack
    // is currently untyped. That can happen if, eg, we have some arguments
    // linked to it but haven't yet seen the operator that consumes them.
    //
    bool haveNextOpVertex = false;
    Vertex nextOpVertex = Vertex();
    // number of argument values associated with the current top keyword
    int numArguments = 0;

    while ( ( readToken = contentsTokenizer.ReadNext(t, kwText, var) ) )
    {
        if (t == ePdfContentsType_Variant)
        {
            // arguments come before operators, but we want to group them up before
            // their operator. Treat the current top of the stack as the operator-to-be
            // and start associating variants with it.
            if (!haveNextOpVertex)
            {
                nextOpVertex = add_vertex(m_graph);
                haveNextOpVertex = true;
            }
            Vertex arg = add_vertex( m_graph );
            add_edge( nextOpVertex, arg, m_graph );
            m_graph[arg] = var;
            ++numArguments;
        }
        else if (t == ePdfContentsType_Keyword)
        {
            const KWInfo & ki ( findKwByName(kwText) );
            Vertex v;
            if (ki.kt != KT_Closing)
            {
                // We're going to need a new vertex, so make sure we have one ready.
                if (haveNextOpVertex)
                    v = nextOpVertex;
                else
                    v = add_vertex( m_graph );
                haveNextOpVertex = false;
            }

            if (ki.kw == KW_Unknown)
            {
                // No idea what this keyword is. We have to assume it's an ordinary
                // one, possibly with arguments, and just push it in as a node at the
                // current level.
                m_graph[v] = string(kwText);
                add_edge( parentage.top(), v, m_graph );
            }
            else if (ki.kt == KT_Standalone)
            {
                // Plain operator, shove it in the newly reserved vertex and add an edge from
                // the top to it.
                assert(ki.kw != KW_Undefined && ki.kw != KW_Unknown && ki.kw != KW_RootNode );
                m_graph[v] = ki.kw;
                add_edge( parentage.top(), v, m_graph );
            }
            else if (ki.kt == KT_Opening)
            {
                // Opening a new level. If we've consumed any arguments that's an error.
                assert(ki.kw != KW_Undefined && ki.kw != KW_Unknown && ki.kw != KW_RootNode );
                PODOFO_RAISE_LOGIC_IF( numArguments, "Paired operator opening had arguments" );
                // Set the node up as a pair of keywords, one of which (the exit keyword) is undefined.
                m_graph[v] = KWPair(ki.kw,KW_Undefined);
                // add an edge from the current top to it
                add_edge( parentage.top(), v, m_graph );
                // and push it to the top of the parentage stack
                parentage.push( v );
            }
            else if (ki.kt == KT_Closing)
            {
                // Closing a level. The top of the stack should contain the matching opening node.
                assert(ki.kw != KW_Undefined && ki.kw != KW_Unknown && ki.kw != KW_RootNode );
                assert(!haveNextOpVertex);
                PODOFO_RAISE_LOGIC_IF( numArguments, "Paired operator opening had arguments" );
                KWPair kp = get<KWPair>( m_graph[parentage.top()] );
                PODOFO_RAISE_LOGIC_IF( kp.first != ki.kwOpposite, "Mismatched open/close" );
                PODOFO_RAISE_LOGIC_IF( kp.second != KW_Undefined, "Closing already closed group" );
                kp.second = ki.kw;
                m_graph[parentage.top()] = kp;
                // Our associated operator is now on the top of the parentage stack. Since its scope
                // has ended, it should also be popped.
                parentage.pop();
            }
            else
            {
                assert(false);
            }
            numArguments = 0;
        }
        else
        {
            assert(false);
        }
    }

    PODOFO_RAISE_LOGIC_IF( haveNextOpVertex, "Stream ended with unconsumed arguments!" );

    PODOFO_RAISE_LOGIC_IF( parentage.size() != 1, "Stream failed to close all levels" );

}

void PdfContentsGraph::Write(PdfOutputStream& outStream)
{
    typedef pair<PrintArrivingVertexVisitor,PrintDepartingVertexVisitor> EVList;
    dfs_visitor<EVList> vis = make_dfs_visitor(
            EVList( PrintArrivingVertexVisitor(&outStream), PrintDepartingVertexVisitor(&outStream) ) );
    depth_first_search(m_graph, visitor(vis));
}

void PdfContentsGraph::WriteToStdErr()
{
    PdfOutputDevice outDev( &cerr );
    PdfDeviceOutputStream outStream( &outDev );
    Write(outStream);
}

} // namespace PoDoFo

#endif // HAVE_BOOST
