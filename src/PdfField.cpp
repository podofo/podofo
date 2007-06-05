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

#include "PdfField.h"

#include "PdfAcroForm.h"

#include "PdfAnnotation.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfDocument.h"
#include "PdfPainter.h"
#include "PdfPage.h"
#include "PdfXObject.h"

namespace PoDoFo {

PdfField::PdfField( EPdfField eField, PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : m_pObject( pWidget->GetObject() ), m_pWidget( pWidget ), m_eField( eField )
{
    Init( pParent );
}

PdfField::PdfField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : m_eField( eField )
{
    m_pWidget = pPage->CreateAnnotation( ePdfAnnotation_Widget, rRect );
    m_pObject = m_pWidget->GetObject();

    Init( pParent );
}

PdfField::PdfField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : m_eField( eField )    
{
    m_pWidget = pPage->CreateAnnotation( ePdfAnnotation_Widget, rRect );
    m_pObject = m_pWidget->GetObject();

    Init( pDoc->GetAcroForm() );
}


void PdfField::Init( PdfAcroForm* pParent )
{
    // Insert into the parents kids array
    PdfObject* pFields = pParent->GetObject()->GetDictionary().GetKey( PdfName("Fields") );
    if( pFields ) 
    {
        pFields->GetArray().push_back( m_pObject->Reference() );
    }
    else
    {
        PODOFO_RAISE_ERROR( ePdfError_NoObject );
    }

    switch( m_eField ) 
    {
        case ePdfField_Button:
            m_pObject->GetDictionary().AddKey( PdfName("FT"), PdfName("Btn") );
            break;
        case ePdfField_Text:
            m_pObject->GetDictionary().AddKey( PdfName("FT"), PdfName("Tx") );
            break;
        case ePdfField_Choice:
            m_pObject->GetDictionary().AddKey( PdfName("FT"), PdfName("Ch") );
            break;
        case ePdfField_Signature:
            m_pObject->GetDictionary().AddKey( PdfName("FT"), PdfName("Sig") );
            break;


        case ePdfField_Unknown:
        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
        }
        break;
    }

    // Add default appearance: black text, 12pt times 
    m_pObject->GetDictionary().AddKey( PdfName("DA"), PdfString("0 0 0 rg /Ti 12 Tf") );
}

PdfField::PdfField( PdfObject* pObject )
    : m_pObject( pObject ), m_eField( ePdfField_Unknown )
{

    PdfName fieldType = m_pObject->GetDictionary().GetKey( PdfName("FT") )->GetName();

    if( fieldType == PdfName("Btn") )
        m_eField = ePdfField_Button;
    else if( fieldType == PdfName("Tx") )
        m_eField = ePdfField_Text;
    else if( fieldType == PdfName("Ch") )
        m_eField = ePdfField_Choice;
    else if( fieldType == PdfName("Sig") )
        m_eField = ePdfField_Signature;
}

void PdfField::SetFieldFlag( long lValue, bool bSet )
{
    long lCur = 0;

    if( m_pObject->GetDictionary().HasKey( PdfName("Ff") ) )
        lCur = m_pObject->GetDictionary().GetKey( PdfName("Ff") )->GetNumber();
    
    if( bSet )
        lCur |= lValue;
    else
    {
        if( (lCur & lValue) == lValue )
            lCur ^= lValue;
    }

    m_pObject->GetDictionary().AddKey( PdfName("Ff"), lCur );
}

bool PdfField::GetFieldFlag( long lValue, bool bDefault ) const
{
    long lCur = 0;

    if( m_pObject->GetDictionary().HasKey( PdfName("Ff") ) )
    {
        lCur = m_pObject->GetDictionary().GetKey( PdfName("Ff") )->GetNumber();

        return (lCur & lValue) == lValue; 
    }
    
    return bDefault;
}

void PdfField::SetFieldName( const PdfString & rsName )
{
    m_pObject->GetDictionary().AddKey( PdfName("T"), rsName );
}

void PdfField::SetAlternateName( const PdfString & rsName )
{
    m_pObject->GetDictionary().AddKey( PdfName("TU"), rsName );
}

void PdfField::SetMappingName( const PdfString & rsName )
{
    m_pObject->GetDictionary().AddKey( PdfName("TM"), rsName );
}

void PdfField::AddAlternativeAction( const PdfName & rsName, const PdfAction & rAction ) 
{
    if( !m_pObject->GetDictionary().HasKey( PdfName("AA") ) ) 
        m_pObject->GetDictionary().AddKey( PdfName("AA"), PdfDictionary() );

    PdfObject* pAA = m_pObject->GetDictionary().GetKey( PdfName("AA") );
    pAA->GetDictionary().AddKey( rsName, rAction.GetObject()->Reference() );
}

/////////////////////////////////////////////////////////////////////////////

PdfPushButton::PdfPushButton( PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : PdfField( ePdfField_Button, pWidget, pParent )
{
    Init();
}

PdfPushButton::PdfPushButton( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : PdfField( ePdfField_Button, pPage, rRect, pParent )
{
    Init();
}

PdfPushButton::PdfPushButton( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : PdfField( ePdfField_Button, pPage, rRect, pDoc )
{
    Init();
}


void PdfPushButton::Init() 
{
    // make a push button
    //m_pObject->GetDictionary().AddKey( PdfName("Ff"), 131072L );
    //m_pObject->GetDictionary().AddKey( PdfName("Ff"), 4L );

    m_pWidget->SetFlags( 4 );

    if( !m_pWidget->HasAppearanceStream() )
    {
        // Create the default appearance stream
        PdfRect    rect( 0.0, 0.0, m_pWidget->GetRect().GetWidth(), m_pWidget->GetRect().GetHeight() );
        PdfXObject xObjOff( rect, m_pObject->GetOwner() );
        PdfXObject xObjYes( rect, m_pObject->GetOwner() );
        PdfPainter painter;
        
        painter.SetPage( &xObjOff );
        painter.SetColor( 0.5, 0.5, 0.5 );
        painter.FillRect( 0, xObjOff.GetPageSize().GetHeight(), xObjOff.GetPageSize().GetWidth(), xObjOff.GetPageSize().GetHeight()  );
        painter.FinishPage();

        painter.SetPage( &xObjYes );
        painter.SetColor( 1.0, 0.0, 0.0 );
        painter.FillRect( 0, xObjYes.GetPageSize().GetHeight(), xObjYes.GetPageSize().GetWidth(), xObjYes.GetPageSize().GetHeight()  );
        painter.FinishPage();


        PdfDictionary dict;
        PdfDictionary internal;

        internal.AddKey( "On", xObjYes.GetObject()->Reference() );
        internal.AddKey( "Off", xObjOff.GetObject()->Reference() );
    
        dict.AddKey( "N", internal );

        m_pWidget->GetObject()->GetDictionary().AddKey( "AP", dict );
        m_pWidget->GetObject()->GetDictionary().AddKey( "AS", PdfName("Off") );

        //pWidget->SetAppearanceStream( &xObj );
   }
}

void PdfPushButton::SetText( const PdfString & rsText )
{
    PdfObject* pMK = NULL;

    if( !m_pObject->GetDictionary().HasKey( PdfName("MK") ) )
    {
        PdfDictionary dictionary;
        m_pObject->GetDictionary().AddKey( PdfName("MK"), dictionary );
    }

    pMK = m_pObject->GetDictionary().GetKey( PdfName("MK") );
    pMK->GetDictionary().AddKey( PdfName("CA"), rsText );

    PdfArray color;
    color.push_back( 0.0 );
    color.push_back( 1.0 );
    color.push_back( 0.0 );

    pMK->GetDictionary().AddKey( PdfName("BG"), color );
}

/////////////////////////////////////////////////////////////////////////////

PdfTextField::PdfTextField( PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : PdfField( ePdfField_Text, pWidget, pParent )
{
    Init();
}

PdfTextField::PdfTextField( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : PdfField( ePdfField_Text, pPage, rRect, pParent )
{
    Init();
}

PdfTextField::PdfTextField( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : PdfField( ePdfField_Text, pPage, rRect, pDoc )
{
    Init();
}

void PdfTextField::Init()
{
    m_pWidget->SetBorderStyle( 0.0, 0.0, 10.0 );
    m_pObject->GetDictionary().AddKey( PdfName("Ff"), 8192L ); // Pasword
    m_pObject->GetDictionary().AddKey( PdfName("Ff"), 4096L ); // Multiline


    this->SetText("PoDoFo Text Field");
}

void PdfTextField::SetText( const PdfString & rsText )
{
    m_pObject->GetDictionary().AddKey( PdfName("V"), rsText );
}

PdfString PdfTextField::GetText() const
{
    PdfString str;

    if( m_pObject->GetDictionary().HasKey( PdfName("V") ) )
        str = m_pObject->GetDictionary().GetKey( PdfName("V") )->GetString();

    return str;
}

void PdfTextField::SetMaxLen( int nMaxLen )
{
    m_pObject->GetDictionary().AddKey( PdfName("MaxLen"), static_cast<long>(nMaxLen) );
}

int PdfTextField::GetMaxLen() const
{
    return m_pObject->GetDictionary().HasKey( PdfName("MaxLen") ) ? 
        m_pObject->GetDictionary().GetKey( PdfName("MaxLen") )->GetNumber() : -1;
}

};
