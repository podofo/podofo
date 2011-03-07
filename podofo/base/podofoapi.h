
#ifndef PODOFO_WRAPPER_PODOFOAPIH
#define PODOFO_WRAPPER_PODOFOAPIH
/*
 * This is a simple wrapper include file that lets you include
 * <podofo/base/podofoapi.h> when building against a podofo build directory
 * rather than an installed copy of podofo. You'll probably need
 * this if you're including your own (probably static) copy of podofo
 * using a mechanism like svn:externals .
 */
#include "../../src/base/podofoapi.h"
#endif
