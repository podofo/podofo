/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_PAINTER_PATH_H
#define PDF_PAINTER_PATH_H

#include "PdfStringStream.h"

namespace PoDoFo {

class PdfPainter;

/**
 * This class describes a manually handled PDF text object
 * (content stream operators surrounded by BT .. ET) being
 * written to a PdfPainter
 */
class PODOFO_API PdfPainterTextObject final
{
    friend class PdfPainter;

private:
    PdfPainterTextObject(PdfPainter& painter);

public:
    /** Begin drawing multiple text strings on a page using a given font object.
     *  You have to call SetFont before calling this function.
     *
     *  If you want more simpler text output and do not need
     *  the advanced text position features of MoveTextPos
     *  use DrawText which is easier.
 
     *
     *  \see AddText()
     *  \see MoveTo()
     *  \see EndText()
     */
    void Begin();

    /** Draw a string on a page.
     *  You have to call BeginText before the first call of this function
     *  and EndText after the last call.
     *
     *  If you want more simpler text output and do not need
     *  the advanced text position features of MoveTextPos
     *  use DrawText which is easier.
     *
     *  \param str the text string which should be printed
     *
     *  \see SetFont()
     *  \see MoveTo()
     *  \see End()
     */
    void AddText(const std::string_view& str);

    /** Move position for text drawing on a page.
     *  You have to call BeginText before calling this function
     *
     *  If you want more simpler text output and do not need
     *  the advanced text position features of MoveTextPos
     *  use DrawText which is easier.
     *
     *  \param x the x offset relative to pos of BeginText or last MoveTextPos
     *  \param y the y offset relative to pos of BeginText or last MoveTextPos
     *
     *  \see Begin()
     *  \see AddText()
     *  \see End()
     */
    void MoveTo(double x, double y);

    /** End drawing multiple text strings on a page
     *
     *  If you want more simpler text output and do not need
     *  the advanced text position features of MoveTextPos
     *  use DrawText which is easier.
     *
     *  \see Begin()
     *  \see AddText()
     *  \see MoveTo()
     */
    void End();

private:
    PdfPainter* m_painter;
};

}

#endif // PDF_PAINTER_PATH_H
