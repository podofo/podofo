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

#ifndef _PDF_FIELD_H_
#define _PDF_FIELD_H_

#include "podofo/base/PdfDefines.h"  
#include "podofo/base/PdfName.h"
#include "podofo/base/PdfString.h"

#include "PdfAnnotation.h"

namespace PoDoFo {

class PdfAcroForm;
class PdfAction;
class PdfAnnotation;
class PdfDocument;
class PdfObject;
class PdfPage;
class PdfRect;
class PdfReference;
class PdfStreamedDocument;

/** The type of PDF field
 */
enum EPdfField {
    ePdfField_PushButton, 
    ePdfField_CheckBox,
    ePdfField_RadioButton,
    ePdfField_TextField,
    ePdfField_ComboBox,
    ePdfField_ListBox,
    ePdfField_Signature,

    ePdfField_Unknown = 0xff
};

/** The possible highlighting modes
 *  for a PdfField. I.e the visual effect
 *  that is to be used when the mouse 
 *  button is pressed.
 *
 *  The default value is 
 *  ePdfHighlightingMode_Invert
 */
enum EPdfHighlightingMode {
    ePdfHighlightingMode_None,           ///< Do no highlighting
    ePdfHighlightingMode_Invert,         ///< Invert the PdfField
    ePdfHighlightingMode_InvertOutline,  ///< Invert the fields border
    ePdfHighlightingMode_Push,           ///< Display the fields down appearance (requires an additional appearance stream to be set)

    ePdfHighlightingMode_Unknown = 0xff
};

class PODOFO_DOC_API PdfField {
    enum { ePdfField_ReadOnly       = 0x0001,
           ePdfField_Required       = 0x0002,
           ePdfField_NoExport       = 0x0004
    };

 protected:
    /** Create a new PdfAcroForm dictionary object
     *  \param pParent parent of this action
     */
    PdfField( EPdfField eField, PdfAnnotation* pWidget, PdfAcroForm* pParent );

    PdfField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent );

    PdfField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc );

    PdfField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfStreamedDocument* pDoc );

    PdfField( EPdfField eField, PdfAnnotation* pWidget, PdfAcroForm* pParent, PdfDocument* pDoc);
    PdfField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc, bool bDefaultApperance);

    /** Create a copy of a PdfField object.
     *  Not the field on the page is copied - only the PdfField
     *  object referring to the field on the page is copied!
     *
     *  \param rhs the field to copy
     *  \returns this field
     */
    //inline virtual const PdfField & operator=( const PdfField & rhs );

    /** 
     *  Set a bit in the field flags value of the fields dictionary.
     *
     *  \param lValue the value specifying the bits to set
     *  \param bSet if true the value will be set otherwise
     *              they will be cleared.
     *
     *  \see GetFieldFlag
     */
    void SetFieldFlag( long lValue, bool bSet );

    /**
     *  \param lValue it is checked if these bits are set
     *  \param bDefault the returned value if no field flags are specified
     *
     *  \returns true if given bits are set in the field flags
     *
     *  \see SetFieldFlag
     */
    bool GetFieldFlag( long lValue, bool bDefault ) const;

    /**
     * \param bCreate create the dictionary if it does not exist
     *
     * \returns a pointer to the appearance characteristics dictionary
     *          of this object or NULL if it does not exists.
     */
    PdfObject* GetAppearanceCharacteristics( bool bCreate ) const;

 public:
    /** Create a PdfAcroForm dictionary object from an existing PdfObject
     *	\param pObject the object to create from
     *  \param pWidget the widget annotation of this field
     */
    PdfField( PdfObject* pObject, PdfAnnotation* pWidget );

    /** Create a copy of a PdfField object.
     *  Not the field on the page is copied - only the PdfField
     *  object referring to the field on the page is copied!
     *
     *  \param rhs the field to copy
     */
    PdfField( const PdfField & rhs );

    virtual ~PdfField() { }

    /** Get the page of this PdfField
     *
     *  \returns the page of this PdfField
     */
    inline PdfPage* GetPage() const;

    /** Set the highlighting mode which should be used when the user
     *  presses the mouse button over this widget.
     *
     *  \param eMode the highliting mode
     *
     *  The default value is ePdfHighlightingMode_Invert
     */
    void SetHighlightingMode( EPdfHighlightingMode eMode );

    /** 
     * \returns the highlighting mode to be used when the user
     *          presses the mouse button over this widget
     */
    EPdfHighlightingMode GetHighlightingMode() const;
   
    /**
     * Sets the border color of the field to be transparent
     */
    void SetBorderColorTransparent();

    /**
     * Sets the border color of the field
     *
     * \param dGray gray value of the color
     */
    void SetBorderColor( double dGray );

    /**
     * Sets the border color of the field
     *
     * \param dRed red
     * \param dGreen green
     * \param dBlue blue
     */
    void SetBorderColor( double dRed, double dGreen, double dBlue );

    /**
     * Sets the border color of the field
     *
     * \param dCyan cyan
     * \param dMagenta magenta
     * \param dYellow yellow
     * \param dBlack black
     */
    void SetBorderColor( double dCyan, double dMagenta, double dYellow, double dBlack );

    /**
     * Sets the background color of the field to be transparent
     */
    void SetBackgroundColorTransparent();

    /**
     * Sets the background color of the field
     *
     * \param dGray gray value of the color
     */
    void SetBackgroundColor( double dGray );

    /**
     * Sets the background color of the field
     *
     * \param dRed red
     * \param dGreen green
     * \param dBlue blue
     */
    void SetBackgroundColor( double dRed, double dGreen, double dBlue );

    /**
     * Sets the background color of the field
     *
     * \param dCyan cyan
     * \param dMagenta magenta
     * \param dYellow yellow
     * \param dBlack black
     */
    void SetBackgroundColor( double dCyan, double dMagenta, double dYellow, double dBlack );

    /** Sets the field name of this PdfField
     *
     *  PdfFields require a field name to work correctly in acrobat reader!
     *  This name can be used to access the field in JavaScript actions.
     *  
     *  \param rsName the field name of this pdf field
     */
    void SetFieldName( const PdfString & rsName );

    /** \returns the field name of this PdfField
     */
    PdfString GetFieldName() const;

    /**
     * Set the alternate name of this field which 
     * is used to display the fields name to the user
     * (e.g. in error messages).
     *
     * \param rsName a name that can be displayed to the user
     */
    void SetAlternateName( const PdfString & rsName );

    /** \returns the fields alternate name
     */
    PdfString GetAlternateName() const;

    /**
     * Sets the fields mapping name which is used when exporting
     * the fields data
     *
     * \param rsName the mapping name of this PdfField
     */
    void SetMappingName( const PdfString & rsName ); 

    /** \returns the mapping name of this field
     */
    PdfString GetMappingName() const;

    /** Set this field to be readonly.
     *  I.e. it will not interact with the user
     *  and respond to mouse button events.
     *
     *  This is useful for fields that are pure calculated.
     *
     *  \param bReadOnly specifies if this field is read-only.
     */
    inline void SetReadOnly( bool bReadOnly );

    /** 
     * \returns true if this field is read-only
     *
     * \see SetReadOnly
     */
    inline bool IsReadOnly() const;

    /** Required fields must have a value
     *  at the time the value is exported by a submit action
     * 
     *  \param bRequired if true this field requires a value for submit actions
     */
    inline void SetRequired( bool bRequired );

    /** 
     * \returns true if this field is required for submit actions
     *
     * \see SetRequired
     */
    inline bool IsRequired() const;

    /** Sets if this field can be exported by a submit action
     *
     *  Fields can be exported by default.
     *
     *  \param bExport if false this field cannot be exported by submit actions
     */
    inline void SetExport( bool bExport );

    /** 
     * \returns true if this field can be exported by submit actions
     *
     * \see SetExport
     */
    inline bool IsExport() const;

    inline void SetMouseEnterAction( const PdfAction & rAction );
    inline void SetMouseLeaveAction( const PdfAction & rAction );
    inline void SetMouseDownAction( const PdfAction & rAction );
    inline void SetMouseUpAction( const PdfAction & rAction );

    inline void SetFocusEnterAction( const PdfAction & rAction );
    inline void SetFocusLeaveAction( const PdfAction & rAction );

    inline void SetPageOpenAction( const PdfAction & rAction );
    inline void SetPageCloseAction( const PdfAction & rAction );

    inline void SetPageVisibleAction( const PdfAction & rAction );
    inline void SetPageInvisibleAction( const PdfAction & rAction );

    /* Peter Petrov 15 October 2008 */
    inline void SetKeystrokeAction( const PdfAction & rAction );
    inline void SetValidateAction( const PdfAction & rAction );
    
    /** 
     * \returns the type of this field
     */
    inline EPdfField GetType() const;

 private:

    /** 
     *  Initialize this PdfField.
     *
     *  \param pParent parent acro forms dictionary
     */
    void Init( PdfAcroForm* pParent );

    void AddAlternativeAction( const PdfName & rsName, const PdfAction & rAction );

 protected:
    PdfObject*     m_pObject;
    PdfAnnotation* m_pWidget;

 private:
    EPdfField  m_eField;

    // Peter Petrov 27 April 2008
 public:
     inline PdfAnnotation* GetWidgetAnnotation() const;
     inline PdfObject* GetFieldObject() const;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
/*
inline const PdfField & PdfField::operator=( const PdfField & rhs )
{
    // DominikS: Reference counted vectors could be nice here. In case
    //           the PdfField handling makes sense the way it is now,
    //           we could discuss using reference counted vectors
    //           and implement PdfAction, PdfAnnotation ... similar to PdfField
    m_pObject = rhs.m_pObject;
    m_pWidget = rhs.m_pWidget;
    m_eField  = rhs.m_eField;

    return *this;
}*/


// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfField::SetReadOnly( bool bReadOnly )
{
    this->SetFieldFlag( static_cast<int>(ePdfField_ReadOnly), bReadOnly );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfField::IsReadOnly() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfField_ReadOnly), false );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfField::SetRequired( bool bRequired )
{
    this->SetFieldFlag( static_cast<int>(ePdfField_Required), bRequired );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfField::IsRequired() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfField_Required), false );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfField::SetExport( bool bExport )
{
    this->SetFieldFlag( static_cast<int>(ePdfField_NoExport), bExport );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfField::IsExport() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfField_NoExport), true );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfPage* PdfField::GetPage() const
{
    return m_pWidget->GetPage();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfField::SetMouseEnterAction( const PdfAction & rAction )
{
    this->AddAlternativeAction( PdfName("E"), rAction );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfField::SetMouseLeaveAction( const PdfAction & rAction )
{
    this->AddAlternativeAction( PdfName("X"), rAction );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfField::SetMouseDownAction( const PdfAction & rAction )
{
    this->AddAlternativeAction( PdfName("D"), rAction );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfField::SetMouseUpAction( const PdfAction & rAction )
{
    this->AddAlternativeAction( PdfName("U"), rAction );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfField::SetFocusEnterAction( const PdfAction & rAction )
{
    this->AddAlternativeAction( PdfName("Fo"), rAction );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfField::SetFocusLeaveAction( const PdfAction & rAction )
{
    this->AddAlternativeAction( PdfName("BI"), rAction );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfField::SetPageOpenAction( const PdfAction & rAction )
{
    this->AddAlternativeAction( PdfName("PO"), rAction );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfField::SetPageCloseAction( const PdfAction & rAction )
{
    this->AddAlternativeAction( PdfName("PC"), rAction );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfField::SetPageVisibleAction( const PdfAction & rAction )
{
    this->AddAlternativeAction( PdfName("PV"), rAction );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfField::SetPageInvisibleAction( const PdfAction & rAction )
{
    this->AddAlternativeAction( PdfName("PI"), rAction );
}

/* Peter Petrov 15 October 2008 */
// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfField::SetKeystrokeAction( const PdfAction & rAction )
{
    this->AddAlternativeAction( PdfName("K"), rAction);
}

/* Peter Petrov 15 October 2008 */
// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfField::SetValidateAction( const PdfAction & rAction )
{
    this->AddAlternativeAction( PdfName("V"), rAction);
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline EPdfField PdfField::GetType() const
{
    return m_eField;
}

// Peter Petrov 27 April 2008
// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfAnnotation* PdfField::GetWidgetAnnotation() const
{
    return m_pWidget;
}

// Peter Petrov 27 April 2008
// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfObject* PdfField::GetFieldObject() const
{
    return m_pObject;
}

class PODOFO_DOC_API PdfButton : public PdfField {
 protected:
    enum { ePdfButton_NoToggleOff      = 0x0004000,
           ePdfButton_Radio            = 0x0008000,
           ePdfButton_PushButton       = 0x0010000,
           ePdfButton_RadioInUnison    = 0x2000000
    };

    /** Create a new PdfButton
     */
    PdfButton( EPdfField eField, PdfAnnotation* pWidget, PdfAcroForm* pParent );

    /** Create a new PdfButton
     */
    PdfButton( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent );

    /** Create a new PdfButton
     */
    PdfButton( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc );

    /** Create a new PdfButton
     */
    PdfButton( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfStreamedDocument* pDoc );

 public:

    /** Create a PdfButton from a PdfField 
     *  \param rhs a PdfField that is a button
     *
     *  Internal usage only.
     */
    PdfButton( const PdfField & rhs );

    /**
     * \returns true if this is a pushbutton
     */
    inline bool IsPushButton() const;

    /**
     * \returns true if this is a checkbox
     */
    inline bool IsCheckBox() const;

    /**
     * \returns true if this is a radiobutton
     */
    inline bool IsRadioButton() const;

    /** Set the normal caption of this button
     *
     *  \param rsText the caption
     */
    void SetCaption( const PdfString & rsText );

    /** 
     *  \returns the caption of this button
     */
    const PdfString GetCaption() const;

};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfButton::IsPushButton() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfButton_PushButton), false );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfButton::IsCheckBox() const
{
    return (!this->GetFieldFlag( static_cast<int>(ePdfButton_Radio), false ) &&
            !this->GetFieldFlag( static_cast<int>(ePdfButton_PushButton), false ) );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfButton::IsRadioButton() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfButton_Radio), false );
}


/** A push button is a button which has no state and value
 *  but can toggle actions.
 */
class PODOFO_DOC_API PdfPushButton : public PdfButton {
 public:
    /** Create a new PdfPushButton
     */
    PdfPushButton( PdfAnnotation* pWidget, PdfAcroForm* pParent );

    /** Create a new PdfPushButton
     */
    PdfPushButton( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent );

    /** Create a new PdfPushButton
     */
    PdfPushButton( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc );

    /** Create a new PdfPushButton
     */
    PdfPushButton( PdfPage* pPage, const PdfRect & rRect, PdfStreamedDocument* pDoc );

    /** Create a PdfPushButton from a PdfField 
     *  \param rhs a PdfField that is a push button button
     *
     *  Raises an error if PdfField::GetType() != ePdfField_PushButton
     */
    PdfPushButton( const PdfField & rhs );

    /** Set the rollover caption of this button
     *  which is displayed when the cursor enters the field
     *  without the mouse button being pressed
     *
     *  \param rsText the caption
     */
    void SetRolloverCaption( const PdfString & rsText );

    /** 
     *  \returns the rollover caption of this button
     */
    const PdfString GetRolloverCaption() const;

    /** Set the alternate caption of this button
     *  which is displayed when the button is pressed.
     *
     *  \param rsText the caption
     */
    void SetAlternateCaption( const PdfString & rsText );

    /** 
     *  \returns the rollover caption of this button
     */
    const PdfString GetAlternateCaption() const;

 private:
    void Init();
};

/** A checkbox can be checked or unchecked by the user
 */
class PODOFO_DOC_API PdfCheckBox : public PdfButton {
 public:
    /** Create a new PdfCheckBox
     */
    PdfCheckBox( PdfAnnotation* pWidget, PdfAcroForm* pParent );

    /** Create a new PdfCheckBox
     */
    PdfCheckBox( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent );

    /** Create a new PdfCheckBox
     */
    PdfCheckBox( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc );

    /** Create a new PdfCheckBox
     */
    PdfCheckBox( PdfPage* pPage, const PdfRect & rRect, PdfStreamedDocument* pDoc );

    /** Create a PdfCheckBox from a PdfField 
     *  \param rhs a PdfField that is a check box
     *
     *  Raises an error if PdfField::GetType() != ePdfField_CheckBox
     */
    PdfCheckBox( const PdfField & rhs );

    /** Set the appearance stream which is displayed when the checkbox
     *  is checked.
     *
     *  \param rXObject an xobject which contains the drawing commands for a checked checkbox
     */
    void SetAppearanceChecked( const PdfXObject & rXObject );

    /** Set the appearance stream which is displayed when the checkbox
     *  is unchecked.
     *
     *  \param rXObject an xobject which contains the drawing commands for an unchecked checkbox
     */
    void SetAppearanceUnchecked( const PdfXObject & rXObject );

    /** Sets the state of this checkbox
     *
     *  \param bChecked if true the checkbox will be checked
     */
    void SetChecked( bool bChecked );

    /**
     * \returns true if the checkbox is checked
     */
    bool IsChecked() const;

 private:
    void Init();

    /** Add a appearance stream to this checkbox
     *
     *  \param rName name of the appearance stream
     *  \param rReference reference to the XObject containing the appearance stream
     */
    void AddAppearanceStream( const PdfName & rName, const PdfReference & rReference );
};

// TODO: Dominiks PdfRadioButton

/** A textfield in a PDF file.
 *  
 *  Users can enter text into a text field.
 *  Single and multi line text is possible,
 *  as well as richtext. The text can be interpreted
 *  as path to a file which is going to be submitted.
 */
class PODOFO_DOC_API PdfTextField : public PdfField {
 private:
    enum { ePdfTextField_MultiLine     = 0x0001000,
           ePdfTextField_Password      = 0x0002000,
           ePdfTextField_FileSelect    = 0x0100000,
           ePdfTextField_NoSpellcheck  = 0x0400000,
           ePdfTextField_NoScroll      = 0x0800000,
           ePdfTextField_Comb          = 0x1000000,
           ePdfTextField_RichText      = 0x2000000
    };

 public:
    /** Create a new PdfTextField
     */
    PdfTextField( PdfAnnotation* pWidget, PdfAcroForm* pParent );

    /** Create a new PdfTextField
     */
    PdfTextField( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent );

    /** Create a new PdfTextField
     */
    PdfTextField( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc );

    /** Create a new PdfTextField
     */
    PdfTextField( PdfPage* pPage, const PdfRect & rRect, PdfStreamedDocument* pDoc );

    /** Create a PdfTextField from a PdfField
     * 
     *  \param rhs a PdfField that is a PdfTextField
     *
     *  Raises an error if PdfField::GetType() != ePdfField_TextField
     */
    PdfTextField( const PdfField & rhs );

    /** Sets the text contents of this text field.
     *
     *  \param rsText the text of this field
     */
    void SetText( const PdfString & rsText );

    /**
     *  \returns the text contents of this text field
     */
    PdfString GetText() const;

    /** Sets the max length in characters of this textfield
     *  \param nMaxLen the max length of this textfields in characters
     */
    void SetMaxLen( pdf_long nMaxLen );

    /** 
     * \returns the max length of this textfield in characters or -1
     *          if no max length was specified
     */
    pdf_long GetMaxLen() const;

    /**
     *  Create a multi-line text field that can contains multiple lines of text.
     *  \param bMultiLine if true a multi line field is generated, otherwise
     *                    the text field can contain only a single line of text.
     *
     *  The default is to create a single line text field.
     */
    inline void SetMultiLine( bool bMultiLine );

    /** 
     * \returns true if this text field can contain multiple lines of text
     */
    inline bool IsMultiLine() const;

    /** 
     *  Create a password text field that should not echo entered
     *  characters visibly to the screen.
     *
     *  \param bPassword if true a password field is created
     *
     *  The default is to create no password field
     */
    inline void SetPasswordField( bool bPassword );

    /**
     * \returns true if this field is a password field that does
     *               not echo entered characters on the screen
     */
    inline bool IsPasswordField() const;

    /** 
     *  Create a file selection field.
     *  The entered contents are treated as filename to a file
     *  whose contents are submitted as the value of the field.
     *
     *  \param bFile if true the contents are treated as a pathname
     *               to a file to submit
     */
    inline void SetFileField( bool bFile );

    /**
     * \returns true if the contents are treated as filename
     */
    inline bool IsFileField() const;

    /** 
     *  Enable/disable spellchecking for this text field
     *
     *  \param bSpellcheck if true spellchecking will be enabled
     *
     *  Text fields are spellchecked by default
     */
    inline void SetSpellcheckingEnabled( bool bSpellcheck );

    /** 
     *  \returns true if spellchecking is enabled for this text field
     */
    inline bool IsSpellcheckingEnabled() const;

    /** 
     *  Enable/disable scrollbars for this text field
     *
     *  \param bScroll if true scrollbars will be enabled
     *
     *  Text fields have scrollbars by default
     */
    inline void SetScrollBarsEnabled( bool bScroll );

    /** 
     *  \returns true if scrollbars are enabled for this text field
     */
    inline bool IsScrollBarsEnabled() const;

    /** 
     *  Divide the text field into max-len equal
     *  combs.
     *
     *  \param bCombs if true enable division into combs
     *
     *  By default combs are disabled. Requires the max-len
     *  property to be set.
     *
     *  \see SetMaxLen
     */
    inline void SetCombs( bool bCombs );

    /**
     * \returns true if the text field has a division into equal combs set on it
     */
    inline bool IsCombs() const;

    /**
     * Creates a richtext field.
     *
     * \param bRichText if true creates a richtext field
     *
     * By default richtext is disabled.
     */
    inline void SetRichText( bool bRichText );

    /** 
     * \returns true if this is a richtext text field
     */
    inline bool IsRichText() const;

 private:
    void Init();
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfTextField::SetMultiLine( bool bMultiLine )
{
    this->SetFieldFlag( static_cast<int>(ePdfTextField_MultiLine), bMultiLine );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfTextField::IsMultiLine() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfTextField_MultiLine), false );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfTextField::SetPasswordField( bool bPassword )
{
    this->SetFieldFlag( static_cast<int>(ePdfTextField_Password), bPassword );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfTextField::IsPasswordField() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfTextField_Password), false );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfTextField::SetFileField( bool bFile )
{
    this->SetFieldFlag( static_cast<int>(ePdfTextField_FileSelect), bFile );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfTextField::IsFileField() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfTextField_FileSelect), false );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfTextField::SetSpellcheckingEnabled( bool bSpellcheck )
{
    this->SetFieldFlag( static_cast<int>(ePdfTextField_NoSpellcheck), !bSpellcheck );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfTextField::IsSpellcheckingEnabled() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfTextField_NoSpellcheck), true );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfTextField::SetScrollBarsEnabled( bool bScroll )
{
    this->SetFieldFlag( static_cast<int>(ePdfTextField_NoScroll), !bScroll );    
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfTextField::IsScrollBarsEnabled() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfTextField_NoScroll), true );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfTextField::SetCombs( bool bCombs )
{
    this->SetFieldFlag( static_cast<int>(ePdfTextField_Comb), bCombs );        
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfTextField::IsCombs() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfTextField_Comb), false );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfTextField::SetRichText( bool bRichText )
{
    this->SetFieldFlag( static_cast<int>(ePdfTextField_RichText), bRichText);        
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfTextField::IsRichText() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfTextField_RichText), false );
}

/** A list of items in a PDF file.
 *  You cannot create this object directly, use
 *  PdfComboBox or PdfListBox instead.
 *  
 *  \see PdfComboBox 
 *  \see PdfListBox
 */
class PODOFO_DOC_API PdfListField : public PdfField {
 protected:
    enum { ePdfListField_Combo         = 0x0020000,
           ePdfListField_Edit          = 0x0040000,
           ePdfListField_Sort          = 0x0080000,
           ePdfListField_MultiSelect   = 0x0200000,
           ePdfListField_NoSpellcheck  = 0x0400000,
           ePdfListField_CommitOnSelChange = 0x4000000
    };

    /** Create a new PdfTextField
     */
    PdfListField( EPdfField eField, PdfAnnotation* pWidget, PdfAcroForm* pParent );

    /** Create a new PdfTextField
     */
    PdfListField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent );

    /** Create a new PdfTextField
     */
    PdfListField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc );

    /** Create a new PdfTextField
     */
    PdfListField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfStreamedDocument* pDoc );

 public:

    /** Create a PdfListField from a PdfField 
     *  \param rhs a PdfField that is a list field
     *
     *  Internal usage only.
     */
    PdfListField( const PdfField & rhs );

    //const PdfString & GetSelectedItem(); /// ???

    /**
     * Inserts a new item into the list
     *
     * @param rsValue the value of the item
     * @param rsDisplayName an optional display string that is displayed in the viewer
     *                      instead of the value
     */
    void InsertItem( const PdfString & rsValue, const PdfString & rsDisplayName = PdfString::StringNull );

    /** 
     * Removes an item for the list
     *
     * @param nIndex index of the item to remove
     */
    void RemoveItem( int nIndex );

    /** 
     * @param nIndex index of the item
     * @returns the value of the item at the specified index
     */
    const PdfString GetItem( int nIndex ) const;

    /** 
     * @param nIndex index of the item
     * @returns the display text of the item or if it has no display text
     *          its value is returned. This call is equivalent to GetItem() 
     *          in this case
     *
     * \see GetItem
     */
    const PdfString GetItemDisplayText( int nIndex ) const;

    /**
     * \returns the number of items in this list
     */
    size_t GetItemCount() const;

    /** Sets the currently selected item
     *  \param nIndex index of the currently selected item
     */
    void SetSelectedItem( int nIndex );

    /** Sets the currently selected item
     *
     *  \returns the selected item or -1 if no item was selected
     */
    int GetSelectedItem() const;
    
#if 0
    // TODO:
#error "Only allow these if multiselect is true!"
    void SetSelectedItems( ... );

    PdfArray GetSelectedItems() ;
#endif


    /** 
     * \returns true if this PdfListField is a PdfComboBox and false
     *               if it is a PdfListBox
     */
    inline bool IsComboBox() const;

    /** 
     *  Enable/disable spellchecking for this combobox
     *
     *  \param bSpellcheck if true spellchecking will be enabled
     *
     *  combobox are spellchecked by default
     */
    inline void SetSpellcheckingEnabled( bool bSpellcheck );
    
    /** 
     *  \returns true if spellchecking is enabled for this combobox
     */
    inline bool IsSpellcheckingEnabled() const;

    /**
     * Enable or disable sorting of items.
     * The sorting does not happen in acrobat reader
     * but whenever adding items using PoDoFo or another
     * PDF editing application.
     *
     * \param bSorted enable/disable sorting
     */
    inline void SetSorted( bool bSorted );

    /**
     * \returns true if sorting is enabled
     */
    inline bool IsSorted() const;

    /**
     * Sets wether multiple items can be selected by the
     * user in the list.
     *
     * \param bMulti if true multiselect will be enabled
     *
     * By default multiselection is turned off.
     */
    inline void SetMultiSelect( bool bMulti );

    /** 
     * \returns true if multi selection is enabled
     *               for this list
     */
    inline bool IsMultiSelect() const;

    inline void SetCommitOnSelectionChange( bool bCommit );
    inline bool IsCommitOnSelectionChange() const;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfListField::IsComboBox() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfListField_Combo), false );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfListField::SetSpellcheckingEnabled( bool bSpellcheck )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_NoSpellcheck), !bSpellcheck );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfListField::IsSpellcheckingEnabled() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfListField_NoSpellcheck), true );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfListField::SetSorted( bool bSorted )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Sort), bSorted );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfListField::IsSorted() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfListField_Sort), false );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfListField::SetMultiSelect( bool bMulti )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_MultiSelect), bMulti );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfListField::IsMultiSelect() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfListField_MultiSelect), false );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfListField::SetCommitOnSelectionChange( bool bCommit )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_CommitOnSelChange), bCommit );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfListField::IsCommitOnSelectionChange() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfListField_CommitOnSelChange), false );
}

/** A combo box with a drop down list of items.
 */
class PODOFO_DOC_API PdfComboBox : public PdfListField {
 public:
    /** Create a new PdfTextField
     */
    PdfComboBox( PdfAnnotation* pWidget, PdfAcroForm* pParent );

    /** Create a new PdfTextField
     */
    PdfComboBox( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent );

    /** Create a new PdfTextField
     */
    PdfComboBox( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc );

    /** Create a new PdfTextField
     */
    PdfComboBox( PdfPage* pPage, const PdfRect & rRect, PdfStreamedDocument* pDoc );

    /** Create a PdfComboBox from a PdfField
     * 
     *  \param rhs a PdfField that is a PdfComboBox
     *
     *  Raises an error if PdfField::GetType() != ePdfField_ComboBox
     */
    PdfComboBox( const PdfField & rhs );

    /**
     * Sets the combobox to be editable
     *
     * \param bEdit if true the combobox can be edited by the user
     *
     * By default a combobox is not editable
     */
    inline void SetEditable( bool bEdit );

    /** 
     *  \returns true if this is an editable combobox
     */
    inline bool IsEditable() const;

};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline void PdfComboBox::SetEditable( bool bEdit )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Edit), bEdit);        
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline bool PdfComboBox::IsEditable() const
{
    return this->GetFieldFlag( static_cast<int>(ePdfListField_Edit), false );
}

/** A list box
 */
class PODOFO_DOC_API PdfListBox : public PdfListField {
 public:
    /** Create a new PdfTextField
     */
    PdfListBox( PdfAnnotation* pWidget, PdfAcroForm* pParent );

    /** Create a new PdfTextField
     */
    PdfListBox( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent );

    /** Create a new PdfTextField
     */
    PdfListBox( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc );

    /** Create a new PdfTextField
     */
    PdfListBox( PdfPage* pPage, const PdfRect & rRect, PdfStreamedDocument* pDoc );

    /** Create a PdfListBox from a PdfField
     * 
     *  \param rhs a PdfField that is a PdfComboBox
     *
     *  Raises an error if PdfField::GetType() != ePdfField_ListBox
     */
    PdfListBox( const PdfField & rhs );

};

};

#endif // _PDF_ACRO_FORM_H__PDF_NAMES_TREE_H_
