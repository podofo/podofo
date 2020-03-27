/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

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
#  ifdef PODOFO_MULTI_THREAD
#    if defined(_WIN32) || defined(_WIN64)
#      ifndef _WIN32_WINNT
#        define _WIN32_WINNT 0x0400 // Make the TryEnterCriticalSection method available
#        include <winsock2.h>       // This will include windows.h, so we have to define _WIN32_WINNT
                                    // if we want to use threads later.
#        undef _WIN32_WINNT 
#      else
#        include <winsock2.h>
#      endif // _WIN32_WINNT
#    endif // _WIN32 || _WIN64
#  else
#    include <winsock2.h>
#  endif // PODOFO_MULTI_THREAD
#endif

#if PODOFO_HAVE_ARPA_INET_H
#  include <arpa/inet.h>
#endif

#ifdef PODOFO_MULTI_THREAD
#  if defined(_WIN32) || defined(_WIN64)
#    if defined(_MSC_VER) && !defined(_WINSOCK2API_)
#      error <winsock2.h> must be included before <windows.h>, config problem?
#    endif
#    ifndef _WIN32_WINNT
#      define _WIN32_WINNT 0x0400 // Make the TryEnterCriticalSection method available
#      include <windows.h>
#      undef _WIN32_WINNT 
#    else
#      include <windows.h>
#    endif // _WIN32_WINNT
#  else
#    include <pthread.h>
#  endif // _WIN32
#endif // PODOFO_MULTI_THREAD

#if defined(_WIN32) || defined(_WIN64)
#  if defined(GetObject)
#    undef GetObject // Horrible windows.h macro definition that breaks things
#  endif
#  if defined(DrawText)
#    undef DrawText // Horrible windows.h macro definition that breaks things
#  endif
#  if defined(CreateFont)
#    undef CreateFont
#  endif
#endif

namespace PoDoFo {
namespace compat {

// Case-insensitive string compare functions aren't very portable, and we must account
// for several flavours.
inline static int strcasecmp( const char * s1, const char * s2) {
#if defined(_WIN32) || defined (_WIN64)
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
#if defined(_WIN32) || defined(_WIN64)
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
#if defined(_WIN32) || defined(_WIN64)
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
#if defined(_WIN32) && defined(_MSC_VER)
   return (pdf_uint32)( ntohl( i ) );
#else
   return static_cast<pdf_uint32>( ntohl( i ) );
#endif // _WIN32
}

inline static pdf_uint16 podofo_ntohs(pdf_uint16 i) {
#if defined(_WIN32) && defined(_MSC_VER)
   return (pdf_uint16)( ntohs( i ) );
#else
   return static_cast<pdf_uint16>( ntohs( i ) );
#endif // _WIN32
}

inline static pdf_uint32 podofo_htonl(pdf_uint32 i) {
#if defined(_WIN32) && defined(_MSC_VER)
    return (pdf_uint32)( htonl( i ) );
#else
    return static_cast<pdf_uint32>( htonl( i ) );
#endif // _WIN32
}

inline static pdf_uint16 podofo_htons(pdf_uint16 i) {
#if defined(_WIN32) && defined(_MSC_VER)
    return (pdf_uint16)( htons( i ) );
#else
    return static_cast<pdf_uint16>( htons( i ) );
#endif // _WIN32
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

// OC 17.08.2010: Activate showing the correct source for Memory Leak Detection in Visual Studio:
// See: <afx.h>  looking for _AFX_NO_DEBUG_CRT
#ifdef _MSC_VER
#if defined(_DEBUG) && defined(DEFINE_NEW_DEBUG_NEW)
  // fuer crtdbg.h und malloc.h
  #define _CRTDBG_MAP_ALLOC
  #include <malloc.h>
  #include <crtdbg.h>
  void* operator new(size_t ai_NewSize, const char* ac_File_, int ai_Line);
  void operator delete(void* av_Ptr_, const char* ac_File_, int ai_Line);
  #define DEBUG_NEW new(__FILE__, __LINE__)
  #define new DEBUG_NEW
  // doesnt work:
  // // _NEW_CRT is defined in <xdebug>
  // // #define new _NEW_CRT
#endif // _DEBUG
#endif // _MSC_VER

// prefer std::unique_ptr over std::auto_ptr
#ifdef PODOFO_HAVE_UNIQUE_PTR
#define PODOFO_UNIQUEU_PTR std::unique_ptr
#else
#define PODOFO_UNIQUEU_PTR std::auto_ptr
#endif

/**
 * \page PoDoFo PdfCompilerCompatPrivate Header
 * 
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
