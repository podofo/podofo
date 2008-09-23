/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _IMAGE_EXTRACTOR_H_
#define _IMAGE_EXTRACTOR_H_

#include <podofo.h>

using namespace PoDoFo;

#ifndef MAX_PATH
#define MAX_PATH 512
#endif // MAX_PATH

/** This class uses the PoDoFo lib to parse 
 *  a PDF file and to write all images it finds
 *  in this PDF document to a given directory.
 */
class ImageExtractor {
 public:
    ImageExtractor();
    virtual ~ImageExtractor();

    /**
     * \param pnNum pointer to an integer were 
     *        the number of processed images can be stored
     *        or null if you do not want this information.
     */
    void Init( const char* pszInput, const char* pszOutput, int* pnNum = NULL );

    /**
     * \returns the number of succesfully extracted images
     */
    inline int GetNumImagesExtracted() const;

 private:
    /** Extracts the image form the given PdfObject
     *  which has to be an XObject with Subtype "Image"
     *  \param pObject a handle to a PDF object
     *  \param bJpeg if true extract as a jpeg, otherwise create a ppm
     *  \returns ErrOk on success
     */
    void ExtractImage( PoDoFo::PdfObject* pObject, bool bJpeg );

    /** This function checks wether a file with the 
     *  given filename does exist.
     *  \returns true if the file exists otherwise false
     */
    bool    FileExists( const char* pszFilename );

 private:
    char*        m_pszOutputDirectory;
    unsigned int m_nSuccess;
    unsigned int m_nCount;

    char         m_szBuffer[MAX_PATH];
};

inline int ImageExtractor::GetNumImagesExtracted() const
{
    return m_nSuccess;
}

#endif // _IMAGE_EXTRACTOR_H_
