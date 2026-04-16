/**
 * SPDX-FileCopyrightText: (C) 2026 David Lilly
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_PAINTER_SAVE_GUARD_H
#define PDF_PAINTER_SAVE_GUARD_H

#include "PdfPainter.h"

namespace PoDoFo {

/** RAII guard for PdfPainter::Save()/Restore() (q/Q operators).
 *
 * Ensures that every Save() is matched by a Restore(), even when
 * an exception unwinds the scope. Follows the std::lock_guard pattern.
 *
 * Usage:
 * \code
 * {
 *     PdfPainterSaveGuard guard(painter);  // q
 *     painter.GraphicsState.SetNonStrokingColor(color);
 *     painter.DrawRectangle(x, y, w, h, PdfPathDrawMode::Fill);
 * }  // Q — automatic, even on exception
 *
 * {
 *     PdfPainterSaveGuard guard(painter);  // q
 *     painter.SetClipRect(x, y, w, h);
 *     painter.DrawText("clipped", x, y);
 *     guard.Restore();                     // Q — early release
 *     painter.DrawText("unclipped", x2, y2);
 * }  // destructor is a no-op
 * \endcode
 */
class PODOFO_API PdfPainterSaveGuard final
{
public:
    /** Construct the guard and call Save() on the painter.
     *  \param painter The painter whose state to guard
     */
    explicit PdfPainterSaveGuard(PdfPainter& painter);

    ~PdfPainterSaveGuard() noexcept;

    /** Restore the painter state early (idempotent).
     *  Subsequent calls and the destructor are no-ops.
     */
    void Restore();

private:
    PdfPainterSaveGuard(const PdfPainterSaveGuard&) = delete;
    PdfPainterSaveGuard& operator=(const PdfPainterSaveGuard&) = delete;
    PdfPainterSaveGuard(PdfPainterSaveGuard&&) = delete;
    PdfPainterSaveGuard& operator=(PdfPainterSaveGuard&&) = delete;
    PdfPainter& m_painter;
    bool m_restored;
};

} // namespace PoDoFo

#endif // PDF_PAINTER_SAVE_GUARD_H
