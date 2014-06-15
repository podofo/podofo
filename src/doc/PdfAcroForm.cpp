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

#include "PdfAcroForm.h"

#include "base/PdfDefinesPrivate.h"
#include "base/PdfArray.h" 
#include "base/PdfDictionary.h"

#include "PdfDocument.h"
#include "PdfFont.h"

#include <sstream>

namespace PoDoFo {

/*
  We use NULL for the PdfElement name, since the AcroForm dict
  does NOT have a /Type key!
*/
PdfAcroForm::PdfAcroForm( PdfDocument* pDoc, EPdfAcroFormDefaulAppearance eDefaultAppearance )
    : PdfElement( NULL, pDoc ), m_pDocument( pDoc )
{
    // Initialize with an empty fields array
    this->GetObject()->GetDictionary().AddKey( PdfName("Fields"), PdfArray() );

    Init( eDefaultAppearance );
}

PdfAcroForm::PdfAcroForm( PdfDocument* pDoc, PdfObject* pObject, EPdfAcroFormDefaulAppearance eDefaultAppearance )
    : PdfElement( NULL, pObject ), m_pDocument( pDoc )
{
    Init( eDefaultAppearance );
}

void PdfAcroForm::Init( EPdfAcroFormDefaulAppearance eDefaultAppearance )
{
    // Add default appearance: black text, 12pt times 
    // -> only if we do not have a DA key yet

    // Peter Petrov 27 April 2008
    //this->GetObject()->GetDictionary().AddKey( PdfName("NeedAppearances"), PdfVariant(true) );

    if( !this->GetObject()->GetDictionary().HasKey("DA") && 
        eDefaultAppearance == ePdfAcroFormDefaultAppearance_BlackText12pt )
    {
        PdfFont* pFont = m_pDocument->CreateFont( "Helvetica", false,
                                                 PdfEncodingFactory::GlobalWinAnsiEncodingInstance(),
                                                 PdfFontCache::eFontCreationFlags_AutoSelectBase14,
                                                 false );
        //PdfFont* pFont = m_pDocument->CreateFont( "Arial" );
        PdfObject* pResource;
        PdfObject* pFontDict;
        
        // Create DR key
        if( !this->GetObject()->GetDictionary().HasKey( PdfName("DR") ) )
            this->GetObject()->GetDictionary().AddKey( PdfName("DR"), PdfDictionary() );
        pResource = this->GetObject()->GetDictionary().GetKey( PdfName("DR") );
        
        if( !pResource->GetDictionary().HasKey( PdfName("Font") ) )
            pResource->GetDictionary().AddKey( PdfName("Font"), PdfDictionary() );
        pFontDict = pResource->GetDictionary().GetKey( PdfName("Font") );
        
        pFontDict->GetDictionary().AddKey( pFont->GetIdentifier(), pFont->GetObject()->Reference() );
        
        // Create DA key
        std::ostringstream oss;
        PdfLocaleImbue(oss);
        oss << "0 0 0 rg /" << pFont->GetIdentifier().GetName() << " 12 Tf";
        this->GetObject()->GetDictionary().AddKey( PdfName("DA"), PdfString( oss.str() ) );
    }
}

/*
int PdfAcroForm::GetCount()
{
    PdfObject* pFields = this->GetObject()->GetDictionary().GetKey( PdfName("Fields") );
    if( pFields ) 
    {
        return pFields->GetArray().size();
    }
    else
    {
        PODOFO_RAISE_ERROR( ePdfError_NoObject );
    }
}
*/

void PdfAcroForm::SetNeedAppearances( bool bNeedAppearances )
{
    this->GetObject()->GetDictionary().AddKey( PdfName("NeedAppearances"), PdfVariant(bNeedAppearances) );    
}

bool PdfAcroForm::GetNeedAppearances() const
{
    return this->GetObject()->GetDictionary().GetKeyAsBool( PdfName("NeedAppearances"), false );
}

};
