#ifndef _PDF_COMPILERCOMPAT_H
#define _PDF_COMPILERCOMPAT_H

//
// *** THIS HEADER IS INCLUDED BY PdfDefines.h ***
// *** DO NOT INCLUDE DIRECTLY ***
#ifndef _PDF_DEFINES_H_
#error Please include PdfDefines.h instead
#endif

#include "podofo_config.h"

// Silence some annoying warnings from Visual Studio
#ifdef _MSC_VER
#if _MSC_VER <= 1200 // Visual Studio 6
#pragma warning(disable: 4786)
#pragma warning(disable: 4251)
#elif _MSC_VER <= 1400 // Visual Studio 2005
#pragma warning(disable: 4251)
#pragma warning(disable: 4275)
#endif // _MSC_VER
#endif // _MSC_VER

// Make sure that DEBUG is defined 
// for debug builds on Windows
// as Visual Studio defines only _DEBUG
#ifdef _DEBUG
#ifndef DEBUG
#define DEBUG 1
#endif // DEBUG
#endif // _DEBUG

#if defined(TEST_BIG)
#  define PODOFO_IS_BIG_ENDIAN
#else
#  define PODOFO_IS_LITTLE_ENDIAN
#endif

#if HAVE_STDINT_H
#include <stdint.h>
#endif

#if HAVE_BASETSD_H
#include <BaseTsd.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_MEM_H
#include <mem.h>
#endif

#if HAVE_CTYPE_H
#include <ctype.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

// Integer types - fixed size types guaranteed to work anywhere
// because we detect the right underlying type name to use with
// CMake. Use typedefs rather than macros for saner error messages
// etc.
namespace PoDoFo {
    typedef PDF_INT8_TYPENAME  pdf_int8;
    typedef PDF_INT16_TYPENAME  pdf_int16;
    typedef PDF_INT32_TYPENAME  pdf_int32;
    typedef PDF_INT64_TYPENAME  pdf_int64;
    typedef PDF_UINT8_TYPENAME pdf_uint8;
    typedef PDF_UINT16_TYPENAME pdf_uint16;
    typedef PDF_UINT32_TYPENAME pdf_uint32;
    typedef PDF_UINT64_TYPENAME pdf_uint64;
};
#undef PDF_INT8_TYPENAME
#undef PDF_INT16_TYPENAME
#undef PDF_INT32_TYPENAME
#undef PDF_INT64_TYPENAME
#undef PDF_UINT8_TYPENAME
#undef PDF_UINT16_TYPENAME
#undef PDF_UINT32_TYPENAME
#undef PDF_UINT64_TYPENAME

// pdf_long used to be defined as ptrdiff_t . It's a 64-bit signed quantity
// used with large file offsets and the like. Unfortunately, it's become
// used for much more in PoDoFo's code, to the point where it's not even clear
// what it's supposed to mean anymore.
//
// pdf_long is DEPRECATED. Please use one of the explicitly sized types
// instead, or define a typedef that meaninfully describes what it's for
// (eg say fileoffset_t) if you really need one.
//
// pdf_long should not be used in new code.
//
namespace PoDoFo {
    typedef pdf_int64 pdf_long;
};


// Different compilers use different format specifiers for 64-bit integers
// (yay!).  Use these macros with C's automatic string concatenation to handle
// that ghastly quirk.
//
// for example:   printf("Value of signed 64-bit integer: %"PDF_FORMAT_INT64" (more blah)", 128LL)
//
#if defined(_MSC_VER)
#  define PDF_FORMAT_INT64 "I64"
#  define PDF_FORMAT_UINT64 "I64"
#else
#  define PDF_FORMAT_INT64 "lld"
#  define PDF_FORMAT_UINT64 "llu"
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

// Use the more informative __FUNCTION__ macro instead of __FILE__ __LINE__ where possible.
#if (defined(_MSC_VER)  &&  _MSC_VER <= 1200)  || defined(__BORLANDC__) || defined(__TURBOC__)
#  define PODOFO__FUNCTION__ (__FILE__ ":" __LINE__)
#else
#  define PODOFO__FUNCTION__ __FUNCTION__
#endif

// for htonl
#if HAVE_WINSOCK2_H
#  include <winsock2.h>
#  if defined(GetObject)
#    undef GetObject // Horrible windows.h macro definition that breaks things
#  endif
#  if defined(DrawText)
#    undef DrawText // Horrible windows.h macro definition that breaks things
#  endif
#endif
#if HAVE_ARPA_INET_H
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

/**
 * \mainpage
 * 
 * <b>PdfCompilerCompat.h</b> gathers up nastyness required for various
 * compiler compatibility into a central place. All compiler-specific defines,
 * wrappers, and the like should be included here and (if necessary) in
 * PdfCompilerCompat.cpp .
 *
 * Please NEVER use symbols from this header or the PoDoFo::compat namespace in
 * a "using" directive. Always explicitly reference names so it's clear that
 * you're pulling them from the compat cruft.
 */

#endif
