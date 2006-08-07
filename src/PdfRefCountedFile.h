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

#ifndef _PDF_REF_COUNTED_FILE_H_
#define _PDF_REF_COUNTED_FILE_H_

#include "PdfDefines.h"

namespace PoDoFo {

/** 
 * A reference counted file object
 * which is closed as soon as the last
 * object having access to it is delteted.
 */
class PdfRefCountedFile {
 public:
    /** Created an empty reference counted file object
     *  The file handle will be initialize to NULL
     */
    PdfRefCountedFile();

    /** Create a new PdfRefCountedFile. 
     *  The file is opened using fopen()
     *  \param pszFilename a filename to be passed to fopen
     *  \param pszMode a mode string that can be passed to fopen
     */
    PdfRefCountedFile( const char* pszFilename, const char* pszMode );

    /** Copy an existing PdfRefCountedFile and increase
     *  the reference count
     *  \param rhs the PdfRefCountedFile to copy
     */
    PdfRefCountedFile( const PdfRefCountedFile & rhs );

    /** Decrease the reference count and close the file
     *  if this is the last owner
     */
    ~PdfRefCountedFile();

    /** Get access to the file handle
     *  \returns the file handle
     */
    inline FILE* Handle() const;

    /** Copy an existing PdfRefCountedFile and increase
     *  the reference count
     *  \param rhs the PdfRefCountedFile to copy
     *  \returns the copied object
     */
    const PdfRefCountedFile & operator=( const PdfRefCountedFile & rhs );

 private:
    /** Detach from the reference counted file
     */
    void Detach();

 private:
    typedef struct TRefCountedFile {
        FILE* m_hFile;
        long  m_lRefCount;
    };

    TRefCountedFile* m_pFile;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline FILE* PdfRefCountedFile::Handle() const
{
    return m_pFile ? m_pFile->m_hFile : NULL;
}

};

#endif // _PDF_REF_COUNTED_FILE_H_

