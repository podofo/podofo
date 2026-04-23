// SPDX-FileCopyrightText: 2007 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2021 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfXObjectForm.h"
#include "PdfDictionary.h"
#include "PdfDocument.h"
#include "PdfMath.h"
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
    // After filling, set correct BBox and Matrix accounting for page rotation
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
    // BBox must be in form space (ISO 32000-1 §8.10.2), but m_Rect arrives
    // post-rotation from GetMediaBox() with W/H already swapped
    switch (page.GetRotation())
    {
        case 90:
        case 270:
        {
            double temp = m_Rect.Width;
            m_Rect.Width = m_Rect.Height;
            m_Rect.Height = temp;
            break;
        }
        default:
            break;
    }

    PdfArray bbox;
    m_Rect.ToArray(bbox);
    GetDictionary().AddKey("BBox"_n, bbox);

    double teta;
    if (!page.TryGetRotationRadians(teta))
    {
        if (m_Rect.X != 0 || m_Rect.Y != 0)
            SetMatrix(Matrix::CreateTranslation(Vector2(-m_Rect.X, -m_Rect.Y)));

        return;
    }

    auto matrix = GetFrameRotationTransform(m_Rect, teta);

    // Shift so DrawXObject(xobj, px, py) places content at (px, py) not (px+X, py+Y)
    if (m_Rect.X != 0 || m_Rect.Y != 0)
        matrix = matrix * Matrix::CreateTranslation(Vector2(-m_Rect.X, -m_Rect.Y));

    SetMatrix(matrix);
}
