
#ifndef _PDF_FONT_H_
#define _PDF_FONT_H_

#include "PdfDefines.h"
#include "PdfName.h"
#include "PdfObject.h"

namespace PoDoFo {

class PdfFontMetrics;
class PdfPage;
class PdfWriter;

/** Before you can draw text on a PDF document, you have to create 
 *  a font object first. You can reuse this font object as often 
 *  as you want.
 *  You will use PdfSimpleWriter::CreateFont most of the time
 *  to create a new font object.
 */
class PdfFont : public PdfObject {
 public:
    /** Create a new PdfFont object which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  The font has a default font size of 12.0pt.
     *
     *  \param objectno the object number
     *  \param generationno gernation number
     */
    PdfFont( unsigned int objectno, unsigned int generationno );
    virtual ~PdfFont();

    /** Initialize a newly generated PdfFont object. If you use the CreateFont
     *  method of PdfSimpleWriter you do not have to call this method.
     *
     *  \param pMetrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object will 
     *         get deleted along with the PdFont object.
     *  \param pParent PdfVecObjects object which is needed so that the Font can create 
     *         other objects on its own
     *  \param bEmbedd specifies whether this font should be embedded in the PDF file.
     *         Embedding fonts is usually a good idea.
     *
     *  \returns ErrOk on success
     */
    PdfError Init( PdfFontMetrics* pMetrics, PdfVecObjects* pParent, bool bEmbedd );

    /** Set the font size before drawing with this font.
     *  \param fSize font size in points
     */
    void SetFontSize( float fSize );

    /** Retrieve the current font size of this font object
     *  \returns the current font size
     */
    inline float FontSize() const;

    /** Set the underlined property of the font
     *  \param bUnder if true any text drawn with this font
     *                by a PdfPainter will be underlined.
     *  Default is false
     */
    inline void SetUnderlined( bool bUnder );

    /** \returns true if the font is underlined
     */
    inline bool IsUnderlined() const;

    /** Returns the identifier of this font how it is known
     *  in the pages resource dictionary.
     *  \returns PdfName containing the identifier (e.g. /Ft13)
     */
    inline const PdfName & Identifier() const;

    /** Returns a handle to the fontmetrics object of this font.
     *  This can be used for size calculations of text strings when
     *  drawn using this font.
     *  \returns a handle to the font metrics object
     */
    inline const PdfFontMetrics* FontMetrics() const;

 private:
    /** Embedd the font file directly into the PDF file.
     *  \param pParent PdfVecObjects object which is needed so that the Font can create 
     *         other objects on its own     
     *  \param pDescriptor font descriptor object
     *  \returns ErrOk on success.
     */
    PdfError EmbeddFont( PdfVecObjects* pParent, PdfObject* pDescriptor );

 private: 
    float m_fFontSize;

    bool  m_bBold;
    bool  m_bItalic;
    bool  m_bUnderlined;

    PdfName m_Identifier;
    PdfName m_BaseFont;

    PdfFontMetrics* m_pMetrics;
};

const PdfName & PdfFont::Identifier() const
{
    return m_Identifier;
}

float PdfFont::FontSize() const
{
    return m_fFontSize;
}

const PdfFontMetrics* PdfFont::FontMetrics() const
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

};

#endif // _PDF_FONT_H_

