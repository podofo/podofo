/***************************************************************************
 *   Copyright (C) 2008 by Dominik Seichter, Craig Ringer                  *
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

#include "../PdfDefines.h"
#include "../PdfDefinesPrivate.h"

#if ! defined(PODOFO_MULTI_THREAD)
#error "Not a multi-thread build. PdfMutex_null.h should be used instead"
#endif

#if defined(_WIN32)
#error "win32 build. PdfMutex_win32.h should be used instead"
#endif

#include <pthread.h>
#include <errno.h>

namespace PoDoFo {
namespace Util {

/**
 * A platform independent reentrant mutex, pthread implementation.
 *  
 * PdfMutex is *NOT* part of PoDoFo's public API.
 *
 * This is the pthread implementation, which is
 * entirely inline.
 */
class PdfMutexImpl {
    pthread_mutex_t m_mutex;
  public:

    inline PdfMutexImpl();

    inline ~PdfMutexImpl();

    inline void Init( const pthread_mutexattr_t *attr );

    /**
     * Lock the mutex
     */
    inline void Lock();

    /**
     * Try locking the mutex. 
     *
     * \returns true if the mutex was locked
     * \returns false if the mutex is already locked
     *                by some other thread
     */
    inline bool TryLock();

    /**
     * Unlock the mutex
     */
    inline void UnLock();
};

PdfMutexImpl::PdfMutexImpl() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init( &m_mutex, &attr );
}

PdfMutexImpl::~PdfMutexImpl()
{
    pthread_mutex_destroy( &m_mutex );
}

void PdfMutexImpl::Lock()
{
    if( pthread_mutex_lock( &m_mutex ) != 0 ) 
    {
	    PODOFO_RAISE_ERROR( ePdfError_MutexError );
    }
}

bool PdfMutexImpl::TryLock()
{
    int nRet = pthread_mutex_trylock( &m_mutex );
    if( nRet == 0 )
	    return true;
    else if( nRet == EBUSY )
	    return false;
    else
    {
	    PODOFO_RAISE_ERROR( ePdfError_MutexError );
    }
}

void PdfMutexImpl::UnLock()
{
    if( pthread_mutex_unlock( &m_mutex ) != 0 )
    {
	    PODOFO_RAISE_ERROR( ePdfError_MutexError );
    }
}

}; // Util
}; // PoDoFo
