// SPDX-FileCopyrightText: 2022 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

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
// The default must fit the deepest guarded walker in the 1 MB default stack of
// Windows, in a Debug build as well: the name tree lookup survives up to 349
// levels there (/RTC1, x64) and up to 1509 in Release, measured with the unit tests
constexpr unsigned MaxRecursionDepthDefault = 256;

PODOFO_EXPORT unsigned s_MaxRecursionDepth = MaxRecursionDepthDefault;

#ifdef NDEBUG
PODOFO_EXPORT PdfLogSeverity s_MaxLogSeverity = PdfLogSeverity::Information;
#else // !NDEBUG
PODOFO_EXPORT PdfLogSeverity s_MaxLogSeverity = PdfLogSeverity::Debug;
#endif // NDEBUG

PODOFO_EXPORT LogMessageCallback s_LogMessageCallback;

PODOFO_EXPORT ssl::OpenSSLMain s_SSL;

static unsigned s_MaxObjectCount = MAX_OBJECT_COUNT;

// A few compressed bytes may decode to orders of magnitude more: this
// fits a color A4 page scanned at 1200 dpi or an A0 one at 300 dpi
static size_t s_MaxDecodedStreamSize = 512 * 1024 * 1024;

#if OPENSSL_VERSION_MAJOR >= 3
OSSL_LIB_CTX* ssl::Init()
#else // OPENSSL_VERSION_MAJOR < 3
void ssl::Init()
#endif // OPENSSL_VERSION_MAJOR >= 3
{
    // Initialize the OpenSSL singleton
    static struct InitOpenSSL
    {
        InitOpenSSL()
        {
            s_SSL.Init();
        }
    } s_init;
#if OPENSSL_VERSION_MAJOR >= 3
    return s_SSL.GetLibCtx();
#endif // OPENSSL_VERSION_MAJOR >= 3
}

bool PdfCommon::IsDebugBuild()
{
#ifdef NDEBUG
    return false;
#else // !NDEBUG
    return true;
#endif // NDEBUG
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
    // NOTE: Leave room for the object number following the last one
    s_MaxObjectCount = std::min<unsigned>(maxObjectCount, numeric_limits<uint32_t>::max() - 1);
}

void PdfCommon::SetMaxRecursionDepth(unsigned maxRecursionDepth)
{
    s_MaxRecursionDepth = maxRecursionDepth;
}

unsigned PdfCommon::GetMaxRecursionDepth()
{
    return s_MaxRecursionDepth;
}

void PdfCommon::SetMaxDecodedStreamSize(size_t maxDecodedStreamSize)
{
    s_MaxDecodedStreamSize = maxDecodedStreamSize;
}

size_t PdfCommon::GetMaxDecodedStreamSize()
{
    return s_MaxDecodedStreamSize;
}
