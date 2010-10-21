/***************************************************************************
 *   Copyright (C) 2009 by Dominik Seichter                                *
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
 ***************************************************************************/

#include "PdfMemoryManagement.h"
#include "PdfDefinesPrivate.h"

namespace PoDoFo {

bool podofo_is_little_endian()
{ 
    int _p = 1;
    return ((reinterpret_cast<char*>(&_p))[0] == 1);
}

void* podofo_malloc( size_t size )
{
    return malloc( size );
}

void* podofo_realloc( void* buffer, size_t size )
{
    return realloc( buffer, size );
}

void podofo_free( void* buffer )
{
    return free( buffer );
}

};

// OC 17.08.2010: Activate showing the correct source for Memory Leak Detection in Visual Studio:
// See: <afx.h>  looking for _AFX_NO_DEBUG_CRT
//
// Annotation:
// defining the new and delete operators and initializeDebugHeap
// have to be done inside the dll if podofo is compiled as dll.
//
// If podofo is compiled as static library
// the main program is responsible for theese definitions and initialisations.
// Specially the new operator may be implemented completely different
// and may not be redefined here.
#ifdef _MSC_VER
#ifdef DEBUG_NEW // #if defined(_DEBUG) && defined(DEFINE_NEW_DEBUG_NEW)
#if defined(COMPILING_SHARED_PODOFO) || defined(podofo_shared_EXPORTS)

#undef new
void* operator new(size_t ai_NewSize, const char* ac_File_, int ai_Line)
{
    return ::operator new(ai_NewSize, _NORMAL_BLOCK, ac_File_, ai_Line);
}
void operator delete(void* av_Ptr_, const char* ac_File_, int ai_Line)
{
    ::operator delete(av_Ptr_);
}
#define new DEBUG_NEW

static int initializeDebugHeap()
{
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); // _CRTDBG_CHECK_ALWAYS_DF|_CRTDBG_DELAY_FREE_MEM_DF
  // test if the memory leak detection works:
  static const char leakdata[] = "*** test memory leak ***";
  char* memleak = new char[sizeof(leakdata)];
  memcpy(memleak, leakdata, sizeof(leakdata));
  return 1;
}
int isDebugHeapInitialized = initializeDebugHeap();

#endif // podofo_shared_EXPORTS
#endif // DEBUG_NEW
#endif // _MSC_VER
