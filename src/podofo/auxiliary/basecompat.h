/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PODOFO_BASE_COMPAT_H
#define PODOFO_BASE_COMPAT_H

/**
 * \file base_compat.h Some base platform specific defines
 */

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

// Make sure that DEBUG is defined
// for debug builds on Windows
// as Visual Studio defines only _DEBUG
#ifdef _DEBUG
#ifndef DEBUG
#define DEBUG 1
#endif // DEBUG
#endif // _DEBUG

// Silence some annoying warnings from Visual Studio
#ifdef _MSC_VER
#pragma warning(disable: 4251)
#pragma warning(disable: 4275)
#endif // _MSC_VER

#endif // PODOFO_BASE_COMPAT_H
