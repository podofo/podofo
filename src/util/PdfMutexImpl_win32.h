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

#include "../PdfDefines.h"
#include "../PdfDefinesPrivate.h"

#if ! defined(PODOFO_MULTI_THREAD)
#error "Not a multi-thread build. PdfMutex_null.h should be used instead"
#endif

#if !defined(_WIN32)
#error "Wrong PdfMutex implementation included!"
#endif

namespace PoDoFo {
namespace Util {

/** 
 * A platform independent reentrant mutex, win32 implementation.
 */
class PdfMutexImpl {
  public:
    /** Construct a new mutex
     */
    inline PdfMutexImpl();

    inline ~PdfMutexImpl();

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

  private:
    CRITICAL_SECTION m_cs;
};

PdfMutexImpl::PdfMutexImpl()
{
    InitializeCriticalSection( &m_cs );
}

PdfMutexImpl::~PdfMutexImpl()
{
    DeleteCriticalSection( &m_cs );
}

void PdfMutexImpl::Lock()
{
    EnterCriticalSection( &m_cs );
}

bool PdfMutexImpl::TryLock()
{
    return (TryEnterCriticalSection( &m_cs ) ? true : false);
}

void PdfMutexImpl::UnLock()
{
    LeaveCriticalSection( &m_cs );
}


}; // Util
}; // PoDoFo
