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

#ifndef _PDF_FILE_SPEC_H_
#define _PDF_FILE_SPEC_H_

#include "PdfDefines.h"

#include "PdfElement.h"

#include "PdfString.h"

namespace PoDoFo {

class PdfStream;

/**
 *  A file specification is used in the PDF file to referr to another file.
 *  The other file can be a file outside of the PDF or can be embedded into
 *  the PDF file itself.
 */
class PdfFileSpec : public PdfElement {
 public:
    PdfFileSpec( const char* pszFilename, bool bEmbedd, PdfVecObjects* pParent );

    PdfFileSpec( PdfObject* pObject );

 private:

    /** Create a file specification string from a filename
     *  \param pszFilename filename 
     *  \returns a file specification string
     */
    PdfString CreateFileSpecification( const char* pszFilename ) const;

    /** Embedd a file into a stream object
     *  \param pStream write the file to this stream
     *  \param pszFilename the file to embedd
     */
    void EmbeddFile( PdfStream* pStream, const char* pszFilename ) const;
};

};

#endif // _PDF_FILE_SPEC_H_

