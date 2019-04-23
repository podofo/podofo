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

#include "PdfTable.h"

#include "base/PdfDefinesPrivate.h"
#include "base/PdfCanvas.h"
#include "base/PdfRect.h"

#include "PdfFont.h"
#include "PdfImage.h"
#include "PdfPainter.h"
#include "PdfPage.h"

#include <stdlib.h>

namespace PoDoFo {

PdfSimpleTableModel::PdfSimpleTableModel()
    : m_pFont( NULL ), m_eAlignment( ePdfAlignment_Left ),
      m_eVerticalAlignment( ePdfVerticalAlignment_Center ),
      m_bWordWrap( false), m_clForeground( 1.0 ),
      m_bBackground( false ), m_clBackground( 0.0 ),
      m_ppData( NULL ), m_nCols( 0 ), m_nRows( 0 ),
	  m_bBorder( true ), m_dBorder( 1.0 )
{

}

PdfSimpleTableModel::PdfSimpleTableModel( int nCols, int nRows )
    : m_pFont( NULL ), m_eAlignment( ePdfAlignment_Left ),
      m_eVerticalAlignment( ePdfVerticalAlignment_Center ),
      m_bWordWrap( false ), m_clForeground( 1.0 ),
      m_bBackground( false ), m_clBackground( 0.0 ),
      m_nCols( nCols ), m_nRows( nRows ),
	  m_bBorder( true ), m_dBorder( 1.0 )
{
    m_ppData = static_cast<PdfString**>(podofo_calloc( nRows, sizeof(PdfString*) ));
    if( !m_ppData )
    {
        PODOFO_RAISE_ERROR( ePdfError_OutOfMemory );
    }

    for( int i=0;i<nRows;i++ ) 
        m_ppData[i] = new PdfString[nCols];
}

PdfSimpleTableModel::~PdfSimpleTableModel()
{
    if( m_ppData ) 
    {
        for( int i=0;i<m_nRows;i++ ) 
            delete [] m_ppData[i];

        podofo_free( m_ppData );
    }
}

PdfTable::PdfTable( int nCols, int nRows ) 
    : m_pModel( NULL ),
      m_nCols( nCols ), m_nRows( nRows ),
      m_dColWidth( 0.0 ), m_dRowHeight( 0.0 ),
      m_dTableWidth( 0.0 ), m_dTableHeight( 0.0 ),
      m_pdColWidths( NULL ), m_pdRowHeights( NULL ),
      m_bAutoPageBreak( false ), m_pCustomData( NULL ),
      m_fpCallback( NULL )
{

}

PdfTable::~PdfTable()
{
	delete [] m_pdColWidths;
	delete [] m_pdRowHeights;
}

void PdfTable::Draw( double dX, double dY, PdfPainter* pPainter, const PdfRect & rClipRect,
                     double* pdLastX, double* pdLastY )
{
    if( !pPainter ) 
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    //RG: TODO Should dCurY variable be initialised with 0? (line 257 may fall through without initialisation!)
    int i = 0;
    int j = 0;
    double  dCurX = 0.0;
    double  dCurY = 0.0;

    double  dWidth = 0.0;
    double  dHeight = 0.0;
    double  dVertical = 0.0;
    double* pdColWidths  = new double[this->GetCols()];
    double* pdRowHeights = new double[this->GetRows()];

	bool bBorders = !m_pModel || m_pModel->HasBorders();

    // Calculate all necessary sizes
    this->CalculateTableSize( dX, dY, pPainter->GetPage(), 
                              pdColWidths, pdRowHeights,
                              &dWidth, &dHeight );
 
    if( !(!static_cast<int>(rClipRect.GetBottom()) && 
          !static_cast<int>(rClipRect.GetLeft()) &&
          !static_cast<int>(rClipRect.GetWidth()) && 
          !static_cast<int>(rClipRect.GetHeight())) ) 
        m_curClipRect = rClipRect;
    else
    {
        m_curClipRect = PdfRect( 0.0, dX, 
                                 pPainter->GetPage()->GetPageSize().GetWidth() - dX,
                                 dY );
    }

    // Draw the table
    pPainter->Save();
    PdfFont* pDefaultFont = pPainter->GetFont(); // get the default font
    PdfFont* pFont;

    // draw contents
    if( m_pModel ) 
    {
		pPainter->SetStrokeWidth( m_pModel->GetBorderWidth() );

		if( bBorders ) // draw top border
            this->DrawHorizontalBorders( 0, dX, dY, pPainter, pdColWidths );

        for( j=0;j<m_nRows;j++ )
        {
			if( this->CheckForNewPage( &dY, &dCurY, pdRowHeights[j], pPainter ) && bBorders )
                // draw top border on new page
                this->DrawHorizontalBorders( j, dX, dY, pPainter, pdColWidths );
    
			dCurX  = 0.0;	
			dCurY += pdRowHeights[j];

            for( i=0;i<m_nCols;i++ ) 
            {
	            // set a clipping rectangle
                pPainter->Save();
                pPainter->SetClipRect( dX + dCurX, dY - dCurY, pdColWidths[i], pdRowHeights[j] );

                // Draw background
				double dBorder = bBorders ? m_pModel->GetBorderWidth()/2.0 : 0.0;
                if( m_pModel->HasBackgroundColor( i, j ) ) 
                {
                    pPainter->Save();
                    pPainter->SetColor( m_pModel->GetBackgroundColor( i, j ) );
					// Make sure that FillRect only fills inside the border
					// rectangle and not over the border. This is necessary
					// because we draw the border first and than the contents.
                    pPainter->Rectangle( dX + dCurX + dBorder, dY - dCurY + dBorder, 
						                pdColWidths[i] - 2.0 * dBorder, 
										pdRowHeights[j] - 2.0 * dBorder );
						  pPainter->Fill();
                    pPainter->Restore();
                }

                // draw an image
                PdfImage* pImage = m_pModel->GetImage( i, j );
                double dImageWidth = 0.0;
                if( m_pModel->HasImage( i, j ) && pImage )
                {
                    double dScaleX = (pdColWidths[i])  / pImage->GetPageSize().GetWidth();
                    double dScaleY = (pdRowHeights[j] - 2.0 * dBorder) / pImage->GetPageSize().GetHeight();
                    double dScale  = PDF_MIN( dScaleX, dScaleY );

                    dImageWidth = pImage->GetPageSize().GetWidth() * dScale;

                    pPainter->DrawImage( dX + dCurX, dY - dCurY + dBorder, pImage, dScale, dScale );
                }

                // Set the correct font
                pFont = m_pModel->GetFont( i, j );
                pFont = pFont ? pFont : pDefaultFont;
                pPainter->SetFont( pFont );
				pPainter->SetColor( m_pModel->GetForegroundColor( i, j ) );

                // draw text
				if( m_pModel->HasWordWrap( i, j ) )
				{
					// Make sure we have at least 1 dot free space at each side of the rectangle
					pPainter->DrawMultiLineText( dX + dCurX + 1.0 + dImageWidth, dY - dCurY, 
                                                 pdColWidths[i] - 2.0 - dImageWidth, pdRowHeights[j],
												 m_pModel->GetText( i, j ), m_pModel->GetAlignment( i, j ),
												 m_pModel->GetVerticalAlignment( i, j ) );
				}
				else
				{
					// calculate vertical alignment
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

					// Make sure we have at least 1 dot free space at each side of the rectangle
					pPainter->DrawTextAligned( dX + dCurX + 1 + dImageWidth, dY - dCurY + dVertical, 
                                               pdColWidths[i] - 2.0 - dImageWidth, m_pModel->GetText( i, j ), m_pModel->GetAlignment( i, j ) );
				}
                
                pPainter->Restore();
				if( bBorders ) // draw left x border
                {
                    // use always the border color of the left to the current cell
                    pPainter->SetStrokingColor( m_pModel->GetBorderColor( i>0 ? i-1 : i, j ) );
					pPainter->DrawLine( dX + dCurX, dY - dCurY, dX + dCurX, dY - dCurY + pdRowHeights[j] );
                }

		        dCurX += pdColWidths[i];    
            }

			if( bBorders ) 
			{
				// Draw last X border
                if( i > 0 )
                {
                    pPainter->SetStrokingColor( m_pModel->GetBorderColor( i-1, j ) );
				    pPainter->DrawLine( dX + dCurX, dY - dCurY, dX + dCurX, dY - dCurY + pdRowHeights[j] );
                }

                // draw border below row    
                this->DrawHorizontalBorders( j, dX, dY - dCurY, pPainter, pdColWidths );
    		}
		}    
	}
    pPainter->Restore();

    if( pdLastX )
        *pdLastX = dX + dWidth;

    if( pdLastY )
        *pdLastY = dY - dCurY;

    // Free allocated memory
    delete [] pdColWidths;
    delete [] pdRowHeights;
}

void PdfTable::DrawHorizontalBorders( int nRow, double dX, double dY, PdfPainter* pPainter, double* pdColWidths ) 
{
    double dCurX = 0.0;
    pPainter->Save();
    pPainter->SetLineCapStyle( ePdfLineCapStyle_Square );
    for( int i=0;i<m_nCols;i++ )
    {
        pPainter->SetStrokingColor( m_pModel->GetBorderColor( i, nRow ) );
	    pPainter->DrawLine( dX + dCurX, dY, dX + dCurX + pdColWidths[i], dY );

        dCurX += pdColWidths[i];
    }
    pPainter->Restore();
}

double PdfTable::GetWidth( double dX, double dY, PdfCanvas* pPage ) const
{
    double  dWidth;
    double  dHeight;
    double* pdColWidths  = new double[this->GetCols()];
    double* pdRowHeights = new double[this->GetRows()];

    // Calculate all necessary sizes
    this->CalculateTableSize( dX, dY, pPage,
                              pdColWidths, pdRowHeights,
                              &dWidth, &dHeight );

    delete [] pdColWidths;
    delete [] pdRowHeights;

    return dWidth;
}

double PdfTable::GetHeight( double dX, double dY, PdfCanvas* pPage ) const
{
    double  dWidth;
    double  dHeight;
    double* pdColWidths  = new double[this->GetCols()];
    double* pdRowHeights = new double[this->GetRows()];

    // Calculate all necessary sizes
    this->CalculateTableSize( dX, dY, pPage,
                              pdColWidths, pdRowHeights,
                              &dWidth, &dHeight );

    delete [] pdColWidths;
    delete [] pdRowHeights;

    return dHeight;
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

bool PdfTable::CheckForNewPage( double* pdY, double* pdCurY, double dRowHeight, PdfPainter* pPainter )
{
    if( !m_bAutoPageBreak )
        return false;

    if( (*pdY - *pdCurY) - dRowHeight < m_curClipRect.GetBottom() )
    {
        pPainter->Restore();

        PdfPage* pPage = (*m_fpCallback)( m_curClipRect, m_pCustomData );
        pPainter->SetPage( pPage );
        pPainter->Save();

        *pdY    = m_curClipRect.GetBottom() + m_curClipRect.GetHeight();
        *pdCurY = 0.0;

        return true;
    }

    return false;
}

void PdfTable::SetColumnWidths( double* pdWidths )
{
    if( m_pdColWidths )
    {
        delete [] m_pdColWidths;
        m_pdColWidths = NULL;
    }

    if( pdWidths ) 
    {
        m_pdColWidths = new double[this->GetCols()];
        memcpy( m_pdColWidths, pdWidths, this->GetCols() * sizeof(double) );
    }
}

void PdfTable::SetRowHeights( double* pdHeights )
{
    if( m_pdRowHeights )
    {
        delete [] m_pdRowHeights;
        m_pdRowHeights = NULL;
    }

    if( pdHeights ) 
    {
        m_pdRowHeights = new double[this->GetRows()];
        memcpy( m_pdRowHeights, pdHeights, this->GetRows() * sizeof(double) );
    }
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
