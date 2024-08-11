/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "podofo/private/OpenSSLInternal.h"
#include "PdfCommon.h"
#include "PdfFontManager.h"

using namespace std;
using namespace PoDoFo;

// Default stack sizes
// Windows: 1MB on x32, x64, ARM https://docs.microsoft.com/en-us/cpp/build/reference/stack-stack-allocations?view=msvc-160
// Windows IIS: 512 KB for 64-bit worker processes, 256 KB for 32-bit worker processes
// macOS: 8MB on main thread, 512KB on secondary threads https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Multithreading/CreatingThreads/CreatingThreads.html
// iOS: 1MB on main thread, 512KB on secondary threads
// Modern Linux distros: usually 8MB on main and secondary threads (but setting ulimit RLIMIT_STACK to unlimited *reduces* the secondary stack size on most architectures: see https://man7.org/linux/man-pages/man3/pthread_create.3.html#NOTES )
// the amount allocated on stack for local variables and function parameters varies between x86 and x64
// in x86 pointers are 32-bit but all function parameters are on stack
// in x64 pointers are 64-bit but first 4 function params are passed in registers
// the biggest difference is between debug and non-debug stacks: a debug stack frame can be around 3x larger
// due to instrumentation like ASAN which put guard bytes around stack variables to detect buffer overflows
constexpr unsigned MaxRecursionDepthDefault = 450;

PODOFO_EXPORT unsigned s_MaxRecursionDepth = MaxRecursionDepthDefault;

#ifdef DEBUG
PODOFO_EXPORT PdfLogSeverity s_MaxLogSeverity = PdfLogSeverity::Debug;
#else
PODOFO_EXPORT PdfLogSeverity s_MaxLogSeverity = PdfLogSeverity::Information;
#endif // DEBUG

PODOFO_EXPORT LogMessageCallback s_LogMessageCallback;

PODOFO_EXPORT ssl::OpenSSLMain s_SSL;

static unsigned s_MaxObjectCount = (1U << 23) - 1;

void ssl::Init()
{
    // Initialize the OpenSSL singleton
    static struct InitOpenSSL
    {
        InitOpenSSL()
        {
            s_SSL.Init();
        }
    } s_init;
}

void PdfCommon::AddFontDirectory(const string_view& path)
{
    PdfFontManager::AddFontDirectory(path);
}

void PdfCommon::SetLogMessageCallback(const LogMessageCallback& logMessageCallback)
{
    s_LogMessageCallback = logMessageCallback;
}

void PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity logSeverity)
{
    s_MaxLogSeverity = logSeverity;
}

PdfLogSeverity PdfCommon::GetMaxLoggingSeverity()
{
    return s_MaxLogSeverity;
}

bool PdfCommon::IsLoggingSeverityEnabled(PdfLogSeverity logSeverity)
{
    return logSeverity <= s_MaxLogSeverity;
}

unsigned PdfCommon::GetMaxObjectCount()
{
    return s_MaxObjectCount;
}

void PdfCommon::SetMaxObjectCount(unsigned maxObjectCount)
{
    s_MaxObjectCount = maxObjectCount;
}

void PdfCommon::SetMaxRecursionDepth(unsigned maxRecursionDepth)
{
    s_MaxRecursionDepth = maxRecursionDepth;
}

unsigned PdfCommon::GetMaxRecursionDepth()
{
    return s_MaxRecursionDepth;
}
