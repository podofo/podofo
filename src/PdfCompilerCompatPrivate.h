#ifndef _PDF_COMPILERCOMPAT_PRIVATE_H
#define _PDF_COMPILERCOMPAT_PRIVATE_H

#ifndef _PDF_DEFINES_PRIVATE_H_
#error Include PdfDefinesPrivate.h instead
#endif

#if defined(__BORLANDC__) || defined( __TURBOC__)
   // Borland Turbo C has a broken "<cmath>" but provides a usable "math.h"
   // and it needs a bunch of other includes
#  include <stdlib.h>
#  include <stdio.h>
#  include <string.h>
#  include <math.h>
#  include <time.h>
#else
   // We can use the ISO C++ headers with other compilers
#  include <cstdlib>
#  include <cstdio>
#  include <cmath>
#  include <cstring>
#  include <ctime>
#endif

#if PODOFO_HAVE_WINSOCK2_H
#  include <winsock2.h>
#  if defined(GetObject)
#    undef GetObject // Horrible windows.h macro definition that breaks things
#  endif
#  if defined(DrawText)
#    undef DrawText // Horrible windows.h macro definition that breaks things
#  endif
#endif
#if PODOFO_HAVE_ARPA_INET_H
#  include <arpa/inet.h>
#endif

namespace PoDoFo {
namespace compat {

// Case-insensitive string compare functions aren't very portable, and we must account
// for several flavours.
inline static int strcasecmp( const char * s1, const char * s2) {
#if defined(_WIN32)
#   if defined(_MSC_VER)
        // MSVC++
        return ::_stricmp(s1, s2);
#   else
        return ::stricmp(s1, s2);
#   endif
#else
    // POSIX.1-2001
    return ::strcasecmp(s1, s2);
#endif
}

inline static int strncasecmp( const char * s1, const char * s2, size_t n) {
#if defined(_WIN32)
#   if defined(_MSC_VER)
        // MSVC++
        return ::_strnicmp(s1, s2, n);
#   else
        return ::strnicmp(s1, s2, n);
#   endif
#else
    // POSIX.1-2001
    return ::strncasecmp(s1, s2, n);
#endif
}

inline static double logb(double x) {
#if defined(_WIN32)
  return ::log(x);
#else
  return ::logb(x);
#endif
}

/*
 * We define inline wrappers for htons and friends here so that
 * any issues with integer types can be contained to just this
 * source file.
 *
 * These functions are defined to do NOTHING when
 * host byte order == network byte order (ie: on big endian hosts)
 * so you do NOT need to #ifdef them. They'll be inlined and
 * then optimized out with any sane compiler and C library.
 */

inline static pdf_uint32 podofo_ntohl(pdf_uint32 i) {
   return static_cast<pdf_uint32>( ntohl( i ) );
}

inline static pdf_uint16 podofo_ntohs(pdf_uint16 i) {
   return static_cast<pdf_uint16>( ntohs( i ) );
}

inline static pdf_uint32 podofo_htonl(pdf_uint32 i) {
   return static_cast<pdf_uint32>( htonl( i ) );
}

inline static pdf_uint16 podofo_htons(pdf_uint16 i) {
   return static_cast<pdf_uint16>( htons( i ) );
}

};}; // end namespace PoDoFo::compat

/*
 * This is needed to enable compilation with VC++ on Windows, which likes to prefix
 * many functions with underscores.
 *
 * TODO: These should probably be inline wrappers instead, and we need to consolidate
 * hacks from the rest of the code where other _underscore_prefixed_names are checked
 * for here.
 */
#ifdef _MSC_VER
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif

#if defined(_WIN64)
#define fseeko _fseeki64
#define ftello _ftelli64
#else
#define fseeko fseek
#define ftello ftell
#endif

/**
 * \def PODOFO_UNUSED( x )
 * Make a certain variable to be unused
 * in the code, without getting a compiler
 * warning.
 */
#ifndef _WIN32
template <typename T>
inline void podofo_unused(T &t) { (void)t; }
#define PODOFO_UNUSED( x ) podofo_unused( x );
#else
#define PODOFO_UNUSED( x ) (void)x;
#endif // _WIN32

/*
 * \mainpage

 * <b>PdfCompilerCompatPrivate.h</b> gathers up nastyness required for various
 * compiler compatibility into a central place. All compiler-specific defines,
 * wrappers, and the like should be included here and (if necessary) in
 * PdfCompilerCompatPrivate.cpp. If the must be visible to library users
 * they're put in PdfCompilerCompat.{cpp,h} instead.
 *
 * PdfCompilerCompatPrivate.h is private to PoDoFo's build process. It is not
 * used by library clients, the tools, or the unit tests. It is not installed
 * with PoDoFo and must never be visible in the public headers.
 *
 * Include PdfCompilerCompatPrivate.h in your .cpp sources, preferably after
 * including other PoDoFo headers.
 *
 * Please NEVER use symbols from this header or the PoDoFo::compat namespace in
 * a "using" directive. Always explicitly reference names so it's clear that
 * you're pulling them from the compat cruft.
 */

#endif
