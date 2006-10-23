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

#ifndef _PDF_INPUT_DEVICE_H_
#define _PDF_INPUT_DEVICE_H_

#include <istream>

#include "PdfDefines.h"

namespace PoDoFo {

/** This class provides an Input device which operates 
 *  either on a file, a buffer in memory or any arbitrary std::istream
 *
 *  This class is suitable for inheritance to provide input 
 *  devices of your own for PoDoFo.
 *  Just overide the required virtual methods.
 */
class PODOFO_API PdfInputDevice {
 public:

    /** Construct a new PdfInputDevice that reads all data from a file.
     *
     *  \param pszFilename path to a file that will be opened and all data
     *                     is read from this file.
     */
    PdfInputDevice( const char* pszFilename );

    /** Construct a new PdfInputDevice that reads all data from a memory buffer.
     *  The buffer will not be owned by this object - it is COPIED.
     *
     *  \param pBuffer a buffer in memory
     *  \param lLen the length of the buffer in memory
     */
    PdfInputDevice( const char* pBuffer, long lLen );

    /** Construct a new PdfInputDevice that reads all data from a std::istream.
     *
     *  \param pInStream read from this std::istream
     */
    PdfInputDevice( const std::istream* pInStream );

    /** Destruct the PdfInputDevice object and close any open files.
     */
    virtual ~PdfInputDevice();

    /** Close the input device.
     *  No further operations may be performed on this device
     *  after calling this function.
     */
    virtual void Close();

    /** Get the current position in file.
     *  /returns the current position in the file
     */
    virtual std::streamoff Tell() const;

    /** Get next char from stream.
     *  \returns the next character from the stream
     */
    virtual int GetChar() const;

    /** Peek at next char in stream.
     *  /returns the next char in the stream
     */
    virtual int Look() const;

    /** Seek the device to the position offset from the begining
     *  \param off from the beginning of the file
     *  \param dir where to start (start, cur, end)
     */
    virtual void Seek( std::streamoff off, std::ios_base::seekdir dir = std::ios_base::beg );

    /** Read a certain number of bytes from the input device.
     *  
     *  \param pBuffer store bytes in this buffer.
     *                 The buffer has to be large enough.
     *  \param lLen    number of bytes to read.
     *  \returns the number of bytes that have been read.
     *           If reading was successfull the number of read bytes
     *           is equal to lLen.
     */
    virtual std::streamoff Read( char* pBuffer, std::streamsize lLen );

 private: 
    /** Initialize all private members
     */
    void Init();

    /** CAN NOT Construct a new PdfInputDevice without an input source. 
     *
     */
    PdfInputDevice();

 private:
    std::istream*  m_pStream;
    bool           m_StreamOwned;
};


};

#endif // _PDF_INPUT_DEVICE_H_
