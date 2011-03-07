
#ifndef PODOFO_WRAPPER_PDFMUTEXIMPLNOOPH
#define PODOFO_WRAPPER_PDFMUTEXIMPLNOOPH
/*
 * This is a simple wrapper include file that lets you include
 * <podofo/base/PdfMutexImpl_noop.h> when building against a podofo build directory
 * rather than an installed copy of podofo. You'll probably need
 * this if you're including your own (probably static) copy of podofo
 * using a mechanism like svn:externals .
 */
#include "../../../src/base/util/PdfMutexImpl_noop.h"
#endif
