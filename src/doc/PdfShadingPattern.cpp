/***************************************************************************
*   Copyright (C) 2007 by Dominik Seichter                                *
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

#include "PdfShadingPattern.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfColor.h"
#include "base/PdfDictionary.h"
#include "base/PdfLocale.h"
#include "base/PdfWriter.h"

#include "PdfFunction.h"

#include <sstream>

namespace PoDoFo {

PdfShadingPattern::PdfShadingPattern( EPdfShadingPatternType eShadingType, PdfVecObjects* pParent )
    : PdfElement( "Pattern", pParent )
{
    std::ostringstream out;
    // We probably aren't doing anything locale sensitive here, but it's
    // best to be sure.
    PdfLocaleImbue(out);

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "Sh" << this->GetObject()->Reference().ObjectNumber();
    m_Identifier = PdfName( out.str().c_str() );

    this->Init( eShadingType );
}

PdfShadingPattern::PdfShadingPattern( EPdfShadingPatternType eShadingType, PdfDocument* pParent )
    : PdfElement( "Pattern", pParent )
{
    std::ostringstream out;
    // We probably aren't doing anything locale sensitive here, but it's
    // best to be sure.
    PdfLocaleImbue(out);

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "Sh" << this->GetObject()->Reference().ObjectNumber();
    m_Identifier = PdfName( out.str().c_str() );

    this->Init( eShadingType );
}

PdfShadingPattern::~PdfShadingPattern()
{
}

void PdfShadingPattern::Init( EPdfShadingPatternType eShadingType )
{
    /*
    switch( eShadingType ) 
    {
        case ePdfShadingPatternType_FunctionBase:
        {
            PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
        }
            break;
        case ePdfShadingPatternType_Radial:
        {
            PdfArray coords;
            coords.push_back( 0.0 );
            coords.push_back( 0.0 );
            coords.push_back( 0.096 );
            coords.push_back( 0.0 );
            coords.push_back( 0.0 );
            coords.push_back( 1.0 );

            PdfArray domain;
            domain.push_back( 0.0 );
            domain.push_back( 1.0 );

            PdfArray c0;
            c0.push_back( 0.929 );
            c0.push_back( 0.357 );
            c0.push_back( 1.0 );
            c0.push_back( 0.298 );

            PdfArray c1a;
            c0.push_back( 0.631 );
            c0.push_back( 0.278 );
            c0.push_back( 1.0 );
            c0.push_back( 0.027 );

            PdfArray c1b;
            c0.push_back( 0.94 );
            c0.push_back( 0.4 );
            c0.push_back( 1.0 );
            c0.push_back( 0.102 );


            PdfExponentialFunction f1( domain, c0, c1a, 1.048, this->GetObject()->GetOwner() );
            PdfExponentialFunction f2( domain, c0, c1b, 1.374, this->GetObject()->GetOwner() );

            PdfFunction::List list;
            list.push_back( f1 );
            list.push_back( f2 );

            PdfArray bounds;
            bounds.push_back( 0.708 );

            PdfArray encode;
            encode.push_back( 1.0 );
            encode.push_back( 0.0 );
            encode.push_back( 0.0 );
            encode.push_back( 1.0 );

            PdfStitchingFunction function( list, domain, bounds, encode, this->GetObject()->GetOwner() );

            shading.AddKey( PdfName("Coords"), coords );
            shading.AddKey( PdfName("Function"), function.GetObject()->Reference() );
            break;
        }
        case ePdfShadingPatternType_FreeForm:
        {
            PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
        }
            break;
        case ePdfShadingPatternType_LatticeForm:
        {
            PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
        }
            break;
        case ePdfShadingPatternType_CoonsPatch:
        {
            PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
        }
            break;
        case ePdfShadingPatternType_TensorProduct:
        {
            PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
        }
            break;
        default:
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidEnumValue, "PdfShadingPattern::Init() failed because of an invalid shading pattern type." );
        }
        };
    */

    // keys common to all shading directories
    PdfDictionary shading;
    shading.AddKey( PdfName("ShadingType"), static_cast<pdf_int64>(eShadingType) );

    this->GetObject()->GetDictionary().AddKey( PdfName("PatternType"), static_cast<pdf_int64>(2LL) ); // Shading pattern
    this->GetObject()->GetDictionary().AddKey( PdfName("Shading"), shading );
}

PdfAxialShadingPattern::PdfAxialShadingPattern( double dX0, double dY0, double dX1, double dY1, const PdfColor & rStart, const PdfColor & rEnd, PdfVecObjects* pParent )
    : PdfShadingPattern( ePdfShadingPatternType_Axial, pParent )
{
    Init( dX0, dY0, dX1, dY1, rStart, rEnd );
}

PdfAxialShadingPattern::PdfAxialShadingPattern( double dX0, double dY0, double dX1, double dY1, const PdfColor & rStart, const PdfColor & rEnd, PdfDocument* pParent )
    : PdfShadingPattern( ePdfShadingPatternType_Axial, pParent )
{
    Init( dX0, dY0, dX1, dY1, rStart, rEnd );
}

void PdfAxialShadingPattern::Init( double dX0, double dY0, double dX1, double dY1, const PdfColor & rStart, const PdfColor & rEnd )
{
    PdfArray coords;
    coords.push_back( dX0 );
    coords.push_back( dY0 );
    coords.push_back( dX1 );
    coords.push_back( dY1 );
            
    if( rStart.GetColorSpace() != rEnd.GetColorSpace() )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "Colorspace of start and end color in PdfAxialShadingPattern does not match." );
    }

    PdfArray c0 = rStart.ToArray();
    PdfArray c1 = rEnd.ToArray();
    PdfArray extend; 
    
    extend.push_back( true );
    extend.push_back( true );

    PdfArray domain;
    domain.push_back( 0.0 );
    domain.push_back( 1.0 );

    PdfExponentialFunction function( domain, c0, c1, 1.0, this->GetObject()->GetOwner() );

    PdfDictionary & shading = this->GetObject()->GetDictionary().GetKey( PdfName("Shading") )->GetDictionary();

	switch( rStart.GetColorSpace() )
	{
		case ePdfColorSpace_DeviceRGB:
	        shading.AddKey( PdfName("ColorSpace"), PdfName("DeviceRGB") );
		break;

		case ePdfColorSpace_DeviceCMYK:
		    shading.AddKey( PdfName("ColorSpace"), PdfName("DeviceCMYK") );
		break;

		case ePdfColorSpace_DeviceGray:
	        shading.AddKey( PdfName("ColorSpace"), PdfName("DeviceGray") );
		break;

		case ePdfColorSpace_CieLab:
		{	
			PdfObject * csp = rStart.BuildColorSpace( this->GetObject()->GetOwner() );

			shading.AddKey( PdfName("ColorSpace"), csp->Reference() );
		}
		break;

		case ePdfColorSpace_Separation:
		{
			PdfObject * csp = rStart.BuildColorSpace( this->GetObject()->GetOwner() );

			shading.AddKey( PdfName("ColorSpace"), csp->Reference() );
		}
		break;

        case ePdfColorSpace_Unknown:
		default:
	        PODOFO_RAISE_ERROR_INFO( ePdfError_CannotConvertColor, "Colorspace not supported in PdfAxialShadingPattern." );
		break;
	}

    shading.AddKey( PdfName("Coords"), coords );
    shading.AddKey( PdfName("Function"), function.GetObject()->Reference() );
    shading.AddKey( PdfName("Extend"), extend );
}

PdfFunctionBaseShadingPattern::PdfFunctionBaseShadingPattern( const PdfColor & rLL, const PdfColor & rUL, const PdfColor & rLR, const PdfColor & rUR, const PdfArray & rMatrix, PdfVecObjects* pParent )
    : PdfShadingPattern( ePdfShadingPatternType_FunctionBase, pParent )
{
    Init( rLL, rUL, rLR, rUR, rMatrix );
}

PdfFunctionBaseShadingPattern::PdfFunctionBaseShadingPattern( const PdfColor & rLL, const PdfColor & rUL, const PdfColor & rLR, const PdfColor & rUR, const PdfArray & rMatrix, PdfDocument* pParent )
    : PdfShadingPattern( ePdfShadingPatternType_FunctionBase, pParent )
{
    Init( rLL, rUL, rLR, rUR, rMatrix );
}

void PdfFunctionBaseShadingPattern::Init( const PdfColor & rLL, const PdfColor & rUL, const PdfColor & rLR, const PdfColor & rUR, const PdfArray & rMatrix )
{
    if( rLL.GetColorSpace() != rUL.GetColorSpace() || rUL.GetColorSpace() != rLR.GetColorSpace() || rLR.GetColorSpace() != rUR.GetColorSpace() )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "Colorspace of start and end color in PdfFunctionBaseShadingPattern does not match." );
    }

    PdfArray domain;
    domain.push_back( 0.0 );
    domain.push_back( 1.0 );
    domain.push_back( 0.0 );
    domain.push_back( 1.0 );

    PdfDictionary & shading = this->GetObject()->GetDictionary().GetKey( PdfName("Shading") )->GetDictionary();
	PdfArray range;
	PdfSampledFunction::Sample samples;

	switch ( rLL.GetColorSpace() )
	{
		case ePdfColorSpace_DeviceRGB:
		{
			range.push_back( 0.0 );
			range.push_back( 1.0 );
			range.push_back( 0.0 );
			range.push_back( 1.0 );
			range.push_back( 0.0 );
			range.push_back( 1.0 );
	
			samples.insert( samples.end(), static_cast<char> ( rLL.GetRed() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rLL.GetGreen() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rLL.GetBlue() *255.0 ) );
	
			samples.insert( samples.end(), static_cast<char> ( rLR.GetRed() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rLR.GetGreen() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rLR.GetBlue() *255.0 ) );
	
			samples.insert( samples.end(), static_cast<char> ( rUL.GetRed() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rUL.GetGreen() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rUL.GetBlue() *255.0 ) );
	
			samples.insert( samples.end(), static_cast<char> ( rUR.GetRed() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rUR.GetGreen() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rUR.GetBlue() *255.0 ) );
	
	        shading.AddKey( PdfName("ColorSpace"), PdfName("DeviceRGB") );
		}
		break;

		case ePdfColorSpace_DeviceCMYK:
		{
			range.push_back( 0.0 );
			range.push_back( 1.0 );
			range.push_back( 0.0 );
			range.push_back( 1.0 );
			range.push_back( 0.0 );
			range.push_back( 1.0 );
			range.push_back( 0.0 );
			range.push_back( 1.0 );
	
			samples.insert( samples.end(), static_cast<char> ( rLL.GetCyan() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rLL.GetMagenta() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rLL.GetYellow() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rLL.GetBlack() *255.0 ) );
	
			samples.insert( samples.end(), static_cast<char> ( rLR.GetCyan() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rLR.GetMagenta() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rLR.GetYellow() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rLR.GetBlack() *255.0 ) );
	
			samples.insert( samples.end(), static_cast<char> ( rUL.GetCyan() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rUL.GetMagenta() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rUL.GetYellow() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rUL.GetBlack() *255.0 ) );
	
			samples.insert( samples.end(), static_cast<char> ( rUR.GetCyan() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rUR.GetMagenta() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rUR.GetYellow() *255.0 ) );
			samples.insert( samples.end(), static_cast<char> ( rUR.GetBlack() *255.0 ) );
	
	        shading.AddKey( PdfName("ColorSpace"), PdfName("DeviceCMYK") );
		}
		break;

		case ePdfColorSpace_DeviceGray:
		{
			range.push_back( 0.0 );
			range.push_back( 1.0 );
	
			samples.insert( samples.end(), static_cast<char> ( rLL.GetGrayScale() *255.0 ) );
	
			samples.insert( samples.end(), static_cast<char> ( rLR.GetGrayScale() *255.0 ) );
	
			samples.insert( samples.end(), static_cast<char> ( rUL.GetGrayScale() *255.0 ) );
	
			samples.insert( samples.end(), static_cast<char> ( rUR.GetGrayScale() *255.0 ) );
	
	        shading.AddKey( PdfName("ColorSpace"), PdfName("DeviceGray") );
		}
		break;

		case ePdfColorSpace_CieLab:
		{
			range.push_back( 0.0 );
			range.push_back( 100.0 );
			range.push_back( -128.0 );
			range.push_back( 127.0 );
			range.push_back( -128.0 );
			range.push_back( 127.0 );

			samples.insert( samples.end(), static_cast<char> ( rLL.GetCieL() *2.55 ) );
			samples.insert( samples.end(), static_cast<char> ( rLL.GetCieA() +128 ) );
			samples.insert( samples.end(), static_cast<char> ( rLL.GetCieB() +128 ) );

			samples.insert( samples.end(), static_cast<char> ( rLR.GetCieL() *2.55 ) );
			samples.insert( samples.end(), static_cast<char> ( rLR.GetCieA() +128 ) );
			samples.insert( samples.end(), static_cast<char> ( rLR.GetCieB() +128 ) );

			samples.insert( samples.end(), static_cast<char> ( rUL.GetCieL() *2.55 ) );
			samples.insert( samples.end(), static_cast<char> ( rUL.GetCieA() +128 ) );
			samples.insert( samples.end(), static_cast<char> ( rUL.GetCieB() +128 ) );

			samples.insert( samples.end(), static_cast<char> ( rUR.GetCieL() *2.55 ) );
			samples.insert( samples.end(), static_cast<char> ( rUR.GetCieA() +128 ) );
			samples.insert( samples.end(), static_cast<char> ( rUR.GetCieB() +128 ) );

			PdfObject * csp = rLL.BuildColorSpace( this->GetObject()->GetOwner() );

			shading.AddKey( PdfName("ColorSpace"), csp->Reference() );
		}
		break;

		case ePdfColorSpace_Separation:
		{
			range.push_back( 0.0 );
			range.push_back( 1.0 );

			samples.insert( samples.end(), static_cast<char> ( rLL.GetDensity() *255.0 ) );

			samples.insert( samples.end(), static_cast<char> ( rLR.GetDensity() *255.0 ) );

			samples.insert( samples.end(), static_cast<char> ( rUL.GetDensity() *255.0 ) );

			samples.insert( samples.end(), static_cast<char> ( rUR.GetDensity() *255.0 ) );

			PdfObject * csp = rLL.BuildColorSpace( this->GetObject()->GetOwner() );

			shading.AddKey( PdfName("ColorSpace"), csp->Reference() );
		}
		break;

        case ePdfColorSpace_Unknown:
		default:
	        PODOFO_RAISE_ERROR_INFO( ePdfError_CannotConvertColor, "Colorspace not supported in PdfFunctionBaseShadingPattern." );
		break;
	}

    PdfSampledFunction function( domain, range, samples, this->GetObject()->GetOwner() );
    shading.AddKey( PdfName("Function"), function.GetObject()->Reference() );
    shading.AddKey( PdfName("Domain"), domain );
    shading.AddKey( PdfName("Matrix"), rMatrix );
}

PdfRadialShadingPattern::PdfRadialShadingPattern( double dX0, double dY0, double dR0, double dX1, double dY1, double dR1, const PdfColor & rStart, const PdfColor & rEnd, PdfVecObjects* pParent )
    : PdfShadingPattern( ePdfShadingPatternType_Radial, pParent )
{
    Init( dX0, dY0, dR0, dX1, dY1, dR1, rStart, rEnd );
}

PdfRadialShadingPattern::PdfRadialShadingPattern( double dX0, double dY0, double dR0, double dX1, double dY1, double dR1, const PdfColor & rStart, const PdfColor & rEnd, PdfDocument* pParent )
    : PdfShadingPattern( ePdfShadingPatternType_Radial, pParent )
{
    Init( dX0, dY0, dR0, dX1, dY1, dR1, rStart, rEnd );
}

void PdfRadialShadingPattern::Init( double dX0, double dY0, double dR0, double dX1, double dY1, double dR1, const PdfColor & rStart, const PdfColor & rEnd )
{
    PdfArray coords;
    coords.push_back( dX0 );
    coords.push_back( dY0 );
    coords.push_back( dR0 );
    coords.push_back( dX1 );
    coords.push_back( dY1 );
    coords.push_back( dR1 );
            
    if( rStart.GetColorSpace() != rEnd.GetColorSpace() )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "Colorspace of start and end color in PdfRadialShadingPattern does not match." );
    }

    PdfArray c0 = rStart.ToArray();
    PdfArray c1 = rEnd.ToArray();
    PdfArray extend; 
    
    extend.push_back( true );
    extend.push_back( true );

    PdfArray domain;
    domain.push_back( 0.0 );
    domain.push_back( 1.0 );

    PdfExponentialFunction function( domain, c0, c1, 1.0, this->GetObject()->GetOwner() );

    PdfDictionary & shading = this->GetObject()->GetDictionary().GetKey( PdfName("Shading") )->GetDictionary();

	switch( rStart.GetColorSpace() )
	{
		case ePdfColorSpace_DeviceRGB:
	        shading.AddKey( PdfName("ColorSpace"), PdfName("DeviceRGB") );
		break;

		case ePdfColorSpace_DeviceCMYK:
		    shading.AddKey( PdfName("ColorSpace"), PdfName("DeviceCMYK") );
		break;

		case ePdfColorSpace_DeviceGray:
	        shading.AddKey( PdfName("ColorSpace"), PdfName("DeviceGray") );
		break;

		case ePdfColorSpace_CieLab:
		{	
			PdfObject * csp = rStart.BuildColorSpace( this->GetObject()->GetOwner() );

			shading.AddKey( PdfName("ColorSpace"), csp->Reference() );
		}
		break;

		case ePdfColorSpace_Separation:
		{
			PdfObject * csp = rStart.BuildColorSpace( this->GetObject()->GetOwner() );

			shading.AddKey( PdfName("ColorSpace"), csp->Reference() );
		}
		break;

        case ePdfColorSpace_Unknown:
		default:
	        PODOFO_RAISE_ERROR_INFO( ePdfError_CannotConvertColor, "Colorspace not supported in PdfRadialShadingPattern." );
		break;
	}

    shading.AddKey( PdfName("Coords"), coords );
    shading.AddKey( PdfName("Function"), function.GetObject()->Reference() );
    shading.AddKey( PdfName("Extend"), extend );
}

}	// end namespace
