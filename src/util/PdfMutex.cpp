/***************************************************************************
 *   Copyright (C) 2008 by Dominik Seichter                                *
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

#include "PdfMutex.h"

#include <errno.h>

namespace PoDoFo {
namespace Util {

bool PdfMutex::IsPoDoFoMultiThread()
{
#ifdef PODOFO_MULTI_THREAD
    return true;
#else
    return false;
#endif // PODOFO_MULTI_THREAD
}

PdfMutex::PdfMutex()
#ifdef PODOFO_MULTI_THREAD
#ifndef _WIN32
    : m_mutex( PTHREAD_MUTEX_INITIALIZER )
#endif // _WIN32
#endif // PODOFO_MULTI_THREAD

{
#ifdef PODOFO_MULTI_THREAD
#ifdef _WIN32
    InitializeCriticalSection( &m_cs );
#endif // _WIN32
#endif // PODOFO_MULTI_THREAD
}

PdfMutex::~PdfMutex()
{
#ifdef PODOFO_MULTI_THREAD
#ifdef _WIN32
    DeleteCriticalSection( &m_cs );
#else
    pthread_mutex_destroy( &m_mutex );
#endif // _WIN32
#endif // PODOFO_MULTI_THREAD
}

void PdfMutex::Lock()
{
#ifdef PODOFO_MULTI_THREAD
#ifdef _WIN32
    EnterCriticalSection( &m_cs );
#else
    if( pthread_mutex_lock( &m_mutex ) != 0 ) 
    {
	    PODOFO_RAISE_ERROR( ePdfError_MutexError );
    }
#endif // _WIN32
#endif // PODOFO_MULTI_THREAD
}

bool PdfMutex::TryLock()
{
#ifdef PODOFO_MULTI_THREAD
#ifdef _WIN32
    return (TryEnterCriticalSection( &m_cs ) ? true : false);
#else
    int nRet = pthread_mutex_trylock( &m_mutex );
    if( nRet == 0 )
	    return true;
    else if( nRet == EBUSY )
	    return false;
    else
    {
	    PODOFO_RAISE_ERROR( ePdfError_MutexError );
    }
#endif // _WIN32
#endif // PODOFO_MULTI_THREAD

    // If we have no multithreading support always
    // simulate succesfull locking
    return true;
}

void PdfMutex::UnLock()
{
#ifdef PODOFO_MULTI_THREAD
#ifdef _WIN32
    LeaveCriticalSection( &m_cs );
#else
    if( pthread_mutex_unlock( &m_mutex ) != 0 )
    {
	    PODOFO_RAISE_ERROR( ePdfError_MutexError );
    }
#endif // _WIN32
#endif // PODOFO_MULTI_THREAD
}


}; // Util
}; // PoDoFo
