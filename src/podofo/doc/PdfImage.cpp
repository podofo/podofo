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

#include "PdfImage.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfColor.h"
#include "base/PdfStream.h"
#include "base/PdfFiltersPrivate.h"

#include <stdio.h>
#include <wchar.h>
#include <sstream>

// TIFF and JPEG headers already included through "base/PdfFiltersPrivate.h",
// although in opposite order (first JPEG, then TIFF), if available of course

#ifdef PODOFO_HAVE_PNG_LIB
#include <png.h>
#endif /// PODOFO_HAVE_PNG_LIB

// <windows.h> defines an annoying GetObject macro that changes uses of GetObject to
// GetObjectA . Since macros aren't scope and namespace aware that breaks our code.
// Since we won't be using the Win32 resource manager API here, just undefine it.
#if defined(GetObject)
#undef GetObject
#endif

using namespace std;

namespace PoDoFo {

PdfImage::PdfImage( PdfVecObjects* pParent, const char* pszPrefix )
    : PdfXObject( "Image", pParent, pszPrefix )
{
    m_rRect = PdfRect();

    this->SetImageColorSpace( ePdfColorSpace_DeviceRGB );
}

PdfImage::PdfImage( PdfDocument* pParent, const char* pszPrefix )
    : PdfXObject( "Image", pParent, pszPrefix )
{
    m_rRect = PdfRect();

    this->SetImageColorSpace( ePdfColorSpace_DeviceRGB );
}

PdfImage::PdfImage( PdfObject* pObject )
    : PdfXObject( "Image", pObject )
{
    m_rRect.SetHeight( static_cast<double>(this->GetObject()->MustGetIndirectKey( "Height" )->GetNumber()) );
    m_rRect.SetWidth ( static_cast<double>(this->GetObject()->MustGetIndirectKey( "Width" )->GetNumber()) );
}

PdfImage::~PdfImage()
{

}

     /* Example: { "JPEG", "TIFF", NULL }
     *
     * \returns a zero terminates list of all supported image formats
     */
const char** PdfImage::GetSupportedFormats()
{
    static const char* ppszFormats[] = {
#ifdef PODOFO_HAVE_JPEG_LIB
        "JPEG",
#endif // PODOFO_HAVE_JPEG_LIB
#ifdef PODOFO_HAVE_PNG_LIB
        "PNG", 
#endif // PODOFO_HAVE_PNG_LIB
#ifdef PODOFO_HAVE_TIFF_LIB
        "TIFF", 
#endif // PODOFO_HAVE_TIFF_LIB
        NULL
    };

    return ppszFormats;
}

void PdfImage::SetImageColorSpace( EPdfColorSpace eColorSpace, const PdfArray *indexedData )
{
    if (eColorSpace == ePdfColorSpace_Indexed) {
        PODOFO_RAISE_LOGIC_IF( !indexedData, "PdfImage::SetImageColorSpace: indexedData cannot be NULL for Indexed color space." );

        PdfArray array(*indexedData);

        array.insert(array.begin(), ColorspaceToName( eColorSpace ));
        this->GetObject()->GetDictionary().AddKey( PdfName("ColorSpace"), array );
    } else {
        this->GetObject()->GetDictionary().AddKey( PdfName("ColorSpace"), ColorspaceToName( eColorSpace ) );
    }
}

void PdfImage::SetImageICCProfile( PdfInputStream* pStream, long lColorComponents, EPdfColorSpace eAlternateColorSpace ) 
{
    // Check lColorComponents for a valid value
    if( lColorComponents != 1 &&
        lColorComponents != 3 &&  
        lColorComponents != 4 )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_ValueOutOfRange, "SetImageICCProfile lColorComponents must be 1,3 or 4!" );
    }

    // Create a colorspace object
    PdfObject* pIccObject = this->GetObject()->GetOwner()->CreateObject();
    pIccObject->GetDictionary().AddKey( PdfName("Alternate"), ColorspaceToName( eAlternateColorSpace ) ); 
    pIccObject->GetDictionary().AddKey( PdfName("N"), static_cast<pdf_int64>(lColorComponents) );
    pIccObject->GetStream()->Set( pStream );
    
    // Add the colorspace to our image
    PdfArray array;
    array.push_back( PdfName("ICCBased") );
    array.push_back( pIccObject->Reference() );
    this->GetObject()->GetDictionary().AddKey( PdfName("ColorSpace"), array );
}

void PdfImage::SetImageSoftmask( const PdfImage* pSoftmask )
{
	GetObject()->GetDictionary().AddKey( "SMask", pSoftmask->GetObject()->Reference() );
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

    this->GetObject()->GetDictionary().AddKey( "Width",  PdfVariant( static_cast<pdf_int64>(nWidth) ) );
    this->GetObject()->GetDictionary().AddKey( "Height", PdfVariant( static_cast<pdf_int64>(nHeight) ) );
    this->GetObject()->GetDictionary().AddKey( "BitsPerComponent", PdfVariant( static_cast<pdf_int64>(nBitsPerComponent) ) );

    PdfVariant var;
    m_rRect.ToVariant( var );
    this->GetObject()->GetDictionary().AddKey( "BBox", var );

    this->GetObject()->GetStream()->Set( pStream, vecFilters );
}

void PdfImage::SetImageDataRaw( unsigned int nWidth, unsigned int nHeight, 
                                unsigned int nBitsPerComponent, PdfInputStream* pStream )
{
    m_rRect.SetWidth( nWidth );
    m_rRect.SetHeight( nHeight );

    this->GetObject()->GetDictionary().AddKey( "Width",  PdfVariant( static_cast<pdf_int64>(nWidth) ) );
    this->GetObject()->GetDictionary().AddKey( "Height", PdfVariant( static_cast<pdf_int64>(nHeight) ) );
    this->GetObject()->GetDictionary().AddKey( "BitsPerComponent", PdfVariant( static_cast<pdf_int64>(nBitsPerComponent) ) );

    PdfVariant var;
    m_rRect.ToVariant( var );
    this->GetObject()->GetDictionary().AddKey( "BBox", var );

    this->GetObject()->GetStream()->SetRawData( pStream, -1 );
}

void PdfImage::LoadFromFile( const char* pszFilename )
{
    if( pszFilename && strlen( pszFilename ) > 3 )
    {
        const char* pszExtension = pszFilename + strlen( pszFilename ) - 3;

#ifdef PODOFO_HAVE_TIFF_LIB
        if( PoDoFo::compat::strncasecmp( pszExtension, "tif", 3 ) == 0 ||
            PoDoFo::compat::strncasecmp( pszExtension, "iff", 3 ) == 0 ) // "tiff"
        {
            LoadFromTiff( pszFilename );
            return;
        }
#endif

#ifdef PODOFO_HAVE_JPEG_LIB
        if( PoDoFo::compat::strncasecmp( pszExtension, "jpg", 3 ) == 0 )
        {
            LoadFromJpeg( pszFilename );
            return;
        }
#endif

#ifdef PODOFO_HAVE_PNG_LIB
        if( PoDoFo::compat::strncasecmp( pszExtension, "png", 3 ) == 0 )
        {
            LoadFromPng( pszFilename );
            return;
        }
#endif

	}
	PODOFO_RAISE_ERROR_INFO( ePdfError_UnsupportedImageFormat, pszFilename );
}
    
#ifdef _WIN32
void PdfImage::LoadFromFile( const wchar_t* pszFilename )
{
    if( pszFilename && wcslen( pszFilename ) > 3 )
    {
        const wchar_t* pszExtension = pszFilename + wcslen( pszFilename ) - 3;

#ifdef PODOFO_HAVE_TIFF_LIB
#if TIFFLIB_VERSION >= 20120922		// TiffOpenW needs at least version 4.0.3
		if( _wcsnicmp( pszExtension, L"tif", 3 ) == 0 ||
            _wcsnicmp( pszExtension, L"iff", 3 ) == 0 ) // "tiff"
        {
            LoadFromTiff( pszFilename );
            return;
        }
#endif
#endif

#ifdef PODOFO_HAVE_JPEG_LIB
        if( _wcsnicmp( pszExtension, L"jpg", 3 ) == 0 )
        {
            LoadFromJpeg( pszFilename );
            return;
        }
#endif

#ifdef PODOFO_HAVE_PNG_LIB
        if( _wcsnicmp( pszExtension, L"png", 3 ) == 0 )
        {
            LoadFromPng( pszFilename );
            return;
        }
#endif
	}

	PdfError e( ePdfError_UnsupportedImageFormat, __FILE__, __LINE__ );
    e.SetErrorInformation( pszFilename );
    throw e;
}
#endif // _WIN32

void PdfImage::LoadFromData(const unsigned char* pData, pdf_long dwLen)
{
    if (dwLen > 4) {
        unsigned char magic[4];
        memcpy(magic, pData, 4);

#ifdef PODOFO_HAVE_TIFF_LIB
        if((magic[0] == 0x4d &&
            magic[1] == 0x4d &&
            magic[2] == 0x00 &&
            magic[3] == 0x2a) ||
           (magic[0] == 0x49 &&
            magic[1] == 0x49 &&
            magic[2] == 0x2a &&
            magic[3] == 0x00))
        {
            LoadFromTiffData(pData, dwLen);
            return;
        }
#endif
        
#ifdef PODOFO_HAVE_JPEG_LIB
        if( magic[0] == 0xff &&
            magic[1] == 0xd8 )
        {
            LoadFromJpegData(pData, dwLen);
            return;
        }
#endif
        
#ifdef PODOFO_HAVE_PNG_LIB
        if( magic[0] == 0x89 &&
            magic[1] == 0x50 &&
            magic[2] == 0x4e &&
            magic[3] == 0x47 )
        {
            LoadFromPngData(pData, dwLen);
            return;
        }
#endif
        
    }
    PODOFO_RAISE_ERROR_INFO( ePdfError_UnsupportedImageFormat, "Unknown magic number" );
}

#ifdef PODOFO_HAVE_JPEG_LIB

void PdfImage::LoadFromJpeg( const char* pszFilename )
    {
    /* Constructor will throw exception */
    PdfFileInputStream stream( pszFilename );
    LoadFromJpegHandle( &stream );
}

#ifdef _WIN32
void PdfImage::LoadFromJpeg( const wchar_t* pszFilename )
{
    PdfFileInputStream stream( pszFilename );
    LoadFromJpegHandle( &stream );
}
#endif // _WIN32

void PdfImage::LoadFromJpegHandle( PdfFileInputStream* pInStream )
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr         jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = &JPegErrorExit;
    jerr.emit_message = &JPegErrorOutput;

    jpeg_create_decompress(&cinfo);

    jpeg_stdio_src(&cinfo, pInStream->GetHandle());

    if( jpeg_read_header(&cinfo, TRUE) <= 0 )
    {
        (void) jpeg_destroy_decompress(&cinfo);

        PODOFO_RAISE_ERROR( ePdfError_UnexpectedEOF );
    }

    jpeg_start_decompress(&cinfo);

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
	{
	    this->SetImageColorSpace( ePdfColorSpace_DeviceCMYK );
	    // The jpeg-doc ist not specific in this point, but cmyk's seem to be stored
	    // in a inverted fashion. Fix by attaching a decode array
	    PdfArray decode;
	    decode.push_back( 1.0 );
	    decode.push_back( 0.0 );
	    decode.push_back( 1.0 );
	    decode.push_back( 0.0 );
	    decode.push_back( 1.0 );
	    decode.push_back( 0.0 );
	    decode.push_back( 1.0 );
	    decode.push_back( 0.0 );
	    
	    this->GetObject()->GetDictionary().AddKey( PdfName("Decode"), decode );
	}
	break;
        default:
            this->SetImageColorSpace( ePdfColorSpace_DeviceGray );
            break;
    }
    
    // Set the filters key to DCTDecode
    this->GetObject()->GetDictionary().AddKey( PdfName::KeyFilter, PdfName( "DCTDecode" ) );
    // Do not apply any filters as JPEG data is already DCT encoded.
    fseeko( pInStream->GetHandle(), 0L, SEEK_SET );
    this->SetImageDataRaw( cinfo.output_width, cinfo.output_height, 8, pInStream );
    
    (void) jpeg_destroy_decompress(&cinfo);
}

void PdfImage::LoadFromJpegData(const unsigned char* pData, pdf_long dwLen)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr         jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = &JPegErrorExit;
    jerr.emit_message = &JPegErrorOutput;

    jpeg_create_decompress(&cinfo);

    jpeg_memory_src(&cinfo, pData, dwLen);

    if( jpeg_read_header(&cinfo, TRUE) <= 0 )
    {
        (void) jpeg_destroy_decompress(&cinfo);

        PODOFO_RAISE_ERROR( ePdfError_UnexpectedEOF );
    }

    jpeg_start_decompress(&cinfo);

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
	{
	    this->SetImageColorSpace( ePdfColorSpace_DeviceCMYK );
	    // The jpeg-doc ist not specific in this point, but cmyk's seem to be stored
	    // in a inverted fashion. Fix by attaching a decode array
	    PdfArray decode;
	    decode.push_back( 1.0 );
	    decode.push_back( 0.0 );
	    decode.push_back( 1.0 );
	    decode.push_back( 0.0 );
	    decode.push_back( 1.0 );
	    decode.push_back( 0.0 );
	    decode.push_back( 1.0 );
	    decode.push_back( 0.0 );
	    
	    this->GetObject()->GetDictionary().AddKey( PdfName("Decode"), decode );
	}
	break;
        default:
            this->SetImageColorSpace( ePdfColorSpace_DeviceGray );
            break;
    }
    
    // Set the filters key to DCTDecode
    this->GetObject()->GetDictionary().AddKey( PdfName::KeyFilter, PdfName( "DCTDecode" ) );
    
    PdfMemoryInputStream fInpStream( (const char*)pData, (pdf_long) dwLen);
    this->SetImageDataRaw( cinfo.output_width, cinfo.output_height, 8, &fInpStream );
    
    (void) jpeg_destroy_decompress(&cinfo);
}
#endif // PODOFO_HAVE_JPEG_LIB

#ifdef PODOFO_HAVE_TIFF_LIB
static void TIFFErrorWarningHandler(const char*, const char*, va_list)
{
    
}

void PdfImage::LoadFromTiffHandle(void* hInHandle) {
    
    TIFF* hInTiffHandle = (TIFF*)hInHandle;
    
    int32 row, width, height;
    uint16 samplesPerPixel, bitsPerSample;
    uint16* sampleInfo;
    uint16 extraSamples;
    uint16 planarConfig, photoMetric, orientation;
    int32 resolutionUnit;
    
    TIFFGetField(hInTiffHandle,	   TIFFTAG_IMAGEWIDTH,		&width);
    TIFFGetField(hInTiffHandle,	   TIFFTAG_IMAGELENGTH,		&height);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_BITSPERSAMPLE,	&bitsPerSample);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_SAMPLESPERPIXEL,     &samplesPerPixel);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_PLANARCONFIG,	&planarConfig);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_PHOTOMETRIC,		&photoMetric);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_EXTRASAMPLES,	&extraSamples, &sampleInfo);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_ORIENTATION,		&orientation);
    
    resolutionUnit = 0;
    float resX;
    float resY;
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_XRESOLUTION,		&resX);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_YRESOLUTION,		&resY);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_RESOLUTIONUNIT,	&resolutionUnit);
    
    int colorChannels = samplesPerPixel - extraSamples;
    
    int bitsPixel = bitsPerSample * samplesPerPixel;
    
    // TODO: implement special cases
    if( TIFFIsTiled(hInTiffHandle) )
    {
        TIFFClose(hInTiffHandle);
        PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
    }
    
    if ( planarConfig != PLANARCONFIG_CONTIG && colorChannels != 1 )
    {
        TIFFClose(hInTiffHandle);
        PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
    }
    
    if ( orientation != ORIENTATION_TOPLEFT )
    {
        TIFFClose(hInTiffHandle);
        PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
    }
    
    switch(photoMetric)
    {
        case PHOTOMETRIC_MINISBLACK:
        {
            if( bitsPixel == 1 )
            {
                PdfArray decode;
                decode.insert( decode.end(), PdfVariant( static_cast<pdf_int64>(0) ) );
                decode.insert( decode.end(), PdfVariant( static_cast<pdf_int64>(1) ) );
                this->GetObject()->GetDictionary().AddKey( PdfName("Decode"), decode );
                this->GetObject()->GetDictionary().AddKey( PdfName("ImageMask"), PdfVariant( true ) );
                this->GetObject()->GetDictionary().RemoveKey( PdfName("ColorSpace") );
            }
            else if ( bitsPixel == 8  ||  bitsPixel == 16)
                SetImageColorSpace(ePdfColorSpace_DeviceGray);
            else
            {
                TIFFClose(hInTiffHandle);
                PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
            }
        }
            break;
            
        case PHOTOMETRIC_MINISWHITE:
        {
            if( bitsPixel == 1 )
            {
                PdfArray decode;
                decode.insert( decode.end(), PdfVariant( static_cast<pdf_int64>(1) ) );
                decode.insert( decode.end(), PdfVariant( static_cast<pdf_int64>(0) ) );
                this->GetObject()->GetDictionary().AddKey( PdfName("Decode"), decode );
                this->GetObject()->GetDictionary().AddKey( PdfName("ImageMask"), PdfVariant( true ) );
                this->GetObject()->GetDictionary().RemoveKey( PdfName("ColorSpace") );
            }
            else if ( bitsPixel == 8  ||  bitsPixel == 16)
                SetImageColorSpace(ePdfColorSpace_DeviceGray);
            else
            {
                TIFFClose(hInTiffHandle);
                PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
            }
        }
            break;
            
        case PHOTOMETRIC_RGB:
            if ( bitsPixel != 24 )
            {
                TIFFClose(hInTiffHandle);
                PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
            }
            SetImageColorSpace(ePdfColorSpace_DeviceRGB);
            break;
            
        case PHOTOMETRIC_SEPARATED:
            if( bitsPixel != 32)
            {
                TIFFClose(hInTiffHandle);
                PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
            }
            SetImageColorSpace(ePdfColorSpace_DeviceCMYK);
            break;
            
        case PHOTOMETRIC_PALETTE:
        {
            int numColors = (1 << bitsPixel);
            
            PdfArray decode;
            decode.insert( decode.end(), PdfVariant( static_cast<pdf_int64>(0) ) );
            decode.insert( decode.end(), PdfVariant( static_cast<pdf_int64>(numColors-1) ) );
            this->GetObject()->GetDictionary().AddKey( PdfName("Decode"), decode );
            
            uint16 * rgbRed;
            uint16 * rgbGreen;
            uint16 * rgbBlue;
            TIFFGetField(hInTiffHandle, TIFFTAG_COLORMAP, &rgbRed, &rgbGreen, &rgbBlue);
            
            char *datap = new char[numColors*3];
            
            for ( int clr = 0; clr < numColors; clr++ )
            {
                datap[3*clr+0] = rgbRed[clr]/257;
                datap[3*clr+1] = rgbGreen[clr]/257;
                datap[3*clr+2] = rgbBlue[clr]/257;
            }
            PdfMemoryInputStream stream( datap, numColors*3 );
            
            // Create a colorspace object
            PdfObject* pIdxObject = this->GetObject()->GetOwner()->CreateObject();
            pIdxObject->GetStream()->Set( &stream );
            
            // Add the colorspace to our image
            PdfArray array;
            array.push_back( PdfName("Indexed") );
            array.push_back( PdfName("DeviceRGB") );
            array.push_back( static_cast<pdf_int64>(numColors-1) );
            array.push_back( pIdxObject->Reference() );
            this->GetObject()->GetDictionary().AddKey( PdfName("ColorSpace"), array );
            
            delete[] datap;
        }
            break;
            
        default:
            TIFFClose(hInTiffHandle);
            PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
            break;
    }
    
    int32 scanlineSize = TIFFScanlineSize(hInTiffHandle);
    long bufferSize = scanlineSize * height;
    char *buffer = new char[bufferSize];
    if( !buffer )
    {
        TIFFClose(hInTiffHandle);
        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
    }
    
    for(row = 0; row < height; row++)
    {
        if(TIFFReadScanline(hInTiffHandle,
                            &buffer[row * scanlineSize],
                            row) == (-1))
        {
            TIFFClose(hInTiffHandle);
            PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
        }
    }
    
    PdfMemoryInputStream stream(buffer, bufferSize);
    
    SetImageData(static_cast<unsigned int>(width),
                 static_cast<unsigned int>(height),
                 static_cast<unsigned int>(bitsPerSample),
                 &stream);
    
    delete[] buffer;
    
    TIFFClose(hInTiffHandle);
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
    
    LoadFromTiffHandle(hInfile);
}

#ifdef _WIN32
void PdfImage::LoadFromTiff( const wchar_t* pszFilename )
{
#if TIFFLIB_VERSION >= 20120922		// TiffOpenW needs at least version 4.0.3
    TIFFSetErrorHandler(TIFFErrorWarningHandler);
    TIFFSetWarningHandler(TIFFErrorWarningHandler);
    
    if( !pszFilename )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    TIFF* hInfile = TIFFOpenW(pszFilename, "rb");

    if( !hInfile )
    {
		PdfError e( ePdfError_FileNotFound, __FILE__, __LINE__ );
        e.SetErrorInformation( pszFilename );
        throw e;
    }

    LoadFromTiffHandle(hInfile);
#else
    PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
#endif // TIFFLIB_VERSION
}
#endif // _WIN32

struct tiffData
{
    tiffData(const unsigned char* data, tsize_t size):_data(data), _pos(0), _size(size) {}
    
    tsize_t read(tdata_t data, tsize_t length)
    {
        tsize_t bytesRead = 0;
        if (length > _size - static_cast<tsize_t>(_pos))
        {
            memcpy(data, &_data[_pos], _size - _pos);
            bytesRead = _size - _pos;
            _pos = _size;
        }
        else
        {
            memcpy(data, &_data[_pos], length);
            bytesRead = length;
            _pos += length;
        }
        return bytesRead;
    }
    
    toff_t size()
    {
        return _size;
    }
    
    toff_t seek(toff_t pos, int whence)
    {
        if (pos == 0xFFFFFFFF) {
            return 0xFFFFFFFF;
        }
        switch(whence)
        {
            case SEEK_SET:
                if (static_cast<tsize_t>(pos) > _size)
                {
                    _pos = _size;
                }
                else
                {
                    _pos = pos;
                }
                break;
            case SEEK_CUR:
                if (static_cast<tsize_t>(pos + _pos) > _size)
                {
                    _pos = _size;
                }
                else
                {
                    _pos += pos;
                }
                break;
            case SEEK_END:
                if (static_cast<tsize_t>(pos) > _size)
                {
                    _pos = 0;
                }
                else
                {
                    _pos = _size - pos;
                }
                break;
        }
        return _pos;
    }
    
private:
    const unsigned char* _data;
    toff_t _pos;
    tsize_t _size;
};
tsize_t tiff_Read(thandle_t st, tdata_t buffer, tsize_t size)
{
    tiffData* data = (tiffData*)st;
    return data->read(buffer, size);
};
tsize_t tiff_Write(thandle_t /*st*/, tdata_t /*buffer*/, tsize_t /*size*/)
{
    return 0;
};
int tiff_Close(thandle_t)
{
    return 0;
};
toff_t tiff_Seek(thandle_t st, toff_t pos, int whence)
{
    tiffData* data = (tiffData*)st;
    return data->seek(pos, whence);
};
toff_t tiff_Size(thandle_t st)
{
    tiffData* data = (tiffData*)st;
    return data->size();
};
int tiff_Map(thandle_t, tdata_t*, toff_t*)
{
    return 0;
};
void tiff_Unmap(thandle_t, tdata_t, toff_t)
{
    return;
};
void PdfImage::LoadFromTiffData(const unsigned char* pData, pdf_long dwLen)
{
    TIFFSetErrorHandler(TIFFErrorWarningHandler);
    TIFFSetWarningHandler(TIFFErrorWarningHandler);
    
    if( !pData )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    tiffData data(pData, dwLen);
    TIFF* hInHandle = TIFFClientOpen("Memory", "r", (thandle_t)&data,
                                     tiff_Read, tiff_Write, tiff_Seek, tiff_Close, tiff_Size,
                                     tiff_Map, tiff_Unmap);
    if( !hInHandle )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    LoadFromTiffHandle(hInHandle);
}
#endif // PODOFO_HAVE_TIFF_LIB
#ifdef PODOFO_HAVE_PNG_LIB
void PdfImage::LoadFromPng( const char* pszFilename )
{
    PdfFileInputStream stream( pszFilename );
    LoadFromPngHandle( &stream );
}

#ifdef _WIN32
void PdfImage::LoadFromPng( const wchar_t* pszFilename )
{
    PdfFileInputStream stream( pszFilename );
    LoadFromPngHandle( &stream );
}
#endif // _WIN32

static void LoadFromPngContent(png_structp pPng, png_infop pInfo, PdfImage *image)
{
    png_set_sig_bytes(pPng, 8);
    png_read_info(pPng, pInfo);

// Begin
    png_uint_32 width;
    png_uint_32 height;
    int depth;
    int color_type;
    int interlace;

    png_get_IHDR (pPng, pInfo,
                  &width, &height, &depth,
                  &color_type, &interlace, NULL, NULL);

    /* convert palette/gray image to rgb */
    /* expand gray bit depth if needed */
    if (color_type == PNG_COLOR_TYPE_GRAY) {
#if PNG_LIBPNG_VER >= 10209
        png_set_expand_gray_1_2_4_to_8 (pPng);
#else
        png_set_gray_1_2_4_to_8 (pPng);
#endif
    } else if (color_type != PNG_COLOR_TYPE_PALETTE && depth < 8) {
        png_set_packing(pPng);
    }

    /* transform transparency to alpha */
    if (color_type != PNG_COLOR_TYPE_PALETTE && png_get_valid (pPng, pInfo, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha (pPng);

    if (depth == 16)
        png_set_strip_16(pPng);

    if (interlace != PNG_INTERLACE_NONE)
        png_set_interlace_handling(pPng);

    //png_set_filler (pPng, 0xff, PNG_FILLER_AFTER);

    /* recheck header after setting EXPAND options */
    png_read_update_info(pPng, pInfo);
    png_get_IHDR (pPng, pInfo,
                  &width, &height, &depth,
                  &color_type, &interlace, NULL, NULL);
// End //
    
    // Read the file
    if( setjmp(png_jmpbuf(pPng)) ) 
    {
        png_destroy_read_struct(&pPng, &pInfo, (png_infopp)NULL);
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    size_t lRowLen = png_get_rowbytes(pPng, pInfo);
    size_t lLen = lRowLen * height;
    char* pBuffer = static_cast<char*>(podofo_calloc(lLen, sizeof(char)));
 	if (!pBuffer)
 	{
 		PODOFO_RAISE_ERROR(ePdfError_OutOfMemory);
 	}

    png_bytepp pRows = static_cast<png_bytepp>(podofo_calloc(height, sizeof(png_bytep)));
 	if (!pRows)
 	{
 		PODOFO_RAISE_ERROR(ePdfError_OutOfMemory);
    }

    for(unsigned int y=0; y<height; y++)
    {
        pRows[y] = reinterpret_cast<png_bytep>(pBuffer + y * lRowLen);
    }

    png_read_image(pPng, pRows);

    png_bytep paletteTrans;
    int numTransColors;
    if (color_type & PNG_COLOR_MASK_ALPHA ||
        (color_type == PNG_COLOR_TYPE_PALETTE && png_get_valid(pPng, pInfo, PNG_INFO_tRNS) && png_get_tRNS(pPng, pInfo, &paletteTrans, &numTransColors, NULL)))
    {
        // Handle alpha channel and create smask
        char *smask = static_cast<char*>(podofo_calloc(height, width));
        png_uint_32 smaskIndex = 0;
        if (color_type == PNG_COLOR_TYPE_PALETTE) {
            for (png_uint_32 r = 0; r < height; r++) {
                png_bytep row = pRows[r];
                for (png_uint_32 c = 0; c < width; c++) {
                    png_byte color;
                    if (depth == 8) {
                        color = row[c];
                    } else if (depth == 4) {
                        color = c % 2 ? row[c / 2] >> 4 : row[c / 2] & 0xF;
                    } else if (depth == 2) {
                        color = (row[c / 4] >> c % 4 * 2) & 3;
                    } else if (depth == 1) {
                        color = (row[c / 4] >> c % 8) & 1;
                    }
                    smask[smaskIndex++] = color < numTransColors ? paletteTrans[color] : 0xFF;
                }
            }
        } else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
            for (png_uint_32 r = 0; r < height; r++) {
                png_bytep row = pRows[r];
                for (png_uint_32 c = 0; c < width; c++) {
                    memmove(pBuffer + 3 * smaskIndex, row + 4 * c, 3); // 3 byte for rgb
                    smask[smaskIndex++] = row[c * 4 + 3]; // 4th byte for alpha
                }
            }
            lLen = 3 * width * height;
        } else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
            for (png_uint_32 r = 0; r < height; r++) {
                png_bytep row = pRows[r];
                for (png_uint_32 c = 0; c < width; c++) {
                    pBuffer[smaskIndex] = row[c * 2]; // 1 byte for gray
                    smask[smaskIndex++] = row[c * 2 + 1]; // 2nd byte for alpha
                }
            }
            lLen = width * height;
        }
        PdfMemoryInputStream smaskstream(smask, width * height);
        PdfImage smakeImage(image->GetObject()->GetOwner());
        smakeImage.SetImageColorSpace(ePdfColorSpace_DeviceGray);
        smakeImage.SetImageData(width, height, 8, &smaskstream);
        image->SetImageSoftmask(&smakeImage);
        podofo_free(smask);
    }

    // Set color space
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_color *pColors;
        int numColors;
        png_get_PLTE(pPng, pInfo, &pColors, &numColors);

        char *datap = new char[numColors * 3];
        for (int i = 0; i < numColors; i++, pColors++)
        {
            datap[3 * i + 0] = pColors->red;
            datap[3 * i + 1] = pColors->green;
            datap[3 * i + 2] = pColors->blue;
        }
        PdfMemoryInputStream stream(datap, numColors * 3);
        PdfObject* pIdxObject = image->GetObject()->GetOwner()->CreateObject();
        pIdxObject->GetStream()->Set(&stream);

        PdfArray array;
        array.push_back(PdfName("DeviceRGB"));
        array.push_back(static_cast<pdf_int64>(numColors - 1));
        array.push_back(pIdxObject->Reference());
        image->SetImageColorSpace(ePdfColorSpace_Indexed, &array);
    } else if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        image->SetImageColorSpace(ePdfColorSpace_DeviceGray);
    } else {
        image->SetImageColorSpace(ePdfColorSpace_DeviceRGB);
    }

    // Set the image data and flate compress it
    PdfMemoryInputStream stream( pBuffer, lLen );
    image->SetImageData( width, height, depth, &stream );
    
    podofo_free(pBuffer);
    podofo_free(pRows);
    
    png_destroy_read_struct(&pPng, &pInfo, (png_infopp)NULL);
}

void PdfImage::LoadFromPngHandle( PdfFileInputStream* pInStream ) 
{
    FILE* hFile = pInStream->GetHandle();
    png_byte header[8];
    if( fread( header, 1, 8, hFile ) != 8 ||
        png_sig_cmp( header, 0, 8 ) )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_UnsupportedImageFormat, "The file could not be recognized as a PNG file." );
    }
    
    png_structp pPng = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if( !pPng )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    png_infop pInfo = png_create_info_struct(pPng);
    if( !pInfo )
    {
        png_destroy_read_struct(&pPng, (png_infopp)NULL, (png_infopp)NULL);
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( setjmp(png_jmpbuf(pPng)) )
    {
        png_destroy_read_struct(&pPng, &pInfo, (png_infopp)NULL);
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    png_init_io(pPng, hFile);

    LoadFromPngContent(pPng, pInfo, this);
}

struct pngData
{
    pngData(const unsigned char* data, png_size_t size):_data(data), _pos(0), _size(size) {}
    
    void read(png_bytep data, png_size_t length)
    {
        if (length > _size - _pos)
        {
            memcpy(data, &_data[_pos], _size - _pos);
            _pos = _size;
        }
        else
        {
            memcpy(data, &_data[_pos], length);
            _pos += length;
        }
    }
    
    private:
    const unsigned char* _data;
    png_size_t _pos;
    png_size_t _size;
};
void pngReadData(png_structp pngPtr, png_bytep data, png_size_t length)
{
    pngData* a = (pngData*)png_get_io_ptr(pngPtr);
    a->read(data, length);
}

void PdfImage::LoadFromPngData(const unsigned char* pData, pdf_long dwLen)
{
    if( !pData )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    pngData data(pData, dwLen);
    png_byte header[8];
    data.read(header, 8);
    if( png_sig_cmp(header, 0, 8) )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_UnsupportedImageFormat, "The file could not be recognized as a PNG file." );
    }
    
    png_structp pPng = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if( !pPng )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    png_infop pInfo = png_create_info_struct(pPng);
    if( !pInfo )
    {
        png_destroy_read_struct(&pPng, (png_infopp)NULL, (png_infopp)NULL);
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    if( setjmp(png_jmpbuf(pPng)) )
    {
        png_destroy_read_struct(&pPng, &pInfo, (png_infopp)NULL);
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    
    png_set_read_fn(pPng, (png_voidp)&data, pngReadData);
    
    LoadFromPngContent(pPng, pInfo, this);
}
#endif // PODOFO_HAVE_PNG_LIB

PdfName PdfImage::ColorspaceToName( EPdfColorSpace eColorSpace )
{
    return PdfColor::GetNameForColorSpace( eColorSpace ).GetName();
}

void PdfImage::SetImageChromaKeyMask(pdf_int64 r, pdf_int64 g, pdf_int64 b, pdf_int64 threshold)
{
    PdfArray array;
    array.push_back(r - threshold);
    array.push_back(r + threshold);
    array.push_back(g - threshold);
    array.push_back(g + threshold);
    array.push_back(b - threshold);
    array.push_back(b + threshold);

    this->GetObject()->GetDictionary().AddKey( "Mask", array);
}

void PdfImage::SetInterpolate(bool bValue)
{
    this->GetObject()->GetDictionary().AddKey( "Interpolate", PdfVariant(bValue));
}
};
