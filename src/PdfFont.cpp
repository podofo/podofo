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

#include <sstream>

#define FIRST_CHAR   0
#define LAST_CHAR  255

using namespace std;

namespace PoDoFo {

PdfFont::PdfFont( PdfFontMetrics* pMetrics, bool bEmbed, bool bBold, bool bItalic, 
                  PdfVecObjects* pParent )
    : PdfElement( "Font", pParent ), m_pMetrics( pMetrics ), m_bBold( bBold ), m_bItalic( bItalic )
{
    this->InitVars();
    this->Init( bEmbed );
}

PdfFont::PdfFont( PdfFontMetrics* pMetrics, bool bBold, bool bItalic,
                  std::vector<int> vecUnicodeCodePoints, PdfVecObjects* pParent )
    : PdfElement( "Font", pParent ), m_pMetrics( pMetrics ), m_bBold( bBold ), m_bItalic( bItalic )
{
    this->InitVars();
    // prefix the base font name with 6 random characters
    // which are not so random at the moment
    std::string sTmp = m_BaseFont.GetName();
    sTmp = "ABCDEF+" + sTmp;
    m_BaseFont = PdfName( sTmp.c_str() );

    this->Init( true );
}
    
PdfFont::~PdfFont()
{
    delete m_pMetrics;
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
    for(int i = 0; i < sTmp.size(); i++)
    {
        if(sTmp[i] != ' ')
            sTmp[curPos++] = sTmp[i];
    }
    sTmp.resize(curPos);
    m_BaseFont = PdfName( sTmp.c_str() );
}

void PdfFont::Init( bool bEmbed )
{
    PdfObject*    pWidth;
    PdfObject*    pDescriptor;
    PdfVariant    var;
    PdfArray      array;

    pWidth = m_pObject->GetOwner()->CreateObject();
    if( !pWidth )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_pMetrics->GetWidthArray( *pWidth, FIRST_CHAR, LAST_CHAR );

    pDescriptor = m_pObject->GetOwner()->CreateObject( "FontDescriptor" );
    if( !pDescriptor )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    EPdfFontType eType = m_pMetrics->GetFontType();
    switch( eType ) 
    {
        case ePdfFontType_TrueType:
            m_pObject->GetDictionary().AddKey( PdfName::KeySubtype, PdfName("TrueType") );
            break;
        case ePdfFontType_Type1Pfa:
        case ePdfFontType_Type1Pfb:
            m_pObject->GetDictionary().AddKey( PdfName::KeySubtype, PdfName("Type1") );
            break;
        case ePdfFontType_Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO( ePdfError_UnsupportedFontFormat, m_pMetrics->GetFilename() );
    }

    m_pObject->GetDictionary().AddKey("BaseFont", m_BaseFont );
    m_pObject->GetDictionary().AddKey("FirstChar", PdfVariant( static_cast<long>(FIRST_CHAR) ) );
    m_pObject->GetDictionary().AddKey("LastChar", PdfVariant( static_cast<long>(LAST_CHAR) ) );
    m_pObject->GetDictionary().AddKey("Encoding", PdfName("WinAnsiEncoding") );
    m_pObject->GetDictionary().AddKey("Widths", pWidth->Reference() );
    m_pObject->GetDictionary().AddKey( "FontDescriptor", pDescriptor->Reference() );

    m_pMetrics->GetBoundingBox( array );

    pDescriptor->GetDictionary().AddKey( "FontName", m_BaseFont );
    //pDescriptor->GetDictionary().AddKey( "FontWeight", (long)m_pMetrics->Weight() );
    pDescriptor->GetDictionary().AddKey( PdfName::KeyFlags, PdfVariant( 32L ) ); // TODO: 0 ????
    pDescriptor->GetDictionary().AddKey( "FontBBox", array );
    pDescriptor->GetDictionary().AddKey( "ItalicAngle", PdfVariant( static_cast<long>(m_pMetrics->GetItalicAngle()) ) );
    pDescriptor->GetDictionary().AddKey( "Ascent", m_pMetrics->GetPdfAscent() );
    pDescriptor->GetDictionary().AddKey( "Descent", m_pMetrics->GetPdfDescent() );
    pDescriptor->GetDictionary().AddKey( "CapHeight", m_pMetrics->GetPdfAscent() ); // //m_pMetrics->CapHeight() );
    pDescriptor->GetDictionary().AddKey( "StemV", PdfVariant( 1L ) ); //m_pMetrics->StemV() );

    if( bEmbed )
    {
        EmbedFont( pDescriptor );
    }
}

void PdfFont::InitSubset()
{
    

}

void PdfFont::EmbedFont( PdfObject* pDescriptor )
{
    EPdfFontType eType = m_pMetrics->GetFontType();

    switch( eType ) 
    {
        case ePdfFontType_TrueType:
            EmbedTrueTypeFont( pDescriptor );
            break;
        case ePdfFontType_Type1Pfa:
        case ePdfFontType_Type1Pfb:
            EmbedType1Font( pDescriptor );
            break;
        case ePdfFontType_Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO( ePdfError_UnsupportedFontFormat, m_pMetrics->GetFilename() );
    }
}

void PdfFont::EmbedTrueTypeFont( PdfObject* pDescriptor )
{
    PdfObject* pContents;
    long       lSize = 0;
        
    pContents = m_pObject->GetOwner()->CreateObject();
    if( !pContents )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
        
    pDescriptor->GetDictionary().AddKey( "FontFile2", pContents->Reference() );
        
    // if the data was loaded from memory - use it from there
    // otherwise, load from disk
    if ( m_pMetrics->GetFontDataLen() && m_pMetrics->GetFontData() ) 
    {
        // FIXME const_cast<char*> is dangerous if string literals may ever be passed
        char* pBuffer = const_cast<char*>( m_pMetrics->GetFontData() );
        lSize = m_pMetrics->GetFontDataLen();
            
        pContents->GetStream()->Set( pBuffer, lSize );
    } 
    else 
    {
        PdfFileInputStream stream( m_pMetrics->GetFilename() );
        pContents->GetStream()->Set( &stream );
            
        lSize = stream.GetFileLength();
    }
        
    pContents->GetDictionary().AddKey( "Length1", PdfVariant( lSize ) );
}

void PdfFont::EmbedType1Font( PdfObject* pDescriptor )
{
    long        lSize    = 0;
    long        lLength1 = 0L;
    long        lLength2 = 0L;
    long        lLength3 = 0L;
    PdfObject*  pContents;
    const char* pBuffer;
    char*       pAllocated = NULL;

    pContents = m_pObject->GetOwner()->CreateObject();
    if( !pContents )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
        
    pDescriptor->GetDictionary().AddKey( "FontFile", pContents->Reference() );

    // if the data was loaded from memory - use it from there
    // otherwise, load from disk
    if ( m_pMetrics->GetFontDataLen() && m_pMetrics->GetFontData() ) 
    {
        pBuffer = m_pMetrics->GetFontData();
        lSize   = m_pMetrics->GetFontDataLen();
    }
    else
    {
        FILE* hFile = fopen( m_pMetrics->GetFilename(), "rb" );
        if( !hFile )
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, m_pMetrics->GetFilename() );
        }

        fseek( hFile, 0L, SEEK_END );
        lSize = ftell( hFile );
        fseek( hFile, 0L, SEEK_SET );

        pAllocated = static_cast<char*>(malloc( sizeof(char) * lSize ));
        if( !pAllocated )
        {
            fclose( hFile );
            PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
        }

        fread( pAllocated, sizeof(char), lSize, hFile );
        fclose( hFile );

        pBuffer = pAllocated;
    }

    // Parse the font data buffer to get the values for length1, length2 and length3
    lLength1 = FindInBuffer( "eexec", pBuffer, lSize );
    if( lLength1 > 0 )
        lLength1 += 6; // 6 == eexec + lf
    else
        lLength1 = 0;
    
    if( lLength1 )
    {
        lLength2 = FindInBuffer( "cleartomark", pBuffer, lSize );
        if( lLength2 > 0 )
            lLength2 = lSize - lLength1 - 520; // 520 == 512 + strlen(cleartomark)
        else
            lLength1 = 0;
    }
    
    lLength3 = lSize - lLength2 - lLength1;
    
    // TODO: Pdf Supports only Type1 fonts with binary encrypted sections and not the hex format
    pContents->GetStream()->Set( pBuffer, lSize );
    if( pAllocated )
        free( pAllocated );

    pContents->GetDictionary().AddKey( "Length1", PdfVariant( lLength1 ) );
    pContents->GetDictionary().AddKey( "Length2", PdfVariant( lLength2 ) );
    pContents->GetDictionary().AddKey( "Length3", PdfVariant( lLength3 ) );
}

void PdfFont::SetFontSize( float fSize )
{
    m_pMetrics->SetFontSize( fSize );
}

void PdfFont::SetFontScale( float fScale )
{
    m_pMetrics->SetFontScale( fScale );
}

void PdfFont::SetFontCharSpace( float fCharSpace )
{
    m_pMetrics->SetFontCharSpace( fCharSpace );
}

long PdfFont::FindInBuffer( const char* pszNeedle, const char* pszHaystack, long lLen )
{
    // if lNeedleLen is 0 the while loop will not be executed and we return -1
    long lNeedleLen      = pszNeedle ? strlen( pszNeedle ) : 0; 
    const char* pszEnd   = pszHaystack + lLen - lNeedleLen; 
    const char* pszStart = pszHaystack;

    while( pszHaystack < pszEnd ) 
    {
        if( strncmp( pszHaystack, pszNeedle, lNeedleLen ) == 0 )
            return pszHaystack - pszStart;

        ++pszHaystack;
    }

    return -1;
}

};

