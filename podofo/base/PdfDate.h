
#ifndef PODOFO_WRAPPER_PDFDATEH
#define PODOFO_WRAPPER_PDFDATEH
/*
 * This is a simple wrapper include file that lets you include
 * <podofo/base/PdfDate.h> when building against a podofo build directory
 * rather than an installed copy of podofo. You'll probably need
 * this if you're including your own (probably static) copy of podofo
 * using a mechanism like svn:externals .
 */
#include "../../src/base/PdfDate.h"
#endif
