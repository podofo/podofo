
#ifndef PODOFO_WRAPPER_PDFDATAH
#define PODOFO_WRAPPER_PDFDATAH
/*
 * This is a simple wrapper include file that lets you include
 * <podofo/base/PdfData.h> when building against a podofo build directory
 * rather than an installed copy of podofo. You'll probably need
 * this if you're including your own (probably static) copy of podofo
 * using a mechanism like svn:externals .
 */
#include "../../src/base/PdfData.h"
#endif
