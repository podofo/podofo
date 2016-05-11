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

#include "PdfFontType1Base14.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfDictionary.h"
#include "base/PdfEncoding.h"
#include "PdfFontMetricsBase14.h"
#include "base/PdfArray.h"

namespace PoDoFo {

PdfFontType1Base14::PdfFontType1Base14( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, 
                                        PdfVecObjects* pParent )
    : PdfFontSimple( pMetrics, pEncoding, pParent )
{
    InitBase14Font( pMetrics );
}

// OC 13.08.2010 New:
PdfFontType1Base14::PdfFontType1Base14( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, 
                                        PdfObject* pObject )
    : PdfFontSimple( pMetrics, pEncoding, pObject )
{
    InitBase14Font( pMetrics );
}

PdfFontType1Base14::~PdfFontType1Base14()
{
    // FontMetrics of base14 fonts may not be deleted
    m_pMetrics = NULL;
}

/*
kausik : April 12th 2010
This is the font dictionary. It gets added to the page resources dictionary of the pdf.
*/
void PdfFontType1Base14::InitBase14Font( PdfFontMetrics* pMetrics )
{
    PdfVariant    var;
    
    this->GetObject()->GetDictionary().AddKey( PdfName::KeySubtype, PdfName("Type1"));
    this->GetObject()->GetDictionary().AddKey("BaseFont", PdfName( pMetrics->GetFontname() ) );

    PdfObject *pWidth = this->GetObject()->GetOwner()->CreateObject();
    if( !pWidth )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pMetrics->GetWidthArray( *pWidth, m_pEncoding->GetFirstChar(), m_pEncoding->GetLastChar(), m_pEncoding );

    this->GetObject()->GetDictionary().AddKey("Widths", pWidth->Reference() );
    this->GetObject()->GetDictionary().AddKey("FirstChar", PdfVariant( static_cast<pdf_int64>(m_pEncoding->GetFirstChar()) ) );
    this->GetObject()->GetDictionary().AddKey("LastChar", PdfVariant( static_cast<pdf_int64>(m_pEncoding->GetLastChar()) ) );

    m_pEncoding->AddToDictionary( this->GetObject()->GetDictionary() ); // Add encoding key
//	pDescriptor->GetDictionary().AddKey( "FontName", this->GetBaseFont() );
    //pDescriptor->GetDictionary().AddKey( "FontWeight", (long)m_pMetrics->Weight() );
//		pDescriptor->GetDictionary().AddKey( PdfName::KeyFlags, PdfVariant( static_cast<pdf_int64>(32LL) ) ); // TODO: 0 ????
//		pDescriptor->GetDictionary().AddKey( "FontBBox", array );
	
    
	
		
//			pDescriptor->GetDictionary().AddKey( "ItalicAngle", PdfVariant( static_cast<pdf_int64>(m_pMetrics->GetItalicAngle()) ) );
//			pDescriptor->GetDictionary().AddKey( "Ascent", m_pMetrics->GetPdfAscent() );
//			pDescriptor->GetDictionary().AddKey( "Descent", m_pMetrics->GetPdfDescent() );
//			pDescriptor->GetDictionary().AddKey( "CapHeight", m_pMetrics->GetPdfAscent() ); // m_pMetrics->CapHeight() );
		 

//		pDescriptor->GetDictionary().AddKey( "StemV", PdfVariant( static_cast<pdf_int64>(1LL) ) );               // m_pMetrics->StemV() );

		// Peter Petrov 24 September 2008
//		m_pDescriptor = pDescriptor;

		 
}

void PdfFontType1Base14::EmbedFontFile( PdfObject* )
{
    // Do nothing, base 14 fonts do not need to be embedded
}


};
