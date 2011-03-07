
#ifndef PODOFO_WRAPPER_PDFMEMSTREAMH
#define PODOFO_WRAPPER_PDFMEMSTREAMH
/*
 * This is a simple wrapper include file that lets you include
 * <podofo/base/PdfMemStream.h> when building against a podofo build directory
 * rather than an installed copy of podofo. You'll probably need
 * this if you're including your own (probably static) copy of podofo
 * using a mechanism like svn:externals .
 */
#include "../../src/base/PdfMemStream.h"
#endif
