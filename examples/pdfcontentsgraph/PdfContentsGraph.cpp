#include "PdfContentsGraph.h"

#if !defined(PODOFO_HAVE_BOOST)
#error This module requires boost::graph
#endif

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

// Visitor used by KWInstance to convert any stored variant to a string.
// kw == KW_Unknown indicates that it should be a no-op, ie return ""
struct KWIVariantAsStringVisitor
    : public static_visitor<string>
{
    string operator()(const std::string& s) const { return s; }
    string operator()(PdfContentStreamKeyword kw) const
    {
        if (kw == KW_Undefined)
            // Variant has no value
            return string();
        PODOFO_RAISE_LOGIC_IF(kw == KW_Unknown, "Variant in invalid state(may not contain KW_Unknown)");
        return PdfContentsGraph::findKwById(kw).kwText;
    }
};

// Visitor used by KWInstance to convert any stored variant to a PdfContentStreamKeyword .
// If the variant contains a string that's not a keyword known to podofo, returns KW_Unknown.
struct KWIVariantAsIdVisitor
    : public static_visitor<PdfContentStreamKeyword>
{
    PdfContentStreamKeyword operator()(PdfContentStreamKeyword kw) const { return kw; }
    PdfContentStreamKeyword operator()(const std::string& s) const
    {
        return PdfContentsGraph::findKwByName(s).kw;
    }
};

// boost graph depth_first_search visitor that prints each node's keyword and
// arguments to the provided stream in proper content stream format.
//
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
    void operator()(const PdfContentsGraph::Vertex & v,
                    const PdfContentsGraph::Graph & g)
    {
        const PdfContentsGraph::KWInstance& i ( Arriving ? g[v].first : g[v].second );
        if (!i.IsRootNode())
            i.PrintToStream( *m_os );
    }
};

// This routine is useful in debugging and error reporting. It formats
// the values associated with the passed stack of vertices into a
// space-separated string, eg "BT g g g"
string formatReversedStack(
        const PdfContentsGraph::Graph & g,
        stack<PdfContentsGraph::Vertex> s,
        ostream& os)
{
    vector<PdfContentsGraph::Vertex> l;
    while ( s.size() > 1 )
    {
        l.push_back(s.top());
        s.pop();
    }

    while ( l.size() )
    {
        os << g[l.back()].first.GetKwString() << ' ';
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
        int tokenNumber,
        PdfContentStreamKeyword gotKW,
        PdfContentStreamKeyword expectedKW)
{
    // Didn't find matching opening operator at top of stack.
    ostringstream err;
    err << "Found mismatching opening/closing operators at token number " << tokenNumber << ". Got: "
        << PdfContentsGraph::findKwById(gotKW).kwText << ", expected "
        << PdfContentsGraph::findKwById(expectedKW).kwText << ". Context stack was: ";
    formatReversedStack(g,s,err);
    err << '.';
    return err.str();
}

// Read ahead to try to find an ordering of close operators that satisfies
// the requirements of the standard.
bool closeFixup( PdfContentsGraph::Graph & g,
                 stack<PdfContentsGraph::Vertex> & s,
                 PdfContentsTokenizer & contentsTokenizer,
                 const PdfContentsGraph::KWInfo& badKw )
{
    // For now we only look ahead one operator, since that's good enough
    // to let use read the PDF references etc.

    EPdfContentsType t;
    const char * kwText;
    PdfVariant var;
    bool readToken;

    // Save a copy of the stack so we can put it back how it was if
    // our readahead fixup fails.
    stack<PdfContentsGraph::Vertex> s_copy ( s );

    // Next item must be a close keyword
    if ( ( readToken = contentsTokenizer.ReadNext(t, kwText, var) ) )
    {
        if ( t == ePdfContentsType_Keyword )
        {
            const PdfContentsGraph::KWInfo & ki ( PdfContentsGraph::findKwByName(kwText) );
            if ( ki.kt == PdfContentsGraph::KT_Closing )
            {
                // We know that the waiting close keyword, badKw,
                // doesn't match the open keyword on the top of the stack.
                // If the one we just read does, and badKw matches the
                // context open outside that, we're OK.
                PdfContentsGraph::NodeData & n1 ( g[s.top()] );
                if ( ki.kw == n1.first.GetKwInfo().kwClose )
                {
                    // The keyword we just read was the right one to
                    // close the top context.
                    n1.second.SetKw( ki.kw );
                    s.pop();
                    // Leaving us with the newly exposed outer context
                    // node and the old keyword. See if it matches.
                    PdfContentsGraph::NodeData & n2 ( g[s.top()] );
                    if ( badKw.kw == n2.first.GetKwInfo().kwClose )
                    {
                        // The old keyword matches the newly exposed
                        // node's close keyword, so everything's OK.
                        n2.second.SetKw( badKw.kw );
                        s.pop();
                        // Fixup succeeded
                        return true;
                    }
                    // Whoops, failed. Restore the copied stack
                    // so error reports don't show the effects of the
                    // lookahead.
                    s = s_copy;
                }
            }
        }
    }

    // Fixup attempt failed
    return false;
}

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
    if ( kw == KW_RootNode )
    {
        PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidEnumValue, "Cannot get KWInfo for root node");
    }
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
    m_graph[v] = MakeNode(KW_RootNode,KW_RootNode);
}

PdfContentsGraph::PdfContentsGraph( PdfContentsTokenizer & contentsTokenizer )
    : m_graph()
{
    EPdfContentsType t;
    const char * kwText;
    PdfVariant var;
    bool readToken;

    // Keep a count of the number of tokens read so we can report errors
    // more usefully.
    int tokenNumber = 0;

    // Set up the node stack and initialize the root node
    stack<Vertex> parentage;
    parentage.push( add_vertex(m_graph) );
    m_graph[parentage.top()] = MakeNode(KW_RootNode,KW_RootNode);

    // Arguments to be associated with the next keyword found
    vector<PdfVariant> args;

    while ( ( readToken = contentsTokenizer.ReadNext(t, kwText, var) ) )
    {
        ++tokenNumber;
        if (t == ePdfContentsType_Variant)
        {
            // arguments come before operators, but we want to group them up before
            // their operator.
            args.push_back(var);
        }
        else if (t == ePdfContentsType_Keyword)
        {
            const KWInfo & ki ( findKwByName(kwText) );
            if (ki.kt != KT_Closing)
            {
                // We're going to need a new vertex, so make sure we have one ready.
                Vertex v = add_vertex( m_graph );
                // Switch any waiting arguments into the new node's data.
                m_graph[v].first.GetArgs().swap( args );
                PODOFO_ASSERT( !args.size() );

                if (ki.kw == KW_Unknown)
                {
                    // No idea what this keyword is. We have to assume it's an ordinary
                    // one, possibly with arguments, and just push it in as a node at the
                    // current level.
                    PODOFO_ASSERT( !m_graph[v].first.IsDefined() );
                    m_graph[v].first.SetKw( string(kwText) );
                    add_edge( parentage.top(), v, m_graph );
                    PODOFO_ASSERT( m_graph[v].first.GetKwId() == ki.kw );
                    PODOFO_ASSERT( m_graph[v].first.GetKwString() == kwText );
                }
                else if (ki.kt == KT_Standalone)
                {
                    // Plain operator, shove it in the newly reserved vertex (which might already contain
                    // arguments) and add an edge from the top to it.
                    PODOFO_ASSERT( ki.kw != KW_Undefined && ki.kw != KW_Unknown && ki.kw != KW_RootNode );
                    PODOFO_ASSERT( !m_graph[v].first.IsDefined() );
                    m_graph[v].first.SetKw( ki.kw );
                    add_edge( parentage.top(), v, m_graph );
                    PODOFO_ASSERT( m_graph[v].first.GetKwId() == ki.kw );
                    PODOFO_ASSERT( m_graph[v].first.GetKwString() == kwText );
                }
                else if (ki.kt == KT_Opening)
                {
                    PrintStack(m_graph, parentage, "OS: ");
                    PODOFO_ASSERT( ki.kw != KW_Undefined && ki.kw != KW_Unknown && ki.kw != KW_RootNode );
                    PODOFO_ASSERT( !m_graph[v].first.IsDefined() );
                    m_graph[v].first.SetKw( ki.kw );
                    // add an edge from the current top to it
                    add_edge( parentage.top(), v, m_graph );
                    // and push it to the top of the parentage stack
                    parentage.push( v );
                    PODOFO_ASSERT( m_graph[v].first.GetKwId() == ki.kw );
                    PODOFO_ASSERT( m_graph[v].first.GetKwString() == kwText );
                    PrintStack(m_graph, parentage, "OF: ");
                }
                else
                {
                    PODOFO_ASSERT( false );
                }
            }
            else if (ki.kt == KT_Closing)
            {
                // This keyword closes a context. The top of the parentage tree should
                // be a node whose KWInstance is the matching opening keyword. We'll check
                // that, then set the second KWInstance appropriately.
                PrintStack(m_graph, parentage, "CS: ");
                PODOFO_ASSERT( ki.kw != KW_Undefined && ki.kw != KW_Unknown && ki.kw != KW_RootNode );
                // Get a reference to the node data for the current parent
                NodeData & n ( m_graph[parentage.top()] );
                PODOFO_RAISE_LOGIC_IF( n.second.IsDefined(), "Closing already closed group" );
                // Ensure that the opening keyword therein is one that this closing keyword is
                // a valid match for
                PdfContentStreamKeyword expectedCloseKw = n.first.GetKwInfo().kwClose;
                // Ensure there aren't any args to the close kw
                PODOFO_ASSERT( !args.size() );
                // and handle the close matching
                if ( ki.kw != expectedCloseKw )
                {
                    // Some PDFs, even Adobe ones, place close operators
                    // in the wrong order. We'll do some lookahead to see
                    // if we can fix things up before we hit a non-close
                    // operator.
                    if ( !closeFixup( m_graph, parentage, contentsTokenizer, ki  ) )
                    {
                        string err = formatMismatchError(m_graph, parentage, tokenNumber, ki.kw, expectedCloseKw);
                        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidContentStream, err.c_str() );
                    }
                }
                else
                {
                    n.second.SetKw( ki.kw );
                    // Our associated operator is now on the top of the
                    // parentage stack. Since its scope has ended, it should
                    // also be popped.
                    parentage.pop();
                }
                PrintStack(m_graph, parentage, "CF: ");
            }
            else
            {
                PODOFO_ASSERT( false );
            }
        }
        else
        {
            PODOFO_ASSERT( false );
        }
    }

    PODOFO_RAISE_LOGIC_IF( args.size(), "Stream ended with unconsumed arguments!" );

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

string PdfContentsGraph::KWInstance::GetKwString() const
{
    return boost::apply_visitor( KWIVariantAsStringVisitor(), m_keyword );
}

PdfContentStreamKeyword PdfContentsGraph::KWInstance::GetKwId() const
{
    return boost::apply_visitor( KWIVariantAsIdVisitor(), m_keyword );
}

void PdfContentsGraph::KWInstance::PrintToStream(PdfOutputStream& os, const char * szSepStr, long lStrLen) const
{
    string s;
    if (m_args.size())
    {
        typedef vector<PdfVariant>::const_iterator Iter;
        const Iter itEnd = m_args.end();
        Iter it = m_args.begin();
        while ( it != itEnd )
        {
            (*it).ToString(s);
            os.Write(s.data(), s.size());
            os.Write(szSepStr,lStrLen);
            ++it;
        }
    }
    s = GetKwString();
    os.Write(s.data(), s.size());
    os.Write(szSepStr,lStrLen);
}

}; // namespace PoDoFo
