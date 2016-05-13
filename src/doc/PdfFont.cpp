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

#include "PdfFont.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfEncoding.h"
#include "base/PdfInputStream.h"
#include "base/PdfStream.h"
#include "base/PdfWriter.h"
#include "base/PdfLocale.h"

#include "PdfFontMetrics.h"
#include "PdfPage.h"

#include <stdlib.h>
#include <string.h>
#include <sstream>

using namespace std;

namespace PoDoFo {

PdfFont::PdfFont( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfVecObjects* pParent )
    : PdfElement( "Font", pParent ), m_pEncoding( pEncoding ), 
      m_pMetrics( pMetrics ), m_bBold( false ), m_bItalic( false ), m_isBase14( false ), m_bIsSubsetting( false )

{
    this->InitVars();
}

PdfFont::PdfFont( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfObject* pObject )
    : PdfElement( "Font", pObject ),
      m_pEncoding( pEncoding ), m_pMetrics( pMetrics ),
      m_bBold( false ), m_bItalic( false ), m_isBase14( false ), m_bIsSubsetting( false )

{
    this->InitVars();

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    ostringstream out;
    PdfLocaleImbue(out);
    out << "PoDoFoFt" << this->GetObject()->Reference().ObjectNumber();
    m_Identifier = PdfName( out.str().c_str() );
}

PdfFont::~PdfFont()
{
    if (m_pMetrics)
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
    out << "Ft" << this->GetObject()->Reference().ObjectNumber();
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

    PdfRefCountedBuffer buffer = m_pEncoding->ConvertToEncoding( rsString, this );
    pdf_long  lLen    = 0;
    char* pBuffer = NULL;

    std::auto_ptr<PdfFilter> pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );    
    pFilter->Encode( buffer.GetBuffer(), buffer.GetSize(), &pBuffer, &lLen );

    pStream->Append( "<", 1 );
    pStream->Append( pBuffer, lLen );
    pStream->Append( ">", 1 );

    podofo_free( pBuffer );
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

void PdfFont::EmbedSubsetFont()
{
	//virtual function is only implemented in derived class
    PODOFO_RAISE_ERROR_INFO( ePdfError_NotImplemented, "Subsetting not implemented for this font type." );
}

void PdfFont::AddUsedSubsettingGlyphs( const PdfString & , long )
{
	//virtual function is only implemented in derived class
    PODOFO_RAISE_ERROR_INFO( ePdfError_NotImplemented, "Subsetting not implemented for this font type." );
}

void PdfFont::AddUsedGlyphname( const char * )
{
	//virtual function is only implemented in derived class
    PODOFO_RAISE_ERROR_INFO( ePdfError_NotImplemented, "Subsetting not implemented for this font type." );
}

void PdfFont::SetBold( bool bBold )
{
    m_bBold = bBold;
}

void PdfFont::SetItalic( bool bItalic )
{
    m_bItalic = bItalic;
}

};
