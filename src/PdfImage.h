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
#include "PdfObject.h"

namespace PoDoFo {

/** A image reference object.
 *  It is needed to draw a PdfImage object using a PdfPainter
 *  to a PdfPage.
 *  You can retrieve also the height and width in pixels of the image 
 *  from this object which is often needed for position calculations.
 *
 *  In most cases you do not have to initialize a PdfImageRef by hand
 *  but can get it from PdfImage.
 *
 *  \see PdfImage::GetImageReference
 */
class PdfImageRef {
 public:
    /** Create an empty PdfImageRef object
     */
    PdfImageRef();

    /** Create a copy of an existing PdfImageRef object
     *  \param rhs image reference to copy
     */
    PdfImageRef( const PdfImageRef & rhs );

    /** Set the identifier of this object.
     *  The identifier is used to draw the object on a page.
     *  \param rIdentifier the identifier
     */
    inline void SetIdentifier( const PdfName & rIdentifier );

    /** Set the object reference of this object (i.e. 3 0 R)
     *  \param rsRef reference to this object
     */
    inline void SetReference( const std::string & rsRef );

    /** Set the width in pixels
     *  \param nWidth width of the image in pixels
     */
    inline void SetWidth( unsigned int nWidth );

    /** Set the height in pixels
     *  \param nHeight height of the image in pixels
     */
    inline void SetHeight( unsigned int nHeight );

    /** Get the identifier used for drawing to this object.
     *  \returns a PdfName
     */
    inline const PdfName & Identifier() const;

    /** Get the reference
     *  \returns a zero terminated string pointer to the reference
     */
    inline const char*  Reference()  const;

    /** Get the width of the object in pixels
     *  \returns width in pixels
     */
    inline unsigned int Width()      const;

    /** Get the height of the object in pixels
     *  \returns height in pixels
     */
    inline unsigned int Height()     const;

    /** Assignment operator
     *  \param rhs object to copy
     */
    const PdfImageRef & operator=( const PdfImageRef & rhs );

 private:
    PdfName       m_Identifier;
    std::string   m_sReference;
    unsigned int  m_nWidth;
    unsigned int  m_nHeight;
};

void PdfImageRef::SetIdentifier( const PdfName & rIdentifier )
{
    m_Identifier = rIdentifier;
}

void PdfImageRef::SetReference( const std::string & rsRef )
{
    m_sReference = rsRef;
}

void PdfImageRef::SetWidth( unsigned int nWidth )
{
    m_nWidth = nWidth;
}

void PdfImageRef::SetHeight( unsigned int nHeight )
{
    m_nHeight = nHeight;
}

const PdfName & PdfImageRef::Identifier() const
{
    return m_Identifier;
} 

const char* PdfImageRef::Reference() const
{
    return m_sReference.c_str();
} 

unsigned int PdfImageRef::Width()  const
{
    return m_nWidth;
}
   
unsigned int PdfImageRef::Height() const
{
    return m_nHeight;
}

/** A PdfImage object is needed when ever you want to embedd an image
 *  file into a PDF document.
 *  The PdfImage object is embedded once and can be drawn as often
 *  as you want on any page in the document using a PdfImageRef object
 *  which has to be retrieved from the PdfImage object before drawing.
 *
 *  \see GetImageReference
 *  \see PdfPainter::DrawImage
 */
class PdfImage : public PdfObject {
 public:
    /** Constuct a new PdfImage object
     *  You will most likely use PdfSimpleWriter::CreateImage to 
     *  create a PdfImage object.
     *
     *  \param objectno object number of this PdfImage
     *  \param generationno generation number
     * 
     *  \see PdfSimpleWriter::CreateImage
     */
    PdfImage( unsigned int objectno, unsigned int generationno );
    ~PdfImage();

    /** Set the color space of this image. The default value is
     *  ePdfColorSpace_DeviceRGB.
     *  \param eColorSpace one of ePdfColorSpace_DeviceGray, ePdfColorSpace_DeviceRGB and
     *                     ePdfColorSpace_DeviceCMYK
     */
    void SetImageColorSpace( EPdfColorSpace eColorSpace );

    /** Set the actual image data which has to be JPEG encoded from a in memory buffer.
     *  \param nWidth width of the image in pixels
     *  \param nHeight height of the image in pixels
     *  \param nBitsPerComponent bits per color component of the image (depends on the image colorspace you have set
     *                           but is 8 in most cases)
     *  \param szBuffer the jpeg encoded image data
     *  \param lLen length the of the image data buffer.
     */
    void SetImageData( unsigned int nWidth, unsigned int nHeight, unsigned int nBitsPerComponent, char* szBuffer, long lLen );

    /** Load the image data from a JPEG file
     *  \param pszFilename
     *  \returns ErrOk on success.
     */
    PdfError LoadFromFile( const char* pszFilename );

    /** Get an image reference object to this image.
     *  You can pass this image reference object to the
     *  DrawImage method of PdfPainter.
     * 
     *  The PdfImageRef is very small in memory and contains
     *  all the information that is needed for drawing. I.e. 
     *  the large PdfImage object can be written to the file
     *  and delete from the memory and you can still draw the image.
     *  This saves memory and increases speed therefore.
     *  \param rRef the PdfImageRef object is initialized with the 
     *         necessary values and can be used for drawing later.
     */
    void GetImageReference( PdfImageRef & rRef );

 private:
    /** Converts a EPdfColorSpace enum to a name key which can be used in a
     *  PDF dictionary.
     *  \param eColorSpace a valid colorspace
     *  \returns a valid key for this colorspace.
     */
    static const char* ColorspaceToName( EPdfColorSpace eColorSpace );

    /** Width of the image data in pixels.
     */
    unsigned int  m_nWidth;

    /** Width of the image data in pixels.
     */
    unsigned int  m_nHeight;
};

};

#endif // _PDF_IMAGE_H_
