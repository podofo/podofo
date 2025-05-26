#pragma once

// Include clean windows headers and undef most offending macros

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef Yield
#undef DrawText
#undef GetObject
#undef CreateFont
#undef GetEnvironmentVariable
