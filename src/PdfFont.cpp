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

#include "PdfFont.h"

#include "PdfArray.h"
#include "PdfEncoding.h"
#include "PdfFontMetrics.h"
#include "PdfInputStream.h"
#include "PdfPage.h"
#include "PdfStream.h"
#include "PdfWriter.h"
#include "PdfLocale.h"
#include "PdfDefinesPrivate.h"

#include <stdlib.h>
#include <string.h>
#include <sstream>

using namespace std;

namespace PoDoFo {

PdfFont::PdfFont( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfVecObjects* pParent )
    : PdfElement( "Font", pParent ), m_pEncoding( pEncoding ), 
      m_pMetrics( pMetrics ), m_bBold( false ), m_bItalic( false ), m_isBase14( false )
{
    this->InitVars();
}

PdfFont::PdfFont( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfObject* pObject )
    : PdfElement( "Font", pObject ),
      m_pEncoding( pEncoding ), m_pMetrics( pMetrics ),
      m_bBold( false ), m_bItalic( false ), m_isBase14( false )
{
    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    ostringstream out;
    PdfLocaleImbue(out);
    out << "PoDoFoFt" << m_pObject->Reference().ObjectNumber();
    m_Identifier = PdfName( out.str().c_str() );
}

/*
Constructor for a base14font. All base14 fonts must be constructed via this.
It generates the object number for the font dictionary which will be wrriten in to the pdf.
- Kaushik April 12th 2010
*/
PdfFont::PdfFont(PODOFO_Base14FontDefData *pMetrics_base14, const PdfEncoding* const pEncoding, 
					PdfVecObjects* pParent )
    :  PdfElement( "Font", pParent ), 
       m_pEncoding( pEncoding ), m_bBold( false ), m_bItalic( false ), m_isBase14(true)
{
	ostringstream out;
    PdfLocaleImbue(out);

	out << "Ft" << m_pObject->Reference().ObjectNumber();
    m_Identifier = PdfName( out.str().c_str() );

#ifdef MYASSERT
	assert(pMetrics_base14 != NULL);
#endif

	if (!pMetrics_base14) PODOFO_RAISE_ERROR(ePdfError_InvalidHandle); // base14changes throw
	m_pMetrics = new PdfFontMetrics(pMetrics_base14);

	m_pMetrics->SetFontSize( 12.0 );
    m_pMetrics->SetFontScale( 100.0 );
    m_pMetrics->SetFontCharSpace( 0.0 );

    // Peter Petrov 24 Spetember 2008
    m_bWasEmbedded = false;
    
    m_bUnderlined = false;
    m_bStrikedOut = false;

	std::string sTmp = m_pMetrics->GetFontname();
	m_BaseFont = PdfName( sTmp.c_str() );
}

PdfFont::~PdfFont()
{
    delete m_pMetrics;
    if( m_pEncoding && m_pEncoding->IsAutoDelete() )
        delete m_pEncoding;
}

void PdfFont::InitVars()
{
    ostringstream out;
    PdfLocaleImbue(out);

    m_pMetrics->SetFontSize( 12.0 );
    m_pMetrics->SetFontScale( 100.0 );
    m_pMetrics->SetFontCharSpace( 0.0 );

    // Peter Petrov 24 Spetember 2008
    m_bWasEmbedded = false;
    
    m_bUnderlined = false;
    m_bStrikedOut = false;

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "Ft" << m_pObject->Reference().ObjectNumber();
    m_Identifier = PdfName( out.str().c_str() );

	

    // replace all spaces in the base font name as suggested in 
    // the PDF reference section 5.5.2#
    int curPos = 0;
    std::string sTmp = m_pMetrics->GetFontname();
    const char* pszPrefix = m_pMetrics->GetSubsetFontnamePrefix();
    if( pszPrefix ) 
    {
	std::string sPrefix = pszPrefix;
	sTmp = sPrefix + sTmp;
    }

    for(unsigned int i = 0; i < sTmp.size(); i++)
    {
        if(sTmp[i] != ' ')
            sTmp[curPos++] = sTmp[i];
    }
    sTmp.resize(curPos);
    m_BaseFont = PdfName( sTmp.c_str() );
}

inline char ToHex( const char byte )
{
    static const char* s_pszHex = "0123456789ABCDEF";

    return s_pszHex[byte % 16];
}

void PdfFont::WriteStringToStream( const PdfString & rsString, PdfStream* pStream )
{
    if( !m_pEncoding )
    {
	PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    PdfString sEncoded = m_pEncoding->ConvertToEncoding( rsString, this );
    if( sEncoded.IsUnicode() ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "ConvertToEncoding must not return a unicode string" );
    }

    pdf_long  lLen    = 0;
    char* pBuffer = NULL;

    std::auto_ptr<PdfFilter> pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );    
    pFilter->Encode( sEncoded.GetString(), sEncoded.GetLength(), &pBuffer, &lLen );

    pStream->Append( "<", 1 );
    pStream->Append( pBuffer, lLen );
    pStream->Append( ">", 1 );

    free( pBuffer );
}

// Peter Petrov 5 January 2009
void PdfFont::EmbedFont()
{
    if (!m_bWasEmbedded)
    {
        // Now we embed the font

        // Now we set the flag
        m_bWasEmbedded = true;
    }
}
/*
kausik : April 12th 2010
This is the font dictionary. It gets added to the page resources dictionary of the pdf.
*/
void PdfFont::InitBase14Font( )
{
 		PdfVariant    var;

		m_pObject->GetDictionary().AddKey( PdfName::KeySubtype, PdfName("Type1"));
		m_pObject->GetDictionary().AddKey("BaseFont", this->GetBaseFont() );
		 
		m_pEncoding->AddToDictionary( m_pObject->GetDictionary() ); // Add encoding key
//		pDescriptor->GetDictionary().AddKey( "FontName", this->GetBaseFont() );
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

};

