
#ifndef PODOFO_WRAPPER_PDFLOCALEH
#define PODOFO_WRAPPER_PDFLOCALEH
/*
 * This is a simple wrapper include file that lets you include
 * <podofo/base/PdfLocale.h> when building against a podofo build directory
 * rather than an installed copy of podofo. You'll probably need
 * this if you're including your own (probably static) copy of podofo
 * using a mechanism like svn:externals .
 */
#include "../../src/base/PdfLocale.h"
#endif
