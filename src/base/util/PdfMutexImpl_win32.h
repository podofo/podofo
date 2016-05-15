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
	// Visual Studio Code Analyzer warns that InitializeCriticalSection must be within try/catch block
	// because InitializeCriticalSection can raise a STATUS_NO_MEMORY exception in low memory conditions
	// on Windows XP / Server 2003. The exception was eliminated in Windows Vista / Server 2008,
	// so don't warn if building for Vista or later.

#if ( WINVER >= _WIN32_WINNT_VISTA )
	#pragma warning(disable : 28125)
#endif

    InitializeCriticalSection( &m_cs );

#if ( WINVER >= _WIN32_WINNT_VISTA )
	#pragma warning(default : 28125)
#endif

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
