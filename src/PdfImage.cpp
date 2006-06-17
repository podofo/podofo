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

#include "PdfStream.h"

#include <stdio.h>
#include <sstream>

extern "C" {
#include "jpeglib.h"
}

using namespace std;

namespace PoDoFo {

PdfImageRef::PdfImageRef()
{
    m_nWidth  = 0;
    m_nHeight = 0;
}
 
PdfImageRef::PdfImageRef( const PdfImageRef & rhs )
{
    operator=(rhs);
}

const PdfImageRef & PdfImageRef::operator=( const PdfImageRef & rhs )
{
    m_Identifier  = rhs.m_Identifier;
    m_reference   = rhs.m_reference;
    m_nWidth      = rhs.m_nWidth;
    m_nHeight     = rhs.m_nHeight;

    return (*this);
}

/////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////

PdfImage::PdfImage( unsigned int objectno, unsigned int generationno )
    : PdfObject( objectno, generationno )
{
    this->AddKey( PdfName::KeySubtype, PdfName( "Image" ) );
    this->AddKey( "ColorSpace", PdfName( ColorspaceToName( ePdfColorSpace_DeviceRGB ) ) );

    m_nHeight = 0;
    m_nWidth  = 0;
}

PdfImage::~PdfImage()
{

}

void PdfImage::SetImageColorSpace( EPdfColorSpace eColorSpace )
{
    this->AddKey( "ColorSpace", PdfName( ColorspaceToName( eColorSpace ) ) );
}

void PdfImage::SetImageData( unsigned int nWidth, unsigned int nHeight, unsigned int nBitsPerComponent, char* szBuffer, long lLen )
{
    this->AddKey( "Width", (long)nWidth );
    this->AddKey( "Height", (long)nHeight );
    this->AddKey( "BitsPerComponent", (long)nBitsPerComponent );
    this->AddKey( "Filter", PdfName("DCTDecode") );

    this->Stream()->Set( szBuffer, lLen );
}

PdfError PdfImage::LoadFromFile( const char* pszFilename )
{
    PdfError eCode;
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

    szBuffer = (char*)malloc( lLen * sizeof(char) );
    if( !szBuffer )
    {
        fclose( hInfile );
        (void) jpeg_destroy_decompress(&cinfo);

        RAISE_ERROR( ePdfError_OutOfMemory );
    }

    if( fread( szBuffer, sizeof( char ), lLen, hInfile ) != lLen )
    {
        fclose( hInfile );
        free( szBuffer );
        (void) jpeg_destroy_decompress(&cinfo);

        RAISE_ERROR( ePdfError_UnexpectedEOF );
    }

    m_nWidth  = cinfo.output_width;
    m_nHeight = cinfo.output_height;

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
    
    this->SetImageData( m_nWidth, m_nHeight, 8 , szBuffer, lLen ); // 8 bits per component

    (void) jpeg_destroy_decompress(&cinfo);

    fclose( hInfile );

    return eCode;
}

void PdfImage::GetImageReference( PdfImageRef & rRef )
{
    ostringstream out;

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Img for images.
    out << "Img" << this->ObjectNumber();

    rRef.SetWidth      ( m_nWidth  );
    rRef.SetHeight     ( m_nHeight );
    rRef.SetIdentifier ( PdfName( out.str().c_str() ) );
    rRef.SetReference  ( this->Reference() );
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
        default:
            return NULL;
    }

    return NULL;
}

};
