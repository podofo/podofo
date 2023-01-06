/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfRect.h"

#include "PdfArray.h"
#include "PdfVariant.h"
#include "PdfMath.h"

#include <iomanip>

static void CreateRect(double x1, double y1, double x2, double y2,
    double& left, double& bottom, double& width, double& height);
static void NormalizeCoordinates(double& coord1, double& coord2);

using namespace std;
using namespace PoDoFo;

PdfRect::PdfRect()
{
    m_Bottom = m_Left = m_Width = m_Height = 0;
}

PdfRect::PdfRect(double left, double bottom, double width, double height)
{
    m_Bottom = bottom;
    m_Left = left;
    m_Width = width;
    m_Height = height;
}

PdfRect PdfRect::FromCorners(double x1, double y1, double x2, double y2)
{
    PdfRect rect;
    CreateRect(x1, y1, x2, y2, rect.m_Left, rect.m_Bottom, rect.m_Width, rect.m_Height);
    return rect;
}

PdfRect::PdfRect(const PdfArray& arr)
{
    m_Bottom = m_Left = m_Width = m_Height = 0;
    FromArray(arr);
}

void PdfRect::ToArray(PdfArray& arr) const
{
    arr.Clear();
    arr.Add(PdfObject(m_Left));
    arr.Add(PdfObject(m_Bottom));
    arr.Add(PdfObject((m_Width + m_Left)));
    arr.Add(PdfObject((m_Height + m_Bottom)));
}

string PdfRect::ToString() const
{
    PdfArray arr;
    string str;
    this->ToArray(arr);
    PdfVariant(arr).ToString(str);
    return str;
}

bool PdfRect::Contains(double x, double y) const
{
	return x >= m_Left && x <= m_Left + m_Width
		&& y >= m_Bottom && y <= m_Bottom + m_Height;
}

void PdfRect::FromArray(const PdfArray& arr)
{
    if (arr.size() == 4)
    {
        double x1 = arr[0].GetReal();
        double y1 = arr[1].GetReal();
        double x2 = arr[2].GetReal();
        double y2 = arr[3].GetReal();

        CreateRect(x1, y1, x2, y2, m_Left, m_Bottom, m_Width, m_Height);
    }
    else
    {
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
    }
}

double PdfRect::GetRight() const
{
    return m_Left + m_Width;
}

double PdfRect::GetTop() const
{
    return m_Bottom + m_Height;
}

void PdfRect::Intersect(const PdfRect& rect)
{
    if (rect.GetBottom() != 0 || rect.GetHeight() != 0 || rect.GetLeft() != 0 || rect.GetWidth() != 0)
    {
        double diff;

        diff = rect.m_Left - m_Left;
        if (diff > 0.0)
        {
            m_Left += diff;
            m_Width -= diff;
        }

        diff = (m_Left + m_Width) - (rect.m_Left + rect.m_Width);
        if (diff > 0.0)
        {
            m_Width -= diff;
        }

        diff = rect.m_Bottom - m_Bottom;
        if (diff > 0.0)
        {
            m_Bottom += diff;
            m_Height -= diff;
        }

        diff = (m_Bottom + m_Height) - (rect.m_Bottom + rect.m_Height);
        if (diff > 0.0)
        {
            m_Height -= diff;
        }
    }
}

PdfRect PdfRect::operator*(const Matrix& m) const
{
    Vector2 corner1(m_Left, m_Bottom);
    Vector2 corner2(GetRight(), GetTop());
    corner1 = corner1 * m;
    corner2 = corner2 * m;
    return PdfRect::FromCorners(corner1.X, corner1.Y, corner2.X, corner2.Y);
}

void CreateRect(double x1, double y1, double x2, double y2, double& left, double& bottom, double& width, double& height)
{
    // See Pdf Reference 1.7, 3.8.4 Rectangles
    NormalizeCoordinates(x1, x2);
    NormalizeCoordinates(y1, y2);

    left = x1;
    bottom = y1;
    width = x2 - x1;
    height = y2 - y1;
}

void NormalizeCoordinates(double& coord1, double& coord2)
{
    if (coord1 > coord2)
    {
        double temp = coord1;
        coord1 = coord2;
        coord2 = temp;
    }
}
