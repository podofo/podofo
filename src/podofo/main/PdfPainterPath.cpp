/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPainter.h"
#include <podofo/private/PdfDrawingOperations.h>

using namespace std;
using namespace PoDoFo;

PdfPainterPath::PdfPainterPath() : m_opened(false) { }

void PdfPainterPath::MoveTo(double x, double y)
{
    PoDoFo::WriteOperator_m(m_stream, x, y);
    m_CurrentPoint = Vector2(x, y);
    m_opened = true;
}

void PdfPainterPath::AddLineTo(double x, double y)
{
    checkOpened();
    PoDoFo::WriteOperator_l(m_stream, x, y);
    m_CurrentPoint = Vector2(x, y);
}

void PdfPainterPath::AddLine(double x1, double y1, double x2, double y2)
{
    PoDoFo::WriteOperator_m(m_stream, x1, y1);
    PoDoFo::WriteOperator_l(m_stream, x2, y2);
    m_CurrentPoint = Vector2(x2, y2);
    m_opened = true;
}

void PdfPainterPath::AddCubicBezierTo(double x1, double y1, double x2, double y2, double x3, double y3)
{
    checkOpened();
    PoDoFo::WriteOperator_c(m_stream, x1, y1, x2, y2, x3, y3);
    m_CurrentPoint = Vector2(x3, y3);
}

void PdfPainterPath::AddCubicBezier(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4)
{
    PoDoFo::WriteOperator_m(m_stream, x1, y1);
    PoDoFo::WriteOperator_c(m_stream, x2, y2, x3, y3, x4, y4);
    m_CurrentPoint = Vector2(x4, y4);
    m_opened = true;
}

void PdfPainterPath::AddArcTo(double x1, double y1, double x2, double y2, double radius)
{
    checkOpened();
    PoDoFo::WriteArcTo(m_stream, m_CurrentPoint.X, m_CurrentPoint.Y, x1, y1, x2, y2, radius, m_CurrentPoint);
}

void PdfPainterPath::AddArc(double x, double y, double radius,
    double angle1, double angle2, bool counterclockwise)
{
    PoDoFo::WriteArc(m_stream, x, y, radius, angle1, angle2, counterclockwise, m_CurrentPoint);
    m_opened = true;
}

void PdfPainterPath::AddCircle(double x, double y, double radius)
{
    PoDoFo::WriteCircle(m_stream, x, y, radius, m_CurrentPoint);
    m_opened = true;
}

void PdfPainterPath::AddRectangle(const PdfRect& rect, double roundX, double roundY)
{
    PoDoFo::WriteRectangle(m_stream, rect.GetLeft(), rect.GetBottom(),
        rect.GetWidth(), rect.GetHeight(), roundX, roundY, m_CurrentPoint);
    m_opened = true;
}

void PdfPainterPath::AddRectangle(double x, double y, double width, double height,
    double roundX, double roundY)
{
    PoDoFo::WriteRectangle(m_stream, x, y, width, height, roundX, roundY, m_CurrentPoint);
    m_opened = true;
}

void PdfPainterPath::AddEllipse(double x, double y, double width, double height)
{
    PoDoFo::WriteEllipse(m_stream, x, y, width, height, m_CurrentPoint);
    m_opened = true;
}

void PdfPainterPath::Close()
{
    checkOpened();
    PoDoFo::WriteOperator_h(m_stream);
    //// CHECK-ME: How to update current point? Probably should track first point of the path
}

void PdfPainterPath::Reset()
{
    m_stream.Clear();
    m_opened = false;
    m_CurrentPoint = Vector2();
}

string_view PdfPainterPath::GetView() const
{
    return m_stream.GetString();
}

const Vector2& PdfPainterPath::GetCurrentPoint() const
{
    checkOpened();
    return m_CurrentPoint;
}

void PdfPainterPath::checkOpened() const
{
    if (!m_opened)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The path must be opened with MoveTo()");
}
