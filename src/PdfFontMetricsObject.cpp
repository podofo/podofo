/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
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

#include "PdfFontMetricsObject.h"

#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfObject.h"
#include "PdfVariant.h"

namespace PoDoFo {

PdfFontMetricsObject::PdfFontMetricsObject( PdfObject* pDescriptor, 
                                            PdfObject* pFontObject,
                                            const PdfEncoding* const pEncoding )
    : PdfFontMetrics( ePdfFontType_Unknown, "", NULL ),
      m_pEncoding( pEncoding )
{
    if( !pDescriptor )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_sName        = pDescriptor->GetDictionary().GetKey( "FontName" )->GetName();
    m_bbox         = pDescriptor->GetDictionary().GetKey( "FontBBox" )->GetArray();
    m_nFirst       = static_cast<int>(pDescriptor->GetDictionary().GetKeyAsLong( "FirstChar", 0L ));
    m_nLast        = static_cast<int>(pDescriptor->GetDictionary().GetKeyAsLong( "LastChar", 0L ));
    m_nWeight      = static_cast<unsigned int>(pDescriptor->GetDictionary().GetKeyAsLong( "FontWeight", 400L ));
    m_nItalicAngle = static_cast<int>(pDescriptor->GetDictionary().GetKeyAsLong( "ItalicAngle", 0L ));

    m_dPdfAscent   = pDescriptor->GetDictionary().GetKeyAsReal( "Ascent", 0.0 );
    m_dAscent      = m_dPdfAscent / 1000.0;
    m_dPdfDescent  = pDescriptor->GetDictionary().GetKeyAsReal( "Descent", 0.0 );
    m_dDescent     = m_dPdfDescent / 1000.0;
    m_dLineSpacing = m_dAscent + m_dDescent;

    // Try to fine some sensible values
    m_dUnderlineThickness = 1.0;
    m_dUnderlinePosition  = 0.0;
    m_dStrikeOutThickness = m_dUnderlinePosition;
    m_dStrikeOutPosition  = m_dAscent / 2.0;

    m_bSymbol = false; // TODO

    if( pFontObject->GetDictionary().HasKey( "Width" ) ) 
    {
        m_width = pDescriptor->GetIndirectKey( "Width" )->GetArray();
    }
}

PdfFontMetricsObject::~PdfFontMetricsObject()
{
}

const char* PdfFontMetricsObject::GetFontname() const
{
    return m_sName.GetName().c_str();
}

void PdfFontMetricsObject::GetBoundingBox( PdfArray & array ) const
{
    array = m_bbox;
}

double PdfFontMetricsObject::CharWidth( unsigned char c ) const
{
    if( c > m_nFirst && c < m_nLast ) 
    { 
        double dWidth = m_width[c - m_nFirst].GetReal();
        
        return dWidth * static_cast<double>(this->GetFontSize() * this->GetFontScale() / 100.0) / 1000.0 +
            static_cast<double>( this->GetFontSize() * this->GetFontScale() / 100.0 * this->GetFontCharSpace() / 100.0);

    }

    return 0.0;
}

double PdfFontMetricsObject::UnicodeCharWidth( unsigned short c ) const
{
    // TODO
    return 0.0;
}

void PdfFontMetricsObject::GetWidthArray( PdfVariant & var, unsigned int nFirst, unsigned int nLast ) const
{
    var = m_width;
}

double PdfFontMetricsObject::GetGlyphWidth( int nGlyphId ) const
{
    // TODO
    return 0.0;
}

long PdfFontMetricsObject::GetGlyphId( long lUnicode ) const
{
    // TODO
    return 0L;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetLineSpacing() const
{
    return m_dLineSpacing * this->GetFontSize();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetUnderlinePosition() const
{
    return m_dUnderlinePosition * this->GetFontSize();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetStrikeOutPosition() const
{
	return m_dStrikeOutPosition * this->GetFontSize();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetUnderlineThickness() const
{
    return m_dUnderlineThickness * this->GetFontSize();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetStrikeoutThickness() const
{
    return m_dStrikeOutThickness * this->GetFontSize();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const char* PdfFontMetricsObject::GetFontData() const
{
    return NULL;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
pdf_long PdfFontMetricsObject::GetFontDataLen() const
{
    return 0;
}  

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned int PdfFontMetricsObject::GetWeight() const
{
    return m_nWeight;
}  

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetAscent() const
{
    return m_dAscent * this->GetFontSize();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetPdfAscent() const
{
    return m_dPdfAscent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetDescent() const
{
    return m_dDescent * this->GetFontSize();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetPdfDescent() const
{
    return m_dPdfDescent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
int PdfFontMetricsObject::GetItalicAngle() const
{
    return m_nItalicAngle;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfFontMetricsObject::IsSymbol() const
{
    return m_bSymbol;
}

};
