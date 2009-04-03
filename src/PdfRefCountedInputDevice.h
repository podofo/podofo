/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#ifndef _PDF_REF_COUNTED_INPUT_DEVICE_H_
#define _PDF_REF_COUNTED_INPUT_DEVICE_H_

#include "PdfDefines.h"

namespace PoDoFo {

class PdfInputDevice;

/** 
 * A reference counted input device object
 * which is closed as soon as the last
 * object having access to it is deleted.
 */
class PODOFO_API PdfRefCountedInputDevice {
 public:
    /** Created an empty reference counted input device object
     *  The input device will be initialize to NULL
     */
    PdfRefCountedInputDevice();

    /** Create a new PdfRefCountedInputDevice which reads from a file. 
     *  The file is opened using fopen()
     *  \param pszFilename a filename to be passed to fopen
     *  \param pszMode a mode string that can be passed to fopen
     */
    PdfRefCountedInputDevice( const char* pszFilename, const char* pszMode );


#ifdef _WIN32
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200			// nicht für Visualstudio 6
#else
    /** Create a new PdfRefCountedInputDevice which reads from a file. 
     *  The file is opened using fopen()
     *  \param pszFilename a filename to be passed to fopen
     *  \param pszMode a mode string that can be passed to fopen
     *
     *  This is an overloaded member function to allow working
     *  with unicode characters. On Unix systes you can also path
     *  UTF-8 to the const char* overload.
     *
     */
    PdfRefCountedInputDevice( const wchar_t* pszFilename, const char* pszMode );
#endif
#endif // _WIN32

    /** Create a new PdfRefCountedInputDevice which operates on a in memory buffer
     *  
     *  \param pBuffer pointer to the buffer
     *  \param lLen length of the buffer
     */
    PdfRefCountedInputDevice( const char* pBuffer, size_t lLen );

    /** Create a new PdfRefCountedInputDevice from an PdfInputDevice
     *  
     *  \param pDevice the input device. It will be owned and deleted by this object.
     */
    PdfRefCountedInputDevice( PdfInputDevice* pDevice );

    /** Copy an existing PdfRefCountedFile and increase
     *  the reference count
     *  \param rhs the PdfRefCountedFile to copy
     */
    PdfRefCountedInputDevice( const PdfRefCountedInputDevice & rhs );

    /** Decrease the reference count and close the file
     *  if this is the last owner
     */
    ~PdfRefCountedInputDevice();

    /** Get access to the file handle
     *  \returns the file handle
     */
    PODOFO_NOTHROW inline PdfInputDevice* Device() const;

    /** Copy an existing PdfRefCountedFile and increase
     *  the reference count
     *  \param rhs the PdfRefCountedFile to copy
     *  \returns the copied object
     */
    const PdfRefCountedInputDevice & operator=( const PdfRefCountedInputDevice & rhs );

 private:
    /** Detach from the reference counted file
     */
    void Detach();

 private:
    typedef struct {
        PdfInputDevice* m_pDevice;
        long            m_lRefCount;
    } TRefCountedInputDevice;

    TRefCountedInputDevice* m_pDevice;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfInputDevice* PdfRefCountedInputDevice::Device() const
{
    return m_pDevice ? m_pDevice->m_pDevice : NULL;
}

};

#endif // _PDF_REF_COUNTED_INPUT_DEVICE_H_


