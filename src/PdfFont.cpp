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
#include "PdfPage.h"
#include "PdfStream.h"
#include "PdfWriter.h"

#include <sstream>

#define FIRST_CHAR   0
#define LAST_CHAR  255

using namespace std;

namespace PoDoFo {

PdfFont::PdfFont( PdfFontMetrics* pMetrics, bool bEmbedd, PdfVecObjects* pParent )
    : PdfElement( "Font", pParent ), m_pMetrics( pMetrics )
{
    ostringstream out;

    m_fFontSize   = 12.0;

    m_bBold       = false;
    m_bItalic     = false;
    m_bUnderlined = false;

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "Ft" << m_pObject->Reference().ObjectNumber();
    m_Identifier = PdfName( out.str().c_str() );

    this->Init( bEmbedd );
}
    
PdfFont::~PdfFont()
{
    delete m_pMetrics;
}

void PdfFont::Init( bool bEmbedd )
{
    unsigned int  i;
    int           curPos = 0;
    PdfObject*    pWidth;
    PdfObject*    pDescriptor;
    PdfVariant    var;
    PdfArray      array;
    std::string   sTmp;

    // replace all spaces in the base font name as suggested in 
    // the PDF reference section 5.5.2
    sTmp = m_pMetrics->GetFontname();
    for(i = 0; i < sTmp.size(); i++)
    {
        if(sTmp[i] != ' ')
            sTmp[curPos++] = sTmp[i];
    }
    sTmp.resize(curPos);
    m_BaseFont = PdfName( sTmp.c_str() );


    pWidth = m_pObject->GetOwner()->CreateObject();
    if( !pWidth )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pMetrics->GetWidthArray( *pWidth, FIRST_CHAR, LAST_CHAR );

    pDescriptor = m_pObject->GetOwner()->CreateObject( "FontDescriptor" );
    if( !pDescriptor )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pObject->GetDictionary().AddKey( PdfName::KeySubtype, PdfName("TrueType") );
    m_pObject->GetDictionary().AddKey("BaseFont", m_BaseFont );
    m_pObject->GetDictionary().AddKey("FirstChar", PdfVariant( static_cast<long>(FIRST_CHAR) ) );
    m_pObject->GetDictionary().AddKey("LastChar", PdfVariant( static_cast<long>(LAST_CHAR) ) );
    m_pObject->GetDictionary().AddKey("Encoding", PdfName("WinAnsiEncoding") );
    m_pObject->GetDictionary().AddKey("Widths", pWidth->Reference() );
    m_pObject->GetDictionary().AddKey( "FontDescriptor", pDescriptor->Reference() );

    m_pMetrics->GetBoundingBox( array );

    pDescriptor->GetDictionary().AddKey( "FontName", m_BaseFont );
    //pDescriptor->GetDictionary().AddKey( "FontWeight", (long)m_pMetrics->Weight() );
    pDescriptor->GetDictionary().AddKey( PdfName::KeyFlags, PdfVariant( 32l ) ); // TODO: 0 ????
    pDescriptor->GetDictionary().AddKey( "FontBBox", array );
    pDescriptor->GetDictionary().AddKey( "ItalicAngle", PdfVariant( static_cast<long>(m_pMetrics->GetItalicAngle()) ) );
    pDescriptor->GetDictionary().AddKey( "Ascent", m_pMetrics->GetPdfAscent() );
    pDescriptor->GetDictionary().AddKey( "Descent", m_pMetrics->GetPdfDescent() );
    pDescriptor->GetDictionary().AddKey( "CapHeight", m_pMetrics->GetPdfAscent() ); // //m_pMetrics->CapHeight() );
    pDescriptor->GetDictionary().AddKey( "StemV", PdfVariant( 1l ) ); //m_pMetrics->StemV() );

    if( bEmbedd )
    {
        EmbeddFont( pDescriptor );
    }
}

void PdfFont::EmbeddFont( PdfObject* pDescriptor )
{
    PdfObject* pContents;
    FILE*      hFile;
    char*      pBuffer = NULL;
    long       lSize = 0;

    pContents = m_pObject->GetOwner()->CreateObject();
    if( !pContents )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    pDescriptor->GetDictionary().AddKey( "FontFile2", pContents->Reference() );

    // if the data was loaded from memory - use it from there
    // otherwise, load from disk
    if ( m_pMetrics->GetFontDataLen() && m_pMetrics->GetFontData() ) 
    {
        // FIXME const_cast<char*> is dangerous if string literals may ever be passed
        pBuffer = const_cast<char*>( m_pMetrics->GetFontData() );
        lSize = m_pMetrics->GetFontDataLen();
    } 
    else 
    {
        hFile = fopen( m_pMetrics->GetFilename(), "rb" );
        if( !hFile )
        {
            RAISE_ERROR( ePdfError_FileNotFound );
        }

        fseek( hFile, 0, SEEK_END );
        lSize = ftell( hFile );
        fseek( hFile, 0, SEEK_SET );
        
        pBuffer = static_cast<char*>(malloc( sizeof(char) * lSize ));
        fread( pBuffer, lSize, sizeof(char), hFile ); 
        
        fclose( hFile );
    }
    
    pContents->GetDictionary().AddKey( "Length1", PdfVariant( lSize ) );
    pContents->GetStream()->Set( pBuffer, lSize, !m_pMetrics->GetFontDataLen() );	// if we loaded from memory, DO NOT let Stream take possession
}

void PdfFont::SetFontSize( float fSize )
{
    m_pMetrics->SetFontSize( fSize );

    m_fFontSize = fSize;
}


};

