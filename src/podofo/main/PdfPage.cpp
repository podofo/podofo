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
    m_Contents(nullptr),
    m_Annotations(*this),
    m_Rotation(-1)
{
    initNewPage(size);
}

PdfPage::PdfPage(PdfObject& obj)
    : PdfPage(obj, vector<PdfObject*>())
{
}

PdfPage::PdfPage(PdfObject& obj, vector<PdfObject*>&& parents) :
    PdfDictionaryElement(obj),
    m_Index(numeric_limits<unsigned>::max()),
    m_parents(std::move(parents)),
    m_Contents(nullptr),
    m_Annotations(*this),
    m_Rotation(-1)
{
    auto contents = GetDictionary().FindKey("Contents");
    if (contents != nullptr)
        m_Contents.reset(new PdfContents(*this, *contents));

    auto resources = findInheritableAttribute("Resources");
    if (resources != nullptr)
        m_Resources.reset(new PdfResources(*resources));
}

Rect PdfPage::GetRect() const
{
    return this->GetMediaBox();
}

Rect PdfPage::GetRectRaw() const
{
    return this->GetMediaBox(true);
}

void PdfPage::SetRect(const Rect& rect)
{
    SetMediaBox(rect);
}

void PdfPage::SetRectRaw(const Rect& rect)
{
    SetMediaBox(rect, true);
}

bool PdfPage::HasRotation(double& teta) const
{
    int rotation = GetRotation();
    if (rotation == 0)
    {
        teta = 0;
        return false;
    }

    // Convert to radians and make it a counterclockwise rotation,
    // as common mathematical notation for rotations
    teta = -rotation * DEG2RAD;
    return true;
}

void PdfPage::initNewPage(const Rect& size)
{
    SetMediaBox(size);
}

void PdfPage::ensureContentsCreated()
{
    if (m_Contents != nullptr)
        return;

    m_Contents.reset(new PdfContents(*this));
    GetDictionary().AddKey("Contents"_n,
        m_Contents->GetObject().GetIndirectReference());
}

void PdfPage::ensureResourcesCreated()
{
    if (m_Resources != nullptr)
        return;

    m_Resources.reset(new PdfResources(*this));
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

Rect PdfPage::getPageBox(const string_view& inBox, bool isInheritable, bool raw) const
{
    Rect pageBox;

    // Take advantage of inherited values - walking up the tree if necessary
    const PdfObject* obj;
    if (isInheritable)
        obj = findInheritableAttribute(inBox);
    else
        obj = GetDictionary().FindKeyParent(inBox);

    // assign the value of the box from the array
    if (obj != nullptr && obj->IsArray())
    {
        pageBox = Rect::FromArray(obj->GetArray());
    }
    else if (inBox == "ArtBox" ||
        inBox == "BleedBox" ||
        inBox == "TrimBox")
    {
        // If those page boxes are not specified then
        // default to CropBox per PDF Spec (3.6.2)
        pageBox = getPageBox("CropBox", true, raw);
    }
    else if (inBox == "CropBox")
    {
        // If crop box is not specified then
        // default to MediaBox per PDF Spec (3.6.2)
        pageBox = getPageBox("MediaBox", true, raw);
    }

    if (!raw)
    {
        switch (GetRotation())
        {
            case 90:
            case 270:
            {
                double temp = pageBox.Width;
                pageBox.Width = pageBox.Height;
                pageBox.Height = temp;
                break;
            }
            case 0:
            case 180:
                break;
            default:
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Invalid rotation");
        }
    }

    return pageBox;
}

void PdfPage::setPageBox(const PdfName& inBox, const Rect& rect, bool raw)
{
    auto actualRect = rect;
    if (!raw)
    {
        switch (GetRotation())
        {
            case 90:
            case 270:
            {
                actualRect.Width = rect.Height;
                actualRect.Height = rect.Width;
                break;
            }
            case 0:
            case 180:
                break;
            default:
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Invalid rotation");
        }
    }

    PdfArray mediaBox;
    actualRect.ToArray(mediaBox);
    this->GetDictionary().AddKey(inBox, mediaBox);
}

void PdfPage::loadRotation()
{
    if (m_Rotation >= 0)
        return;

    m_Rotation = utls::NormalizePageRotation(GetRotationRaw());
}

double PdfPage::GetRotationRaw() const
{
    auto obj = findInheritableAttribute("Rotate");
    double rotation;
    if (obj == nullptr || !obj->TryGetReal(rotation))
        return 0;

    return rotation;
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

PdfField& PdfPage::CreateField(const string_view& name, PdfFieldType fieldType, const Rect& rect, bool rawRect)
{
    auto& annotation = static_cast<PdfAnnotationWidget&>(GetAnnotations()
        .CreateAnnot(PdfAnnotationType::Widget, rect, rawRect));
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

void PdfPage::EnsureResourcesCreated()
{
    ensureResourcesCreated();
}

void PdfPage::CopyContentsTo(OutputStream& stream) const
{
    if (m_Contents == nullptr)
        return;

    m_Contents->CopyTo(stream);
}

void PdfPage::SetMediaBox(const Rect& rect, bool raw)
{
    setPageBox("MediaBox"_n, rect, raw);
}

void PdfPage::SetCropBox(const Rect& rect, bool raw)
{
    setPageBox("CropBox"_n, rect, raw);
}

void PdfPage::SetTrimBox(const Rect& rect, bool raw)
{
    setPageBox("TrimBox"_n, rect, raw);
}

void PdfPage::SetBleedBox(const Rect& rect, bool raw)
{
    setPageBox("BleedBox"_n, rect, raw);
}

void PdfPage::SetArtBox(const Rect& rect, bool raw)
{
    setPageBox("ArtBox"_n, rect, raw);
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

    return &const_cast<PdfContents&>(*m_Contents).GetObject();
}

PdfDictionaryElement& PdfPage::getElement()
{
    return const_cast<PdfPage&>(*this);
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
    ensureResourcesCreated();
    return *m_Resources;
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

const PdfResources& PdfPage::MustGetResources() const
{
    if (m_Resources == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    return *m_Resources;
}

PdfResources& PdfPage::MustGetResources()
{
    if (m_Resources == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    return *m_Resources;
}

Rect PdfPage::GetMediaBox(bool raw) const
{
    return getPageBox("MediaBox", true, raw);
}

Rect PdfPage::GetCropBox(bool raw) const
{
    return getPageBox("CropBox", true, raw);
}

Rect PdfPage::GetTrimBox(bool raw) const
{
    return getPageBox("TrimBox", false, raw);
}

Rect PdfPage::GetBleedBox(bool raw) const
{
    return getPageBox("BleedBox", false, raw);
}

Rect PdfPage::GetArtBox(bool raw) const
{
    return getPageBox("ArtBox", false, raw);
}

unsigned PdfPage::GetRotation() const
{
    const_cast<PdfPage&>(*this).loadRotation();
    return (unsigned)m_Rotation;
}
