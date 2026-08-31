/**
 * SPDX-FileCopyrightText: (C) 2026 David Lilly
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPainterSaveGuard.h"

using namespace PoDoFo;

PdfPainterSaveGuard::PdfPainterSaveGuard(PdfPainter& painter)
    : m_painter(painter), m_restored(false)
{
    m_painter.Save();
}

PdfPainterSaveGuard::~PdfPainterSaveGuard() noexcept
{
    if (m_restored)
        return;

    if (m_painter.GetCanvas() == nullptr)
        return;

    try
    {
        m_painter.Restore();
    }
    catch (...)
    {
    }
}

void PdfPainterSaveGuard::Restore()
{
    if (m_restored)
        return;

    m_restored = true;
    m_painter.Restore();
}
