// SPDX-FileCopyrightText: 2005 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

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
    #define PODOFO_DEPRECATED
#else
    // NOTE: In non MSVC compilers https://gcc.gnu.org/wiki/Visibility,
    // it's not necessary to distinct between exporting and importing
    // the symbols and for correct working of RTTI features is better
    // always set default visibility both when compiling and when using
    // the library. The symbol will not be re-exported by other libraries
    #define PODOFO_EXPORT __attribute__ ((visibility("default")))
    #define PODOFO_IMPORT __attribute__ ((visibility("default")))
    #define PODOFO_DEPRECATED __attribute__((__deprecated__))
#endif

#if defined(PODOFO_BUILD)
#define PODOFO_API PODOFO_EXPORT
#else
#define PODOFO_API PODOFO_IMPORT
#endif

#endif

// If detected, undefine some macros that are defined by Windows
// headers and that may cause errors when consuming PoDoFo. To
// avoid this behavior, define PODOFO_WIN32_SKIP_UNDEF_MACROS
// before including PoDoFo headers.
#if defined(_WIN32) && !defined(PODOFO_WIN32_SKIP_UNDEF_MACROS)
#ifdef min
#undef min
#endif // min

#ifdef max
#undef max
#endif // max

#ifdef GetObject
#undef GetObject
#endif // GetObject

#ifdef CreateFont
#undef CreateFont
#endif // CreateFont

#ifdef DrawText
#undef DrawText
#endif // DrawText
#endif

// Set up some other compiler-specific but not platform-specific macros

/// Suppress the warnings on the use of deprecated declarations in the code
/// enclosed by the push/pop pair. It's needed where a deprecated entity must
/// still be referenced, eg. a deprecated field that is still part of a structure
#if defined(_MSC_VER)
#define PODOFO_SUPPRESS_DEPRECATED_PUSH __pragma(warning(push)) __pragma(warning(disable: 4996))
#define PODOFO_SUPPRESS_DEPRECATED_POP __pragma(warning(pop))
#elif defined(__GNUC__) || defined(__clang__)
#define PODOFO_SUPPRESS_DEPRECATED_PUSH _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define PODOFO_SUPPRESS_DEPRECATED_POP _Pragma("GCC diagnostic pop")
#else
#define PODOFO_SUPPRESS_DEPRECATED_PUSH
#define PODOFO_SUPPRESS_DEPRECATED_POP
#endif

/// Specify the friend identifier is defined in private symbols only
#define PODOFO_PRIVATE_FRIEND(identifier)

#ifndef PODOFO_3RDPARTY_INTEROP_ENABLED
/// Define if interoperability with 3rd party APIs (such as
/// libraries like libxml2, Fontconfig) is enabled. Caution
/// is needed, as linkage of internally used structures
/// and user consumed must be the same
#define PODOFO_3RDPARTY_INTEROP_ENABLED 0
#endif // PODOFO_3RDPARTY_INTEROP_ENABLED

// Include some useful compatibility defines
#include "basecompat.h"

// Include the configuration file
#include "podofo_config.h"

#endif // PODOFO_BASE_DEFS_H
