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

#ifndef _PDF_MUTEX_WRAPPER_H_
#define _PDF_MUTEX_WRAPPER_H_

#include "../PdfDefines.h"
#include "PdfMutex.h"

namespace PoDoFo {
namespace Util {

/** 
 * A wrapper around PdfMutex.
 * The mutex is locked in the constructor
 * and unlocked in the destructor.
 * 
 * In debug builds all exceptions thrown by the mutex implementation
 * are caught and logged before being rethrown.
 *  
 * Note that PdfMutexWrapper is *not* part of PoDoFo's public API.
 */
class PdfMutexWrapper {
  public:
    /** Lock a mutex.
     * 
     *  \param rMutex the mutex to be locked.
     */
    PODOFO_NOTHROW inline PdfMutexWrapper( PdfMutex & rMutex );

    /** Unlocks the mutex on destruction
     */
    inline ~PdfMutexWrapper();

  private:
    /** default constructor, not implemented
     */
    PdfMutexWrapper(void);
    /** copy constructor, not implemented
     */
    PdfMutexWrapper(const PdfMutexWrapper& rhs);
    /** assignment operator, not implemented
     */
    PdfMutexWrapper& operator=(const PdfMutexWrapper& rhs);

    PdfMutex& m_rMutex;
};

PdfMutexWrapper::PdfMutexWrapper( PdfMutex & rMutex )
    : m_rMutex( rMutex )
{
    m_rMutex.Lock();
}


PdfMutexWrapper::~PdfMutexWrapper()
{
#if defined(DEBUG)
    try {
	m_rMutex.UnLock();
    }
    catch( const PdfError & rError ) 
    {
	rError.PrintErrorMsg();
        throw rError;
    }
#else
    m_rMutex.UnLock();
#endif
}

}; // Util
}; // PoDoFo

#endif // _PDF_MUTEX_H_
