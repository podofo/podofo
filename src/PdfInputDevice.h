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
 */
class PdfInputDevice {
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

	/** Get next char from stream.
	 *  \returns the next character from the stream
	 */
	virtual int getc() const ;

	/** Peek at next char in stream.
	 *  /returns the next char in the stream
     */
	virtual int look() const ;

	/** Get current position in file.
	 *  /returns the current position in the file
	 */
	virtual int tell() const ;
	virtual int getPos() const { return tell(); }

	/** Go to a position in the stream.  If <dir> is negative, the
	 * position is from the end of the file; otherwise the position is
	 * from the start of the file.
	 * /param off the offset from the dir
	 * /param dir where to start (start, pos, end)
	 */
	virtual void seek( std::streamoff off, std::ios_base::seekdir dir = 0 ) const;
	virtual void setPos( std::streamoff off, std::ios_base::seekdir dir = 0 ) const { seek( off, dir ); }

	/** read a certain number of bytes from the stream
	 *  /param outData the data that is read
	 *  /param inNumBytes the number of bytes to read
	 */
	virtual void read( char* outData, std::streamsize inNumBytes ) const;

	/** read a certain number of objects from the stream
	 *  /param outData the data that is read
	 *  /param inNumObjs the number of objects to read
	 *  /param inObjSize the size of each object
	 */
	virtual void read( char* outData, std::streamsize inNumObjs, int inObjSize ) 
	{
		read( outData, inNumObjs * inObjSize );
	}

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

