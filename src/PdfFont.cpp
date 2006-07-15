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

PdfFont::PdfFont( unsigned int objectno, unsigned int generationno )
    : PdfObject( objectno, generationno, "Font" )
{
    ostringstream out;

    m_fFontSize   = 12.0;

    m_bBold       = false;
    m_bItalic     = false;
    m_bUnderlined = false;

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "Ft" << this->ObjectNumber();
    m_Identifier = PdfName( out.str().c_str() );
}
    
PdfFont::~PdfFont()
{
    delete m_pMetrics;
}

void PdfFont::Init( PdfFontMetrics* pMetrics, PdfVecObjects* pParent, bool bEmbedd )
{
    unsigned int  i;
    int           curPos = 0;
    PdfObject*    pWidth;
    PdfObject*    pDescriptor;
    PdfVariant    var;
    PdfArray      array;
    std::string   sTmp;

    m_pMetrics = pMetrics;

    // replace all spaces in the base font name as suggested in 
    // the PDF reference section 5.5.2
    sTmp = m_pMetrics->Fontname();
    for(i = 0; i < sTmp.size(); i++)
    {
        if(sTmp[i] != ' ')
            sTmp[curPos++] = sTmp[i];
    }
    sTmp.resize(curPos);
    m_BaseFont = PdfName( sTmp.c_str() );


    pWidth = pParent->CreateObject();
    if( !pWidth )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pMetrics->GetWidthArray( *pWidth, FIRST_CHAR, LAST_CHAR );

    pDescriptor = pParent->CreateObject( "FontDescriptor" );
    if( !pDescriptor )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    this->GetDictionary().AddKey( PdfName::KeySubtype, PdfName("TrueType") );
    this->GetDictionary().AddKey("BaseFont", m_BaseFont );
    this->GetDictionary().AddKey("FirstChar", PdfVariant( (long)FIRST_CHAR ) );
    this->GetDictionary().AddKey("LastChar", PdfVariant( (long)LAST_CHAR ) );
    this->GetDictionary().AddKey("Encoding", PdfName("WinAnsiEncoding") );
    this->GetDictionary().AddKey("Widths", pWidth->Reference() );
    this->GetDictionary().AddKey( "FontDescriptor", pDescriptor->Reference() );

    m_pMetrics->GetBoundingBox( array );

    pDescriptor->GetDictionary().AddKey( "FontName", m_BaseFont );
    //pDescriptor->GetDictionary().AddKey( "FontWeight", (long)m_pMetrics->Weight() );
    pDescriptor->GetDictionary().AddKey( PdfName::KeyFlags, PdfVariant( (long)32 ) ); // TODO: 0 ????
    pDescriptor->GetDictionary().AddKey( "FontBBox", array );
    pDescriptor->GetDictionary().AddKey( "ItalicAngle", PdfVariant( (long)m_pMetrics->ItalicAngle() ) );
    pDescriptor->GetDictionary().AddKey( "Ascent", m_pMetrics->Ascent() );
    pDescriptor->GetDictionary().AddKey( "Descent", m_pMetrics->Descent() );
    pDescriptor->GetDictionary().AddKey( "CapHeight", m_pMetrics->Ascent() ); // //m_pMetrics->CapHeight() );
    pDescriptor->GetDictionary().AddKey( "StemV", PdfVariant( (long)1 ) ); //m_pMetrics->StemV() );

    if( bEmbedd )
    {
        EmbeddFont( pParent, pDescriptor );
    }
}

void PdfFont::EmbeddFont( PdfVecObjects* pParent, PdfObject* pDescriptor )
{
    PdfObject* pContents;
    FILE*      hFile;
    char*      pBuffer = NULL;
    long       lSize = 0;

    pContents = pParent->CreateObject();
    if( !pContents )
    {
        RAISE_ERROR( ePdfError_InvalidHandle );
    }

    pDescriptor->GetDictionary().AddKey( "FontFile2", pContents->Reference() );

    // if the data was loaded from memory - use it from there
    // otherwise, load from disk
    if ( m_pMetrics->FontDataLen() && m_pMetrics->FontData() ) {
        pBuffer = const_cast<char*>( m_pMetrics->FontData() );
        lSize = m_pMetrics->FontDataLen();
    } else {
        hFile = fopen( m_pMetrics->Filename(), "rb" );
        if( !hFile )
        {
            RAISE_ERROR( ePdfError_FileNotFound );
        }

        fseek( hFile, 0, SEEK_END );
        lSize = ftell( hFile );
        fseek( hFile, 0, SEEK_SET );
        
        pBuffer = (char*)malloc( sizeof(char) * lSize );
        fread( pBuffer, lSize, sizeof(char), hFile ); 
        
        fclose( hFile );
    }
    
    pContents->GetDictionary().AddKey( "Length1", PdfVariant( lSize ) );
    pContents->Stream()->Set( pBuffer, lSize, !m_pMetrics->FontDataLen() );	// if we loaded from memory, DO NOT let Stream take possession
}

void PdfFont::SetFontSize( float fSize )
{
    m_pMetrics->SetFontSize( fSize );

    m_fFontSize = fSize;
}


};

