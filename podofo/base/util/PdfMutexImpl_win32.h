
#ifndef PODOFO_WRAPPER_PDFMUTEXIMPLWIN32H
#define PODOFO_WRAPPER_PDFMUTEXIMPLWIN32H
/*
 * This is a simple wrapper include file that lets you include
 * <podofo/base/PdfMutexImpl_win32.h> when building against a podofo build directory
 * rather than an installed copy of podofo. You'll probably need
 * this if you're including your own (probably static) copy of podofo
 * using a mechanism like svn:externals .
 */
#include "../../../src/base/util/PdfMutexImpl_win32.h"
#endif
