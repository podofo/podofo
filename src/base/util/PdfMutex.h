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

#ifndef PDF_PDFMUTEX_H
#define PDF_PDFMUTEX_H

#if defined(BUILDING_PODOFO)

/* Import the platform-specific implementation of PdfMutex */
#if defined(PODOFO_MULTI_THREAD)
#  if defined(_WIN32)
#    include "PdfMutexImpl_win32.h"
#  else
#    include "PdfMutexImpl_pthread.h"
#  endif
#else
#  include "PdfMutexImpl_noop.h"
#endif

namespace PoDoFo { namespace Util {

/**
 * Reentrant mutex implemented by win32 CRITICAL_SECTION or pthread recursive mutex.
 *
 * If PODOFO_MULTI_THREAD is not set, all operations are no-ops and always succeed.
 *
 * A held (locked) PdfMutex may not be acquired (locked) by a thread other than
 * the thread that currently holds it.
 *
 * The thread holding a PdfMutex may acquire it repeatedly. Every acquision must be matched
 * by a release.
 *
 * When a PdfMutex is not held by any thread (ie it is newly allocated or has been released)
 * then exactly one thread attempting to acquire it will succeed. If there is more than one
 * thread trying to acquire a PdfMutex, which thread will succeed is undefined.
 *
 */
class PdfMutex : public PdfMutexImpl
{
  // This wrapper/extension class is provided so we can add platform-independent
  // functionality and helpers if desired.
  public:
    PdfMutex() { }
    ~PdfMutex() { }
};

};};

#else // BUILDING_PODOFO
// Only a forward-declaration is available for PdfMutex for sources outside the
// PoDoFo library build its self. PdfMutex is not public API.
namespace PoDoFo { namespace Util { class PdfMutex; }; };
#endif

#endif
