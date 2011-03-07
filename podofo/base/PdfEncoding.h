
#ifndef PODOFO_WRAPPER_PDFENCODINGH
#define PODOFO_WRAPPER_PDFENCODINGH
/*
 * This is a simple wrapper include file that lets you include
 * <podofo/base/PdfEncoding.h> when building against a podofo build directory
 * rather than an installed copy of podofo. You'll probably need
 * this if you're including your own (probably static) copy of podofo
 * using a mechanism like svn:externals .
 */
#include "../../src/base/PdfEncoding.h"
#endif
