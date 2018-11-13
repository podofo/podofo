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

#ifndef _PDF_COMPILERCOMPAT_H
#define _PDF_COMPILERCOMPAT_H

//
// *** THIS HEADER IS INCLUDED BY PdfDefines.h ***
// *** DO NOT INCLUDE DIRECTLY ***
#ifndef _PDF_DEFINES_H_
#error Please include PdfDefines.h instead
#endif

#include "podofo_config.h"

#ifndef PODOFO_COMPILE_RC

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


#if defined(__BORLANDC__) || defined( __TURBOC__)
#  include <stddef.h>
#else
#  include <cstddef>
#endif

#if defined(TEST_BIG)
#  define PODOFO_IS_BIG_ENDIAN
#else
#  define PODOFO_IS_LITTLE_ENDIAN
#endif

#if PODOFO_HAVE_STDINT_H
#include <stdint.h>
#endif

#if PODOFO_HAVE_BASETSD_H
#include <BaseTsd.h>
#endif

#if PODOFO_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if PODOFO_HAVE_MEM_H
#include <mem.h>
#endif

#if PODOFO_HAVE_CTYPE_H
#include <ctype.h>
#endif

#if PODOFO_HAVE_STRINGS_H
#include <strings.h>
#endif

// alloca() is defined only in <cstdlib> on Mac OS X,
// only in <malloc.h> on win32, and in both on Linux.
#if defined(_WIN32)
#include <malloc.h>
#endif

// Disable usage of min() and max() macros 
#if defined(_WIN32) && !defined(__MINGW32__)
#define NOMINMAX
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


/* 
 * Some elderly compilers, notably VC6, don't support LL literals.
 * In those cases we can use the oversized literal without any suffix.
 */
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200 // Visual Studio 6
#  define PODOFO_LL_LITERAL(x) x
#  define PODOFO_ULL_LITERAL(x) x
#else
#if defined(PODOFO_COMPILER_LACKS_LL_LITERALS)
#  define PODOFO_LL_LITERAL(x) x
#  define PODOFO_ULL_LITERAL(x) x
#else
#  define PODOFO_LL_LITERAL(x) x##LL
#  define PODOFO_ULL_LITERAL(x) x##ULL
#endif
#endif

#if defined(_MSC_VER)  &&  _MSC_VER <= 1200 // Visual Studio 6
#   define PODOFO_MIN(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#else
#   define PODOFO_MIN(_a, _b) (std::min((_a), (_b)))
#endif

// pdf_long is defined as ptrdiff_t . It's a pointer-sized signed quantity
// used throughout the code for a variety of purposes.
//
// pdf_long is DEPRECATED. Please use one of the explicitly sized types
// instead, or define a typedef that meaningfully describes what it's for.
// Good choices in many cases include size_t (string and buffer sizes) and
// ptrdiff_t (offsets and pointer arithmetic).
//
// pdf_long should not be used in new code.
//
namespace PoDoFo {
    typedef ptrdiff_t pdf_long;
};


// Different compilers use different format specifiers for 64-bit integers
// (yay!).  Use these macros with C's automatic string concatenation to handle
// that ghastly quirk.
//
// for example:   printf("Value of signed 64-bit integer: %"PDF_FORMAT_INT64" (more blah)", 128LL)
//
#if defined(_MSC_VER)
#  define PDF_FORMAT_INT64 "I64d"
#  define PDF_FORMAT_UINT64 "I64u"
#  define PDF_SIZE_FORMAT "Iu"
#elif defined(SZ_INT64) && defined(SZ_LONG) && SZ_INT64 == SZ_LONG
#  define PDF_FORMAT_INT64 "ld"
#  define PDF_FORMAT_UINT64 "lu"
#  define PDF_SIZE_FORMAT "zu"
#else
#  define PDF_FORMAT_INT64 "lld"
#  define PDF_FORMAT_UINT64 "llu"
#  define PDF_SIZE_FORMAT "zu"
#endif



// Different compilers express __FUNC__ in different ways and with different
// capabilities. Try to find the best option.
//
// Note that __LINE__ and __FILE__ are *NOT* included.
// Further note that you can't use compile-time string concatenation on __FUNC__ and friends
// on many compilers as they're defined to behave as if they were a:
//    static const char* __func__ = 'nameoffunction';
// just after the opening brace of each function.
//
#if (defined(_MSC_VER)  &&  _MSC_VER <= 1200)
#  define PODOFO__FUNCTION__ __FUNCTION__
#elif defined(__BORLANDC__) || defined(__TURBOC__)
#  define PODOFO__FUNCTION__ __FUNC__
#elif defined(__GNUC__)
#  define PODOFO__FUNCTION__ __PRETTY_FUNCTION__
#else
#  define PODOFO__FUNCTION__ __FUNCTION__
#endif

#if defined(_WIN32)

// Undefined stuff which windows does define that breaks the build
// e.g. GetObject is defined to either GetObjectA or GetObjectW
#ifdef GetObject
#undef GetObject
#endif // GetObject

#ifdef CreateFont
#undef CreateFont
#endif // CreateFont

#ifdef DrawText
#undef DrawText
#endif // DrawText

#endif // defined(_WIN32)

/**
 * \page PoDoFo PdfCompilerCompat Header
 * 
 * <b>PdfCompilerCompat.h</b> gathers up nastyness required for various
 * compiler compatibility into a central place. All compiler-specific defines,
 * wrappers, and the like should be included here and (if necessary) in
 * PdfCompilerCompat.cpp if they must be visible to public users of the library.
 *
 * If the nasty platform and compiler specific hacks can be kept to PoDoFo's
 * build and need not be visible to users of the library, put them in
 * PdfCompilerCompatPrivate.{cpp,h} instead.
 *
 * Please NEVER use symbols from this header or the PoDoFo::compat namespace in
 * a "using" directive. Always explicitly reference names so it's clear that
 * you're pulling them from the compat cruft.
 */

#endif // !PODOFO_COMPILE_RC

#endif
