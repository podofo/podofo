#ifndef PODOFO_VERSION_H
#define PODOFO_VERSION_H

// NOTE: Keep the relative import to accommodate
// the installation layout
#include "podofo_config.h"

/**
 * PoDoFo version - 24-bit integer representation.
 * Version is 0xMMmmpp  where M is major, m is minor and p is patch
 * eg 0.7.0  is represented as 0x000700
 * eg 0.7.99 is represented as 0x000763
 *
 * Note that the PoDoFo version is available in parts as individual 8-bit
 * integer literals in PODOFO_VERSION_MAJOR, PODOFO_VERSION_MINOR and
 * PODOFO_VERSION_PATCH .
 */
#define PODOFO_MAKE_VERSION_REAL(M,m,p) ( (M<<16)+(m<<8)+(p) )
#define PODOFO_MAKE_VERSION(M,m,p) PODOFO_MAKE_VERSION_REAL(M,m,p)
#define PODOFO_VERSION PODOFO_MAKE_VERSION(PODOFO_VERSION_MAJOR, PODOFO_VERSION_MINOR, PODOFO_VERSION_PATCH)

 /**
  * PoDoFo version represented as a string literal, eg '0.7.99'
  */
  // The \0 is from Win32 example resources and the other values in PoDoFo's one
#define PODOFO_MAKE_VERSION_STR_REAL(M,m,p) M ## . ## m ## . ## p
#define PODOFO_STR(x) #x "\0"
#define PODOFO_XSTR(x) PODOFO_STR(x)
#define PODOFO_MAKE_VERSION_STR(M,m,p) PODOFO_XSTR(PODOFO_MAKE_VERSION_STR_REAL(M,m,p))
#define PODOFO_VERSION_STRING PODOFO_MAKE_VERSION_STR(PODOFO_VERSION_MAJOR, PODOFO_VERSION_MINOR, PODOFO_VERSION_PATCH)

#endif // PODOFO_VERSION_H
