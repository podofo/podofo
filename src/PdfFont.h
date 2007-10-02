
#ifndef _PDF_FONT_H_
#define _PDF_FONT_H_

#include "PdfDefines.h"
#include "PdfName.h"
#include "PdfElement.h"
#include "PdfFontMetrics.h"

namespace PoDoFo {

class PdfObject;
class PdfPage;
class PdfWriter;

/** Before you can draw text on a PDF document, you have to create 
 *  a font object first. You can reuse this font object as often 
 *  as you want.
 *  You will use PdfSimpleWriter::CreateFont most of the time
 *  to create a new font object.
 */
class PODOFO_API PdfFont : public PdfElement {
 public:
    /** Create a new PdfFont object which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  The font has a default font size of 12.0pt.
     *
     *  \param pMetrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is 
     *         deleted along with the font.
     *  \param bEmbed specifies whether this font should be embedded in the PDF file.
     *         Embedding fonts is usually a good idea.
     *  \param bBold specifies if this font is a bold font
     *  \param bItalic specifies if this font is an italic font
     *  \param pParent parent of the font object
     *  
     */
    PdfFont( PdfFontMetrics* pMetrics, bool bEmbed, bool bBold, bool bItalic, 
             PdfVecObjects* pParent );

    /** Create a new PdfFont object which is a font subset of a font.
     *
     *  The font has a default font size of 12.0pt.
     *
     *  \param pMetrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is 
     *         deleted along with the font.
     *  \param bBold specifies if this font is a bold font
     *  \param bItalic specifies if this font is an italic font
     *  \param vecUnicodeCodePoints the unicode code points that are part of this subset
     *  \param pParent parent of the font object
     *  
     */
    PdfFont( PdfFontMetrics* pMetrics, bool bBold, bool bItalic,
             std::vector<int> vecUnicodeCodePoints, PdfVecObjects* pParent );

    virtual ~PdfFont();

    /** Set the font size before drawing with this font.
     *  \param fSize font size in points
     */
    void SetFontSize( float fSize );

    /** Retrieve the current font size of this font object
     *  \returns the current font size
     */
    inline float GetFontSize() const;

    /** Set the horizontal scaling of the font for compressing (< 100) and expanding (>100)
     *  \param fScale scaling in percent
     */
    void SetFontScale( float fScale );

    /** Retrieve the current horizontal scaling of this font object
     *  \returns the current font scaling
     */
    inline float GetFontScale() const;

    /** Set the character spacing of the font
     *  \param fCharSpace character spacing in percent
     */
    void SetFontCharSpace( float fCharSpace );

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

    /** Returns a handle to the fontmetrics object of this font.
     *  This can be used for size calculations of text strings when
     *  drawn using this font.
     *  \returns a handle to the font metrics object
     */
    inline const PdfFontMetrics* GetFontMetrics() const;

 private:
    /** Embed the font file directly into the PDF file.
     *  \param pDescriptor font descriptor object
     */
    void EmbedFont( PdfObject* pDescriptor );

    /** Embed the font file directly into the PDF file.
     *
     *  The font file is a true type file.
     *
     *  \param pDescriptor font descriptor object
     */
    void EmbedTrueTypeFont( PdfObject* pDescriptor ); 

    /** Embed the font file directly into the PDF file.
     *
     *  The font file is a true type file.
     *
     *  \param pDescriptor font descriptor object
     */
    void EmbedType1Font( PdfObject* pDescriptor ); 

    /** Initialize the object
     *  \param bEmbed if true the font will be embeded into the PDF 
     */
    void Init( bool bEmbed );

    /** Set the basic variables to their initial values
     *  (those who are shared between real fonts and subsets)
     */
    void InitVars();

    /** Initialise a font subset
     */
    void InitSubset();

    /** A custom helper function for Type1 fontembeeding.
     *  Searched a string in a binary buffer and returns the offset it was found at.
     *
     *  \param pszNeedle the string to search
     *  \param pszHaystack the buffer in which we search
     *  \param lLen the length of the buffer
     *
     *  \returns the offset of the found string or -1 if the string was not found
     */
    long FindInBuffer( const char* pszNeedle, const char* pszHaystack, long lLen );

 private: 
    PdfFontMetrics* m_pMetrics;

    bool  m_bBold;
    bool  m_bItalic;
    bool  m_bUnderlined;
    bool  m_bStrikedOut;

    PdfName m_Identifier;
    PdfName m_BaseFont;
};

const PdfName & PdfFont::GetIdentifier() const
{
    return m_Identifier;
}

float PdfFont::GetFontSize() const
{
    return m_pMetrics->GetFontSize();
}

float PdfFont::GetFontScale() const
{
    return m_pMetrics->GetFontScale();
}

float PdfFont::GetFontCharSpace() const
{
    return m_pMetrics->GetFontCharSpace();
}

const PdfFontMetrics* PdfFont::GetFontMetrics() const
{
    return m_pMetrics;
}

void PdfFont::SetUnderlined( bool bUnder )
{
    m_bUnderlined = bUnder;
}

bool PdfFont::IsUnderlined() const
{
    return m_bUnderlined;
}

void PdfFont::SetStrikeOut( bool bStrikeOut )
{
    m_bStrikedOut = bStrikeOut;
}

bool PdfFont::IsStrikeOut() const
{
    return m_bStrikedOut;
}

bool PdfFont::IsBold() const
{
	return m_bBold;
}

bool PdfFont::IsItalic() const
{
	return m_bItalic;
}

};

#endif // _PDF_FONT_H_

