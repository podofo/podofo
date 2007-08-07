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

#include "PdfImage.h"

#include "PdfDocument.h"
#include "PdfStream.h"

#include <stdio.h>
#include <sstream>

#ifdef PODOFO_HAVE_TIFF_LIB
extern "C" {
#include "tiffio.h"
#ifdef WIN32		// Collision between tiff and jpeg-headers
#define XMD_H
#undef FAR
#endif
}
#endif // PODOFO_HAVE_TIFF_LIB

#ifdef PODOFO_HAVE_JPEG_LIB
extern "C" {
#include "jpeglib.h"
}
#endif // PODOFO_HAVE_JPEG_LIB


using namespace std;

namespace PoDoFo {

PdfImage::PdfImage( PdfVecObjects* pParent )
    : PdfXObject( "Image", pParent )
{
    m_rRect = PdfRect();

    this->SetImageColorSpace( ePdfColorSpace_DeviceRGB );
}

PdfImage::PdfImage( PdfDocument* pParent )
    : PdfXObject( "Image", pParent )
{
    m_rRect = PdfRect();

    this->SetImageColorSpace( ePdfColorSpace_DeviceRGB );
}

PdfImage::PdfImage( PdfObject* pObject )
    : PdfXObject( "Image", pObject )
{
    m_rRect.SetHeight( m_pObject->GetDictionary().GetKey( "Height" )->GetNumber() );
    m_rRect.SetWidth ( m_pObject->GetDictionary().GetKey( "Width" )->GetNumber() );
}

PdfImage::~PdfImage()
{

}

void PdfImage::SetImageColorSpace( EPdfColorSpace eColorSpace )
{
    m_pObject->GetDictionary().AddKey( "ColorSpace", PdfName( ColorspaceToName( eColorSpace ) ) );
}

void PdfImage::SetImageData( unsigned int nWidth, unsigned int nHeight, 
                             unsigned int nBitsPerComponent, PdfInputStream* pStream )
{
    TVecFilters vecFlate;
    vecFlate.push_back( ePdfFilter_FlateDecode );

    this->SetImageData( nWidth, nHeight, nBitsPerComponent, pStream, vecFlate );
}

void PdfImage::SetImageData( unsigned int nWidth, unsigned int nHeight, 
                             unsigned int nBitsPerComponent, PdfInputStream* pStream, 
                             const TVecFilters & vecFilters )
{
    m_rRect.SetWidth( nWidth );
    m_rRect.SetHeight( nHeight );

    m_pObject->GetDictionary().AddKey( "Width",  PdfVariant( static_cast<long>(nWidth) ) );
    m_pObject->GetDictionary().AddKey( "Height", PdfVariant( static_cast<long>(nHeight) ) );
    m_pObject->GetDictionary().AddKey( "BitsPerComponent", PdfVariant( static_cast<long>(nBitsPerComponent) ) );

    PdfVariant var;
    m_rRect.ToVariant( var );
    m_pObject->GetDictionary().AddKey( "BBox", var );

    m_pObject->GetStream()->Set( pStream, vecFilters );
}

void PdfImage::SetImageDataRaw( unsigned int nWidth, unsigned int nHeight, 
                                unsigned int nBitsPerComponent, PdfInputStream* pStream )
{
    m_rRect.SetWidth( nWidth );
    m_rRect.SetHeight( nHeight );

    m_pObject->GetDictionary().AddKey( "Width",  PdfVariant( static_cast<long>(nWidth) ) );
    m_pObject->GetDictionary().AddKey( "Height", PdfVariant( static_cast<long>(nHeight) ) );
    m_pObject->GetDictionary().AddKey( "BitsPerComponent", PdfVariant( static_cast<long>(nBitsPerComponent) ) );

    PdfVariant var;
    m_rRect.ToVariant( var );
    m_pObject->GetDictionary().AddKey( "BBox", var );

    m_pObject->GetStream()->SetRawData( pStream, -1 );
}

void PdfImage::LoadFromFile( const char* pszFilename )
{
    if( pszFilename && strlen( pszFilename ) > 3 )
    {
        const char* pszExtension = pszFilename + strlen( pszFilename ) - 3;

#ifdef PODOFO_HAVE_TIFF_LIB
#ifdef _MSC_VER
        if( _strnicmp( pszExtension, "tif", 3 ) == 0 || 
            _strnicmp( pszExtension, "iff", 3 ) == 0 ) // "tiff"
#else
        if( strncasecmp( pszExtension, "tif", 3 ) == 0 ||
            strncasecmp( pszExtension, "iff", 3 ) == 0 ) // "tiff"
#endif
        {
            LoadFromTiff( pszFilename );
            return;
		}
#endif

#ifdef PODOFO_HAVE_JPEG_LIB
#ifdef _MSC_VER
        if( _strnicmp( pszExtension, "jpg", 3 ) == 0 )
#else
		if( strncasecmp( pszExtension, "jpg", 3 ) == 0 )
#endif
        {
            LoadFromJpeg( pszFilename );
			return;
		}
#endif
	}
	PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
}

#ifdef PODOFO_HAVE_JPEG_LIB
void PdfImage::LoadFromJpeg( const char* pszFilename )
{
    FILE*                         hInfile;    
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr         jerr;

    if( !pszFilename )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    hInfile = fopen(pszFilename, "rb");
    if( !hInfile )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, pszFilename );
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, hInfile);

    if( jpeg_read_header(&cinfo, TRUE) <= 0 )
    {
        fclose( hInfile );
        (void) jpeg_destroy_decompress(&cinfo);

        PODOFO_RAISE_ERROR( ePdfError_UnexpectedEOF );
    }

    jpeg_start_decompress(&cinfo);
    fclose( hInfile );

    m_rRect.SetWidth( cinfo.output_width );
    m_rRect.SetHeight( cinfo.output_height );

    // I am not sure wether this switch is fully correct.
    // it should handle all cases though.
    // Index jpeg files might look strange as jpeglib+
    // returns 1 for them.
    switch( cinfo.output_components )
    {
        case 3:
            this->SetImageColorSpace( ePdfColorSpace_DeviceRGB );
            break;
        case 4:
            this->SetImageColorSpace( ePdfColorSpace_DeviceCMYK );
            break;
        default:
            this->SetImageColorSpace( ePdfColorSpace_DeviceGray );
            break;
    }

    PdfFileInputStream stream( pszFilename );
    // Set the filters key to DCTDecode
    m_pObject->GetDictionary().AddKey( PdfName::KeyFilter, PdfName( "DCTDecode" ) );
    // Do not apply any filters as JPEG data is already DCT encoded.
    this->SetImageDataRaw( cinfo.output_width, cinfo.output_height, 8, &stream );
    
    (void) jpeg_destroy_decompress(&cinfo);
}
#endif // PODOFO_HAVE_JPEG_LIB

#ifdef PODOFO_HAVE_TIFF_LIB
static void TIFFErrorWarningHandler(const char*, const char*, va_list)
{
    
}

void PdfImage::LoadFromTiff( const char* pszFilename )
{
    TIFFSetErrorHandler(TIFFErrorWarningHandler);
    TIFFSetWarningHandler(TIFFErrorWarningHandler);
    
    if( !pszFilename )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    TIFF* hInfile = TIFFOpen(pszFilename, "rb");

    if( !hInfile )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, pszFilename );
    }

    int32 row, width, height;
    uint16 samplesPerPixel, bitsPerSample;
    uint16* sampleInfo;
    uint16 extraSamples;
    uint16 planarConfig, photoMetric, orientation;
    int32 resolutionUnit;

    TIFFGetField(hInfile,	   TIFFTAG_IMAGEWIDTH,		&width);
    TIFFGetField(hInfile,	   TIFFTAG_IMAGELENGTH,		&height);
    TIFFGetFieldDefaulted(hInfile, TIFFTAG_BITSPERSAMPLE,	&bitsPerSample);
    TIFFGetFieldDefaulted(hInfile, TIFFTAG_SAMPLESPERPIXEL,     &samplesPerPixel);
    TIFFGetFieldDefaulted(hInfile, TIFFTAG_PLANARCONFIG,	&planarConfig);
    TIFFGetFieldDefaulted(hInfile, TIFFTAG_PHOTOMETRIC,		&photoMetric);
    TIFFGetFieldDefaulted(hInfile, TIFFTAG_EXTRASAMPLES,	&extraSamples, &sampleInfo);
    TIFFGetFieldDefaulted(hInfile, TIFFTAG_ORIENTATION,		&orientation);
	
    resolutionUnit = 0;
    float resX;
    float resY;
    TIFFGetFieldDefaulted(hInfile, TIFFTAG_XRESOLUTION,		&resX);
    TIFFGetFieldDefaulted(hInfile, TIFFTAG_YRESOLUTION,		&resY);
    TIFFGetFieldDefaulted(hInfile, TIFFTAG_RESOLUTIONUNIT,	&resolutionUnit);

    int colorChannels = samplesPerPixel - extraSamples;

    int bitsPixel = bitsPerSample * samplesPerPixel;

    // TODO: implement special cases
    if( TIFFIsTiled(hInfile) )
    {
        TIFFClose(hInfile);
        PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
    }
		
    if( bitsPixel != 8 && bitsPixel != 24 && bitsPixel != 32)
    {
        TIFFClose(hInfile);
        PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
    }

    if ( planarConfig != PLANARCONFIG_CONTIG && colorChannels != 1 )
    {
        TIFFClose(hInfile);
        PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
    }

    if ( photoMetric == PHOTOMETRIC_PALETTE )
    {
        TIFFClose(hInfile);
        PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
    }

    int32 scanlineSize = TIFFScanlineSize(hInfile);
    int32 bufferSize = scanlineSize * height;
    char *buffer = new char[bufferSize];
    PdfMemoryInputStream stream(buffer, bufferSize);

    switch(bitsPixel)
    {
        case 8:
            SetImageColorSpace(ePdfColorSpace_DeviceGray);
            break;
        case 24:
            SetImageColorSpace(ePdfColorSpace_DeviceRGB);
            break;
        case 32:
            SetImageColorSpace(ePdfColorSpace_DeviceCMYK);
            break;
        default:
            SetImageColorSpace(ePdfColorSpace_Unknown);
            break;
    }

    for(row = 0; row < height; row++)
    {
        if(TIFFReadScanline(hInfile,
                            &buffer[row * scanlineSize],
                            row) == (-1))
        {
            TIFFClose(hInfile);
            PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
        }
    }

    SetImageData(static_cast<unsigned int>(width), 
                 static_cast<unsigned int>(height),
                 static_cast<unsigned int>(bitsPerSample), 
                 &stream);

    delete buffer;

    TIFFClose(hInfile);
}
#endif // PODOFO_HAVE_TIFF_LIB

const char* PdfImage::ColorspaceToName( EPdfColorSpace eColorSpace )
{
    switch( eColorSpace )
    {
        case ePdfColorSpace_DeviceGray:
            return "DeviceGray";
        case ePdfColorSpace_DeviceRGB:
            return "DeviceRGB";
        case ePdfColorSpace_DeviceCMYK:
            return "DeviceCMYK";
        case ePdfColorSpace_Unknown:
        default:
            return NULL;
    }

    return NULL;
}

};
