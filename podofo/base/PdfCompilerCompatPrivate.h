
#ifndef PODOFO_WRAPPER_PDFCOMPILERCOMPATPRIVATEH
#define PODOFO_WRAPPER_PDFCOMPILERCOMPATPRIVATEH
/*
 * This is a simple wrapper include file that lets you include
 * <podofo/base/PdfCompilerCompatPrivate.h> when building against a podofo build directory
 * rather than an installed copy of podofo. You'll probably need
 * this if you're including your own (probably static) copy of podofo
 * using a mechanism like svn:externals .
 */
#include "../../src/base/PdfCompilerCompatPrivate.h"
#endif
