/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#ifndef _PDF_IMAGE_H_
#define _PDF_IMAGE_H_

#include "PdfDefines.h"
#include "PdfXObject.h"

namespace PoDoFo {

class PdfDocument;
class PdfObject;
class PdfVecObjects;

/** A PdfImage object is needed when ever you want to embedd an image
 *  file into a PDF document.
 *  The PdfImage object is embedded once and can be drawn as often
 *  as you want on any page in the document using a PdfImageRef object
 *  which has to be retrieved from the PdfImage object before drawing.
 *
 *  \see GetImageReference
 *  \see PdfPainter::DrawImage
 */
class PdfImage : public PdfXObject {
 public:
    /** Constuct a new PdfImage object
     *
     *  \param pParent parent vector of this image
     */
    PdfImage( PdfVecObjects* pParent );

    /** Constuct a new PdfImage object
     *  This is an overloaded constructor.
     *
     *  \param pParent parent document
     */
    PdfImage( PdfDocument* pParent );

    /** Construct an image from an existing PdfObject
     *  
     *  \param pObject a PdfObject that has to be an image
     */
    PdfImage( PdfObject* pObject );

    ~PdfImage();

    /** Set the color space of this image. The default value is
     *  ePdfColorSpace_DeviceRGB.
     *  \param eColorSpace one of ePdfColorSpace_DeviceGray, ePdfColorSpace_DeviceRGB and
     *                     ePdfColorSpace_DeviceCMYK
     */
    void SetImageColorSpace( EPdfColorSpace eColorSpace );

   /** Set the compression filter of this image. The default value is "" (uncompressed)
     *  \param inName one of "FlateDecode" (ZIP) or "DCTDecode" (JPEG) currently
     */
    void SetImageFilter( const PdfName & inName );

    /** Set the actual image data from an in memory buffer.
     *     IF you wish your data to compressed correctly, you MUST call 
     *     SetImageFilter before calling this method
     *  \param nWidth width of the image in pixels
     *  \param nHeight height of the image in pixels
     *  \param nBitsPerComponent bits per color component of the image (depends on the image colorspace you have set
     *                           but is 8 in most cases)
     *  \param szBuffer the EITHER jpeg encoded OR raw image data
     *  \param lLen length the of the image data buffer.
     *  \param bTakeOwnership if true the buffer will be owned by the PdfImage object
     */
    void SetImageData( unsigned int nWidth, unsigned int nHeight, unsigned int nBitsPerComponent, 
                       char* szBuffer, long lLen, bool bTakeOwnership = false );

    /** Load the image data from a JPEG file
     *  \param pszFilename
     */
    void LoadFromFile( const char* pszFilename );

    /** Get the width of the image when drawn in PDF units
     *  \returns the width in PDF units
     */
    inline double GetWidth() const;

    /** Get the height of the image when drawn in PDF units
     *  \returns the height in PDF units
     */
    inline double GetHeight() const;

 private:
    /** Converts a EPdfColorSpace enum to a name key which can be used in a
     *  PDF dictionary.
     *  \param eColorSpace a valid colorspace
     *  \returns a valid key for this colorspace.
     */
    static const char* ColorspaceToName( EPdfColorSpace eColorSpace );

 private:
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline double PdfImage::GetWidth() const
{
    return this->GetPageSize().GetWidth();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline double PdfImage::GetHeight() const
{
    return this->GetPageSize().GetHeight();
}

};

#endif // _PDF_IMAGE_H_
