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
#include <list>
#include <sstream>

#include <boost/graph/depth_first_search.hpp>

using namespace std;
using namespace boost;
using namespace PoDoFo;

// Enable some more verbose debugging output
#if !defined(DEBUG_CONTENTS_GRAPH)
//#define DEBUG_CONTENTS_GRAPH
#endif

namespace {

//
// This static structure describes the content stream keywords PoDoFo knows
// about.  Anything unrecognised will be assumed to be standalone keyword that
// doesn't open or close a scope.
//
// See PDF Reference, table, 4.1, "Operator categories".
//
static const PdfContentsGraph::KWInfo kwInfo[] = {
    { PdfContentsGraph::KT_Standalone, KW_m,       KW_Undefined, "m",     "MoveTo" },
    { PdfContentsGraph::KT_Standalone, KW_l,       KW_Undefined, "l",     "LineTo" },
    { PdfContentsGraph::KT_Opening,    KW_q,       KW_Q,         "q",     "Save State" },
    { PdfContentsGraph::KT_Closing,    KW_Q,       KW_Undefined, "Q",     "Restore State" },
    { PdfContentsGraph::KT_Opening,    KW_ST,      KW_ET,        "BT",    "Begin Text" },
    { PdfContentsGraph::KT_Closing,    KW_ET,      KW_Undefined, "ET",    "End Text" },
    { PdfContentsGraph::KT_Opening,    KW_BDC,     KW_EMC,       "BDC",   "Begin marked content" },
    { PdfContentsGraph::KT_Opening,    KW_BMC,     KW_EMC,       "BMC",   "Begin marked content with property list" },
    { PdfContentsGraph::KT_Closing,    KW_EMC,     KW_Undefined, "EMC",   "End marked content" },
    // Sentinel
    { PdfContentsGraph::KT_Undefined, KW_Undefined, KW_Undefined, "\0",   NULL }
};

//
// This value is returned when an unknown keyword is encountered.
//
static const PdfContentsGraph::KWInfo kwInfoUnknown = { PdfContentsGraph::KT_Standalone, KW_Unknown, KW_Undefined, "\0", NULL };

// This function populates kwNameMap at startup, permitting use to look up
// KWInfo structures by keyword string value.
map<string,const PdfContentsGraph::KWInfo*> generateKWNameMap()
{
    map<string,const PdfContentsGraph::KWInfo*> m;
    const PdfContentsGraph::KWInfo* ki = &(kwInfo[0]);
    do {
        m.insert( pair<string,const PdfContentsGraph::KWInfo*>(ki->kwText,ki) );
        ki ++;
    } while ( ki->kt != PdfContentsGraph::KT_Undefined );
    return m;
}

// This function populates kwIdMap at startup, permitting use to look up KWInfo
// structures by keyword enum value.
map<PdfContentStreamKeyword,const PdfContentsGraph::KWInfo*> generateKWIdMap()
{
    map<PdfContentStreamKeyword,const PdfContentsGraph::KWInfo*> m;
    const PdfContentsGraph::KWInfo* ki = &(kwInfo[0]);
    do {
        m.insert( pair<PdfContentStreamKeyword,const PdfContentsGraph::KWInfo*>(ki->kw,ki) );
        ki ++;
    } while ( ki->kt != PdfContentsGraph::KT_Undefined );
    return m;
}

// Mapping table from keyword string value to KWInfo
static const map<string,const PdfContentsGraph::KWInfo*> kwNameMap = generateKWNameMap();
// Mapping table from keyword enum value to KWInfo
static const map<PdfContentStreamKeyword,const PdfContentsGraph::KWInfo*> kwIdMap = generateKWIdMap();

// A boost::variant visitor that prints the value of the variant to a
// PdfOutputStream It's somewhat clumsy and inefficient since PdfVariant can't
// write straight to a stream, we incur a virtual function call for writing the
// newline, etc.
//
// The first parameter is the output stream to write to. The second indicates
// whether the node is being arrived at initially (true) or just about to be
// left after visiting all children (false), IOW true is "white", false is
// "black".
struct PrintVariantVisitor
    : public static_visitor<void>
{
    PdfOutputStream * const m_os;
    mutable string s;
    mutable bool arriving;

    PrintVariantVisitor(PdfOutputStream* os, bool arriving)
        : static_visitor<void>(), m_os(os), s(), arriving(arriving) { }
    PrintVariantVisitor(const PrintVariantVisitor& rhs)
        : static_visitor<void>(), m_os(rhs.m_os), s(), arriving(arriving) { }
    void printKW(PdfContentStreamKeyword op) const
    {
        m_os->Write( PdfContentsGraph::findKwById(op).kwText );
    }
    void operator()(PdfContentsGraph::KWPair kp) const
    {
        if (arriving)
            printKW(kp.first);
        else
            printKW(kp.second);
        m_os->Write( "\n", 1 );
    }
    void operator()(PdfContentStreamKeyword op) const
    {
        if ( arriving || op == KW_RootNode ) return;
        printKW(op);
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

// Formats a variant to a string
struct FormatVariantVisitor
    : public static_visitor<std::string>
{
    mutable string s;
    bool arriving;

    FormatVariantVisitor(bool arriving) : static_visitor<std::string>(), s(), arriving(arriving) { }
    string operator()(PdfContentsGraph::KWPair kp) const
    {
        if (arriving)
            return PdfContentsGraph::findKwById(kp.first).kwText;
        else
            return PdfContentsGraph::findKwById(kp.second).kwText;
    }
    string operator()(PdfContentStreamKeyword op) const
    {
        if ( arriving || op == KW_RootNode ) return string();
        return PdfContentsGraph::findKwById(op).kwText;
    }
    string operator()(const string& str) const
    {
        if ( arriving ) return string();
        return str;
    }
    string operator()(const PdfVariant& var) const
    {
        if ( arriving ) return string();
        var.ToString(s);
        return s;
    }
};

// boost graph depth_first_search visitor that invokes PrintVariantVisitor on
// each node's variant value.
// The `arriving' param is set to true if the
// variant visitor this graph visitor invokes is being called on node discovery
// (in which case EV must be  boost::on_discover_vertex)
// and false if it's being called on node exit
// (in which case EV must be boost::on_finish_vertex ).
template<typename EV, bool Arriving>
class PrintVertexVisitor
{
    PdfOutputStream * m_os;
public:
    PrintVertexVisitor(PdfOutputStream* os) : m_os(os) { }
    typedef EV event_filter;
    void operator()(const boost::graph_traits<PdfContentsGraph::Graph>::vertex_descriptor & v,
                    const PdfContentsGraph::Graph & g)
    {
        boost::apply_visitor( PrintVariantVisitor(m_os, Arriving), g[v] );
    }
};

} // end anon namespace

namespace PoDoFo {

const PdfContentsGraph::KWInfo& PdfContentsGraph::findKwByName(const string & kwText)
{
    static const map<string,const KWInfo*>::const_iterator itEnd = kwNameMap.end();
    map<string,const KWInfo*>::const_iterator it = kwNameMap.find(kwText);
    if (it == itEnd)
        return kwInfoUnknown;
    else
        return *((*it).second);
}

const PdfContentsGraph::KWInfo& PdfContentsGraph::findKwById(PdfContentStreamKeyword kw)
{
    static const map<PdfContentStreamKeyword,const KWInfo*>::const_iterator itEnd = kwIdMap.end();
    map<PdfContentStreamKeyword,const KWInfo*>::const_iterator it = kwIdMap.find(kw);
    if ( it == itEnd)
    {
        PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidEnumValue, "Bad keyword ID");
    }
    return *((*it).second);
}

PdfContentsGraph::PdfContentsGraph()
    : m_graph()
{
    // Init the root node, leaving an otherwise empty graph.
    Vertex v = add_vertex(m_graph);
    m_graph[v] = KW_RootNode;
}

// This routine is useful in debugging and error reporting. It formats
// the values associated with the passed stack of vertices into a
// space-separated string, eg "BT g g g"
string formatReversedStack(
        const PdfContentsGraph::Graph & g,
        stack<PdfContentsGraph::Vertex> s,
        ostream& os)
{
    FormatVariantVisitor vis( true );
    vector<PdfContentsGraph::Vertex> l;
    while ( s.size() > 1 )
    {
        l.push_back(s.top());
        s.pop();
    }

    while ( l.size() )
    {
        string str = boost::apply_visitor( vis, g[l.back()] );
        os << str;
        os << ' ';
        l.pop_back();
    }
    return string();
}

#if defined(DEBUG_CONTENTS_GRAPH)
//
// This debuging routine prints the current context stack to stderr.
//
void PrintStack(
        const PdfContentsGraph::Graph & g,
        const stack<PdfContentsGraph::Vertex> & s,
        const string & prefix)
{
    PdfOutputDevice outDev( &cerr );
    PdfDeviceOutputStream outStream( &outDev );

    ostringstream ss;
    ss << prefix
       << ' '
       << (s.size() - 1)
       << ' ';
    formatReversedStack(g,s,ss);
    ss << '\n';
    string out = ss.str();
    outStream.Write( out.data(), out.size() );
}
#else
// Do nothing ; this will inline away nicely and avoids the need for debug
// ifdefs or ugly macros.
inline void PrintStack(
        const PdfContentsGraph::Graph &,
        const stack<PdfContentsGraph::Vertex> &,
        const string &)
{
}
#endif

//
// Format an error message reporting an open/close operator mismatch error.
//
std::string formatMismatchError(
        const PdfContentsGraph::Graph & g,
        const stack<PdfContentsGraph::Vertex> & s,
        PdfContentStreamKeyword gotKW,
        PdfContentStreamKeyword expectedKW)
{
    // Didn't find matching opening operator at top of stack.
    ostringstream err;
    err << "Found mismatching opening/closing operators. Got: "
        << PdfContentsGraph::findKwById(gotKW).kwText << ", expected "
        << PdfContentsGraph::findKwById(expectedKW).kwText << ". Context stack was: ";
    formatReversedStack(g,s,err);
    err << '.';
    return err.str();
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
                PrintStack(m_graph, parentage, "OS: ");
                // Opening a new level. If we've consumed any arguments that's an error.
                assert(ki.kw != KW_Undefined && ki.kw != KW_Unknown && ki.kw != KW_RootNode );
                // Set the node up as a pair of keywords, one of which (the exit keyword) is undefined.
                m_graph[v] = KWPair(ki.kw,KW_Undefined);
                // add an edge from the current top to it
                add_edge( parentage.top(), v, m_graph );
                // and push it to the top of the parentage stack
                parentage.push( v );
                PrintStack(m_graph, parentage, "OF: ");
            }
            else if (ki.kt == KT_Closing)
            {
                PrintStack(m_graph, parentage, "CS: ");
                // Closing a level. The top of the stack should contain the
                // matching opening node.
                assert(ki.kw != KW_Undefined && ki.kw != KW_Unknown && ki.kw != KW_RootNode );
                assert(!haveNextOpVertex);
                PODOFO_RAISE_LOGIC_IF( numArguments, "Paired operator opening had arguments" );
                KWPair kp = get<KWPair>( m_graph[parentage.top()] );
                PdfContentStreamKeyword expectedCloseKw = findKwById(kp.first).kwClose;
                if ( ki.kw != expectedCloseKw )
                {
                    string err = formatMismatchError(m_graph, parentage, ki.kw, expectedCloseKw);
                    PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidContentStream, err.c_str() );
                }
                PODOFO_RAISE_LOGIC_IF( kp.second != KW_Undefined, "Closing already closed group" );
                kp.second = ki.kw;
                m_graph[parentage.top()] = kp;
                // Our associated operator is now on the top of the parentage stack. Since its scope
                // has ended, it should also be popped.
                parentage.pop();
                PrintStack(m_graph, parentage, "CF: ");
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
    typedef pair<PrintVertexVisitor<on_discover_vertex,true>,PrintVertexVisitor<on_finish_vertex,false> > EVList;
    dfs_visitor<EVList> vis = make_dfs_visitor(
            EVList( PrintVertexVisitor<on_discover_vertex,true>(&outStream),
                    PrintVertexVisitor<on_finish_vertex,false>(&outStream) ) );
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
