/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#include "PdfCanvas.h"

#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfColor.h"
#include "PdfFunction.h"
#include "PdfStream.h"
#include "PdfDefinesPrivate.h"

namespace PoDoFo {

PdfArray PdfCanvas::s_procset;

const PdfArray & PdfCanvas::GetProcSet()
{
    if( s_procset.empty() ) 
    {
        s_procset.push_back( PdfName( "PDF" ) );
        s_procset.push_back( PdfName( "Text" ) );
        s_procset.push_back( PdfName( "ImageB" ) );
        s_procset.push_back( PdfName( "ImageC" ) );
        s_procset.push_back( PdfName( "ImageI" ) );
    }

    return s_procset;
}

void PdfCanvas::AddColorResource( const PdfColor & rColor )
{
    PdfObject* pResource = GetResources();
    
    if( !pResource )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

	switch( rColor.GetColorSpace() )
	{
		case ePdfColorSpace_Separation:
		{
			std::string csPrefix( "ColorSpace" );
			std::string csName = rColor.GetName();
			std::string temp( csPrefix + csName );
            
			if ( 
				! pResource->GetDictionary().HasKey( "ColorSpace" )	||
				! pResource->GetDictionary().GetKey( "ColorSpace" )->GetDictionary().HasKey( csPrefix + csName )
                )
			{
				// Build color-spaces for separation
				PdfObject* csTintFunc = GetContents()->GetOwner()->CreateObject();
                
				csTintFunc->GetDictionary().AddKey( "BitsPerSample", static_cast<pdf_int64>(8LL) );

				PdfArray decode;
				decode.push_back( static_cast<pdf_int64>(0LL) );
				decode.push_back( static_cast<pdf_int64>(1LL) );
				decode.push_back( static_cast<pdf_int64>(0LL) );
				decode.push_back( static_cast<pdf_int64>(1LL) );
				decode.push_back( static_cast<pdf_int64>(0LL) );
				decode.push_back( static_cast<pdf_int64>(1LL) );
				decode.push_back( static_cast<pdf_int64>(0LL) );
                decode.push_back( static_cast<pdf_int64>(1LL) );
				csTintFunc->GetDictionary().AddKey( "Decode", decode );

				PdfArray domain;
				domain.push_back( static_cast<pdf_int64>(0LL) );
				domain.push_back( static_cast<pdf_int64>(1LL) );
				csTintFunc->GetDictionary().AddKey( "Domain", domain );

				PdfArray encode;
				encode.push_back( static_cast<pdf_int64>(0LL) );
				encode.push_back( static_cast<pdf_int64>(1LL) );
				csTintFunc->GetDictionary().AddKey( "Encode", encode );

				csTintFunc->GetDictionary().AddKey( "Filter", PdfName( "FlateDecode" ) );
				csTintFunc->GetDictionary().AddKey( "FunctionType", PdfVariant( static_cast<pdf_int64>(ePdfFunctionType_Sampled) ) );

 				switch ( rColor.GetAlternateColorSpace() )
 				{
 					case ePdfColorSpace_DeviceGray:
 					{
 						char data[1*2];
 						data[0] = 0;
 						data[1] = static_cast<char> (rColor.GetGrayScale());
  
 						PdfMemoryInputStream stream( data, 1*2 );
 						csTintFunc->GetStream()->Set( &stream );
  
 						PdfArray range;
 						range.push_back( static_cast<pdf_int64>(0LL) );
                        range.push_back( static_cast<pdf_int64>(1LL) );
 						csTintFunc->GetDictionary().AddKey( "Range", range );
  
 						PdfArray size;
 						size.push_back( static_cast<pdf_int64>(2LL) );
 						csTintFunc->GetDictionary().AddKey( "Size", size );
  
 						PdfArray csArr;
 						csArr.push_back( PdfName("Separation") );
 						csArr.push_back( PdfName( csName ) );
 						csArr.push_back( PdfName("DeviceGray") );
 						csArr.push_back( csTintFunc->Reference() );
  
 						PdfObject* csp = GetContents()->GetOwner()->CreateObject( csArr );
  
 						AddResource( csPrefix + csName, csp->Reference(), PdfName("ColorSpace") );
 					}
 					break;
 
 					case ePdfColorSpace_DeviceRGB:
 					{
 						char data[3*2];
 						data[0] =
 							data[1] =
 							data[2] = 0;
 						data[3] = static_cast<char> (rColor.GetRed() * 255);
 						data[4] = static_cast<char> (rColor.GetGreen() * 255);
 						data[5] = static_cast<char> (rColor.GetBlue() * 255);
 
 						PdfMemoryInputStream stream( data, 3*2 );
 						csTintFunc->GetStream()->Set( &stream );
 
 						PdfArray range;
 						range.push_back( static_cast<pdf_int64>(0LL) );
 						range.push_back( static_cast<pdf_int64>(1LL) );
 						range.push_back( static_cast<pdf_int64>(0LL) );
 						range.push_back( static_cast<pdf_int64>(1LL) );
 						range.push_back( static_cast<pdf_int64>(0LL) );
 						range.push_back( static_cast<pdf_int64>(1LL) );
 						csTintFunc->GetDictionary().AddKey( "Range", range );
 
 						PdfArray size;
 						size.push_back( static_cast<pdf_int64>(2LL) );
 						csTintFunc->GetDictionary().AddKey( "Size", size );
 
 						PdfArray csArr;
 						csArr.push_back( PdfName("Separation") );
 						csArr.push_back( PdfName( csName ) );
 						csArr.push_back( PdfName("DeviceRGB") );
 						csArr.push_back( csTintFunc->Reference() );
 
 						PdfObject* csp = GetContents()->GetOwner()->CreateObject( csArr );
 
 						AddResource( csPrefix + csName, csp->Reference(), PdfName("ColorSpace") );
 					}
 					break;
 
 					case ePdfColorSpace_DeviceCMYK:
 					{
 						char data[4*2];
 						data[0] =
 							data[1] =
 							data[2] = 
 							data[3] = 0;
 						data[4] = static_cast<char> (rColor.GetCyan() * 255);
 						data[5] = static_cast<char> (rColor.GetMagenta() * 255);
 						data[6] = static_cast<char> (rColor.GetYellow() * 255);
 						data[7] = static_cast<char> (rColor.GetBlack() * 255);
 
 						PdfMemoryInputStream stream( data, 4*2 );
 						csTintFunc->GetStream()->Set( &stream );
 
 						PdfArray range;
 						range.push_back( static_cast<pdf_int64>(0LL) );
 						range.push_back( static_cast<pdf_int64>(1LL) );
 						range.push_back( static_cast<pdf_int64>(0LL) );
 						range.push_back( static_cast<pdf_int64>(1LL) );
 						range.push_back( static_cast<pdf_int64>(0LL) );
 						range.push_back( static_cast<pdf_int64>(1LL) );
 						range.push_back( static_cast<pdf_int64>(0LL) );
 						range.push_back( static_cast<pdf_int64>(1LL) );
 						csTintFunc->GetDictionary().AddKey( "Range", range );
 
 						PdfArray size;
 						size.push_back( static_cast<pdf_int64>(2LL) );
 						csTintFunc->GetDictionary().AddKey( "Size", size );
 
 						PdfArray csArr;
 						csArr.push_back( PdfName("Separation") );
 						csArr.push_back( PdfName( csName ) );
 						csArr.push_back( PdfName("DeviceCMYK") );
 						csArr.push_back( csTintFunc->Reference() );
 
 						PdfObject* csp = GetContents()->GetOwner()->CreateObject( csArr );
 
 						AddResource( csPrefix + csName, csp->Reference(), PdfName("ColorSpace") );
 					}
 					break;
 
 					case ePdfColorSpace_CieLab:
 					{
 						char data[3*2];
 						data[0] =
 							data[1] =
 							data[2] = 0;
 						data[3] = static_cast<char> (rColor.GetCieL() * 255);
 						data[4] = static_cast<char> (rColor.GetCieA() * 255);
 						data[5] = static_cast<char> (rColor.GetCieB() * 255);
 
 						PdfMemoryInputStream stream( data, 3*2 );
 						csTintFunc->GetStream()->Set( &stream );
 
 						PdfArray range;
 						range.push_back( static_cast<pdf_int64>(0LL) );
 						range.push_back( static_cast<pdf_int64>(1LL) );
 						range.push_back( static_cast<pdf_int64>(0LL) );
 						range.push_back( static_cast<pdf_int64>(1LL) );
						range.push_back( static_cast<pdf_int64>(0LL) );
 						range.push_back( static_cast<pdf_int64>(1LL) );
 						csTintFunc->GetDictionary().AddKey( "Range", range );
 
 						PdfArray size;
 						size.push_back( static_cast<pdf_int64>(2LL) );
 						csTintFunc->GetDictionary().AddKey( "Size", size );
 
 						PdfArray csArr;
 						csArr.push_back( PdfName("Separation") );
 						csArr.push_back( PdfName( csName ) );
 						csArr.push_back( PdfName("Lab") );
						csArr.push_back( csTintFunc->Reference() );
 
 						PdfObject* csp = GetContents()->GetOwner()->CreateObject( csArr );
 
 						AddResource( csPrefix + csName, csp->Reference(), PdfName("ColorSpace") );
 					}
 					break;
 				}
			}
		}
		break;

		case ePdfColorSpace_CieLab:
		{
			if ( 
				! pResource->GetDictionary().HasKey( "ColorSpace" )	||
				! pResource->GetDictionary().GetKey( "ColorSpace" )->GetDictionary().HasKey( "ColorSpaceLab" )
			   )
			{
				// Build color-spaces for CIE-lab
				PdfDictionary labDict;

				// D65-whitepoint
				PdfArray wpArr;
				wpArr.push_back( 0.9505 );
				wpArr.push_back( 1.0000 );
				wpArr.push_back( 1.0890 );
				labDict.AddKey( PdfName("WhitePoint" ), wpArr );

				// Range for A,B, L is implicit 0..100
				PdfArray rangeArr;
				rangeArr.push_back( static_cast<pdf_int64>(-128LL) );
				rangeArr.push_back( static_cast<pdf_int64>(127LL) );
				rangeArr.push_back( static_cast<pdf_int64>(-128LL) );
				rangeArr.push_back( static_cast<pdf_int64>(127LL) );
				labDict.AddKey( PdfName("Range" ), rangeArr );

				PdfArray labArr;
				labArr.push_back( PdfName("Lab") );
				labArr.push_back( labDict );

				PdfObject* labp = GetContents()->GetOwner()->CreateObject( labArr );

				AddResource( "ColorSpaceCieLab", labp->Reference(), PdfName("ColorSpace") );
			}
		}
		break;

        case ePdfColorSpace_DeviceGray:
        case ePdfColorSpace_DeviceRGB:
        case ePdfColorSpace_DeviceCMYK:
		default:
		break;
	}
}

void PdfCanvas::AddResource( const PdfName & rIdentifier, const PdfReference & rRef, const PdfName & rName )
{
    if( !rName.GetLength() || !rIdentifier.GetLength() )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    PdfObject* pResource = this->GetResources();
    
    if( !pResource )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( !pResource->GetDictionary().HasKey( rName ) )
    {
        pResource->GetDictionary().AddKey( rName, PdfDictionary() );
    }

    // Peter Petrov: 18 December 2008. Bug fix
	if (ePdfDataType_Reference == pResource->GetDictionary().GetKey( rName )->GetDataType())
    {
        PdfObject *directObject = pResource->GetOwner()->GetObject(pResource->GetDictionary().GetKey( rName )->GetReference());

        if (0 == directObject)
        {
            PODOFO_RAISE_ERROR( ePdfError_NoObject );
        }

        if( !directObject->GetDictionary().HasKey( rIdentifier ) )
            directObject->GetDictionary().AddKey( rIdentifier, rRef );
    }else
    {

        if( !pResource->GetDictionary().GetKey( rName )->GetDictionary().HasKey( rIdentifier ) )
            pResource->GetDictionary().GetKey( rName )->GetDictionary().AddKey( rIdentifier, rRef );
    }
}

};

