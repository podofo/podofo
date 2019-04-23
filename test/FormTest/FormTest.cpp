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

#include <podofo.h>

#include "../PdfTest.h"

#include <iostream>
#include <cstdio>

using namespace PoDoFo;

#define CONVERSION_CONSTANT 0.002834645669291339

void CreateComplexForm( PdfPage* pPage, PdfStreamedDocument* pDoc )
{
    PdfRect rect = pPage->GetPageSize();

    PdfPainter painter;
    PdfFont*   pFont = pDoc->CreateFont( "Courier" );

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
    y -= 11000.0 * CONVERSION_CONSTANT;
    painter.DrawText( x, y, "I wan't to use PoDoFo in an Open Source application" );
    PdfCheckBox checkOpenSource( pPage, PdfRect( 120000.0 * CONVERSION_CONSTANT, y - 2500.0 * CONVERSION_CONSTANT,
                                                 h, h ), pDoc );
    checkOpenSource.SetFieldName("field_check_oss");

    // Commercial
    y -= 11000.0 * CONVERSION_CONSTANT;
    painter.DrawText( x, y, "I wan't to use PoDoFo in a commercial application" );
    PdfCheckBox checkCom( pPage, PdfRect( 120000.0 * CONVERSION_CONSTANT, y - 2500.0 * CONVERSION_CONSTANT,
                                          h, h ), pDoc );
    checkCom.SetFieldName("field_check_com");

    y -= 10000.0 * CONVERSION_CONSTANT;
    painter.DrawText( x, y, "Some comments you want to send to the PoDoFo developers:" );
    PdfTextField textComment( pPage, PdfRect( 20000.0 * CONVERSION_CONSTANT, y - 120000.0 * CONVERSION_CONSTANT,
                                              160000.0 * CONVERSION_CONSTANT, 100000.0 * CONVERSION_CONSTANT ), pDoc );
    textComment.SetFieldName("field_comment");
    textComment.SetMultiLine( true );
    textComment.SetRichText( true );
    textComment.SetText( "<?xml version=\"1.0\"?><body xmlns=\"http://www.w3.org/1999/xtml\" xmlns:xfa=\"http://www.xfa.org/schema/xfa-data/1.0/\" xfa:contentType=\"text/html\" xfa:APIVersion=\"Acrobat:8.0.0\" xfa:spec=\"2.4\"><p style=\"text-align:left\"><b><i>Here is some bold italic text</i></b></p><p style=\"font-size:16pt\">This text uses default text state parameters but changes the font size to 16.</p></body>");

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

    PdfAction actionClear( ePdfAction_JavaScript, pDoc );
    actionClear.SetScript(
        PdfString("this.getField(\"field_name\").value = \"\";" \
                  "this.getField(\"field_mail\").value = \"\";" \
                  "this.getField(\"field_combo\").value = \"\";"
                  "this.getField(\"field_check_oss.\").checkThisBox( 0, false );" \
                  "this.getField(\"field_check_com.\").checkThisBox( 0, false );" \
                  "this.getField(\"field_comment\").value = \"\";" ) );


    buttonClear.SetMouseDownAction( actionClear );

    PdfAction actionSubmit( ePdfAction_SubmitForm, pDoc );

    buttonSend.SetMouseDownAction( actionSubmit );

    painter.FinishPage();
}

void CreateSimpleForm( PdfPage* pPage, PdfStreamedDocument* pDoc )
{
    PdfPainter painter;
    PdfFont*   pFont = pDoc->CreateFont( "Courier" );

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


    PdfAction action( ePdfAction_JavaScript, pDoc );
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
    printf("Count     : %i\n", static_cast<int>(combo.GetItemCount()) );
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

void FillTextField( PdfTextField & rField )
{
    const char* pszCur = rField.GetText().GetString();
    std::cout << "  Current value:" << (pszCur ? pszCur : "") << std::endl;

    std::string value;
    std::cout << "  Enter new value (if empty value is unchanged):" << std::endl;
#if defined(_MSC_VER)  &&  _MSC_VER <= 1200 // Visual Studio 6
	 {
		 char buff[10240];
		 if (gets(buff))
			 value = buff;
	 }
#else
    getline( std::cin, value );
#endif

    if( value.length() )
    {
        rField.SetText( value );
    }
}

void FillListField( PdfListField & rField )
{
    const char* pszCur = ( rField.GetSelectedItem() == -1 ? NULL :
                           rField.GetItemDisplayText( rField.GetSelectedItem() ).GetString() );
    std::cout << "  Current value:" << (pszCur ? pszCur : "") << std::endl;
    std::cout << "  Values:" << std::endl;

    for( int i=0;i<static_cast<int>(rField.GetItemCount());i++ )
    {
        pszCur = rField.GetItemDisplayText( i ).GetString();
        std::cout << "     " << i << " " << (pszCur ? pszCur : "") << std::endl;
    }

    int nValue;
    std::cout << "  Enter index of new value:" << std::endl;
    std::cin >> nValue;

    if( nValue >= 0 && nValue < static_cast<int>(rField.GetItemCount()) )
        rField.SetSelectedItem( nValue );
}

void FillForm( const char* pszFilename, const char* pszOutput )
{
    PdfMemDocument doc( pszFilename );
    PdfPage*       pPage;
    int            nPageCount = doc.GetPageCount();
    int            nFieldCount;

    for( int i=0;i<nPageCount;i++ )
    {
        pPage       = doc.GetPage( i );
        nFieldCount = pPage->GetNumFields();

        std::cout << "Page " << i + 1 << " contains " << nFieldCount << " fields." << std::endl;

        for( int n=0;n<nFieldCount;n++ )
        {
            PdfField  field = pPage->GetField( n );
            EPdfField eType = field.GetType();

            std::cout << "  Field: " << field.GetFieldName().GetString() << std::endl;
            std::cout << "  Type : ";

            switch( eType )
            {
                case ePdfField_PushButton:
                    std::cout << "PushButton" << std::endl;
                    break;
                case ePdfField_CheckBox:
                    std::cout << "CheckBox" << std::endl;
                    break;
                case ePdfField_RadioButton:
                    std::cout << "RadioButton" << std::endl;
                    break;
                case ePdfField_TextField:
                {
                    std::cout << "TextField" << std::endl;
                    PdfTextField text( field );
                    FillTextField( text );
                    break;
                }
                case ePdfField_ComboBox:
                {
                    std::cout << "ComboBox" << std::endl;
                    PdfListField lst( field );
                    FillListField( lst );
                    break;
                }
                case ePdfField_ListBox:
                {
                    std::cout << "ListBox" << std::endl;
                    PdfListField lst( field );
                    FillListField( lst );
                    break;
                }
                case ePdfField_Signature:
                    std::cout << "Signature" << std::endl;
                    break;
                case ePdfField_Unknown:
                default:
                    std::cout << "Unknown" << std::endl;
                    break;
            }

            std::cout << std::endl;
        }
    }

    doc.Write( pszOutput );
}

int main( int argc, char* argv[] )
{
    PdfPage*            pPage;

    if( argc != 2 && argc != 3 )
    {
        printf("Usage: FormTest [output_filename]\n");
        printf("       - Create a new example PDF form\n");
        printf("       Formtest [input_filename] [output_filename]\n");
        printf("       - Fill out an existing form and save it to a PDF file\n");
        return 0;
    }

    try {
        if( argc == 3 )
        {
            TEST_SAFE_OP( FillForm( argv[1], argv[2] ) );
        }
        else
        {
            PdfStreamedDocument writer( argv[1] );

            pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
            TEST_SAFE_OP( CreateComplexForm( pPage, &writer ) );

            pPage = writer.CreatePage( PdfPage::CreateStandardPageSize( ePdfPageSize_A4 ) );
            TEST_SAFE_OP( CreateSimpleForm( pPage, &writer ) );

            TEST_SAFE_OP( writer.Close() );
        }
    } catch( PdfError & e ) {
        std::cerr << "Error: An error " << e.GetError() << " ocurred." << std::endl;
        e.PrintErrorMsg();
        return e.GetError();
    }

    return 0;
}
