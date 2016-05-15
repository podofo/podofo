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

#ifndef _PDF_IMAGE_H_
#define _PDF_IMAGE_H_

#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfFilter.h"
#include "PdfXObject.h"

namespace PoDoFo {

class PdfArray;
class PdfDocument;
class PdfInputStream;
class PdfObject;
class PdfVecObjects;

/** A PdfImage object is needed when ever you want to embedd an image
 *  file into a PDF document.
 *  The PdfImage object is embedded once and can be drawn as often
 *  as you want on any page in the document using PdfPainter
 *
 *  \see GetImageReference
 *  \see PdfPainter::DrawImage
 *
 *  \see SetImageData
 */
class PODOFO_DOC_API PdfImage : public PdfXObject {
 public:
    /** Constuct a new PdfImage object
     *
     *  \param pParent parent vector of this image
	 *  \param pszPrefix optional prefix for XObject-name
     */
    PdfImage( PdfVecObjects* pParent, const char* pszPrefix = NULL );

    /** Constuct a new PdfImage object
     *  This is an overloaded constructor.
     *
     *  \param pParent parent document
	 *  \param pszPrefix optional prefix for XObject-name
     */
    PdfImage( PdfDocument* pParent, const char* pszPrefix = NULL );

    /** Construct an image from an existing PdfObject
     *  
     *  \param pObject a PdfObject that has to be an image
     */
    PdfImage( PdfObject* pObject );

    ~PdfImage();

    /**
     * Get a list of all image formats supported by this PoDoFo build.
     *
     * Example: { "JPEG", "TIFF", NULL }
     *
     * \returns a zero terminates list of all supported image formats
     */
    static const char** GetSupportedFormats();

    /** Set the color space of this image. The default value is
     *  ePdfColorSpace_DeviceRGB.
     *  \param eColorSpace one of ePdfColorSpace_DeviceGray, ePdfColorSpace_DeviceRGB and
     *                     ePdfColorSpace_DeviceCMYK, ePdfColorSpace_Indexed
     *  \param indexedData this parameter is required only for ePdfColorSpace_Indexed and
     *       it contains string with one number and then color palette, like "/DeviceRGB 15 <000000 00FF00...>"
     *       or the string array can be a resource name.
     *
     *  \see SetImageICCProfile to set an ICC profile instead of a simple colorspace
     */
    void SetImageColorSpace( EPdfColorSpace eColorSpace, const PdfArray *indexedData = NULL );

    /** Set an ICC profile for this image.
     *
     *  \param pStream an input stream from which the ICC profiles data can be read
     *  \param lColorComponents the number of colorcomponents of the ICC profile
     *  \param eAlternateColorSpace an alternate colorspace to use if the ICC profile cannot be used
     *
     *  \see SetImageColorSpace to set an colorspace instead of an ICC profile for this image
     */
    void SetImageICCProfile( PdfInputStream* pStream, long lColorComponents, 
                             EPdfColorSpace eAlternateColorSpace = ePdfColorSpace_DeviceRGB );

    //EPdfColorSpace GetImageColorSpace() const;

    /** Set a softmask for this image.
     *  \param pSoftmask a PdfImage pointer to the image, which is to be set as softmask, must be 8-Bit-Grayscale
     *
     */
    void SetImageSoftmask( const PdfImage* pSoftmask );

	/** Get the width of the image when drawn in PDF units
     *  \returns the width in PDF units
     */
    inline double GetWidth() const;

    /** Get the height of the image when drawn in PDF units
     *  \returns the height in PDF units
     */
    inline double GetHeight() const;

    /** Set the actual image data from an input stream
     *  
     *  The image data will be flate compressed.
     *  If you want no compression or another filter to be applied
     *  use the overload of SetImageData which takes a TVecFilters
     *  as argument.
     *  
     *  \param nWidth width of the image in pixels
     *  \param nHeight height of the image in pixels
     *  \param nBitsPerComponent bits per color component of the image (depends on the image colorspace you have set
     *                           but is 8 in most cases)
     *  \param pStream stream supplieding raw image data
     *
     *  \see SetImageData
     */
    void SetImageData( unsigned int nWidth, unsigned int nHeight, 
                       unsigned int nBitsPerComponent, PdfInputStream* pStream );

    /** Set the actual image data from an input stream
     *  
     *  \param nWidth width of the image in pixels
     *  \param nHeight height of the image in pixels
     *  \param nBitsPerComponent bits per color component of the image (depends on the image colorspace you have set
     *                           but is 8 in most cases)
     *  \param pStream stream supplieding raw image data
     *  \param vecFilters these filters will be applied to compress the image data
     */
    void SetImageData( unsigned int nWidth, unsigned int nHeight, 
                       unsigned int nBitsPerComponent, PdfInputStream* pStream, const TVecFilters & vecFilters );

    /** Set the actual image data from an input stream.
     *  The data has to be encoded already and an appropriate
     *  filters key entry has to be set manually before!
     *  
     *  \param nWidth width of the image in pixels
     *  \param nHeight height of the image in pixels
     *  \param nBitsPerComponent bits per color component of the image (depends on the image colorspace you have set
     *                           but is 8 in most cases)
     *  \param pStream stream supplieding raw image data
     */
    void SetImageDataRaw( unsigned int nWidth, unsigned int nHeight, 
                          unsigned int nBitsPerComponent, PdfInputStream* pStream );

    /** Load the image data from a file
     *  \param pszFilename
     */
    void LoadFromFile( const char* pszFilename );
    
    /** Load the image data from bytes
     *  \param pData bytes
     *  \param dwLen number of bytes
     */
    void LoadFromData(const unsigned char* pData, pdf_long dwLen);

#ifdef _WIN32
    /** Load the image data from a file
     *  \param pszFilename
     *
     *  This is an overloaded member function to allow working
     *  with unicode characters. On Unix systems you can also pass
     *  UTF-8 to the const char* overload.
     */
    void LoadFromFile( const wchar_t* pszFilename );
#endif // _WIN32

#ifdef PODOFO_HAVE_JPEG_LIB
    /** Load the image data from a JPEG file
     *  \param pszFilename
     */
    void LoadFromJpeg( const char* pszFilename );

    /** Load the image data from JPEG bytes
     *  \param pData JPEG bytes
     *  \param dwLen number of bytes
     */
    void LoadFromJpegData(const unsigned char* pData, pdf_long dwLen);

#ifdef _WIN32
    /** Load the image data from a JPEG file
     *  \param pszFilename
     *
     *  This is an overloaded member function to allow working
     *  with unicode characters. On Unix systems you can also pass
     *  UTF-8 to the const char* overload.
     */
    void LoadFromJpeg( const wchar_t* pszFilename );
#endif // _WIN32
#endif // PODOFO_HAVE_JPEG_LIB
#ifdef PODOFO_HAVE_TIFF_LIB
    /** Load the image data from a TIFF file
     *  \param pszFilename
     */
    void LoadFromTiff( const char* pszFilename );
    
    /** Load the image data from TIFF bytes
     *  \param pData TIFF bytes
     *  \param dwLen number of bytes
     */
    void LoadFromTiffData(const unsigned char* pData, pdf_long dwLen);

#ifdef _WIN32
    /** Load the image data from a TIFF file
     *  \param pszFilename
     *
     *  This is an overloaded member function to allow working
     *  with unicode characters. On Unix systems you can also pass
     *  UTF-8 to the const char* overload.
     */
    void LoadFromTiff( const wchar_t* pszFilename );
#endif // _WIN32
#endif // PODOFO_HAVE_TIFF_LIB
#ifdef PODOFO_HAVE_PNG_LIB
    /** Load the image data from a PNG file
     *  \param pszFilename
     */
    void LoadFromPng( const char* pszFilename );
    
    /** Load the image data from PNG bytes
     *  \param pData PNG bytes
     *  \param dwLen number of bytes
     */
    void LoadFromPngData(const unsigned char* pData, pdf_long dwLen);

#ifdef _WIN32
    /** Load the image data from a PNG file
     *  \param pszFilename
     *
     *  This is an overloaded member function to allow working
     *  with unicode characters. On Unix systems you can also pass
     *  UTF-8 to the const char* overload.
     */
    void LoadFromPng( const wchar_t* pszFilename );
#endif // _WIN32
#endif // PODOFO_HAVE_PNG_LIB

    /** Set an color/chroma-key mask on an image.
     *  The masked color will not be painted, i.e. masked as being transparent.
     *
     *  \param r red RGB value of color that should be masked
     *  \param g green RGB value of color that should be masked
     *  \param b blue RGB value of color that should be masked
     *  \param threshold colors are masked that are in the range [(r-threshold, r+threshold),(g-threshold, g+threshold),(b-threshold, b+threshold)]
     */
    void SetImageChromaKeyMask(pdf_int64 r, pdf_int64 g, pdf_int64 b, pdf_int64 threshold = 0);

    /**
     * Apply an interpolation to the image if the source resolution
     * is lower than the resolution of the output device.
     * Default is false.
     * \param bValue whether the image should be interpolated
     */
    void SetInterpolate(bool bValue);

 private:

    /** Converts a EPdfColorSpace enum to a name key which can be used in a
     *  PDF dictionary.
     *  \param eColorSpace a valid colorspace
     *  \returns a valid key for this colorspace.
     */
    static PdfName ColorspaceToName( EPdfColorSpace eColorSpace );

#ifdef PODOFO_HAVE_JPEG_LIB
	void LoadFromJpegHandle( PdfFileInputStream* pInStream );
#endif // PODOFO_HAVE_JPEG_LIB
#ifdef PODOFO_HAVE_TIFF_LIB
    void LoadFromTiffHandle( void* pInStream );
#endif // PODOFO_HAVE_TIFF_LIB
#ifdef PODOFO_HAVE_PNG_LIB
	void LoadFromPngHandle( PdfFileInputStream* pInStream );
#endif // PODOFO_HAVE_PNG_LIB
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
