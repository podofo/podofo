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

PdfPainterPath::PdfPainterPath() { }

void PdfPainterPath::MoveTo(double x, double y)
{
    open(x, y);
    PoDoFo::WriteOperator_m(m_stream, x, y);
    m_CurrentPoint = Vector2(x, y);
}

void PdfPainterPath::AddLineTo(double x, double y)
{
    checkOpened();
    PoDoFo::WriteOperator_l(m_stream, x, y);
    m_CurrentPoint = Vector2(x, y);
}

void PdfPainterPath::AddLine(double x1, double y1, double x2, double y2)
{
    open(x1, y1);
    PoDoFo::WriteOperator_m(m_stream, x1, y1);
    PoDoFo::WriteOperator_l(m_stream, x2, y2);
    m_CurrentPoint = Vector2(x2, y2);
}

void PdfPainterPath::AddCubicBezierTo(double x1, double y1, double x2, double y2, double x3, double y3)
{
    checkOpened();
    PoDoFo::WriteOperator_c(m_stream, x1, y1, x2, y2, x3, y3);
    m_CurrentPoint = Vector2(x3, y3);
}

void PdfPainterPath::AddCubicBezier(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4)
{
    open(x1, y1);
    PoDoFo::WriteOperator_m(m_stream, x1, y1);
    PoDoFo::WriteOperator_c(m_stream, x2, y2, x3, y3, x4, y4);
    m_CurrentPoint = Vector2(x4, y4);
}

void PdfPainterPath::AddArcTo(double x1, double y1, double x2, double y2, double radius)
{
    checkOpened();
    PoDoFo::WriteArcTo(m_stream, m_CurrentPoint.X, m_CurrentPoint.Y, x1, y1, x2, y2, radius, m_CurrentPoint);
}

void PdfPainterPath::AddArc(double x, double y, double radius,
    double angle1, double angle2, bool clockwise)
{
    open(x, y);
    PoDoFo::WriteArc(m_stream, x, y, radius, angle1, angle2, clockwise, m_CurrentPoint);
}

void PdfPainterPath::AddCircle(double x, double y, double radius)
{
    open(x, y);
    PoDoFo::WriteCircle(m_stream, x, y, radius, m_CurrentPoint);
}

void PdfPainterPath::AddRectangle(const Rect& rect, double roundX, double roundY)
{
    open(m_CurrentPoint.X, m_CurrentPoint.Y);
    PoDoFo::WriteRectangle(m_stream, rect.X, rect.Y,
        rect.Width, rect.Height, roundX, roundY, m_CurrentPoint);
}

void PdfPainterPath::AddPath(const PdfPainterPath& path, bool connect)
{
    auto& first = path.GetFirstPoint();
    if (connect && m_FirstPoint != nullptr)
        PoDoFo::WriteOperator_l(m_stream, first.X, first.Y);

    open(first.X, first.Y);
    static_cast<OutputStream&>(m_stream).Write(path.GetContent());
    m_CurrentPoint = path.GetCurrentPoint();
}

void PdfPainterPath::AddRectangle(double x, double y, double width, double height,
    double roundX, double roundY)
{
    open(x, y);
    PoDoFo::WriteRectangle(m_stream, x, y, width, height, roundX, roundY, m_CurrentPoint);
}

void PdfPainterPath::AddEllipse(double x, double y, double width, double height)
{
    open(x, y);
    PoDoFo::WriteEllipse(m_stream, x, y, width, height, m_CurrentPoint);
}

void PdfPainterPath::Close()
{
    checkOpened();
    PoDoFo::WriteOperator_h(m_stream);
    m_CurrentPoint = *m_FirstPoint;
}

void PdfPainterPath::Reset()
{
    m_stream.Clear();
    m_FirstPoint = nullptr;
    m_CurrentPoint = Vector2();
}

string_view PdfPainterPath::GetContent() const
{
    return m_stream.GetString();
}

const Vector2& PdfPainterPath::GetFirstPoint() const
{
    checkOpened();
    return *m_FirstPoint;
}

const Vector2& PdfPainterPath::GetCurrentPoint() const
{
    checkOpened();
    return m_CurrentPoint;
}

void PdfPainterPath::checkOpened() const
{
    if (m_FirstPoint == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "The path must be opened with MoveTo()");
}

void PdfPainterPath::open(double x, double y)
{
    if (m_FirstPoint != nullptr)
        return;

    m_FirstPoint = Vector2(x, y);
}
