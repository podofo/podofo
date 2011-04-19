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

#include "PdfFontSimple.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfEncoding.h"
#include "base/PdfFilter.h"
#include "base/PdfName.h"
#include "base/PdfStream.h"

namespace PoDoFo {

PdfFontSimple::PdfFontSimple( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfVecObjects* pParent )
    : PdfFont( pMetrics, pEncoding, pParent )
{
}

PdfFontSimple::PdfFontSimple( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfObject* pObject )
    : PdfFont( pMetrics, pEncoding, pObject )
{
}

void PdfFontSimple::Init( bool bEmbed, const PdfName & rsSubType )
{
    PdfObject*    pWidth;
    PdfObject*    pDescriptor;
    PdfVariant    var;
    PdfArray      array;

    pWidth = this->GetObject()->GetOwner()->CreateObject();
    if( !pWidth )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pMetrics->GetWidthArray( *pWidth, m_pEncoding->GetFirstChar(), m_pEncoding->GetLastChar() );

    pDescriptor = this->GetObject()->GetOwner()->CreateObject( "FontDescriptor" );
    if( !pDescriptor )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

	std::string name;
	if ( m_bIsSubsetting )
		name = this->GetObject()->GetOwner()->GetNextSubsetPrefix();
	name += this->GetBaseFont().GetName();

    this->GetObject()->GetDictionary().AddKey( PdfName::KeySubtype, rsSubType );
    this->GetObject()->GetDictionary().AddKey("BaseFont", PdfName( name ) );
    this->GetObject()->GetDictionary().AddKey("FirstChar", PdfVariant( static_cast<pdf_int64>(m_pEncoding->GetFirstChar()) ) );
    this->GetObject()->GetDictionary().AddKey("LastChar", PdfVariant( static_cast<pdf_int64>(m_pEncoding->GetLastChar()) ) );
    m_pEncoding->AddToDictionary( this->GetObject()->GetDictionary() ); // Add encoding key

    this->GetObject()->GetDictionary().AddKey("Widths", pWidth->Reference() );
    this->GetObject()->GetDictionary().AddKey( "FontDescriptor", pDescriptor->Reference() );

    m_pMetrics->GetBoundingBox( array );

    pDescriptor->GetDictionary().AddKey( "FontName", PdfName( name ) );
    //pDescriptor->GetDictionary().AddKey( "FontWeight", (long)m_pMetrics->Weight() );
    pDescriptor->GetDictionary().AddKey( PdfName::KeyFlags, PdfVariant( static_cast<pdf_int64>(32LL) ) ); // TODO: 0 ????
    pDescriptor->GetDictionary().AddKey( "FontBBox", array );
    pDescriptor->GetDictionary().AddKey( "ItalicAngle", PdfVariant( static_cast<pdf_int64>(m_pMetrics->GetItalicAngle()) ) );
    pDescriptor->GetDictionary().AddKey( "Ascent", m_pMetrics->GetPdfAscent() );
    pDescriptor->GetDictionary().AddKey( "Descent", m_pMetrics->GetPdfDescent() );
    pDescriptor->GetDictionary().AddKey( "CapHeight", m_pMetrics->GetPdfAscent() ); // m_pMetrics->CapHeight() );
    pDescriptor->GetDictionary().AddKey( "StemV", PdfVariant( static_cast<pdf_int64>(1LL) ) );               // m_pMetrics->StemV() );

    // Peter Petrov 24 September 2008
    m_pDescriptor = pDescriptor;

    if( bEmbed )
    {
        this->EmbedFontFile( pDescriptor );
        m_bWasEmbedded = true;
    }
}

void PdfFontSimple::EmbedFont()
{
    if (!m_bWasEmbedded)
    {
        this->EmbedFontFile( m_pDescriptor );
        m_bWasEmbedded = true;
    }
}

};
