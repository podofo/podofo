#ifndef _PDF_DEFINES_PRIVATE_H_
#define _PDF_DEFINES_PRIVATE_H_

#ifndef BUILDING_PODOFO
#error PdfDefinesPrivate.h is only available for use in the core PoDoFo src/ build .cpp files
#endif

// Right now, just pulls in the private parts of the compiler compat hacks.
#include "PdfCompilerCompatPrivate.h"

/**
 * \page <PoDoFo PdfDefinesPrivate Header>
 *
 * <b>PdfDefinesPrivate.h</b> contains preprocessor definitions, inline functions, templates, 
 * compile-time const variables, and other things that must be visible across the entirety of
 * the PoDoFo library code base but should not be visible to users of the library's headers.
 *
 * This header is private to the library build. It is not installed with PoDoFo and must not be
 * referenced in any way from any public, installed header.
 */

#endif
