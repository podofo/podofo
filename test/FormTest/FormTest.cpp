/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "../../src/podofo.h"

#include "../PdfTest.h"

using namespace PoDoFo;

#define CONVERSION_CONSTANT 0.002834645669291339

void CreateComplexForm( PdfPage* pPage, PdfDocument* pDoc )
{
    PdfRect rect = pPage->GetPageSize();

    PdfPainter painter;
    PdfFont*   pFont = pDoc->CreateFont( "Courier", false );

    painter.SetPage( pPage );
    painter.SetFont( pFont );

    const char* pszTitle = "PoDoFo Sample Feedback Form";
    pFont->SetFontSize( 18.0 );

    double x = (rect.GetWidth() - pFont->GetFontMetrics()->StringWidth( pszTitle )) / 2.0;
    double y = rect.GetHeight() - (20000.0 * CONVERSION_CONSTANT);

    painter.DrawText( x, y, pszTitle );
    pFont->SetFontSize( 10.0 );

    y -= 10000.0 * CONVERSION_CONSTANT;
    x  = 10000.0 * CONVERSION_CONSTANT;

    double h = 10000.0 * CONVERSION_CONSTANT;
    // Name
    y -= 10000.0 * CONVERSION_CONSTANT;
    painter.DrawText( x, y, "Your Name:" );
    PdfTextField textName( pPage, PdfRect( 80000.0 * CONVERSION_CONSTANT, y - 2500.0 * CONVERSION_CONSTANT, 
                                           80000.0 * CONVERSION_CONSTANT, h ), pDoc );
    textName.SetFieldName("field_name");
    textName.SetBorderColor( 1.0 );

    // E-Mail
    y -= 10000.0 * CONVERSION_CONSTANT;
    painter.DrawText( x, y, "E-Mail Address:" );
    PdfTextField textMail( pPage, PdfRect( 80000.0 * CONVERSION_CONSTANT, y - 2500.0 * CONVERSION_CONSTANT, 
                                           80000.0 * CONVERSION_CONSTANT, h ), pDoc );
    textMail.SetFieldName("field_mail");
    textMail.SetBorderColor( 1.0 );
    
    // Interest
    y -= 10000.0 * CONVERSION_CONSTANT;
    painter.DrawText( x, y, "Job:" );

    PdfComboBox comboJob( pPage, PdfRect( 80000.0 * CONVERSION_CONSTANT, y - 2500.0 * CONVERSION_CONSTANT, 
                                          80000.0 * CONVERSION_CONSTANT, h ), pDoc );
    comboJob.SetFieldName("field_combo");
    comboJob.SetBorderColor( 1.0 );

    comboJob.InsertItem( "Software Engineer" );
    comboJob.InsertItem( "Student" );
    comboJob.InsertItem( "Publisher" );
    comboJob.InsertItem( "Other" );

    // Open Source
    y -= 10000.0 * CONVERSION_CONSTANT;
    painter.DrawText( x, y, "I wan't to use PoDoFo in an Open Source application" );
    PdfCheckBox checkOpenSource( pPage, PdfRect( 120000.0 * CONVERSION_CONSTANT, y - 2500.0 * CONVERSION_CONSTANT, 
                                                 40000.0 * CONVERSION_CONSTANT, h ), pDoc );
    checkOpenSource.SetFieldName("field_check_oss");

    // Commercial
    y -= 10000.0 * CONVERSION_CONSTANT;
    painter.DrawText( x, y, "I wan't to use PoDoFo in a commercial application" );
    PdfCheckBox checkCom( pPage, PdfRect( 120000.0 * CONVERSION_CONSTANT, y - 2500.0 * CONVERSION_CONSTANT, 
                                          40000.0 * CONVERSION_CONSTANT, h ), pDoc );
    checkCom.SetFieldName("field_check_com");

    y -= 10000.0 * CONVERSION_CONSTANT;
    painter.DrawText( x, y, "Some comments you want to send to the PoDoFo developers:" );
    PdfTextField textComment( pPage, PdfRect( 20000.0 * CONVERSION_CONSTANT, y - 120000.0 * CONVERSION_CONSTANT,
                                              160000.0 * CONVERSION_CONSTANT, 100000.0 * CONVERSION_CONSTANT ), pDoc );
    textComment.SetFieldName("field_comment");
    textComment.SetMultiLine( true );
    textComment.SetRichText( true );

    PdfPushButton buttonSend( pPage, PdfRect( 10000 * CONVERSION_CONSTANT, 10000 * CONVERSION_CONSTANT,
                                              25000 * CONVERSION_CONSTANT, 25000 * CONVERSION_CONSTANT ), pDoc );
    buttonSend.SetFieldName("field_send");
    buttonSend.SetCaption("Send");
    buttonSend.SetBackgroundColor( 0.5 );

    PdfPushButton buttonClear( pPage, PdfRect( 40000 * CONVERSION_CONSTANT, 10000 * CONVERSION_CONSTANT,
                                               25000 * CONVERSION_CONSTANT, 25000 * CONVERSION_CONSTANT ), pDoc );
    buttonClear.SetFieldName("field_clear");
    buttonClear.SetCaption("Clear");
    buttonClear.SetBackgroundColor( 0.5 );

    PdfAction actionClear( ePdfAction_JavaScript, &(pDoc->GetObjects()) );
    actionClear.SetScript( 
        PdfString("this.getField(\"field_name\").value = \"\";" \
                  "this.getField(\"field_mail\").value = \"\";" \
                  "this.getField(\"field_combo\").value = \"\";" 
                  "this.getField(\"field_check_oss.\").checkThisBox( 0, false );" \
                  "this.getField(\"field_check_com.\").checkThisBox( 0, false );" \
                  "this.getField(\"field_comment\").value = \"\";" ) );


    buttonClear.SetMouseDownAction( actionClear );
                  
    PdfAction actionSubmit( ePdfAction_SubmitForm, &(pDoc->GetObjects()) );

    buttonSend.SetMouseDownAction( actionSubmit );
    
    painter.FinishPage();
}

void CreateSimpleForm( PdfPage* pPage, PdfDocument* pDoc )
{
    PdfPainter painter;
    PdfFont*   pFont = pDoc->CreateFont( "Courier", false );

    painter.SetPage( pPage );
    painter.SetFont( pFont );
    painter.DrawText( 10000 * CONVERSION_CONSTANT, 280000 * CONVERSION_CONSTANT, "PoDoFo Interactive Form Fields Test" );
    painter.FinishPage();

    PdfPushButton button( pPage, PdfRect( 10000 * CONVERSION_CONSTANT, 10000 * CONVERSION_CONSTANT,
                                          50000 * CONVERSION_CONSTANT, 50000 * CONVERSION_CONSTANT ), pDoc );

    button.SetFieldName("ButtonFieldName");
    button.SetAlternateName("ButtonAlternateName");
    button.SetMappingName("ButtonMappingName");
    button.SetCaption("Hallo Welt");


    PdfAction action( ePdfAction_JavaScript, &(pDoc->GetObjects()) );
    action.SetScript( 
        PdfString("var str = this.getField(\"TextFieldName\").value;" \
                  "var j = 4*5;" \
                  "app.alert(\"Hello World! 4 * 5 = \" + j + \" Text Field: \" + str );") );

    button.SetMouseDownAction( action );

    PdfTextField text( pPage, PdfRect( 70000 * CONVERSION_CONSTANT, 10000 * CONVERSION_CONSTANT,
                                       50000 * CONVERSION_CONSTANT, 50000 * CONVERSION_CONSTANT ), pDoc );

    text.SetFieldName("TextFieldName");
    text.SetMultiLine( true );
    text.SetMultiLine( false );


    text.SetFileField( true );
    printf("Text IsMultiLine: %i\n", text.IsMultiLine() );

    PdfComboBox combo( pPage, PdfRect( 10000 * CONVERSION_CONSTANT, 250000 * CONVERSION_CONSTANT,
                                         50000 * CONVERSION_CONSTANT, 10000 * CONVERSION_CONSTANT ), pDoc );

    combo.SetFieldName("ComboFieldName");
    combo.InsertItem( "Value1" );
    combo.InsertItem( "Value2" );
    combo.InsertItem( "Value3" );
    combo.InsertItem( "XXXX", "Displayed Text" );
    combo.SetEditable( true );
    combo.SetSelectedItem( 1 );

    printf("IsComboBox: %i\n", combo.IsComboBox() );
    printf("Count     : %i\n", combo.GetItemCount() );
    printf("Selected  : %i\n", combo.GetSelectedItem() );

    PdfListBox listBox( pPage, PdfRect( 70000 * CONVERSION_CONSTANT, 200000 * CONVERSION_CONSTANT,
                                        50000 * CONVERSION_CONSTANT, 50000 * CONVERSION_CONSTANT ), pDoc );

    listBox.SetFieldName("ListBoxFieldName");
    listBox.InsertItem( "Value1", "Display 1" );
    listBox.InsertItem( "Value2", "Display 2" );
    listBox.InsertItem( "Value3", "Display 3" );
    //listBox.InsertItem( "XXXX", "Displayed Text" );
    listBox.SetMultiSelect( true );
    listBox.SetSelectedItem( 2 );
}

int main( int argc, char* argv[] ) 
{
    PdfDocument     writer;
    PdfPage*        pPage;

    if( argc != 2 )
    {
        printf("Usage: FormTest [output_filename]\n");
        return 0;
    }

    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    TEST_SAFE_OP( CreateComplexForm( pPage, &writer ) );

    pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
    TEST_SAFE_OP( CreateSimpleForm( pPage, &writer ) );

    TEST_SAFE_OP( writer.Write( argv[1] ) );

    return 0;
}
