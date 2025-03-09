/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPage.h"

#include "PdfDictionary.h"
#include "PdfColor.h"
#include "PdfDocument.h"
#include "PdfPageCollection.h"

using namespace std;
using namespace PoDoFo;

PdfPage::PdfPage(PdfDocument& parent, const Rect& size) :
    PdfDictionaryElement(parent, "Page"_n),
    m_Index(numeric_limits<unsigned>::max()),
    m_Rotation(0),
    m_Resources(new PdfResources(*this)), // A resource dictionary is actually required for pages
    m_Annotations(*this)
{
    SetMediaBox(size);
}

PdfPage::PdfPage(PdfObject& obj)
    : PdfPage(obj, vector<PdfObject*>())
{
}

PdfPage::PdfPage(PdfObject& obj, vector<PdfObject*>&& parents) :
    PdfDictionaryElement(obj),
    m_Index(numeric_limits<unsigned>::max()),
    m_Rotation(0),
    m_parents(std::move(parents)),
    m_Annotations(*this)
{
    auto contents = GetDictionary().FindKey("Contents");
    if (contents != nullptr)
        m_Contents.reset(new PdfContents(*this, *contents));

    auto resources = findInheritableAttribute("Resources");
    if (resources != nullptr)
        m_Resources.reset(new PdfResources(*resources));

    double rotation;
    if (TryGetRotationRaw(rotation))
        m_Rotation = utls::NormalizePageRotation(rotation);

    // NOTE: Rotation must be fetched before computing normalized rect
    m_Rect = GetMediaBox();
}

Corners PdfPage::GetRectRaw() const
{
    return this->GetMediaBoxRaw();
}

void PdfPage::SetRectRaw(const Corners& rect)
{
    PdfArray mediaBox;
    rect.ToArray(mediaBox);
    this->GetDictionary().AddKey("MediaBox"_n, mediaBox);
    m_Rect = rect.GetNormalized();
    adjustRectToCurrentRotation(m_Rect);
}

void PdfPage::SetRect(const Rect& rect)
{
    SetMediaBox(rect);
}

bool PdfPage::TryGetRotationRadians(double& teta) const
{
    if (m_Rotation == 0)
    {
        teta = 0;
        return false;
    }

    // Convert to radians and make it a counterclockwise rotation,
    // as common mathematical notation for rotations
    teta = -(m_Rotation * DEG2RAD);
    return true;
}

double PdfPage::GetRotationRadians() const
{
    // Convert to radians and make it a counterclockwise rotation,
    // as common mathematical notation for rotations
    return -(m_Rotation * DEG2RAD);
}

void PdfPage::ensureContentsCreated()
{
    if (m_Contents != nullptr)
        return;

    m_Contents.reset(new PdfContents(*this));
    GetDictionary().AddKey("Contents"_n,
        m_Contents->GetObject().GetIndirectReference());
}

PdfObjectStream& PdfPage::GetOrCreateContentsStream(PdfStreamAppendFlags flags)
{
    ensureContentsCreated();
    return m_Contents->CreateStreamForAppending(flags);
}

PdfObjectStream& PdfPage::ResetContentsStream()
{
    ensureContentsCreated();
    m_Contents->Reset();
    return m_Contents->CreateStreamForAppending();
}

Rect PdfPage::CreateStandardPageSize(const PdfPageSize pageSize, bool landscape)
{
    Rect rect;

    switch (pageSize)
    {
        case PdfPageSize::A0:
            rect.Width = 2384;
            rect.Height = 3370;
            break;

        case PdfPageSize::A1:
            rect.Width = 1684;
            rect.Height = 2384;
            break;

        case PdfPageSize::A2:
            rect.Width = 1191;
            rect.Height = 1684;
            break;

        case PdfPageSize::A3:
            rect.Width = 842;
            rect.Height = 1190;
            break;

        case PdfPageSize::A4:
            rect.Width = 595;
            rect.Height = 842;
            break;

        case PdfPageSize::A5:
            rect.Width = 420;
            rect.Height = 595;
            break;

        case PdfPageSize::A6:
            rect.Width = 297;
            rect.Height = 420;
            break;

        case PdfPageSize::Letter:
            rect.Width = 612;
            rect.Height = 792;
            break;

        case PdfPageSize::Legal:
            rect.Width = 612;
            rect.Height = 1008;
            break;

        case PdfPageSize::Tabloid:
            rect.Width = 792;
            rect.Height = 1224;
            break;

        default:
            break;
    }

    if (landscape)
    {
        double tmp = rect.Width;
        rect.Width = rect.Height;
        rect.Height = tmp;
    }

    return rect;
}

Rect PdfPage::getPageBox(const string_view& inBox, bool isInheritable) const
{
    auto ret = Rect::FromCorners(getPageBoxRaw(inBox, isInheritable));
    adjustRectToCurrentRotation(ret);
    return ret;
}

Corners PdfPage::getPageBoxRaw(const string_view& inBox, bool isInheritable) const
{
    // Take advantage of inherited values - walking up the tree if necessary
    const PdfObject* obj;
    if (isInheritable)
        obj = findInheritableAttribute(inBox);
    else
        obj = GetDictionary().FindKeyParent(inBox);

    // assign the value of the box from the array
    if (obj != nullptr && obj->IsArray())
    {
        return Corners::FromArray(obj->GetArray());
    }
    else if (inBox == "ArtBox" ||
        inBox == "BleedBox" ||
        inBox == "TrimBox")
    {
        // If those page boxes are not specified then
        // default to CropBox per PDF Spec (3.6.2)
        return getPageBoxRaw("CropBox", true);
    }
    else if (inBox == "CropBox")
    {
        // If crop box is not specified then
        // default to MediaBox per PDF Spec (3.6.2)
        return getPageBoxRaw("MediaBox", true);
    }

    return Corners();
}

void PdfPage::setPageBox(const PdfName& inBox, const Rect& rect)
{
    auto actualRect = rect;
    adjustRectToCurrentRotation(actualRect);
    PdfArray mediaBox;
    actualRect.ToArray(mediaBox);
    this->GetDictionary().AddKey(inBox, mediaBox);
}

void PdfPage::adjustRectToCurrentRotation(Rect& rect) const
{
    switch (GetRotation())
    {
        case 90:
        case 270:
        {
            double temp = rect.Width;
            rect.Width = rect.Height;
            rect.Height = temp;
            break;
        }
        case 0:
        case 180:
            break;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Invalid rotation");
    }
}

bool PdfPage::TryGetRotationRaw(double& rotation) const
{
    auto obj = findInheritableAttribute("Rotate");
    if (obj == nullptr || !obj->TryGetReal(rotation))
    {
        rotation = 0;
        return false;
    }

    return true;
}

void PdfPage::SetRotation(int rotation)
{
    if (rotation % 90 != 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Page rotation {} is invalid, must be a multiple of 90", rotation);

    // We perform a normalization anyway
    rotation = utls::NormalizePageRotation(rotation);
    this->GetDictionary().AddKey("Rotate"_n, PdfVariant(static_cast<int64_t>(rotation)));
    m_Rotation = rotation;
}

bool PdfPage::MoveTo(unsigned index)
{
    if (index == m_Index)
        return false;

    auto& pages = GetDocument().GetPages();
    return pages.TryMovePageTo(m_Index, index);
}

PdfField& PdfPage::CreateField(const string_view& name, PdfFieldType fieldType, const Rect& rect)
{
    auto& annotation = static_cast<PdfAnnotationWidget&>(GetAnnotations()
        .CreateAnnot(PdfAnnotationType::Widget, rect));
    return PdfField::Create(name, annotation, fieldType);
}

void PdfPage::FlattenStructure()
{
    if (m_parents.size() == 0)
        return;

    static PdfName inheritableAttributes[] = { "Resources"_n, "MediaBox"_n, "CropBox"_n, "Rotate"_n };

    bool isShallow;
    // Move inherited attributes to current dictionary
    for (unsigned i = 0; i < std::size(inheritableAttributes); i++)
    {
        auto obj = findInheritableAttribute(inheritableAttributes[i], isShallow);
        if (obj != nullptr && !isShallow)
            GetDictionary().AddKeyIndirectSafe(inheritableAttributes[i], *obj);
    }

    // Finally clear the parents
    m_parents.clear();
}

void PdfPage::CopyContentsTo(OutputStream& stream) const
{
    if (m_Contents == nullptr)
        return;

    m_Contents->CopyTo(stream);
}

void PdfPage::SetMediaBox(const Rect& rect)
{
    setPageBox("MediaBox"_n, rect);
    m_Rect = rect;
}

void PdfPage::SetCropBox(const Rect& rect)
{
    setPageBox("CropBox"_n, rect);
}

void PdfPage::SetTrimBox(const Rect& rect)
{
    setPageBox("TrimBox"_n, rect);
}

void PdfPage::SetBleedBox(const Rect& rect)
{
    setPageBox("BleedBox"_n, rect);
}

void PdfPage::SetArtBox(const Rect& rect)
{
    setPageBox("ArtBox"_n, rect);
}

unsigned PdfPage::GetPageNumber() const
{
    return m_Index + 1;
}

PdfPageFieldIterable PdfPage::GetFieldsIterator()
{
    return PdfPageFieldIterable(*this);
}

PdfPageConstFieldIterable PdfPage::GetFieldsIterator() const
{
    return PdfPageConstFieldIterable(const_cast<PdfPage&>(*this));
}

PdfContents& PdfPage::GetOrCreateContents()
{
    ensureContentsCreated();
    return *m_Contents;
}

PdfResources* PdfPage::getResources()
{
    return m_Resources.get();
}

PdfObject* PdfPage::getContentsObject()
{
    if (m_Contents == nullptr)
        return nullptr;

    return &m_Contents->GetObject();
}

PdfDictionaryElement& PdfPage::getElement()
{
    return *this;
}

PdfObject* PdfPage::findInheritableAttribute(const string_view& name) const
{
    bool isShallow;
    return findInheritableAttribute(name, isShallow);
}

PdfObject* PdfPage::findInheritableAttribute(const string_view& name, bool& isShallow) const
{
    auto& dict = const_cast<PdfPage&>(*this).GetDictionary();
    auto obj = dict.FindKeyParent(name);
    if (obj != nullptr)
    {
        isShallow = true;
        return obj;
    }

    isShallow = false;
    for (unsigned i = 0; i < m_parents.size(); i++)
    {
        obj = m_parents[i]->GetDictionary().FindKeyParent(name);
        if (obj != nullptr)
            return obj;
    }

    return nullptr;
}

PdfResources& PdfPage::GetOrCreateResources()
{
    return GetResources();
}

const PdfContents& PdfPage::MustGetContents() const
{
    if (m_Contents == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    return *m_Contents;
}

PdfContents& PdfPage::MustGetContents()
{
    if (m_Contents == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    return *m_Contents;
}

const PdfResources& PdfPage::GetResources() const
{
    return const_cast<PdfPage&>(*this).GetResources();
}

PdfResources& PdfPage::GetResources()
{
    if (m_Resources != nullptr)
        return *m_Resources;

    m_Resources.reset(new PdfResources(*this));
    return *m_Resources;
}

Rect PdfPage::GetMediaBox() const
{
    return getPageBox("MediaBox", true);
}

Corners PdfPage::GetMediaBoxRaw() const
{
    return getPageBoxRaw("MediaBox", true);
}

Rect PdfPage::GetCropBox() const
{
    return getPageBox("CropBox", true);
}

Corners PdfPage::GetCropBoxRaw() const
{
    return getPageBoxRaw("CropBox", true);
}

Rect PdfPage::GetTrimBox() const
{
    return getPageBox("TrimBox", false);
}

Corners PdfPage::GetTrimBoxRaw() const
{
    return getPageBoxRaw("TrimBox", false);
}

Rect PdfPage::GetBleedBox() const
{
    return getPageBox("BleedBox", false);
}

Corners PdfPage::GetBleedBoxRaw() const
{
    return getPageBoxRaw("BleedBox", false);
}

Rect PdfPage::GetArtBox() const
{
    return getPageBox("ArtBox", false);
}

Corners PdfPage::GetArtBoxRaw() const
{
    return getPageBoxRaw("ArtBox", false);
}
