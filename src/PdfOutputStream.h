/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#ifndef _PDF_OUTPUT_STREAM_H_
#define _PDF_OUTPUT_STREAM_H_

#include "PdfDefines.h"

namespace PoDoFo {

class PdfOutputDevice;

/** An interface for writing blocks of data to 
 *  a data source.
 */     
class PODOFO_API PdfOutputStream {
 public:

    virtual ~PdfOutputStream();

    /** Write data to the output stream
     *  
     *  \param pBuffer the data is read from this buffer
     *  \param lLen    the size of the buffer 
     *
     *  \returns the number of bytes written, -1 if an error ocurred
     */
    virtual long Write( const char* pBuffer, long lLen ) = 0;

};

/** An output stream that writes data to a file
 */
class PODOFO_API PdfFileOutputStream : public PdfOutputStream {
 public:
    
    /** Open a file for writing data
     *  
     *  \param pszFilename the filename of the file to read
     */
    PdfFileOutputStream( const char* pszFilename );
    ~PdfFileOutputStream();

    /** Write data to the output stream
     *  
     *  \param pBuffer the data is read from this buffer
     *  \param lLen    the size of the buffer 
     *
     *  \returns the number of bytes written, -1 if an error ocurred
     */
    virtual long Write( const char* pBuffer, long lLen );

 private:
    FILE* m_hFile;
};

/** An output stream that writes data to a memory buffer
 */
class PODOFO_API PdfMemoryOutputStream : public PdfOutputStream {
 public:
    
    /** 
     *  Construct a new PdfMemoryOutputStream
     *
     *  \param pBuffer buffer to read from
     *  \param lBufferLen length of the buffer
     */
    PdfMemoryOutputStream( char* pBuffer, long lBufferLen );
    ~PdfMemoryOutputStream();

    /** Write data to the output stream
     *  
     *  \param pBuffer the data is read from this buffer
     *  \param lLen    the size of the buffer 
     *
     *  \returns the number of bytes written, -1 if an error ocurred
     */
    virtual long Write( const char* pBuffer, long lLen );

 private:
    char* m_pBuffer;
    char* m_pCur;
    long        m_lBufferLen;
};

/** An output stream that writes to a PdfOutputDevice
 */
class PODOFO_API PdfDeviceOutputStream : public PdfOutputStream {
 public:
    
    /** 
     *  Write to an already opened input device
     * 
     *  \param pDevice an output device
     */
    PdfDeviceOutputStream( PdfOutputDevice* pDevice );
    ~PdfDeviceOutputStream();

    /** Write data to the output stream
     *  
     *  \param pBuffer the data is read from this buffer
     *  \param lLen    the size of the buffer 
     *
     *  \returns the number of bytes written, -1 if an error ocurred
     */
    virtual long Write( const char* pBuffer, long lLen );

 private:
    PdfOutputDevice* m_pDevice;
};

};

#endif // _PDF_OUTPUT_STREAM_H_
