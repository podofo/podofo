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
 * All exceptions that might be thrown by PdfMutex
 * are catched and logged.
 *  
 */
class PODOFO_API PdfMutexWrapper {
  public:
    /** Lock a mutex.
     * 
     *  \param rMutex the mutex to be locked.
     */
    PODOFO_NOTHROW PdfMutexWrapper( PdfMutex & rMutex );

    /** Unlocks the mutex on destruction
     */
    ~PdfMutexWrapper();

  private:
    PdfMutex& m_rMutex;
};

}; // Util
}; // PoDoFo

#endif // _PDF_MUTEX_H_
