/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PODOFO_DEFS_H
#define PODOFO_DEFS_H

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

// Automatically defined by CMake when building a shared library
#if defined(podofo_shared_EXPORTS)
    #define COMPILING_SHARED_PODOFO
#endif

// Sanity check - can't be both compiling and using shared PoDoFo
#if defined(SHARED_PODOFO) && defined(STATIC_PODOFO)
    #error "Both SHARED_PODOFO and STATIC_PODOFO defined!"
#endif

// Define SHARED_PODOFO when building the PoDoFo library as a
// DLL. When building code that uses that DLL, define STATIC_PODOFO.
//
// Building or linking to a static library does not require either
// preprocessor symbol.
#ifdef PODOFO_STATIC

#define PODOFO_API
#define PODOFO_EXPORT
#define PODOFO_IMPORT

#elif PODOFO_SHARED

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

#if defined(COMPILING_SHARED_PODOFO)
#define PODOFO_API PODOFO_EXPORT
#else
#define PODOFO_API PODOFO_IMPORT
#endif

#else
#error Neither STATIC_PODOFO or SHARED_PODOFO defined
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

#ifndef PODOFO_UNIT_TEST
#define PODOFO_UNIT_TEST(classname)
#endif

// Disable warnings
#ifdef _MSC_VER
#pragma warning(disable: 4251)
#pragma warning(disable: 4309)
#endif // _WIN32

#endif // PODOFO_DEFS_H
