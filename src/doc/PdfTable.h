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

#ifndef _PDF_TABLE_H_
#define _PDF_TABLE_H_

#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfColor.h"
#include "podofo/base/PdfRect.h"
#include "podofo/base/PdfString.h"

#include <string.h>

namespace PoDoFo {

class PdfCanvas;
class PdfFont;
class PdfImage;
class PdfPainter;
class PdfPage;

/**
 * This is an abstract interface of a model that can provide
 * data and formatting informations to a PdfTable.
 *
 * You can implement your own PdfTableModel to supply data
 * to a PdfTable.
 * PdfSimpleTableModel is an example of a simple model.
 * 
 *
 * \see PdfTable
 * \see PdfSimpleTableModel
 */
class PODOFO_DOC_API PdfTableModel {
 public:
    virtual ~PdfTableModel() {};

    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns the contents string of this table cell
     */
    virtual PdfString GetText ( int col, int row ) const = 0;

    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns the horizontal alignment of the contents in the cell
     */
    virtual EPdfAlignment GetAlignment ( int col, int row ) const = 0;

    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns the vertical alignment of the contents in the cell
     */
    virtual EPdfVerticalAlignment GetVerticalAlignment ( int col, int row ) const = 0;

    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns the font of this table cell or NULL to use the default font
     */
    virtual PdfFont*  GetFont ( int col, int row ) const = 0;
    
    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns true if this cell has a background color
     */
    virtual bool HasBackgroundColor( int col, int row ) const = 0;

    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns the background color of the specified cell
     */
    virtual PdfColor GetBackgroundColor( int col, int row ) const = 0;

    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns the foreground (text) color of the specified cell
     */
    virtual PdfColor GetForegroundColor( int col, int row ) const = 0;

    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns true if the specified cell should use wordwrapping
     */
    virtual bool HasWordWrap( int col, int row ) const = 0;

	/** 
	 * \returns true if the table should have
	 *          a border around all cells.
	 * \returns false if no cell border should be visible
	 *
	 * Cell borders are always drawn using the current PdfPainter
	 * settings.
	 */
	virtual bool HasBorders() const = 0;

	/** 
	 * \returns the stroke witdth of the border line
	 */
	virtual double GetBorderWidth() const = 0;

    /**
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns the border color 
     */
    virtual PdfColor GetBorderColor( int col, int row ) const = 0;

    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns true if the table cell contains an image
     */
    virtual bool HasImage( int col, int row ) const = 0;

    /**
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns the image for the specified cell or NULL if the cell has no image
     */     
    virtual PdfImage* GetImage( int col, int row ) const = 0;
};

/**
 * An implementation of a simple PdfTableModel.
 *
 */
class PODOFO_DOC_API PdfSimpleTableModel : public PdfTableModel {
 public:
    /** Creates an empty PdfSimpleTableModel 
     *  that does not contain any data.
     *
     *  Using this model will result in drawing an empty table!
     */
    PdfSimpleTableModel();

    /** Creates an empty PdfSimpleTableModel 
     *  that does not contain any data.
     *
     *  Using this model will result in drawing an empty table!
     *
     *  \param nCols number of columns of the data in this table model (must match the PdfTable object)
     *  \param nRows number of rows of the data in this table model (must match the PdfTable object)
     *
     *  You can set the tables data using SetText.
     *  \see SetText
     */
    PdfSimpleTableModel( int nCols, int nRows );

    virtual ~PdfSimpleTableModel();

    /** Set the font that will be used to draw all table contents.
     *
     *  \param pFont the font for the table contents
     */
    inline void SetFont( PdfFont* pFont );

    /** Set the horizontal alignment of the contents in all table cells
     *
     *  \param eAlignment the horizontal alignment of text in a table cell
     */
    inline void SetAlignment( EPdfAlignment eAlignment );

    /** Set the vertical alignment of the contents in all table cells
     *
     *  \param eAlignment the vertiical alignment of text in a table cell
     */
    inline void SetAlignment( EPdfVerticalAlignment eAlignment );

    /** Set the background color of the table cells
     *
     *  \param rColor the background color
     */
    inline void SetBackgroundColor( const PdfColor & rColor );

    /** Set the foreground color of the table cells
     *
     *  \param rColor the foreground color
     */
    inline void SetForegroundColor( const PdfColor & rColor );

    /** Sets wether all cells have a background color or not
     *
     *  \param bEnable if true all cells have a background color
     */
    inline void SetBackgroundEnabled( bool bEnable );

    /** Sets wether all cells have wordwrapping or not
     *
     *  \param bEnable if true all cells have wordwrapping
     */
    inline void SetWordWrapEnabled( bool bEnable );

    /** Sets wether all cells have a border or not.
     *  
     *  \param bEnable if true a border will be drawn
     *                 using the current PdfPainter settings
     */
    inline void SetBorderEnabled( bool bEnable );
    
    /** Sets the stroke width of the border around
     *  the table.
     *
     *  \param dWidth the stroke width of the border
     */
    inline void SetBorderWidth( double dWidth );
    
    /** Sets the contents of a specific cell
     *
     * \param col the column of the table cell
     * \param row the row of the table cell
     * \param rsString the contents of this cell
     */
    inline void SetText( int col, int row, const PdfString & rsString );

    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns the contents string of this table cell
     */
    inline virtual PdfString GetText ( int col, int row ) const;

    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns the horizontal alignment of the contents in the cell
     */
    inline virtual EPdfAlignment GetAlignment ( int col, int row ) const;

    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns the vertical alignment of the contents in the cell
     */
    inline virtual EPdfVerticalAlignment GetVerticalAlignment ( int col, int row ) const;

    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns the font of this table cell or NULL to use the default font
     */
    inline virtual PdfFont*  GetFont ( int col, int row ) const;
    
    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns true if this cell has a background color
     */
    inline virtual bool HasBackgroundColor( int col, int row ) const;

    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns the background color of the specified cell
     */
    inline virtual PdfColor GetBackgroundColor( int col, int row ) const;


    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns the foreground (text) color of the specified cell
     */
    inline virtual PdfColor GetForegroundColor( int col, int row ) const;

    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns true if the specified cell should use wordwrapping
     */
    inline virtual bool HasWordWrap( int col, int row ) const;

    /** 
     * \returns true if the table should have
     *          a border around all cells.
     * \returns false if no cell border should be visible
     *
     * Cell borders are always drawn using the current PdfPainter
     * settings.
     */
    inline virtual bool HasBorders() const;
    
    /** 
     * \returns the stroke witdth of the border line
     */
    inline virtual double GetBorderWidth() const;

    /**
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns the border color 
     */
    inline virtual PdfColor GetBorderColor( int col, int row ) const;

    /** 
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns true if the table cell contains an image
     */
    inline virtual bool HasImage( int col, int row ) const;

    /**
     * \param col the column of the table cell
     * \param row the row of the table cell
     *
     * \returns the image for the specified cell or NULL if the cell has no image
     */     
    inline virtual PdfImage* GetImage( int col, int row ) const;

 private:
    PdfFont*              m_pFont;

    EPdfAlignment         m_eAlignment;
    EPdfVerticalAlignment m_eVerticalAlignment;
    
    bool                  m_bWordWrap;
    PdfColor              m_clForeground;
    bool                  m_bBackground;
    PdfColor              m_clBackground;
        
    PdfString**           m_ppData;

    int                   m_nCols;
    int                   m_nRows;

	bool                  m_bBorder;
	double                m_dBorder;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfSimpleTableModel::SetFont( PdfFont* pFont )
{
    m_pFont = pFont;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfSimpleTableModel::SetAlignment( EPdfAlignment eAlignment )
{
    m_eAlignment = eAlignment;
}


// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfSimpleTableModel::SetAlignment( EPdfVerticalAlignment eAlignment )
{
    m_eVerticalAlignment = eAlignment;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfSimpleTableModel::SetBackgroundEnabled( bool bEnable )
{
    m_bBackground = bEnable;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfSimpleTableModel::SetWordWrapEnabled( bool bEnable )
{
    m_bWordWrap = bEnable;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfSimpleTableModel::SetBorderEnabled( bool bEnable )
{
	m_bBorder = bEnable;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfSimpleTableModel::SetBorderWidth( double dWidth )
{
	m_dBorder = dWidth;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfSimpleTableModel::SetBackgroundColor( const PdfColor & rColor )
{
    m_clBackground = rColor;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfSimpleTableModel::SetForegroundColor( const PdfColor & rColor )
{
    m_clForeground = rColor;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfSimpleTableModel::SetText( int col, int row, const PdfString & rsString ) 
{
    if( !m_ppData || row >= m_nRows || col >= m_nCols )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    m_ppData[row][col] = rsString;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfString PdfSimpleTableModel::GetText ( int col, int row ) const
{
    if( !m_ppData || row >= m_nRows || col >= m_nCols )
        return PdfString();
    else
        return m_ppData[row][col].IsValid() ? m_ppData[row][col] : PdfString("");
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfAlignment PdfSimpleTableModel::GetAlignment ( int, int ) const
{
    return m_eAlignment;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
EPdfVerticalAlignment PdfSimpleTableModel::GetVerticalAlignment ( int, int ) const
{
    return m_eVerticalAlignment;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfFont* PdfSimpleTableModel::GetFont ( int, int ) const
{
    return m_pFont;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfSimpleTableModel::HasBackgroundColor ( int, int ) const
{
    return m_bBackground;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfColor PdfSimpleTableModel::GetBackgroundColor ( int, int ) const
{
    return m_clBackground;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfColor PdfSimpleTableModel::GetForegroundColor( int, int ) const
{
    return m_clForeground;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfSimpleTableModel::HasWordWrap( int, int ) const
{
    return m_bWordWrap;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfSimpleTableModel::HasBorders() const
{
	return m_bBorder;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfSimpleTableModel::GetBorderWidth() const
{
	return m_dBorder;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfColor PdfSimpleTableModel::GetBorderColor( int, int ) const
{
    // always return black
    return PdfColor( 0.0, 0.0, 0.0 );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfSimpleTableModel::HasImage( int, int ) const
{
    return false;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
PdfImage* PdfSimpleTableModel::GetImage( int, int ) const
{
    return NULL;
}

/**
 * This is a high level class of a table which can be drawn to a PdfPainter.
 *
 * Use this class if you have to include data into your PDF as an table.
 * 
 */
class PODOFO_DOC_API PdfTable {
 public:
    /** Callback to create a new page for PdfTable.
     *
     *  \see SetAutoPageBreak
     *
     *  \param rClipRect this is an output parameter which has
     *                   to be set to the clipping rectangle
     *                   of the new page. If the new page has
     *                   no clipping rectangle set it to
     *                   PdfRect( 0, 0, PdfPage::GetPageSize().GetWidth(), PdfPage::GetPageSize().GetHeight() )
     *  \param pCustom pointer to user defined data
     */
    typedef PdfPage* (*CreatePageCallback)( PdfRect & rClipRect, void* pCustom );

    /** Create a new PdfTable object.
     *
     *  \param nCols number of columns in the table.
     *  \param nRows number of rows in the table.
     */
    PdfTable( int nCols, int nRows );

    virtual ~PdfTable();

    /** Draw the table with its current settings
     *  on a PdfPainter.
     *
     *  \param dX x coordinate of top left of the table
     *  \param dY y coordinate of top left of the table
     *  \param pPainter the painter to draw on. The painter has to have a page set currently.
     *  \param rClipRect the clipping rectangle on the current page
     *  \param pdLastX the last used X position by the table on the current page will be written to this value (usually bottom right)
     *  \param pdLastY the last used Y positon by the table on the current page will be written to this value (usually bottom right)
     */
    virtual void Draw( double dX, double dY, PdfPainter* pPainter, const PdfRect & rClipRect = PdfRect(),
                       double* pdLastX = NULL, double* pdLastY = NULL );

    /** Get the width of the table when drawn with the current settings at a certain position.
     *  \param dX x coordinate of top left of the table
     *  \param dY y coordinate of top left of the table
     *  \param pPage the page on which the table will be drawn
     *
     *  \returns the width of the table
     */
    virtual double GetWidth( double dX, double dY, PdfCanvas* pPage ) const;

    /** Get the width of the table when drawn with the current settings at a certain position.
     *  \param dX x coordinate of top left of the table
     *  \param dY y coordinate of top left of the table
     *  \param pPage the page on which the table will be drawn
     *
     *  \returns the width of the table
     */
    virtual double GetHeight( double dX, double dY, PdfCanvas* pPage ) const;

    /** Set the PdfTableModel that will supply all
     *  contents and formatting informations to the table.
     *
     *  \param pModel a PdfTableModel
     *
     *  The model will not be owned by the PdfTable and has to be deleted
     *  by the caller.
     *
     *  \see GetModel
     */
    inline void SetModel( PdfTableModel* pModel );
    
    /** Get the current PdfTableModel
     *
     *  \returns the currently set PdfTableModel or NULL if none was set
     */
    inline const PdfTableModel* GetModel() const;

    /** Set the width of all columns.
     *  
     *  \param pdWidths a pointer to an array of GetCols() doubles
     *                  which are the individual width of a column.
     *
     *  \see GetCols()
     */
    void SetColumnWidths( double* pdWidths );

    /** Set the height of all rows.
     *  
     *  \param pdHeights a pointer to an array of GetRows() doubles
     *                   which are the individual heights of a row.
     *
     *  \see GetRows()
     */
    void SetRowHeights( double* pdHeights );

    /** Set all columns to have the same width.
     *
     *  \param dWidth the width of every column
     *
     *  By default the column with is calculated automatically
     *  from either the table width or if no table width is set
     *  from the width of the page on which the table is drawn.
     */
    inline void SetColumnWidth( double dWidth );

    /** Set all rows to have the same height.
     *
     *  \param dHeight the height of every row
     *
     *  By default the row height is calculated automatically
     *  from either the table height or if no table height is set
     *  from the height of the page on which the table is drawn.
     */
    inline void SetRowHeight( double dHeight );

    /** Set the width of the table.
     *
     *  \param dWidth the width of the whole table.
     *
     *  This width is used if no column width is set
     *  to calculate the width of every column.
     *  If this width is not set, the width of the page
     *  on which this table is drawn is used.
     */
    inline void SetTableWidth( double dWidth );

    /** Set the height of the table.
     *
     *  \param dHeight the height of the whole table.
     *
     *  This height is used if no row height is set
     *  to calculate the height of every row.
     *  If this height is not set, the height of the page
     *  on which this table is drawn is used.
     */
    inline void SetTableHeight( double dHeight );

    /** Automatically create a new page and continue
     *  drawing the table on the new page,
     *  if there is not enough space on the current page.
     *
     *  The newly created page will be set as the current page
     *  on the painter used to draw and will be created using the
     *  same size as the old page.
     *
     *  \param bPageBreak if true automatically create new pages
     *         if required.
     *  \param callback a callback function that is called to create
     *         a new page. Please note: PdfTable cannot create new pages on its
     *         own. You always have to implement a callback which does the new page 
     *         creation for the PdfTable.
     *  \param pCustomData custom data that is passed to the callback
     *
     *  By default this feature is turned off and contents are clipped
     *  that do not fit on the current page.
     *
     *  \see GetAutoPageBreak
     */
    inline void SetAutoPageBreak( bool bPageBreak, CreatePageCallback callback, 
                                  void* pCustomData = NULL);

    /** 
     *  \returns true if a new page is created automatically if more
     *           space is required to draw the table.
     *
     *  \see SetAutoPageBreak
     */
    inline bool GetAutoPageBreak() const;

    /**
     * \returns the number of columns in the table.
     */
    inline int GetCols() const;

    /**
     * \returns the number of rows in the table.
     */
    inline int GetRows() const;

 protected:
    /** Internal functions that calculates the total table size
     *  for a table with the current settings when drawn on a certain page.
     *
     *  \param dX the X coordinate of top left at which is drawn
     *  \param dY the Y coordinate of top left at which is drawn
     *  \param pCanvas the canvas object (usually a page) on which the table will be drawn.
     *  \param pdWidths pointer to an array with GetCols() doubles
     *                  where the width for each column will be stored
     *  \param pdHeights pointer to an array with GetRows() doublesd
     *                  where the height for each row will be stored
     *
     *  \param pdWidth pointer to a double where the total width of the table will be stored
     *  \param pdHeight pointer to a double where the total height of the table will be stored
     */
    void CalculateTableSize( const double dX, const double dY, const PdfCanvas* pCanvas, 
                             double* pdWidths, double* pdHeights,
                             double* pdWidth, double* pdHeight ) const;

    /** Draw one row of horizontal cell borders using the correct color
     *  for each cell.
     *
     *  @param nRow the current row
     *  @param dX left x coordinate
     *  @param dY y coordinate
     *  @param pPainter use this painter object
     *  @param pdColWidths an array containing all colomun widths
     */
    void DrawHorizontalBorders( int nRow, double dX, double dY, PdfPainter* pPainter, double* pdColWidths );

    /** Checks if there is enough space on the current page
     *  for one row! If necessary a new page is created.
     *
     *  If GetAutoPageBreak is false, this method does nothing.
     *
     *  \param pdY top of the table
     *  \param pdCurY pointer to the current y position on the page. 
     *                Might be reset to a new y position.
     *  \param dRowHeight height of the next row.
     *  \param pPainter painter used for drawing
     *
     *  \returns true if a new page was created, otherwise false
     */
    bool CheckForNewPage( double* pdY, double* pdCurY, double dRowHeight, PdfPainter* pPainter );
    
 protected:
    PdfTableModel* m_pModel;

    int     m_nCols;
    int     m_nRows;

    double  m_dColWidth;
    double  m_dRowHeight;
    double  m_dTableWidth;
    double  m_dTableHeight;
    
    double* m_pdColWidths;
    double* m_pdRowHeights;

    bool    m_bAutoPageBreak;
    void*   m_pCustomData;
    CreatePageCallback m_fpCallback;

    PdfRect m_curClipRect;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfTable::SetModel( PdfTableModel* pModel )
{
    m_pModel = pModel;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfTableModel* PdfTable::GetModel() const
{
    return m_pModel;
}


// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfTable::SetColumnWidth( double dWidth )
{
    m_dColWidth = dWidth;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfTable::SetRowHeight( double dHeight )
{
    m_dRowHeight = dHeight;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfTable::SetTableWidth( double dWidth )
{
    m_dTableWidth = dWidth;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfTable::SetTableHeight( double dHeight )
{
    m_dTableHeight = dHeight;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfTable::SetAutoPageBreak( bool bPageBreak, CreatePageCallback callback, 
                                 void* pCustomData  )
{
    m_bAutoPageBreak = bPageBreak;
    m_fpCallback     = callback;
    m_pCustomData    = pCustomData;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfTable::GetAutoPageBreak() const
{
    return m_bAutoPageBreak;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
int PdfTable::GetCols() const
{
    return m_nCols;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
int PdfTable::GetRows() const
{
    return m_nRows;
}

};


#endif // _PDF_TABLE_H_
 
