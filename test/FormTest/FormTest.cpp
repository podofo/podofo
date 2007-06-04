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
    button.SetText("Hallo Welt");


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
    TEST_SAFE_OP( CreateSimpleForm( pPage, &writer ) );

    TEST_SAFE_OP( writer.Write( argv[1] ) );

    return 0;
}
