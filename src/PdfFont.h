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

#ifndef _PDF_FONT_H_
#define _PDF_FONT_H_

#include "PdfDefines.h"
#include "PdfName.h"
#include "PdfElement.h"
#include "PdfEncoding.h"
#include "PdfFontMetrics.h"

namespace PoDoFo {

class PdfObject;
class PdfPage;
class PdfWriter;

/** Before you can draw text on a PDF document, you have to create 
 *  a font object first. You can reuse this font object as often 
 *  as you want.
 *
 *  Use PdfDocument::CreateFont to create a new font object.
 *  It will choose a correct subclass using PdfFontFactory.
 *
 *  This is only an abstract base class which is implemented
 *  for different font formats.
 */
class PODOFO_API PdfFont : public PdfElement {
 friend class PdfFontFactory;

 public:

	/** Always use this static declaration,
	 *  if you need an instance of PdfDocEncoding
	 *  as heap allocation is expensive for PdfDocEncoding.
	 */
	static const PODOFO_API PdfDocEncoding DocEncoding;

    /** Always use this static declaration,
     *  if you need an instance of PdfWinAnsiEncoding
     *  as heap allocation is expensive for PdfWinAnsiEncoding.
     */
    static const PdfWinAnsiEncoding WinAnsiEncoding;

    /** Always use this static declaration,
     *  if you need an instance of PdfWinAnsiEncoding
     *  as heap allocation is expensive for PdfWinAnsiEncoding.
     */
    static const PdfMacRomanEncoding MacRomanEncoding;
 
    /** Create a new PdfFont object which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  The font has a default font size of 12.0pt.
     *
     *  \param pMetrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is 
     *         deleted along with the font.
     *  \param pEncoding the encoding of this font. The font will not take ownership of this object.
     *  \param pParent parent of the font object
     *  
     */
    PdfFont( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfVecObjects* pParent );

    /** Create a PdfFont based on an existing PdfObject
     *  \param pMetrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is 
     *         deleted along with the font.
     *  \param pEncoding the encoding of this font. The font will not take ownership of this object.
     *  \param pObject an existing PdfObject
     */
    PdfFont( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfObject* pObject );

    virtual ~PdfFont();

    /** Set the font size before drawing with this font.
     *  \param fSize font size in points
     */
    inline void SetFontSize( float fSize );

    /** Retrieve the current font size of this font object
     *  \returns the current font size
     */
    inline float GetFontSize() const;

    /** Set the horizontal scaling of the font for compressing (< 100) and expanding (>100)
     *  \param fScale scaling in percent
     */
    inline void SetFontScale( float fScale );

    /** Retrieve the current horizontal scaling of this font object
     *  \returns the current font scaling
     */
    inline float GetFontScale() const;

    /** Set the character spacing of the font
     *  \param fCharSpace character spacing in percent
     */
    inline void SetFontCharSpace( float fCharSpace );

    /** Retrieve the current character spacing of this font object
     *  \returns the current font character spacing
     */
    inline float GetFontCharSpace() const;

    /** Set the underlined property of the font
     *  \param bUnder if true any text drawn with this font
     *                by a PdfPainter will be underlined.
     *  Default is false
     */
    inline void SetUnderlined( bool bUnder );

    /** \returns true if the font is underlined
     *  \see IsBold
     *  \see IsItalic
     */
    inline bool IsUnderlined() const;

    /** \returns true if this font is bold
     *  \see IsItalic
     *  \see IsUnderlined
     */
    inline bool IsBold() const;
    
    /** \returns true if this font is italic
     *  \see IsBold
     *  \see IsUnderlined
     */
    inline bool IsItalic() const;
    
    /** Set the strikeout property of the font
     *  \param bStrikeOut if true any text drawn with this font
     *                    by a PdfPainter will be strikedout.
     *  Default is false
     */
    inline void SetStrikeOut( bool bStrikeOut );
    
    /** \returns true if the font is striked out
     */
    inline bool IsStrikeOut() const;
    
    /** Returns the identifier of this font how it is known
     *  in the pages resource dictionary.
     *  \returns PdfName containing the identifier (e.g. /Ft13)
     */
    inline const PdfName & GetIdentifier() const;

    /** Returns a reference to the fonts encoding
     *  \returns a PdfEncoding object.
     */
    inline const PdfEncoding* GetEncoding() const;

    /** Returns a handle to the fontmetrics object of this font.
     *  This can be used for size calculations of text strings when
     *  drawn using this font.
     *  \returns a handle to the font metrics object
     */
    inline const PdfFontMetrics* GetFontMetrics() const;

    /** Write a PdfString to a PdfStream in a format so that it can 
     *  be used with this font.
     *  This is used by PdfPainter::DrawText to display a text string.
     *  The following PDF operator will be Tj
     *
     *  \param rsString a unicode or ansi string which will be displayed
     *  \param pStream the string will be appended to pStream without any leading
     *                 or following whitespaces.
     */
    virtual void WriteStringToStream( const PdfString & rsString, PdfStream* pStream );

 protected:
    /** Get the base font name of this font
     *
     *  \returns the base font name
     */
    inline const PdfName& GetBaseFont() const;

 private:
    /** Initialize all variables
     */
    void InitVars();

    /** Used to specify if this represents a bold font
     *  \param bBold if true this is a bold font.
     *
     *  \see IsBold
     *
     *  This can be called by PdfFontFactory to tell this font 
     *  object that it belongs to a bold font.
     */
    inline void SetBold( bool bBold );

    /** Used to specify if this represents an italic font
     *  \param bItalic if true this is an italic font.
     *
     *  \see IsItalc
     *
     *  This can be called by PdfFontFactory to tell this font 
     *  object that it belongs to an italic font.
     */
    inline void SetItalic( bool bItalic );

 private:
    PdfName m_BaseFont;

 protected: 
    const PdfEncoding* const m_pEncoding;
    PdfFontMetrics*          m_pMetrics;

    bool  m_bBold;
    bool  m_bItalic;
    bool  m_bUnderlined;
    bool  m_bStrikedOut;

    PdfName m_Identifier;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfFont::SetBold( bool bBold )
{
    m_bBold = bBold;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfFont::SetItalic( bool bItalic )
{
    m_bItalic = bItalic;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfName& PdfFont::GetBaseFont() const
{
    return m_BaseFont;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfName & PdfFont::GetIdentifier() const
{
    return m_Identifier;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfFont::SetFontSize( float fSize )
{
    m_pMetrics->SetFontSize( fSize );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
float PdfFont::GetFontSize() const
{
    return m_pMetrics->GetFontSize();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfFont::SetFontScale( float fScale )
{
    m_pMetrics->SetFontScale( fScale );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
float PdfFont::GetFontScale() const
{
    return m_pMetrics->GetFontScale();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfFont::SetFontCharSpace( float fCharSpace )
{
    m_pMetrics->SetFontCharSpace( fCharSpace );
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
float PdfFont::GetFontCharSpace() const
{
    return m_pMetrics->GetFontCharSpace();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfEncoding* PdfFont::GetEncoding() const
{
    return m_pEncoding;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const PdfFontMetrics* PdfFont::GetFontMetrics() const
{
    return m_pMetrics;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfFont::SetUnderlined( bool bUnder )
{
    m_bUnderlined = bUnder;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfFont::IsUnderlined() const
{
    return m_bUnderlined;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
void PdfFont::SetStrikeOut( bool bStrikeOut )
{
    m_bStrikedOut = bStrikeOut;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfFont::IsStrikeOut() const
{
    return m_bStrikedOut;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfFont::IsBold() const
{
	return m_bBold;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfFont::IsItalic() const
{
	return m_bItalic;
}

};

#endif // _PDF_FONT_H_

