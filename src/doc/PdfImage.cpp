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

#include "base/PdfDefinesPrivate.h"

#include "base/PdfColor.h"
#include "base/PdfStream.h"

#include <stdio.h>
#include <wchar.h>
#include <sstream>

#define PODOFO_JPEG_RUNTIME_COMPATIBLE
#ifdef PODOFO_HAVE_TIFF_LIB
extern "C" {
#  include "tiffio.h"
#  ifdef _WIN32		// Collision between tiff and jpeg-headers
#    ifndef XMD_H
#    define XMD_H
#    endif
#    undef FAR
#  endif
}
#endif // PODOFO_HAVE_TIFF_LIB

#ifdef PODOFO_HAVE_JPEG_LIB
extern "C" {
#  ifndef XMD_H
#    define XMD_H
#  endif
#  include "jpeglib.h"
}
#endif // PODOFO_HAVE_JPEG_LIB

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
    m_rRect.SetHeight( static_cast<double>(this->GetObject()->GetDictionary().GetKey( "Height" )->GetNumber()) );
    m_rRect.SetWidth ( static_cast<double>(this->GetObject()->GetDictionary().GetKey( "Width" )->GetNumber()) );
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

void PdfImage::SetImageColorSpace( EPdfColorSpace eColorSpace )
{
    this->GetObject()->GetDictionary().AddKey( PdfName("ColorSpace"), ColorspaceToName( eColorSpace ) );
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

#ifdef PODOFO_HAVE_JPEG_LIB
#if !defined(PODOFO_JPEG_RUNTIME_COMPATIBLE)
void jpeg_memory_src (j_decompress_ptr cinfo, const JOCTET * buffer, size_t bufsize);
#endif // PODOFO_JPEG_RUNTIME_COMPATIBLE

extern "C" {
static void JPegErrorExit(j_common_ptr cinfo)
{
#if 1	
    char buffer[JMSG_LENGTH_MAX];

    /* Create the message */
    (*cinfo->err->format_message) (cinfo, buffer);
#endif
    jpeg_destroy(cinfo);
    PODOFO_RAISE_ERROR_INFO( ePdfError_UnsupportedImageFormat, buffer);
}

static void JPegErrorOutput(j_common_ptr cinfo, int msg_level)
{
}

};


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

#if !defined(PODOFO_JPEG_RUNTIME_COMPATIBLE)
    const long lSize = 1024;
    PdfRefCountedBuffer buffer( lSize );
    fread( buffer.GetBuffer(), sizeof(char), lSize, hInfile );
    
    // On WIN32, you can only pass a FILE Handle to DLLs which where compiled using the same
    // C library. This is usually not the case with LibJpeg on WIN32. 
    // As a reason we use a memory buffer to determine the header information.
    //
    // If you are sure that libJpeg is compiled against the same C library as your application
    // you can removed this ifdef.
    jpeg_memory_src ( &cinfo, reinterpret_cast<JOCTET*>(buffer.GetBuffer()), buffer.GetSize() );
#else
    jpeg_stdio_src(&cinfo, pInStream->GetHandle());
#endif // PODOFO_JPEG_RUNTIME_COMPATIBLE

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
		
    if ( planarConfig != PLANARCONFIG_CONTIG && colorChannels != 1 )
    {
        TIFFClose(hInfile);
        PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
    }

	if ( orientation != ORIENTATION_TOPLEFT )
    {
        TIFFClose(hInfile);
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
			else if ( bitsPixel == 8)
	            SetImageColorSpace(ePdfColorSpace_DeviceGray);
			else
			{
				TIFFClose(hInfile);
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
			else if ( bitsPixel == 8)
	            SetImageColorSpace(ePdfColorSpace_DeviceGray);
			else
			{
				TIFFClose(hInfile);
				PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
			}
		}
		break;

		case PHOTOMETRIC_RGB:
            if ( bitsPixel != 24 )
			{
				TIFFClose(hInfile);
				PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
			}
			SetImageColorSpace(ePdfColorSpace_DeviceRGB);
		break;

		case PHOTOMETRIC_SEPARATED: 
			if( bitsPixel != 32)
			{
				TIFFClose(hInfile);
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
			TIFFGetField(hInfile, TIFFTAG_COLORMAP, &rgbRed, &rgbGreen, &rgbBlue);

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
	        TIFFClose(hInfile);
	        PODOFO_RAISE_ERROR( ePdfError_UnsupportedImageFormat );
		break;
	}

	int32 scanlineSize = TIFFScanlineSize(hInfile);
    long bufferSize = scanlineSize * height;
    char *buffer = new char[bufferSize];
    if( !buffer ) 
    {
        TIFFClose(hInfile);
        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
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

	PdfMemoryInputStream stream(buffer, bufferSize);

    SetImageData(static_cast<unsigned int>(width), 
                 static_cast<unsigned int>(height),
                 static_cast<unsigned int>(bitsPerSample), 
                 &stream);

    delete[] buffer;

    TIFFClose(hInfile);
}
#endif // PODOFO_HAVE_TIFF_LIB
#ifdef PODOFO_HAVE_PNG_LIB
void PdfImage::LoadFromPng( const char* pszFilename )
{
    if( !pszFilename )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    FILE* hFile = fopen(pszFilename, "rb");
    if( !hFile )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, pszFilename );
    }

    png_byte header[8];
    fread(header, 1, 8, hFile);
    if( png_sig_cmp(header, 0, 8) )
    {
        fclose( hFile );
        PODOFO_RAISE_ERROR_INFO( ePdfError_UnsupportedImageFormat, "The file could not be recognized as a PNG file." );
    }
    
    png_structp pPng = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if( !pPng )
    {
        fclose( hFile );
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    png_infop pInfo = png_create_info_struct(pPng);
    if( !pInfo )
    {
        png_destroy_read_struct(&pPng, (png_infopp)NULL, (png_infopp)NULL);
        fclose( hFile );
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( setjmp(png_jmpbuf(pPng)) )
    {
        png_destroy_read_struct(&pPng, &pInfo, (png_infopp)NULL);
        fclose( hFile );
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    png_init_io(pPng, hFile);
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
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(pPng);

    if (color_type & PNG_COLOR_MASK_ALPHA)
       png_set_strip_alpha(pPng);
#if 0
    /* expand gray bit depth if needed */
    if (color_type == PNG_COLOR_TYPE_GRAY) {
#if PNG_LIBPNG_VER >= 10209
        png_set_expand_gray_1_2_4_to_8 (pPng);
#else
        png_set_gray_1_2_4_to_8 (pPng);
#endif
    }
#endif
    /* transform transparency to alpha */
    if (png_get_valid (pPng, pInfo, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha (pPng);

    if (depth == 16)
        png_set_strip_16(pPng);

    if (depth < 8)
        png_set_packing(pPng);
#if 0
    /* convert grayscale to RGB */
    if (color_type == PNG_COLOR_TYPE_GRAY ||
	color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
	png_set_gray_to_rgb (pPng);
    }
#endif
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
        fclose( hFile );
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }


    long lLen = static_cast<long>(png_get_rowbytes(pPng, pInfo) * height);
    char* pBuffer = static_cast<char*>(malloc(sizeof(char) * lLen));
    png_bytepp pRows = static_cast<png_bytepp>(malloc(sizeof(png_bytep)*height));
    for(int y=0; y<height; y++)
    {
        pRows[y] = reinterpret_cast<png_bytep>(pBuffer + (y * png_get_rowbytes(pPng, pInfo)));
    }

    png_read_image(pPng, pRows);
    fclose(hFile);

    m_rRect.SetWidth( width );
    m_rRect.SetHeight( height );

    switch( png_get_channels( pPng, pInfo ) )
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

    // Set the image data and flate compress it
    PdfMemoryInputStream stream( pBuffer, lLen );
    this->SetImageData( width, height, depth, &stream );
    
    free(pBuffer);
    free(pRows);
    
    png_destroy_read_struct(&pPng, &pInfo, (png_infopp)NULL);

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
