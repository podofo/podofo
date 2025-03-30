/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfXObjectForm.h"
#include "PdfDictionary.h"
#include "PdfDocument.h"
#include "PdfPage.h"

using namespace std;
using namespace PoDoFo;

PdfXObjectForm::PdfXObjectForm(PdfDocument& doc, const Rect& rect)
    : PdfXObject(doc, PdfXObjectType::Form), m_Rect(rect)
{
    initXObject(rect);
}

PdfXObjectForm::PdfXObjectForm(PdfObject& obj)
    : PdfXObject(obj, PdfXObjectType::Form)
{
    auto& dict = GetDictionary();
    const PdfArray* arr;
    if (dict.TryFindKeyAs("BBox", arr))
        m_Rect = Rect::FromArray(*arr);

    if (dict.TryFindKeyAs("Matrix", arr))
        m_Matrix = Matrix::FromArray(*arr);

    auto resources = dict.FindKey("Resources");
    if (resources != nullptr)
        m_Resources.reset(new PdfResources(*resources));
}

void PdfXObjectForm::FillFromPage(const PdfPage& page, bool useTrimBox)
{
    // After filling set correct BBox, independent of rotation
    m_Rect = GetDocument().FillXObjectFromPage(*this, page, useTrimBox);
    initAfterPageInsertion(page);
}

bool PdfXObjectForm::TryGetRotationRadians(double& teta) const
{
    teta = 0;
    return false;
}

void PdfXObjectForm::SetRect(const Rect& rect)
{
    PdfArray bbox;
    rect.ToArray(bbox);
    GetDictionary().AddKey("BBox"_n, bbox);
    m_Rect = rect;
}

void PdfXObjectForm::SetMatrix(const Matrix& m)
{
    PdfArray arr;
    arr.Add(m[0]);
    arr.Add(m[1]);
    arr.Add(m[2]);
    arr.Add(m[3]);
    arr.Add(m[4]);
    arr.Add(m[5]);

    GetDictionary().AddKey("Matrix"_n, std::move(arr));
    m_Matrix = m;
}

const Matrix& PdfXObjectForm::GetMatrix() const
{
    return m_Matrix;
}

PdfResources* PdfXObjectForm::getResources()
{
    return m_Resources.get();
}

PdfDictionaryElement& PdfXObjectForm::getElement()
{
    return *this;
}

PdfObjectStream& PdfXObjectForm::GetOrCreateContentsStream(PdfStreamAppendFlags flags)
{
    (void)flags; // Flags have no use here
    return GetObject().GetOrCreateStream();
}

PdfObjectStream& PdfXObjectForm::ResetContentsStream()
{
    auto& ret = GetObject().GetOrCreateStream();
    ret.Clear();
    return ret;
}

void PdfXObjectForm::CopyContentsTo(OutputStream& stream) const
{
    auto objStream = GetObject().GetStream();
    if (objStream == nullptr)
        return;

    objStream->CopyTo(stream);
}

Rect PdfXObjectForm::GetRect() const
{
    return m_Rect;
}

Corners PdfXObjectForm::GetRectRaw() const
{
    return Corners::FromCorners(m_Rect.GetLeftBottom(), m_Rect.GetRightTop());
}

PdfObject* PdfXObjectForm::getContentsObject()
{
    return &GetObject();
}

PdfResources& PdfXObjectForm::GetOrCreateResources()
{
    if (m_Resources == nullptr)
        m_Resources.reset(new PdfResources(*this));

    // A Form XObject must have a stream
    GetObject().ForceCreateStream();
    return *m_Resources;
}

const PdfXObjectForm* PdfXObjectForm::GetForm() const
{
    return this;
}

void PdfXObjectForm::initXObject(const Rect& rect)
{
    // Initialize static data
    PdfArray arr;
    arr.Add(PdfObject(static_cast<int64_t>(m_Matrix[0])));
    arr.Add(PdfObject(static_cast<int64_t>(m_Matrix[1])));
    arr.Add(PdfObject(static_cast<int64_t>(m_Matrix[2])));
    arr.Add(PdfObject(static_cast<int64_t>(m_Matrix[3])));
    arr.Add(PdfObject(static_cast<int64_t>(m_Matrix[4])));
    arr.Add(PdfObject(static_cast<int64_t>(m_Matrix[5])));

    PdfArray bbox;
    rect.ToArray(bbox);
    GetDictionary().AddKey("BBox"_n, bbox);
    GetDictionary().AddKey("FormType"_n, PdfVariant(static_cast<int64_t>(1))); // only 1 is only defined in the specification.
    GetDictionary().AddKey("Matrix"_n, arr);
}

void PdfXObjectForm::initAfterPageInsertion(const PdfPage& page)
{
    PdfArray bbox;
    m_Rect.ToArray(bbox);
    GetDictionary().AddKey("BBox"_n, bbox);

    int rotation = page.GetRotation();
    // Swap offsets/width/height for vertical rotation
    switch (rotation)
    {
        case 90:
        case 270:
        {
            double temp;

            temp = m_Rect.Width;
            m_Rect.Width = m_Rect.Height;
            m_Rect.Height = temp;

            temp = m_Rect.X;
            m_Rect.X = m_Rect.Y;
            m_Rect.Y = temp;
        }
        break;

        default:
            break;
    }

    // Build matrix for rotation and cropping
    double alpha = -rotation / 360.0 * 2.0 * numbers::pi;

    double a, b, c, d, e, f;

    a = cos(alpha);
    b = sin(alpha);
    c = -sin(alpha);
    d = cos(alpha);

    switch (rotation)
    {
        case 90:
            e = -m_Rect.X;
            f = m_Rect.Y + m_Rect.Height;
            break;

        case 180:
            e = m_Rect.X + m_Rect.Width;
            f = m_Rect.Y + m_Rect.Height;
            break;

        case 270:
            e = m_Rect.X + m_Rect.Width;
            f = -m_Rect.Y;
            break;

        case 0:
        default:
            e = -m_Rect.X;
            f = -m_Rect.Y;
            break;
    }

    PdfArray matrix;
    matrix.Add(PdfObject(a));
    matrix.Add(PdfObject(b));
    matrix.Add(PdfObject(c));
    matrix.Add(PdfObject(d));
    matrix.Add(PdfObject(e));
    matrix.Add(PdfObject(f));

    GetDictionary().AddKey("Matrix"_n, matrix);
}
