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
#include "PdfFontMetrics.h"
#include "PdfInputStream.h"
#include "PdfPage.h"
#include "PdfStream.h"
#include "PdfWriter.h"
#include "PdfLocale.h"

#include <stdlib.h>
#include <string.h>
#include <sstream>

using namespace std;

namespace PoDoFo {

const PdfWinAnsiEncoding  PdfFont::WinAnsiEncoding;
const PdfMacRomanEncoding PdfFont::MacRomanEncoding;

PdfFont::PdfFont( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfVecObjects* pParent )
    : PdfElement( "Font", pParent ), m_pEncoding( pEncoding ), 
      m_pMetrics( pMetrics ), m_bBold( false ), m_bItalic( false )
{
    this->InitVars();
}

PdfFont::PdfFont( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfObject* pObject )
    : PdfElement( "Font", pObject ),
      m_pEncoding( pEncoding ), m_pMetrics( pMetrics ),
      m_bBold( false ), m_bItalic( false )
{

}

PdfFont::~PdfFont()
{
    delete m_pMetrics;
    if( m_pEncoding->IsAutoDelete() )
        delete m_pEncoding;
}

void PdfFont::InitVars()
{
    ostringstream out;
    PdfLocaleImbue(out);

    m_pMetrics->SetFontSize( 12.0 );
    m_pMetrics->SetFontScale( 100.0 );
    m_pMetrics->SetFontCharSpace( 0.0 );
    
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
    PdfString sEncoded = m_pEncoding->ConvertToEncoding( rsString, this );
    if( sEncoded.IsUnicode() ) 
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InternalLogic, "ConvertToEncoding must not return a unicode string" );
    }

    long  lLen    = 0;
    char* pBuffer = NULL;

    std::auto_ptr<PdfFilter> pFilter = PdfFilterFactory::Create( ePdfFilter_ASCIIHexDecode );    
    pFilter->Encode( sEncoded.GetString(), sEncoded.GetLength(), &pBuffer, &lLen );

    pStream->Append( "<", 1 );
    pStream->Append( pBuffer, lLen );
    pStream->Append( ">", 1 );

    free( pBuffer );
}


};

