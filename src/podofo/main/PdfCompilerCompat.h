/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_COMPILER_COMPAT_H
#define PDF_COMPILER_COMPAT_H

//
// *** THIS HEADER IS INCLUDED BY PdfDeclarations.h ***
// *** DO NOT INCLUDE DIRECTLY ***
#ifndef PDF_DEFINES_H
#error Please include PdfDeclarations.h instead
#endif

#ifndef PODOFO_COMPILE_RC

// Silence some annoying warnings from Visual Studio
#ifdef _MSC_VER
#pragma warning(disable: 4251)
#pragma warning(disable: 4275)
#endif // _MSC_VER

// Make sure that DEBUG is defined 
// for debug builds on Windows
// as Visual Studio defines only _DEBUG
#ifdef _DEBUG
#ifndef DEBUG
#define DEBUG 1
#endif // DEBUG
#endif // _DEBUG

#include <cstdint>
#include <cstddef>
#include <cstring>

// Declare ssize_t as a signed alternative to size_t,
// useful for example to provide optional size argument
#if defined(_MSC_VER)
 // Fix missing posix "ssize_t" typedef in MSVC
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else
// Posix has ssize_t
#include <sys/types.h>
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
#if defined(__GNUC__)
#  define PODOFO__FUNCTION__ __PRETTY_FUNCTION__
#else
#  define PODOFO__FUNCTION__ __FUNCTION__
#endif

/**
 * \page PoDoFo PdfCompilerCompat Header
 * 
 * <b>PdfCompilerCompat.h</b> gathers up nastiness required for various
 * compiler compatibility into a central place. All compiler-specific defines,
 * wrappers, and the like should be included here and (if necessary) in
 * PdfCompilerCompat.cpp if they must be visible to public users of the library.
 *
 * If the nasty platform and compiler specific hacks can be kept to PoDoFo's
 * build and need not be visible to users of the library, put them in
 * PdfCompilerCompatPrivate.{cpp,h} instead.
 *
 * Please NEVER use symbols from this header in  a "using" directive.
 * Always explicitly reference names so it's clear that you're pulling
 * them from the compat cruft.
 */

#endif // !PODOFO_COMPILE_RC

#endif // PDF_COMPILER_COMPAT_H
