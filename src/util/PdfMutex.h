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

#ifndef _PDF_MUTEX_H_
#define _PDF_MUTEX_H_

#include "../PdfDefines.h"

#ifdef PODOFO_MULTI_THREAD
#ifdef _WIN32

#else
#include <pthread.h>
#endif // _WIN32
#endif // PODOFO_MULTI_THREAD

namespace PoDoFo {
namespace Util {

/** A plattform independent mutex.
 *
 *  Uses pthreads on Unix and critical sections
 *  on Windows.
 *
 *  If PODOFO_MULTI_THREAD is not defined during
 *  the build, this class does nothing.
 *  
 */
class PODOFO_API PdfMutex {
  public:
    /** Construct a new mutex
     */
    PdfMutex();

    ~PdfMutex();

    /** 
     * Query if this is a multithreaded PoDoFo build.
     */
    static bool IsPoDoFoMultiThread();

    /**
     * Lock the mutex
     */
    void Lock();

    /**
     * Try locking the mutex. 
     *
     * \returns true if the mutex was locked
     * \returns false if the mutex is already locked
     *                by some other thread
     */
    bool TryLock();

    /**
     * Unlock the mutex
     */
    void UnLock();

  private:
#ifdef PODOFO_MULTI_THREAD
#ifdef _WIN32

#else
    pthread_mutex_t m_mutex;    
#endif // _WIN32
#endif // PODOFO_MULTI_THREAD

};

}; // Util
}; // PoDoFo

#endif // _PDF_MUTEX_H_
