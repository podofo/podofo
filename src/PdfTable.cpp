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

#include "PdfTable.h"

#include "PdfCanvas.h"
#include "PdfFont.h"
#include "PdfPainter.h"
#include "PdfRect.h"

namespace PoDoFo {

PdfSimpleTableModel::PdfSimpleTableModel()
    : m_pFont( NULL ), m_eAlignment( ePdfAlignment_Left ),
      m_eVerticalAlignment( ePdfVerticalAlignment_Center )
{

}

PdfSimpleTableModel::~PdfSimpleTableModel()
{

}

PdfString PdfSimpleTableModel::GetText ( int col, int row ) const
{
    std::ostringstream oss;
    oss << "Cell: " << col << " " << row;

    return PdfString( oss.str().c_str() );
}


PdfTable::PdfTable( int nCols, int nRows ) 
    : m_pModel( NULL ),
      m_nCols( nCols ), m_nRows( nRows ),
      m_dColWidth( 0.0 ), m_dRowHeight( 0.0 ),
      m_dTableWidth( 0.0 ), m_dTableHeight( 0.0 ),
      m_pdColWidths( NULL ), m_pdRowHeights( NULL ),
      m_bAutoPageBreak( false )
{

}

PdfTable::~PdfTable()
{
    if( m_pdColWidths )
        delete [] m_pdColWidths;

    if( m_pdRowHeights )
        delete [] m_pdRowHeights;
}

void PdfTable::Draw( double dX, double dY, PdfPainter* pPainter )
{
    if( !pPainter ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    int    i, j;
    double dCurX, dCurY;

    double  dWidth;
    double  dHeight;
    double  dVertical;
    double  dHorizontal;
    double* pdColWidths  = new double[this->GetCols()];
    double* pdRowHeights = new double[this->GetRows()];

    // Calculate all necessary sizes
    this->CalculateTableSize( dX, dY, pPainter->GetPage(), 
                              pdColWidths, pdRowHeights,
                              &dWidth, &dHeight );
 
    // Draw the table
    pPainter->Save();
    PdfFont* pDefaultFont = pPainter->GetFont(); // get the default font
    PdfFont* pFont;

    // draw contents
    if( m_pModel ) 
    {
        dCurX = 0.0;
        for( i=0;i<m_nCols;i++ ) 
        {
            dCurY = 0.0;
            for( j=0;j<m_nRows;j++ )
            {
                // set a clipping rectangle
                pPainter->Save();
                pPainter->SetClipRect( dX + dCurX, dY - dCurY, pdColWidths[i], pdRowHeights[j] );

                // Draw background
                pPainter->Save();
                if( i % 2 )
                    pPainter->SetColor( 1.0, 0.0, 0.0 );
                else
                    pPainter->SetColor( 0.0, 0.0, 1.0 );

                if( j % 2 )
                    pPainter->FillRect( dX + dCurX, dY - dCurY, pdColWidths[i], pdRowHeights[j] );
                pPainter->Restore();

                // Set the correct font
                pFont = m_pModel->GetFont( i, j );
                pFont = pFont ? pFont : pDefaultFont;
                pPainter->SetFont( pFont );

                // calculate horizontal and vertical alignment
                dCurY      += pdRowHeights[j];

                switch( m_pModel->GetAlignment( i, j ) ) 
                {
                    default:
                    case ePdfAlignment_Left:
                        dHorizontal = 0.0;
                        break;
                    case ePdfAlignment_Center:
                        dHorizontal = (pdColWidths[i] - pFont->GetFontMetrics()->StringWidth( m_pModel->GetText( i, j ) )) / 2.0;
                        break;
                    case ePdfAlignment_Right:
                        dHorizontal = (pdColWidths[i] - pFont->GetFontMetrics()->StringWidth( m_pModel->GetText( i, j ) ));
                        break;

                }

                switch( m_pModel->GetVerticalAlignment( i, j ) ) 
                {
                    default:
                    case ePdfVerticalAlignment_Top:
                        dVertical = 0.0;
                        break;
                    case ePdfVerticalAlignment_Center:
                        dVertical = (pdRowHeights[j] - pFont->GetFontMetrics()->GetLineSpacing()) / 2.0;
                        break;
                    case ePdfVerticalAlignment_Bottom:
                        dVertical = (pdRowHeights[j] - pFont->GetFontMetrics()->GetLineSpacing());
                        break;
                }

                // draw text
                pPainter->DrawText( dX + dCurX + dHorizontal, dY - dCurY + dVertical, m_pModel->GetText( i, j ) );
                pPainter->Restore();
            }

            dCurX += pdColWidths[i];
        }        
    }

    // draw borders
    dCurY = 0.0;
    for( i=0;i<=m_nRows;i++ )
    {
        pPainter->DrawLine( dX, dY - dCurY, dX + dWidth, dY - dCurY);
        dCurY += pdRowHeights[i];
    }

    dCurX = 0.0;
    for( i=0;i<=m_nCols;i++ ) 
    {

        pPainter->DrawLine( dX + dCurX, dY, dX + dCurX, dY - dHeight );
        dCurX += pdColWidths[i];
    }

    pPainter->Restore();

    // Free allocated memory
    delete [] pdColWidths;
    delete [] pdRowHeights;
}

void PdfTable::CalculateTableSize( const double dX, const double dY, const PdfCanvas* pCanvas, 
                                   double* pdWidths, double* pdHeights,
                                   double* pdWidth, double* pdHeight ) const
{
    int i;

    double dWidth  = m_dColWidth;
    double dHeight = m_dRowHeight;

    // -----------------------------------------------------
    // This functions works as follows: 
    // (Description only for width, but the is true for height)
    //
    // If the user specified an array of row-widths using SetColumnWidths
    // just copy the array and use this values.
    //
    // Else check if the user has specified a total width for the table
    // devide the table width through the amount of rows and use the same
    // width for each row.
    //
    // If the user has not specified a table width, use the page width
    // and devide the page width through the amount of rows.
    // -----------------------------------------------------
    
    if( m_pdColWidths ) 
        memcpy( pdWidths, m_pdColWidths, sizeof(double) * m_nCols );
    else
    {
        if( dWidth <= 0.0 )
        {
            double dTableWidth = m_dTableWidth;
            
            if( (dTableWidth <= 0.0) )
            {
                // Remove the x border at both sides of the table!
                dTableWidth = pCanvas->GetPageSize().GetWidth() - dX * 2.0;
            }

            dWidth = dTableWidth / static_cast<double>(m_nCols);
        }
        
        for(i=0;i<m_nCols;i++)
            pdWidths[i] = dWidth;
    }
    
    if( m_pdRowHeights ) 
        memcpy( pdHeights, m_pdRowHeights, sizeof(double) * m_nRows );
    else
    {
        if( dHeight <= 0.0 )
        {
            double dTableHeight = m_dTableHeight;
            
            if( dTableHeight <= 0.0 )
            {
                // The gap from the top is only removed once!!!
                dTableHeight = dY;
            }
            
            dHeight = dTableHeight / static_cast<double>(m_nRows);
        }
        
        for(i=0;i<m_nRows;i++)
            pdHeights[i] = dHeight;
    }

    // Sum up all widths and heights values
    // to get the total width of the table
    *pdWidth  = 0.0;
    *pdHeight = 0.0;

    for(i=0;i<m_nCols;i++)
        *pdWidth += pdWidths[i];

    for(i=0;i<m_nRows;i++)
        *pdHeight += pdHeights[i];
}


/*
void CReport::CreateTable( double dX, double dY, int iCols, int iRows, 
                                                   const char** apsTable, double* pdColWidths, double* pdRowHeights,
                                                   bool bFillBackground )
{
        int i, j;
        double dWidth  = 0.0;
        double dHeight = 0.0;
        const double dcTableBorder = 1000.0 * CONVERSION;

        mcPainter.Save();

        PdfFont* pFont = mpDocument->CreateFont( "Arial" );
        pFont->SetFontSize( 8.0f );
        mcPainter.SetFont( pFont );
        mcPainter.SetStrokeWidth( 1.0 * CONVERSION );

        for( i=0;i<iCols;i++ )
                dWidth += pdColWidths[i] + 2 * dcTableBorder;

        for( i=0;i<iRows;i++ )
                dHeight += pdRowHeights[i] + 2 * dcTableBorder;

        mcPainter.SetColor( 0, 0, 0 );
        double dCurX = 0.0;
        double dCurY = 0.0;
        dCurX = 0.0;
        for( i=0;i<iCols;i++ ) 
        {
                dCurY = 0.0;
                for( j=0;j<iRows;j++ )
                {
                        // draw cell background
                        if( bFillBackground ) 
                        {
                                mcPainter.Save();
                                mcPainter.SetGray( 0.7  );
                                double dBackW = pdColWidths[i] + 2 * dcTableBorder;
                                double dBackH = pdRowHeights[j] + 2 * dcTableBorder;
                                mcPainter.FillRect( dX + dCurX, dY + dCurY + dBackH, 
                                                                        dBackW, 
                                                                        dBackH );
                                mcPainter.Restore();
                        }

                        // draw border 
                        mcPainter.DrawLine( dX, dY + dCurY, dX + dWidth, dY + dCurY);

                        // draw cell contents
                        mcPainter.DrawText( dX + dCurX + dcTableBorder, dY + dCurY + dcTableBorder, apsTable[i+(iRows-j-1)*iCols]);
                        dCurY += pdRowHeights[j] + 2 * dcTableBorder;
                }
                mcPainter.DrawLine( dX, dY + dCurY, dX + dWidth, dY + dCurY);
                mcPainter.DrawLine( dX + dCurX, dY, dX + dCurX, dY + dHeight );
                dCurX += pdColWidths[i] + 2 * dcTableBorder;
        }
        mcPainter.DrawLine( dX + dCurX, dY, dX + dCurX, dY + dHeight );
        mcPainter.Restore();
}*/
};
