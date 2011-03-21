#ifndef _PODOFO_PDFCONTENTSGRAPH_H
#define _PODOFO_PDFCONTENTSGRAPH_H

#include "podofo/podofo.h"

#if defined(PODOFO_HAVE_BOOST)

#include <utility>
#include <string>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/variant.hpp>

namespace PoDoFo {

class PdfInputStream;
class PdfOutputStream;
class PdfContentsTokenizer;

enum PdfContentStreamKeyword
{
    // Special node for undefined/unset value. Should never get used except when default-constructing
    // a node variant.
    KW_Undefined = 0,
    // Normal PDF operators
    KW_q,
    KW_Q,
    KW_ST,
    KW_ET,
    KW_BMC,
    KW_BDC,
    KW_EMC,
    KW_m,
    KW_l,
    // Special node with no associated keyword, used to identify the root node that anchors
    // the graph.
    KW_RootNode = 0xfe,
    // Value returned by findKwByName(...) when no enum for the keyword is known.
    KW_Unknown = 0xff
};


/**
 * PdfContentsGraph provides a concrete representation of a content stream as an
 * in-memory graph. It can be created as a blank structure to be populated by hand,
 * or from an existing content stream via PdfContentsTokenizer . It can serialize
 * its state as a PDF content stream.
 *
 * This class does not track the resources used by the content stream. \see PdfCanvas .
 *
 * This class is only available when the Boost library, specifically the boost graph
 * library, has been configured for use.
 */
class PdfContentsGraph
{
public:
    /**
     * The KWType enumeration is used to identify whether a given keyword
     * should be expected to open or close a new scope (think q/Q pairs) or
     * whether it's just a plain unscoped operator.
     */
    enum KWType
    {
        KT_Undefined = 0, /**< Only used for sentinel */
        KT_Standalone,    /**< Keyword doesn't open or close a scope. */
        KT_Opening,       /**< Keyword opens a new scope. */
        KT_Closing        /**< Keyword closes an open scope. */
    };

    /**
     * KWInfo describes a single PDF keyword's characteristics. See kwInfo[] .
     */
    struct KWInfo {
        /// Keyword type ( ends scope, begins scope, or scope neutral )
        KWType kt;
        /// Keyword ID (enum)
        PdfContentStreamKeyword kw;
        /// ID enum of context closing keyword (only to be set if this
        /// is a context opening keyword), eg KW_Q if kw = KW_q .
        PdfContentStreamKeyword kwClose;
        /// null-terminated keyword text
        const char kwText[6];
        /// Short description text (optional, set to NULL if undesired).
        const char * kwDesc;
    };

    // KWInstance stores a keyword and any associated arguments. The keyword
    // may be stored as an enumerated ID (if it's known to PoDoFo) or a string
    // (if it's not).
    class KWInstance
    {
        typedef boost::variant<PdfContentStreamKeyword,std::string> KWVariant;
        typedef std::vector<PoDoFo::PdfVariant> KWArgs;
        KWVariant m_keyword;
        KWArgs m_args;
    public:
        /** Construct a default(KW_Undefined) KWInstance */
        KWInstance() : m_keyword(KW_Undefined) { }
        /** Construct a KWInstance from an enum id. \see SetKw */
        KWInstance(PdfContentStreamKeyword kw) : m_keyword(kw)
        {
            if (kw == KW_Undefined)
                PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidEnumValue, "Cannot explicitly init KWInstance to KW_Undefined");
        }
        /** Construct a KWInstance from a string. \see SetKw */
        KWInstance(const std::string& kwStr) { SetKw(kwStr); }
        /** True iff this instance is defined. */
        bool IsDefined() const { return GetKwId() != KW_Undefined; }
        /** True iff this is a root node object. */
        bool IsRootNode() const { return GetKwId() == KW_RootNode; }
        /** Return a string representation of the keyword (as it will appear in a content stream). */
        std::string GetKwString() const;
        /** Returns KW_Unknown if we're a string-based KWInstance for an unknown keyword */
        PdfContentStreamKeyword GetKwId() const;
        /**
         * Returns a reference to the KWInfo structure for this keyword, or the
         * special kwInfoUnknown structure (kw=KW_Unknown,
         * kwType=KT_Standalone) if the keyword is not known. If !IsDefined(),
         * returns a record with kw=KW_Undefined and kwType=KT_Undefined .
         *
         *  \see findKwById \see findKwByName
         */
        const KWInfo& GetKwInfo() const { return findKwById(GetKwId()); }
        /** return a reference to the argument array of this keyword */
        KWArgs & GetArgs() { return m_args; }
        /** Return a reference to the argument array of this keyword. */
        const KWArgs & GetArgs() const { return m_args; }
        /**
         * Set this keyword to the string `str'. If `str' is recognised by PoDoFo,
         * it'll be converted to a keyword enum.
         */
        inline void SetKw(const std::string& kwStr);
        /** Set this keyword to the enum value kw */
        void SetKw(PdfContentStreamKeyword kw) { m_keyword = kw; }
        /** Print this keyword and its arguments to the passed stream in proper content stream format.
         *  If the node is of type KW_Undefined this is a no-op, so it's always safe to call on both sides
         *  of a NodeData pair.
         *  An optional whitespace string `szSepStr' may be provided to override the newline that's
         *  normally written after each argument and keyword. The provided length must NOT include the
         *  trailing NULL (if any);
         */
        void PrintToStream(PdfOutputStream& os, const char * szSepStr = " ", long lSepLen = 1L ) const;
    };

    // Each node actually has two values. Internal nodes have both defined, with the first being the
    // keyword opening the context and the second being the keyword closing the context. Leaf nodes
    // have only the first defined (the keyword and its arguments) with the second being undefined.
    typedef std::pair<KWInstance,KWInstance> NodeData;
    typedef boost::adjacency_list<boost::listS,boost::vecS,boost::directedS,NodeData> Graph;
    typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;

    /**
     * Construct a new, blank PdfContentsGraph
     */
    PdfContentsGraph();

    /**
     * Construct a PdfContentsGraph from a PdfContentsTokenizer's output.
     */
    PdfContentsGraph(PdfContentsTokenizer & contentsTokenizer);

    /**
     * Destroy the PdfContentsGraph.
     */
    ~PdfContentsGraph() { }

    /**
     * Serialize the PdfContentsGraph to the passed output stream.
     * The output format is valid PDF content stream data.
     */
    void Write(PdfOutputStream& outStream);

    /**
     * For quick and easy debugging, serialize the graph to stderr as
     * a PDF content stream.
     */
    void WriteToStdErr();

    /**
     * Look up a keyword string and return a reference to the associated
     * keyword info struct If the keyword string is not known, return
     * a reference to the special kwInfoUnknown structure that sets:
     *
     * kwType = PdfContentsGraph::KT_Standalone
     * kw     = KW_Unknown
     *
     * (The rest of the members should not be relied upon).
     */
    static const KWInfo& findKwByName(const std::string & kwText);

    /**
     * Look up an operator code and return the associated keyword string. All
     * defined enums MUST exist in kwIdMap .
     */
    static const KWInfo& findKwById(PdfContentStreamKeyword kw);

    /**
     * Provide access to the internal graph used by ContentStreamGraph to
     * represent the content stream. The caller may safely modify this graph
     * so long as:
     *
     *  - No cyclic references are created, ie it remains a simple tree
     *  - The root node is not altered/removed/replaced
     *  - All internal nodes (ie nodes with children) have variant type
     *    KWPair, where the first value in the pair is the PdfContentStreamKeyword
     *    for a valid context opening keyword, and the second in the pair is the
     *    corresponding closing keyword.
     *  - Nodes of variant type PdfContentStreamKeyword must not contain a context
     *    opening or closing keyword.
     *
     * You can use the findKwById and findKwByName functions to determine the attributes
     * of a keyword - for example, whether it's a context opening / closing keyword.
     *
     * For many complex operations on PDF content streams you will want to modify this
     * graph directly or use it as input for one of the Boost Graph Library algorithms
     * in combination with a custom visitor. To see how this works, have a look at the
     * implementation of this class's Write(...) method. Another example can be found in
     * test/ContentsParser.
     *
     * \see findKwById \see findKwByName
     */
    PODOFO_NOTHROW Graph & GetGraph() { return m_graph; }

    /**
     * Provide access to a read only view of the internal graph.
     *
     * \see GetGraph()
     */
    PODOFO_NOTHROW const Graph & GetGraph() const { return m_graph; }

    /**
     * Return a string-formatted version of the passed KWInstance.
     */
    static std::string formatVariant( const KWInstance& var );

    /**
     * Make a KWNode from a pair of possible keyword values (each of which may
     * be a type convertable to std::string or a PdfContentStreamKeyword)
     */
    template<typename T1, typename T2>
    std::pair<KWInstance,KWInstance> MakeNode( const T1 & kw1, const T2 & kw2 )
    {
        return std::pair<KWInstance,KWInstance>(kw1,kw2);
    }

    /**
     * Make a KWNode from a possible keyword value ( a type convertable to
     * std::string or a PdfContentStreamKeyword ), with the second part of
     * the node being KW_Undefined.
     */
    template<typename T1>
    std::pair<KWInstance,KWInstance> MakeNode( const T1 & kw )
    {
        return std::pair<KWInstance,KWInstance>(kw,KWInstance());
    }

private:
    // private member variables
    Graph m_graph;
};

/**
 * Set this keyword to the string `str'. If `str' is recognised by PoDoFo,
 * it'll be converted to a keyword enum.
 */
inline void PdfContentsGraph::KWInstance::SetKw(const std::string& kwStr)
{
    const KWInfo& kwInfo = findKwByName(kwStr);
    if (kwInfo.kw == KW_Unknown)
        m_keyword = kwStr;
    else
        m_keyword = kwInfo.kw;
}

} // namespace PoDoFo

#endif // defined(PODOFO_HAVE_BOOST)

#endif
