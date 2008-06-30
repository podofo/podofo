PdfContentsGraph is a demo that uses the Boost::Graph library to construct a graph of
the PDF contents stream(s) that can then be operated on with the usual BGL facilities
for graph walking and modification.

At present the graph code is incomplete; it makes some assumptions about the structure
of PDF content streams that the standard does not require to be true, so it may fail
to parse some content streams.

The test code in main.cpp parses the stream into a graph. It then walks the graph,
using PdfContentsTokenizer to read through the original stream and compare each reached
node in the graph to the matching token in the stream. If the graph reflects the content
stream and is being walked correctly then the sequence of nodes walked in the graph should
match the sequence of tokens read from the stream by PdfContentsTokenizer.

This isn't exactly an exciting application. It should be possible to build more useful
applications, like a PDF contents stream validator, from the same base.

-- Craig Ringer <craig@postnewspapers.com.au>
