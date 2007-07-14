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
    : PdfXObject( "Image", &(pParent->m_vecObjects) )
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

    m_pObject->GetStream()->SetRawData( pStream, -1 );
}

#ifdef PODOFO_HAVE_JPEG_LIB
void PdfImage::LoadFromFile( const char* pszFilename )
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
