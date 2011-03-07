
#ifndef PODOFO_WRAPPER_PDFXREFH
#define PODOFO_WRAPPER_PDFXREFH
/*
 * This is a simple wrapper include file that lets you include
 * <podofo/base/PdfXRef.h> when building against a podofo build directory
 * rather than an installed copy of podofo. You'll probably need
 * this if you're including your own (probably static) copy of podofo
 * using a mechanism like svn:externals .
 */
#include "../../src/base/PdfXRef.h"
#endif
