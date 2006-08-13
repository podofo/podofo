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

#include "PdfSimpleWriter.h"

#include "PdfDictionary.h"
#include "PdfDate.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfObject.h"
#include "PdfPage.h"
#include "PdfImage.h"

#ifndef _WIN32
#include <fontconfig.h>
#endif

#include <algorithm>
#include <sstream>

namespace PoDoFo {

using namespace std;

class FontComperator { 
public:
    FontComperator( const string & sPath )
        {
            m_sPath = sPath;
        }
    
    bool operator()(const PdfFont* pFont) 
        { 
            return (m_sPath == pFont->FontMetrics()->Filename());
        }
private:
    string m_sPath;
};


PdfSimpleWriter::PdfSimpleWriter()
{
    m_pPageTree     = NULL;
    m_nPageTreeSize = 0;
    m_bInitDone     = false;

#ifndef _WIN32
    m_pFcConfig     = (void*)FcInitLoadConfigAndFonts();
#endif

    this->Init();
}    

PdfSimpleWriter::~PdfSimpleWriter()
{
#ifndef _WIN32
    FcConfigDestroy( (FcConfig*)m_pFcConfig );
#endif

    if( m_bInitDone && m_ftLibrary ) 
    {    
        FT_Done_FreeType( m_ftLibrary );
        m_ftLibrary = NULL;
    }
}

void PdfSimpleWriter::Init()
{
    PdfDate   cDate;
    PdfString sDate;

    if( FT_Init_FreeType( &m_ftLibrary ) )
    {
        RAISE_ERROR( ePdfError_FreeType );
    }
    m_bInitDone = true;

    PdfWriter::Init();

    m_pPageTree = m_vecObjects.CreateObject( "Pages" );
    m_pPageTree->GetDictionary().AddKey( "Kids", PdfArray() );

    this->GetCatalog()->GetDictionary().AddKey( "Pages", m_pPageTree->Reference() );

    cDate.ToString( sDate );
    this->GetInfo()->GetDictionary().AddKey( "Producer", PdfString("PoDoFo") );
    this->GetInfo()->GetDictionary().AddKey( "CreationDate", sDate );
}

PdfPage* PdfSimpleWriter::CreatePage( const PdfRect & rSize )
{
    PdfPage* pPage = new PdfPage( rSize, &m_vecObjects );
    pPage->Object()->GetDictionary().AddKey( "Parent", m_pPageTree->Reference() );

    m_vecPageReferences.push_back( pPage->Object()->Reference() );

    m_pPageTree->GetDictionary().AddKey( "Count", PdfVariant( (long)++m_nPageTreeSize ) );
    m_pPageTree->GetDictionary().AddKey( "Kids",  m_vecPageReferences );

    return pPage;
}

void PdfSimpleWriter::SetDocumentAuthor( const PdfString & sAuthor )
{
    this->GetInfo()->GetDictionary().AddKey( "Author", sAuthor );
}

void PdfSimpleWriter::SetDocumentCreator( const PdfString & sCreator )
{
    this->GetInfo()->GetDictionary().AddKey( "Creator", sCreator );
}

void PdfSimpleWriter::SetDocumentKeywords( const PdfString & sKeywords )
{
    this->GetInfo()->GetDictionary().AddKey( "Keywords", sKeywords );
}

void PdfSimpleWriter::SetDocumentSubject( const PdfString & sSubject )
{
    this->GetInfo()->GetDictionary().AddKey( "Subject", sSubject );
}

void PdfSimpleWriter::SetDocumentTitle( const PdfString & sTitle )
{
    this->GetInfo()->GetDictionary().AddKey( "Title", sTitle );
}

}; 
