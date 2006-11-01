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

extern "C" {
#include "jpeglib.h"
}

using namespace std;

namespace PoDoFo {

PdfImage::PdfImage( PdfVecObjects* pParent )
    : PdfXObject( "Image", pParent )
{
    m_rRect = PdfRect();

    this->SetImageColorSpace( ePdfColorSpace_DeviceRGB );
}

PdfImage::PdfImage( PdfDocument* pParent )
    : PdfXObject( "Image", &(pParent->GetObjects()) )
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

void PdfImage::SetImageFilter( const PdfName & inName )
{
	m_pObject->GetDictionary().AddKey( "Filter", inName );
}

void PdfImage::SetImageData( unsigned int nWidth, unsigned int nHeight, 
                             unsigned int nBitsPerComponent, 
                             char* szBuffer, long lLen, bool bTakeOwnership )
{
    m_rRect.SetWidth( nWidth );
    m_rRect.SetHeight( nHeight );

    m_pObject->GetDictionary().AddKey( "Width", PdfVariant( static_cast<long>(nWidth) ) );
    m_pObject->GetDictionary().AddKey( "Height", PdfVariant( static_cast<long>(nHeight) ) );
    m_pObject->GetDictionary().AddKey( "BitsPerComponent", PdfVariant( static_cast<long>(nBitsPerComponent) ) );

    m_pObject->GetStream()->Set( szBuffer, lLen, bTakeOwnership );
    if ( m_pObject->GetDictionary().GetKey( "Filter" )->GetName() == "FlateDecode" ) 
    {
        // compress any stream that has been marked as using FlateDecode
        m_pObject->GetStream()->FlateCompress();
    }
}

void PdfImage::LoadFromFile( const char* pszFilename )
{
    FILE*    hInfile;    
    long     lLen;
    char*    szBuffer;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    if( !pszFilename )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    hInfile = fopen(pszFilename, "rb");
    if( !hInfile )
    {
        RAISE_ERROR( ePdfError_FileNotFound );
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, hInfile);

    if( jpeg_read_header(&cinfo, TRUE) <= 0 )
    {
        fclose( hInfile );
        (void) jpeg_destroy_decompress(&cinfo);

        RAISE_ERROR( ePdfError_UnexpectedEOF );
    }

    jpeg_start_decompress(&cinfo);

    fseek( hInfile, 0, SEEK_END );
    lLen = ftell( hInfile );
    fseek( hInfile, 0, SEEK_SET );

    szBuffer = static_cast<char*>(malloc( lLen * sizeof(char) ));
    if( !szBuffer )
    {
        fclose( hInfile );
        (void) jpeg_destroy_decompress(&cinfo);

        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    if( static_cast<long>(fread( szBuffer, sizeof( char ), lLen, hInfile )) != lLen )
    {
        fclose( hInfile );
        free( szBuffer );
        (void) jpeg_destroy_decompress(&cinfo);

        RAISE_ERROR( ePdfError_UnexpectedEOF );
    }

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
    
	this->SetImageFilter( PdfName("DCTDecode") );	// DCTDecode == JPEG
        this->SetImageData( static_cast<unsigned int>(m_rRect.GetWidth()), static_cast<unsigned int>(m_rRect.GetHeight()), 8 , szBuffer, lLen ); // 8 bits per component

    (void) jpeg_destroy_decompress(&cinfo);

    fclose( hInfile );
}

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
