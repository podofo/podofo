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

#include "PdfTilingPattern.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfColor.h"
#include "base/PdfDictionary.h"
#include "base/PdfLocale.h"
#include "base/PdfRect.h"
#include "base/PdfStream.h"
#include "base/PdfWriter.h"

#include "PdfFunction.h"
#include "PdfImage.h"

#include <iostream>
#include <iomanip>
#include <sstream>

namespace PoDoFo {

PdfTilingPattern::PdfTilingPattern( EPdfTilingPatternType eTilingType,
		 double strokeR, double strokeG, double strokeB,
		 bool doFill, double fillR, double fillG, double fillB,
		 double offsetX, double offsetY,
		 PdfImage *pImage,
		 PdfVecObjects* pParent)
    : PdfElement( "Pattern", pParent )
{
    std::ostringstream out;
    // We probably aren't doing anything locale sensitive here, but it's
    // best to be sure.
    PdfLocaleImbue(out);

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "Ptrn" << this->GetObject()->Reference().ObjectNumber();
    m_Identifier = PdfName( out.str().c_str() );

    this->Init( eTilingType, strokeR, strokeG, strokeB,
		 doFill, fillR, fillG, fillB, offsetX, offsetY, pImage);
}

PdfTilingPattern::PdfTilingPattern( EPdfTilingPatternType eTilingType,
		 double strokeR, double strokeG, double strokeB,
		 bool doFill, double fillR, double fillG, double fillB,
		 double offsetX, double offsetY,
		 PdfImage *pImage,
		 PdfDocument* pParent)
    : PdfElement( "Pattern", pParent )
{
    std::ostringstream out;
    // We probably aren't doing anything locale sensitive here, but it's
    // best to be sure.
    PdfLocaleImbue(out);

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "Ptrn" << this->GetObject()->Reference().ObjectNumber();

    m_Identifier = PdfName( out.str().c_str() );

    this->Init( eTilingType, strokeR, strokeG, strokeB,
		 doFill, fillR, fillG, fillB, offsetX, offsetY, pImage);
}

PdfTilingPattern::~PdfTilingPattern()
{
}

void PdfTilingPattern::AddToResources(const PdfName &rIdentifier, const PdfReference &rRef, const PdfName &rName)
{
	PdfObject* pResource = GetObject()->GetDictionary().GetKey( "Resources" );

	if( !pResource ) {
		PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
	}

	if( !pResource->GetDictionary().HasKey( rName ) ) {
		pResource->GetDictionary().AddKey( rName, PdfDictionary() );
	}
	if (ePdfDataType_Reference == pResource->GetDictionary().GetKey( rName )->GetDataType()) {
		PdfObject *directObject = pResource->GetOwner()->GetObject(pResource->GetDictionary().GetKey( rName )->GetReference());

		if (0 == directObject) {
         PODOFO_RAISE_ERROR( ePdfError_NoObject );
		}

		if( !directObject->GetDictionary().HasKey( rIdentifier ) )
         directObject->GetDictionary().AddKey( rIdentifier, rRef );
	}else {
		if( !pResource->GetDictionary().GetKey( rName )->GetDictionary().HasKey( rIdentifier ) )
         pResource->GetDictionary().GetKey( rName )->GetDictionary().AddKey( rIdentifier, rRef );
	}
}

void PdfTilingPattern::Init( EPdfTilingPatternType eTilingType,
		 double strokeR, double strokeG, double strokeB,
		 bool doFill, double fillR, double fillG, double fillB,
		 double offsetX, double offsetY,
		 PdfImage *pImage)
{
	if (eTilingType == ePdfTilingPatternType_Image && pImage == NULL) {
		PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
	}

	if (eTilingType != ePdfTilingPatternType_Image && pImage != NULL) {
		PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
	}

	PdfRect rRect;
	rRect.SetLeft(0);
	rRect.SetBottom(0);

	if (pImage) {
		rRect.SetWidth(pImage->GetWidth());
		rRect.SetHeight(-pImage->GetHeight());
	} else {
		rRect.SetWidth(8);
		rRect.SetHeight(8);
	}

	PdfVariant var;
   rRect.ToVariant( var );

	this->GetObject()->GetDictionary().AddKey( PdfName("PatternType"), static_cast<pdf_int64>(1L) ); // Tiling pattern
	this->GetObject()->GetDictionary().AddKey( PdfName("PaintType"), static_cast<pdf_int64>(1L) ); // Colored
	this->GetObject()->GetDictionary().AddKey( PdfName("TilingType"), static_cast<pdf_int64>(1L) ); // Constant spacing
	this->GetObject()->GetDictionary().AddKey( PdfName("BBox"), var );
	this->GetObject()->GetDictionary().AddKey( PdfName("XStep"), static_cast<pdf_int64>(rRect.GetWidth()) );
	this->GetObject()->GetDictionary().AddKey( PdfName("YStep"), static_cast<pdf_int64>(rRect.GetHeight()) );
	this->GetObject()->GetDictionary().AddKey( PdfName("Resources"), PdfObject( PdfDictionary() ) );

	if (offsetX < -1e-9 || offsetX > 1e-9 || offsetY < -1e-9 || offsetY > 1e-9) {
		PdfArray array;

		array.push_back (static_cast<pdf_int64>(1));
		array.push_back (static_cast<pdf_int64>(0));
		array.push_back (static_cast<pdf_int64>(0));
		array.push_back (static_cast<pdf_int64>(1));
		array.push_back (offsetX);
		array.push_back (offsetY);

		this->GetObject()->GetDictionary().AddKey( PdfName("Matrix"), array );
	}

   std::ostringstream out;
   out.flags( std::ios_base::fixed );
   out.precision( 1L /* clPainterDefaultPrecision */ );
   PdfLocaleImbue(out);

	if (pImage) {
		AddToResources(pImage->GetIdentifier(), pImage->GetObjectReference(), PdfName("XObject"));

      out << rRect.GetWidth() << " 0 0 "
          << rRect.GetHeight() << " "
          << rRect.GetLeft() << " " 
          << rRect.GetBottom() << " cm" << std::endl;
		out << "/" << pImage->GetIdentifier().GetName() << " Do" << std::endl;
	} else {
		if (doFill) {
			out << fillR << " " << fillG << " " << fillB << " rg" << " ";
			out << rRect.GetLeft() << " " << rRect.GetBottom() << " " << rRect.GetWidth() << " " << rRect.GetHeight() << " re" << " ";
			out << "f" <<  " "; //fill rect
		}

		out << strokeR << " " << strokeG << " " << strokeB << " RG" << " ";
		out << "2 J" << " "; // line capability style
		out << "0.5 w" <<  " "; //line width

		double left, bottom, right, top, whalf, hhalf;
		left = rRect.GetLeft();
		bottom = rRect.GetBottom();
		right = left + rRect.GetWidth();
		top = bottom + rRect.GetHeight();
		whalf = rRect.GetWidth() / 2;
		hhalf = rRect.GetHeight() / 2;

		switch (eTilingType) {
		case ePdfTilingPatternType_BDiagonal:
			out << left          << " " << bottom         << " m " << right         << " " << top            << " l ";
			out << left - whalf  << " " << top - hhalf    << " m " << left + whalf  << " " << top + hhalf    << " l ";
			out << right - whalf << " " << bottom - hhalf << " m " << right + whalf << " " << bottom + hhalf << " l" << std::endl;
			break;
		case ePdfTilingPatternType_Cross:
			out << left          << " " << bottom + hhalf << " m " << right         << " " << bottom + hhalf << " l ";
			out << left + whalf  << " " << bottom         << " m " << left + whalf  << " " << top            << " l" << std::endl;
			break;
		case ePdfTilingPatternType_DiagCross:
			out << left          << " " << bottom         << " m " << right         << " " << top            << " l ";
			out << left          << " " << top            << " m " << right         << " " << bottom         << " l" << std::endl;
			break;
		case ePdfTilingPatternType_FDiagonal:
			out << left          << " " << top            << " m " << right         << " " << bottom         << " l ";
			out << left - whalf  << " " << bottom + hhalf << " m " << left + whalf  << " " << bottom - hhalf << " l ";
			out << right - whalf << " " << top + hhalf    << " m " << right + whalf << " " << top - hhalf    << " l" << std::endl;
			break;
		case ePdfTilingPatternType_Horizontal:
			out << left          << " " << bottom + hhalf << " m " << right         << " " << bottom + hhalf << " l ";
			break;
		case ePdfTilingPatternType_Vertical:
			out << left + whalf  << " " << bottom         << " m " << left + whalf  << " " << top            << " l" << std::endl;
			break;
		case ePdfTilingPatternType_Image:
			/* This is handled above, based on the 'pImage' variable */
		default:
			PODOFO_RAISE_ERROR (ePdfError_InvalidEnumValue);
			break;

		}

		out << "S"; //stroke path
	}

	TVecFilters vecFlate;
	vecFlate.push_back( ePdfFilter_FlateDecode );

	std::string str = out.str();
	PdfMemoryInputStream stream(str.c_str(), str.length());

	this->GetObject()->GetStream()->Set(&stream, vecFlate);
}

}	// end namespace
