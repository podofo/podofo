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

    m_pWidget->SetBorderStyle( 0.0, 0.0, 5.0 );
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

PdfObject* PdfField::GetAppearanceCharacteristics( bool bCreate ) const
{
    PdfObject* pMK = NULL;

    if( !m_pObject->GetDictionary().HasKey( PdfName("MK") ) && bCreate )
    {
        PdfDictionary dictionary;
        const_cast<PdfField*>(this)->m_pObject->GetDictionary().AddKey( PdfName("MK"), dictionary );
    }

    pMK = m_pObject->GetDictionary().GetKey( PdfName("MK") );

    return pMK;
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

void PdfField::SetHighlightingMode( EPdfHighlightingMode eMode )
{
    PdfName value;

    switch( eMode ) 
    {
        case ePdfHighlightingMode_None:
            value = PdfName("N");
            break;
        case ePdfHighlightingMode_Invert:
            value = PdfName("I");
            break;
        case ePdfHighlightingMode_InvertOutline:
            value = PdfName("O");
            break;
        case ePdfHighlightingMode_Push:
            value = PdfName("P");
            break;
        case ePdfHighlightingMode_Unknown:
        default:
            PODOFO_RAISE_ERROR( ePdfError_InvalidName );
            break;
    }

    m_pObject->GetDictionary().AddKey( PdfName("H"), value );
}

EPdfHighlightingMode PdfField::GetHighlightingMode() const
{
    EPdfHighlightingMode eMode = ePdfHighlightingMode_Invert;

    if( m_pObject->GetDictionary().HasKey( PdfName("H") ) )
    {
        PdfName value = m_pObject->GetDictionary().GetKey( PdfName("H") )->GetName();
        if( value == PdfName("N") )
            return ePdfHighlightingMode_None;
        else if( value == PdfName("I") )
            return ePdfHighlightingMode_Invert;
        else if( value == PdfName("O") )
            return ePdfHighlightingMode_InvertOutline;
        else if( value == PdfName("P") )
            return ePdfHighlightingMode_Push;
    }

    return eMode;
}

void PdfField::SetBorderColorTransparent()
{
    PdfArray array;

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BC"), array );
}

void PdfField::SetBorderColor( double dGray )
{
    PdfArray array;
    array.push_back( dGray );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BC"), array );
}

void PdfField::SetBorderColor( double dRed, double dGreen, double dBlue )
{
    PdfArray array;
    array.push_back( dRed );
    array.push_back( dGreen );
    array.push_back( dBlue );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BC"), array );
}

void PdfField::SetBorderColor( double dCyan, double dMagenta, double dYellow, double dBlack )
{
    PdfArray array;
    array.push_back( dCyan );
    array.push_back( dMagenta );
    array.push_back( dYellow );
    array.push_back( dBlack );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BC"), array );
}

void PdfField::SetBackgroundColorTransparent()
{
    PdfArray array;

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BG"), array );
}

void PdfField::SetBackgroundColor( double dGray )
{
    PdfArray array;
    array.push_back( dGray );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BG"), array );
}

void PdfField::SetBackgroundColor( double dRed, double dGreen, double dBlue )
{
    PdfArray array;
    array.push_back( dRed );
    array.push_back( dGreen );
    array.push_back( dBlue );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BG"), array );
}

void PdfField::SetBackgroundColor( double dCyan, double dMagenta, double dYellow, double dBlack )
{
    PdfArray array;
    array.push_back( dCyan );
    array.push_back( dMagenta );
    array.push_back( dYellow );
    array.push_back( dBlack );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BG"), array );
}

void PdfField::SetFieldName( const PdfString & rsName )
{
    m_pObject->GetDictionary().AddKey( PdfName("T"), rsName );
}

PdfString PdfField::GetFieldName() const
{
    if( m_pObject->GetDictionary().HasKey( PdfName("T" ) ) )
        return m_pObject->GetDictionary().GetKey( PdfName("T" ) )->GetString();

    return PdfString::StringNull;
}

void PdfField::SetAlternateName( const PdfString & rsName )
{
    m_pObject->GetDictionary().AddKey( PdfName("TU"), rsName );
}

PdfString PdfField::GetAlternateName() const
{
    if( m_pObject->GetDictionary().HasKey( PdfName("TU" ) ) )
        return m_pObject->GetDictionary().GetKey( PdfName("TU" ) )->GetString();

    return PdfString::StringNull;
}

void PdfField::SetMappingName( const PdfString & rsName )
{
    m_pObject->GetDictionary().AddKey( PdfName("TM"), rsName );
}

PdfString PdfField::GetMappingName() const
{
    if( m_pObject->GetDictionary().HasKey( PdfName("TM" ) ) )
        return m_pObject->GetDictionary().GetKey( PdfName("TM" ) )->GetString();

    return PdfString::StringNull;
}
void PdfField::AddAlternativeAction( const PdfName & rsName, const PdfAction & rAction ) 
{
    if( !m_pObject->GetDictionary().HasKey( PdfName("AA") ) ) 
        m_pObject->GetDictionary().AddKey( PdfName("AA"), PdfDictionary() );

    PdfObject* pAA = m_pObject->GetDictionary().GetKey( PdfName("AA") );
    pAA->GetDictionary().AddKey( rsName, rAction.GetObject()->Reference() );
}

/////////////////////////////////////////////////////////////////////////////

PdfButton::PdfButton( PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : PdfField( ePdfField_Button, pWidget, pParent )
{
}

PdfButton::PdfButton( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : PdfField( ePdfField_Button, pPage, rRect, pParent )
{
}

PdfButton::PdfButton( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : PdfField( ePdfField_Button, pPage, rRect, pDoc )
{
}

void PdfButton::SetCaption( const PdfString & rsText )
{
    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("CA"), rsText );
}

const PdfString PdfButton::GetCaption() const
{
    PdfObject* pMK = this->GetAppearanceCharacteristics( false );
    
    if( pMK && pMK->GetDictionary().HasKey( PdfName("CA") ) )
        return pMK->GetDictionary().GetKey( PdfName("CA") )->GetString();

    return PdfString::StringNull;
}

/////////////////////////////////////////////////////////////////////////////

PdfPushButton::PdfPushButton( PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : PdfButton( pWidget, pParent )
{
    Init();
}

PdfPushButton::PdfPushButton( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : PdfButton( pPage, rRect, pParent )
{
    Init();
}

PdfPushButton::PdfPushButton( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : PdfButton( pPage, rRect, pDoc )
{
    Init();
}


void PdfPushButton::Init() 
{
    // make a push button
    this->SetFieldFlag( static_cast<int>(ePdfButton_PushButton), true );
    //m_pWidget->SetFlags( 4 );

    /*
    m_pObject->GetDictionary().AddKey( PdfName("H"), PdfName("I") );
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
    */
}

void PdfPushButton::SetRolloverCaption( const PdfString & rsText )
{
    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("RC"), rsText );
}

const PdfString PdfPushButton::GetRolloverCaption() const
{
    PdfObject* pMK = this->GetAppearanceCharacteristics( false );
    
    if( pMK && pMK->GetDictionary().HasKey( PdfName("RC") ) )
        return pMK->GetDictionary().GetKey( PdfName("RC") )->GetString();

    return PdfString::StringNull;
}

void PdfPushButton::SetAlternateCaption( const PdfString & rsText )
{
    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("AC"), rsText );

}

const PdfString PdfPushButton::GetAlternateCaption() const
{
    PdfObject* pMK = this->GetAppearanceCharacteristics( false );
    
    if( pMK && pMK->GetDictionary().HasKey( PdfName("AC") ) )
        return pMK->GetDictionary().GetKey( PdfName("AC") )->GetString();

    return PdfString::StringNull;
}

/////////////////////////////////////////////////////////////////////////////

PdfCheckBox::PdfCheckBox( PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : PdfButton( pWidget, pParent )
{
    Init();
}

PdfCheckBox::PdfCheckBox( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : PdfButton( pPage, rRect, pParent )
{
    Init();
}

PdfCheckBox::PdfCheckBox( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : PdfButton( pPage, rRect, pDoc )
{
    Init();
}

void PdfCheckBox::Init()
{

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

/////////////////////////////////////////////////////////////////////////////

PdfListField::PdfListField( PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : PdfField( ePdfField_Choice, pWidget, pParent )
{

}

PdfListField::PdfListField( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : PdfField( ePdfField_Choice, pPage, rRect, pParent )
{

}

PdfListField::PdfListField( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : PdfField( ePdfField_Choice, pPage, rRect, pDoc )
{

}

void PdfListField::InsertItem( const PdfString & rsValue, const PdfString & rsDisplayName )
{
    PdfVariant var;
    PdfArray   opt;

    if( rsDisplayName == PdfString::StringNull ) 
        var = rsValue;
    else
    {
        PdfArray array;
        array.push_back( rsValue );
        array.push_back( rsDisplayName );

        var = array;
    }

    if( m_pObject->GetDictionary().HasKey( PdfName("Opt") ) )
        opt = m_pObject->GetDictionary().GetKey( PdfName("Opt") )->GetArray();

    // TODO: Sorting
    opt.push_back( var );
    m_pObject->GetDictionary().AddKey( PdfName("Opt"), opt );

    /*
    m_pObject->GetDictionary().AddKey( PdfName("V"), rsValue );

    PdfArray array;
    array.push_back( 0L );
    m_pObject->GetDictionary().AddKey( PdfName("I"), array );
    */
}

void PdfListField::RemoveItem( int nIndex )
{
    PdfArray   opt;

    if( m_pObject->GetDictionary().HasKey( PdfName("Opt") ) )
        opt = m_pObject->GetDictionary().GetKey( PdfName("Opt") )->GetArray();
    
    if( nIndex < 0 || nIndex > static_cast<int>(opt.size()) )
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    opt.erase( opt.begin() + nIndex );
    m_pObject->GetDictionary().AddKey( PdfName("Opt"), opt );
}

const PdfString & PdfListField::GetItem( int nIndex ) const
{
    PdfArray   opt;
    
    if( m_pObject->GetDictionary().HasKey( PdfName("Opt") ) )
        opt = m_pObject->GetDictionary().GetKey( PdfName("Opt") )->GetArray();
    
    if( nIndex < 0 || nIndex > static_cast<int>(opt.size()) )
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    PdfVariant var = opt[nIndex];
    if( var.IsArray() ) 
    {
        if( var.GetArray().size() < 2 ) 
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
        }
        else
            return var.GetArray()[0].GetString();
    }

    return var.GetString();
}

const PdfString & PdfListField::GetItemDisplayText( int nIndex ) const
{
    PdfArray   opt;
    
    if( m_pObject->GetDictionary().HasKey( PdfName("Opt") ) )
        opt = m_pObject->GetDictionary().GetKey( PdfName("Opt") )->GetArray();
    
    if( nIndex < 0 || nIndex > static_cast<int>(opt.size()) )
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    PdfVariant var = opt[nIndex];
    if( var.IsArray() ) 
    {
        if( var.GetArray().size() < 2 ) 
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
        }
        else
            return var.GetArray()[1].GetString();
    }

    return var.GetString();
}

int PdfListField::GetItemCount() const
{
    PdfArray   opt;
    
    if( m_pObject->GetDictionary().HasKey( PdfName("Opt") ) )
        opt = m_pObject->GetDictionary().GetKey( PdfName("Opt") )->GetArray();
    
    return opt.size();
}

void PdfListField::SetSelectedItem( int nIndex )
{
    PdfString selected = this->GetItem( nIndex );

    m_pObject->GetDictionary().AddKey( PdfName("V"), selected );
}

int PdfListField::GetSelectedItem() const
{
    if( m_pObject->GetDictionary().HasKey( PdfName("V") ) )
    {
        PdfObject* pValue = m_pObject->GetDictionary().GetKey( PdfName("V") );
        if( pValue->IsString() )
        {
            PdfString value = pValue->GetString();
            for( int i=0;i<this->GetItemCount();i++ ) 
            {
                if( this->GetItem( i ) == value )
                    return i;
            }
        }
    }

    return -1;
}


/////////////////////////////////////////////////////////////////////////////

PdfComboBox::PdfComboBox( PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : PdfListField( pWidget, pParent )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Combo), true );        
    m_pWidget->SetBorderStyle( 0.0, 0.0, 1.0 );
}

PdfComboBox::PdfComboBox( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : PdfListField( pPage, rRect, pParent )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Combo), true );        
    m_pWidget->SetBorderStyle( 0.0, 0.0, 1.0 );
}

PdfComboBox::PdfComboBox( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : PdfListField( pPage, rRect, pDoc )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Combo), true );        
    m_pWidget->SetBorderStyle( 0.0, 0.0, 1.0 );
}

/////////////////////////////////////////////////////////////////////////////

PdfListBox::PdfListBox( PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : PdfListField( pWidget, pParent )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Combo), false );        
    m_pWidget->SetBorderStyle( 0.0, 0.0, 1.0 );
}

PdfListBox::PdfListBox( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : PdfListField( pPage, rRect, pParent )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Combo), false );        
    m_pWidget->SetBorderStyle( 0.0, 0.0, 1.0 );
}

PdfListBox::PdfListBox( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : PdfListField( pPage, rRect, pDoc )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Combo), false );        
    m_pWidget->SetBorderStyle( 0.0, 0.0, 1.0 );
}

};
