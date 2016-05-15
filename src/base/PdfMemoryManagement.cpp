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
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#include "PdfMemoryManagement.h"
#include "PdfDefinesPrivate.h"

#ifdef _WIN32
#ifndef SIZE_MAX
#ifdef _WIN64
#define SIZE_MAX _UI64_MAX
#else
#define SIZE_MAX UINT_MAX
#endif
#endif
#endif // _WIN32

#if defined(_MSC_VER) &&  ( _MSC_VER <= 1200 )
// errno.h isn't available in Visual C++ 6 (definitions are in stdlib.h which is already included)
#else
#include <errno.h>
#endif

namespace PoDoFo {

bool podofo_is_little_endian()
{ 
    int _p = 1;
    return ((reinterpret_cast<char*>(&_p))[0] == 1);
}

void* podofo_malloc( size_t size )
{
	/*
		malloc behaviour with size==0 is platform specific

		Windows Visual C++
		MSDN: If size is 0, malloc allocates a zero-length item in the heap and returns a valid pointer to that item.
		Code: malloc_base.c code in VS 2015 shows it shows it allocates a 1-byte block when size==0

		OpenBSD
		Man: If size is equal to 0, a unique pointer to an access protected, zero sized object is returned. 
        Access via this pointer will generate a SIGSEGV exception.

		Linux
		Man: If size is 0, then malloc() returns either NULL, or a unique pointer value that can later be successfully passed to free().

		OS X
		Man: Behaviour unspecified
		Code: Apple malloc.c source code shows it allocates a 1-byte block when size==0

		Behaviour on Linux is interesting: http://www.unix.com/man-page/redhat/3/malloc/
		Linux follows an optimistic memory allocation strategy. This means that when malloc()
		returns non-NULL there is no guarantee that the memory really is available. In case it
		turns out that the system is out of memory, one or more processes will be killed by the
		infamous OOM killer.
	*/

	if (size == 0)
		size = 1;

    return malloc( size );
}

void* podofo_calloc(size_t nmemb, size_t size)
{
	/*
		calloc behaviour with size==0 or nmemb==0 is platform specific

		Windows Visual C++
		Behaviour unspecified in MSDN (allocates 1 byte in VC 2015 calloc_base.c)

		OpenBSD
		If size or nmemb is equal to 0, a unique pointer to an access protected, zero sized object is returned. 
        Access via this pointer will generate a SIGSEGV exception.

		Linux
		If size or nmemb is equal to 0 then calloc() returns either NULL, or a unique pointer value that can later be successfully passed to free().

		OS X
		Behaviour unspecified
	*/

	if (size == 0)
		size = 1;
    
	if (nmemb == 0)
		nmemb = 1;

	/*
		This overflow check is from OpenBSD reallocarray.c, and is also used in GifLib 5.1.2 onwards.
		
        Very old versions of calloc() in NetBSD and OS X 10.4 just multiplied size*nmemb which can
        overflow size_t and allocate much less memory than expected e.g. 2*(SIZE_MAX/2+1) = 2 bytes. 
        The calloc() overflow is also present in GCC 3.1.1, GNU Libc 2.2.5 and Visual C++ 6.
        http://cert.uni-stuttgart.de/ticker/advisories/calloc.html

		MUL_NO_OVERFLOW is sqrt(SIZE_MAX+1), as s1*s2 <= SIZE_MAX
		if both s1 < MUL_NO_OVERFLOW and s2 < MUL_NO_OVERFLOW
	*/
	#define MUL_NO_OVERFLOW	((size_t)1 << (sizeof(size_t) * 4))
    
	if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
		nmemb > 0 && SIZE_MAX / nmemb < size) 
	{
		errno = ENOMEM;
		return NULL;
	}

	return calloc(nmemb, size);
}

void* podofo_realloc( void* buffer, size_t size )
{
	/*
		realloc behaviour with size==0 is platform specific (and dangerous in Visual C++)
	
		Windows Visual C++
		If size is zero, then the block pointed to by memblock is freed; the return value is NULL, and buffer is left pointing at a freed block.
		NOTE: this is very dangerous, since NULL is also returned when there's not enough memory, but the block ISN'T freed

		OpenBSD
		If size is equal to 0, a unique pointer to an access protected, zero sized object is returned. 
        Access via this pointer will generate a SIGSEGV exception.

		Linux
		If size was equal to 0, either NULL or a pointer suitable to be passed to free() is returned.

		OS X
		If size is zero and buffer is not NULL, a new, minimum sized object is allocated and the original object is freed.	
	*/

	if (size == 0)
		size = 1;

    return realloc( buffer, size );
}

void podofo_free( void* buffer )
{
    free( buffer );
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
