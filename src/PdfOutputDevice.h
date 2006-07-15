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

#ifndef _PDF_OUTPUT_DEVICE_H_
#define _PDF_OUTPUT_DEVICE_H_

#include <ostream>

#include "PdfDefines.h"

namespace PoDoFo {

/** This class provides an output device which operates 
 *  either on a file or on a buffer in memory.
 *  Additionally it can count the bytes written to the device.
 */
class PdfOutputDevice {
 public:

    /** Construct a new PdfOutputDevice.
     *  You have to call one of the Init methods afterwards
     *  to initialize the object and to specifiy wether it 
     *  operates on a real file, on a buffer in memory
     *  or if it shall count the bytes written to it.
     *
     *  \see Init
     */
    PdfOutputDevice();

    /** Destruct the PdfOutputDevice object and close any open files.
     */
    virtual ~PdfOutputDevice();

    /** Initialize the PdfOutputDevice and cause all data to be written
     *  to the file specified.
     *  \param pszFilename path to a file that will be opened and all data
     *                     is written to this file.
     */
    void Init( const char* pszFilename );

    /** Initialize the PdfOutputDevice and cause all data to be written
     *  to a buffer in memory. The buffer will not be owned by this object
     *  and has to be allocated before.
     *  \param pBuffer a buffer in memory
     *  \param lLen the length of the buffer in memory
     */
    void Init( char* pBuffer, long lLen );

    /** Initialize the PdfOutputDevice and cause all data to be written
     *  to a std::ostream. 
     *
     *  \param pOutStream write to this std::ostream
     *
     */
    void Init( const std::ostream* pOutStream );

    /** Initialize the PdfOutputDevice and do not write any data but 
     *  count the length of the written data.
     *  \returns ErrOk
     *
     *  \see Length
     */
    void Init();

    /** The number of bytes written to this object.
     *  \returns the number of bytes written to this object.
     *  
     *  \see Init
     */
    inline unsigned long Length() const;

    /** Write to the PdfOutputDevice. Usage is as the usage of printf.
     * 
     *  \param pszFormat a format string as you would use it with printf
     *  \returns ErrOk on success
     *
     *  \see Write
     */
    void Print( const char* pszFormat, ... );

    /** Write data to the buffer. Use this call instead of Print if you 
     *  want to write binary data to the PdfOutputDevice.
     *
     *  \param pBuffer a pointer to the data buffer
     *  \param lLen write lLen bytes of pBuffer to the PdfOutputDevice
     *  \returns ErrOk on success
     * 
     *  \see Print
     */
    void Write( const char* pBuffer, long lLen );

 private:
    unsigned long m_ulLength;

    FILE*         m_hFile;
    char*         m_pBuffer;
    unsigned long m_lBufferLen;

    std::ostream*  m_pStream;
};

unsigned long PdfOutputDevice::Length() const
{
    return m_ulLength;
}

};

#endif // _PDF_OUTPUT_DEVICE_H_

