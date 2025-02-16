/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PODOFO_BASE_DEFS_H
#define PODOFO_BASE_DEFS_H

/*
 * This header provides a macro to handle correct symbol imports/exports
 * on platforms that require explicit instructions to make symbols public,
 * or differentiate between exported and imported symbols.
 * 
 * Win32 compilers use this information, and gcc4 can use it on *nix
 * to reduce the size of the export symbol table and get faster runtime
 * linking.
 *
 * All declarations of public API should be marked with the PODOFO_API macro.
 * Separate definitions need not be annotated, even in headers.
 *
 * Usage examples:
 *
 * class PODOFO_API PdfArray : public PdfDataContainer {
 *     ...
 * };
 *
 * bool PODOFO_API doThatThing();
 *
 * For an exception type that may be thrown across a DSO boundary, you must
 * use:
 *
 * class PODOFO_EXCEPTION_API(PODOFO_API) MyException
 * {
 *     ...
 * };
 *
 */

// Sanity check, can't compile both shared and static library
#if defined(PODOFO_SHARED) && defined(PODOFO_STATIC)
    #error "Both PODOFO_SHARED and PODOFO_STATIC defined!"
#endif

#ifdef PODOFO_STATIC

#define PODOFO_API
#define PODOFO_EXPORT
#define PODOFO_IMPORT

#else // PODOFO_SHARED
#ifndef PODOFO_SHARED
#define PODOFO_SHARED
#endif

#if defined(_MSC_VER)
    #define PODOFO_EXPORT __declspec(dllexport)
    #define PODOFO_IMPORT __declspec(dllimport)
#else
    // NOTE: In non MSVC compilers https://gcc.gnu.org/wiki/Visibility,
    // it's not necessary to distinct between exporting and importing
    // the symbols and for correct working of RTTI features is better
    // always set default visibility both when compiling and when using
    // the library. The symbol will not be re-exported by other libraries
    #define PODOFO_EXPORT __attribute__ ((visibility("default")))
    #define PODOFO_IMPORT __attribute__ ((visibility("default")))
#endif

#if defined(PODOFO_BUILD)
#define PODOFO_API PODOFO_EXPORT
#else
#define PODOFO_API PODOFO_IMPORT
#endif

#endif

// Throwable classes must always be exported by all binaries when
// using gcc. Marking exception classes with PODOFO_EXCEPTION_API
// ensures this.
#ifdef _WIN32
  #define PODOFO_EXCEPTION_API(api) api
#else
  #define PODOFO_EXCEPTION_API(api) PODOFO_API
#endif

// Set up some other compiler-specific but not platform-specific macros

#ifdef __GNU__
  #define PODOFO_HAS_GCC_ATTRIBUTE_DEPRECATED 1
#elif defined(__has_attribute)
  #if __has_attribute(__deprecated__)
    #define PODOFO_HAS_GCC_ATTRIBUTE_DEPRECATED 1
  #endif
#endif

#ifdef PODOFO_HAS_GCC_ATTRIBUTE_DEPRECATED
    // gcc (or compat. clang) will issue a warning if a function or variable so annotated is used
    #define PODOFO_DEPRECATED __attribute__((__deprecated__))
#else
    #define PODOFO_DEPRECATED
#endif

// Specify the friend identifier is defined in private symbols only
#define PODOFO_PRIVATE_FRIEND(identifier)

// Include some useful compatibility defines
#include "basecompat.h"

// Include the configuration file
#include "podofo_config.h"

#endif // PODOFO_BASE_DEFS_H
