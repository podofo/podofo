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

PdfXObjectForm::PdfXObjectForm(PdfDocument& doc, const Rect& rect, const string_view& prefix)
    : PdfXObject(doc, PdfXObjectType::Form, prefix), m_Rect(rect)
{
    initXObject(rect);
}

PdfXObjectForm::PdfXObjectForm(PdfObject& obj)
    : PdfXObject(obj, PdfXObjectType::Form)
{
    if (obj.GetDictionary().HasKey("BBox"))
        m_Rect = Rect::FromArray(obj.GetDictionary().MustFindKey("BBox").GetArray());

    auto resources = obj.GetDictionary().FindKey("Resources");
    if (resources != nullptr)
        m_Resources.reset(new PdfResources(*resources));
}

void PdfXObjectForm::FillFromPage(const PdfPage& page, bool useTrimBox)
{
    // After filling set correct BBox, independent of rotation
    m_Rect = GetDocument().FillXObjectFromPage(*this, page, useTrimBox);
    initAfterPageInsertion(page);
}

void PdfXObjectForm::EnsureResourcesCreated()
{
    if (m_Resources == nullptr)
        m_Resources.reset(new PdfResources(GetDictionary()));

    // A Form XObject must have a stream
    GetObject().ForceCreateStream();
}

bool PdfXObjectForm::HasRotation(double& teta) const
{
    teta = 0;
    return false;
}

void PdfXObjectForm::SetRect(const Rect& rect)
{
    PdfArray bbox;
    rect.ToArray(bbox);
    GetObject().GetDictionary().AddKey("BBox", bbox);
    m_Rect = rect;
}

PdfResources* PdfXObjectForm::getResources()
{
    return m_Resources.get();
}

PdfElement& PdfXObjectForm::getElement()
{
    return const_cast<PdfXObjectForm&>(*this);
}

inline PdfObjectStream& PdfXObjectForm::GetStreamForAppending(PdfStreamAppendFlags flags)
{
    (void)flags; // Flags have no use here
    return GetObject().GetOrCreateStream();
}

Rect PdfXObjectForm::GetRect() const
{
    return m_Rect;
}

Rect PdfXObjectForm::GetRectRaw() const
{
    return m_Rect;
}

PdfObject* PdfXObjectForm::getContentsObject()
{
    return &const_cast<PdfXObjectForm&>(*this).GetObject();
}

PdfResources& PdfXObjectForm::GetOrCreateResources()
{
    EnsureResourcesCreated();
    return *m_Resources;
}

void PdfXObjectForm::initXObject(const Rect& rect)
{
    // Initialize static data
    if (m_Matrix.IsEmpty())
    {
        // This matrix is the same for all PdfXObjects so cache it
        m_Matrix.Add(PdfObject(static_cast<int64_t>(1)));
        m_Matrix.Add(PdfObject(static_cast<int64_t>(0)));
        m_Matrix.Add(PdfObject(static_cast<int64_t>(0)));
        m_Matrix.Add(PdfObject(static_cast<int64_t>(1)));
        m_Matrix.Add(PdfObject(static_cast<int64_t>(0)));
        m_Matrix.Add(PdfObject(static_cast<int64_t>(0)));
    }

    PdfArray bbox;
    rect.ToArray(bbox);
    this->GetObject().GetDictionary().AddKey("BBox", bbox);
    this->GetObject().GetDictionary().AddKey("FormType", PdfVariant(static_cast<int64_t>(1))); // only 1 is only defined in the specification.
    this->GetObject().GetDictionary().AddKey("Matrix", m_Matrix);
}

void PdfXObjectForm::initAfterPageInsertion(const PdfPage& page)
{
    PdfArray bbox;
    m_Rect.ToArray(bbox);
    this->GetObject().GetDictionary().AddKey("BBox", bbox);

    int rotation = page.GetRotationRaw();
    // correct negative rotation
    if (rotation < 0)
        rotation = 360 + rotation;

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

    this->GetObject().GetDictionary().AddKey("Matrix", matrix);
}
