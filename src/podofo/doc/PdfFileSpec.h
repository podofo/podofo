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

#ifndef _PDF_FILE_SPEC_H_
#define _PDF_FILE_SPEC_H_

#include "podofo/base/PdfDefines.h"

#include "podofo/base/PdfString.h"

#include "PdfElement.h"

namespace PoDoFo {

class PdfDocument;

/**
 *  A file specification is used in the PDF file to referr to another file.
 *  The other file can be a file outside of the PDF or can be embedded into
 *  the PDF file itself.
 */
class PODOFO_DOC_API PdfFileSpec : public PdfElement {
 public:
    PdfFileSpec( const char* pszFilename, bool bEmbedd, PdfDocument* pParent, bool bStripPath = false);

    PdfFileSpec( const char* pszFilename, bool bEmbedd, PdfVecObjects* pParent, bool bStripPath = false );

    /* Petr P. Petrov 17 September 2009*/
    /** Embeds the file in memory from "data" buffer under "pszFileName" fie name.
      */
    PdfFileSpec( const char* pszFilename, const unsigned char* data, ptrdiff_t size, PdfVecObjects* pParent, bool bStripPath = false);
    PdfFileSpec( const char* pszFilename, const unsigned char* data, ptrdiff_t size, PdfDocument* pParent, bool bStripPath = false);

#ifdef _WIN32
    PdfFileSpec( const wchar_t* pszFilename, bool bEmbedd, PdfDocument* pParent, bool bStripPath = false );
    PdfFileSpec( const wchar_t* pszFilename, bool bEmbedd, PdfVecObjects* pParent, bool bStripPath = false );
    PdfFileSpec( const wchar_t* pszFilename, const unsigned char* data, ptrdiff_t size, PdfVecObjects* pParent, bool bStripPath = false);
    PdfFileSpec( const wchar_t* pszFilename, const unsigned char* data, ptrdiff_t size, PdfDocument* pParent, bool bStripPath = false);
#endif

    PdfFileSpec( PdfObject* pObject );

    /** Gets file name for the FileSpec
     *  \param canUnicode Whether can return file name in unicode (/UF)
     *  \returns the filename of this file specification.
     *           if no general name is available 
     *           it will try the Unix, Mac and DOS keys too.
     */
    const PdfString & GetFilename(bool canUnicode) const;

 private:

    /** Initialize a filespecification from a filename
     *  \param pszFilename filename 
     *  \param bEmbedd embedd the file data into the PDF file
     *  \param bStripPath whether to strip path from the file name string
     */
    void Init( const char* pszFilename, bool bEmbedd, bool bStripPath );

    /** Initialize a filespecification from an in-memory buffer
     *  \param pszFilename filename 
     *  \param data Data of the file
     *  \param size size of the data buffer
     *  \param bStripPath whether to strip path from the file name string
     */
    void Init( const char* pszFilename, const unsigned char* data, ptrdiff_t size, bool bStripPath );

    /** Create a file specification string from a filename
     *  \param pszFilename filename 
     *  \returns a file specification string
     */
    PdfString CreateFileSpecification( const char* pszFilename ) const;

    /** Embedd a file into a stream object
     *  \param pStream write the file to this objects stream
     *  \param pszFilename the file to embedd
     */
    void EmbeddFile( PdfObject* pStream, const char* pszFilename ) const;

    /** Strips path from a file, according to \a bStripPath
     *  \param pszFilename a file name string
     *  \param bStripPath whether to strip path from the file name string
     *  \returns Either unchanged \a pszFilename, if \a bStripPath is false;
     *     or \a pszFilename without a path part, if \a bStripPath is true
     */
    const char *MaybeStripPath( const char* pszFilename, bool bStripPath ) const;
#ifdef _WIN32
    void Init( const wchar_t* pszFilename, bool bEmbedd, bool bStripPath );
    void Init( const wchar_t* pszFilename, const unsigned char* data, ptrdiff_t size, bool bStripPath );
    PdfString CreateFileSpecification( const wchar_t* pszFilename ) const;
    void EmbeddFile( PdfObject* pStream, const wchar_t* pszFilename ) const;
    const wchar_t *MaybeStripPath( const wchar_t* pszFilename, bool bStripPath ) const;
#endif

    /* Petr P. Petrov 17 September 2009*/
    /** Embeds the file from memory
      */
    void EmbeddFileFromMem( PdfObject* pStream, const unsigned char* data, ptrdiff_t size ) const;

};

};

#endif // _PDF_FILE_SPEC_H_

