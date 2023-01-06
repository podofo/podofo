#pragma once

// Include clean windows headers and undef most offending macros

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#undef Yield
#undef DrawText
#undef GetObject
#undef CreateFont
