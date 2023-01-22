/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPage.h"

#include "PdfDictionary.h"
#include "PdfRect.h"
#include "PdfVariant.h"
#include "PdfWriter.h"
#include "PdfObjectStream.h"
#include "PdfColor.h"
#include "PdfDocument.h"

using namespace std;
using namespace PoDoFo;

static int normalize(int value, int start, int end);
static PdfResources* getResources(PdfObject& obj, const deque<PdfObject*>& listOfParents);

PdfPage::PdfPage(PdfDocument& parent, unsigned index, const PdfRect& size) :
    PdfDictionaryElement(parent, "Page"),
    m_Index(index),
    m_Contents(nullptr),
    m_Annotations(*this)
{
    initNewPage(size);
}

PdfPage::PdfPage(PdfObject& obj, unsigned index, const deque<PdfObject*>& listOfParents) :
    PdfDictionaryElement(obj),
    m_Index(index),
    m_Contents(nullptr),
    m_Resources(::getResources(obj, listOfParents)),
    m_Annotations(*this)
{
    PdfObject* contents = obj.GetDictionary().FindKey("Contents");
    if (contents != nullptr)
        m_Contents.reset(new PdfContents(*this, *contents));
}

PdfRect PdfPage::GetRect() const
{
    return this->GetMediaBox();
}

bool PdfPage::HasRotation(double& teta) const
{
    int rotationRaw = normalize(GetRotationRaw(), 0, 360);
    if (rotationRaw == 0)
    {
        teta = 0;
        return false;
    }

    // Convert to radians and make it a counterclockwise rotation,
    // as common mathematical notation for rotations
    teta = -rotationRaw * DEG2RAD;
    return true;
}

void PdfPage::initNewPage(const PdfRect& size)
{
    SetMediaBox(size);
}

void PdfPage::ensureContentsCreated()
{
    if (m_Contents != nullptr)
        return;

    m_Contents.reset(new PdfContents(*this));
    GetDictionary().AddKey(PdfName::KeyContents,
        m_Contents->GetObject().GetIndirectReference());
}

void PdfPage::ensureResourcesCreated()
{
    if (m_Resources != nullptr)
        return;

    m_Resources.reset(new PdfResources(GetDictionary()));
}

PdfObjectStream& PdfPage::GetStreamForAppending(PdfStreamAppendFlags flags)
{
    ensureContentsCreated();
    return m_Contents->GetStreamForAppending(flags);
}

PdfRect PdfPage::CreateStandardPageSize(const PdfPageSize pageSize, bool landscape)
{
    PdfRect rect;

    switch (pageSize)
    {
        case PdfPageSize::A0:
            rect.SetWidth(2384.0);
            rect.SetHeight(3370.0);
            break;

        case PdfPageSize::A1:
            rect.SetWidth(1684.0);
            rect.SetHeight(2384.0);
            break;

        case PdfPageSize::A2:
            rect.SetWidth(1191.0);
            rect.SetHeight(1684.0);
            break;

        case PdfPageSize::A3:
            rect.SetWidth(842.0);
            rect.SetHeight(1190.0);
            break;

        case PdfPageSize::A4:
            rect.SetWidth(595.0);
            rect.SetHeight(842.0);
            break;

        case PdfPageSize::A5:
            rect.SetWidth(420.0);
            rect.SetHeight(595.0);
            break;

        case PdfPageSize::A6:
            rect.SetWidth(297.0);
            rect.SetHeight(420.0);
            break;

        case PdfPageSize::Letter:
            rect.SetWidth(612.0);
            rect.SetHeight(792.0);
            break;

        case PdfPageSize::Legal:
            rect.SetWidth(612.0);
            rect.SetHeight(1008.0);
            break;

        case PdfPageSize::Tabloid:
            rect.SetWidth(792.0);
            rect.SetHeight(1224.0);
            break;

        default:
            break;
    }

    if (landscape)
    {
        double dTmp = rect.GetWidth();
        rect.SetWidth(rect.GetHeight());
        rect.SetHeight(dTmp);
    }

    return rect;
}

PdfRect PdfPage::getPageBox(const string_view& inBox) const
{
    PdfRect	pageBox;

    // Take advantage of inherited values - walking up the tree if necessary
    auto obj = GetDictionary().FindKeyParent(inBox);

    // assign the value of the box from the array
    if (obj != nullptr && obj->IsArray())
    {
        pageBox.FromArray(obj->GetArray());
    }
    else if (inBox == "ArtBox" ||
        inBox == "BleedBox" ||
        inBox == "TrimBox")
    {
        // If those page boxes are not specified then
        // default to CropBox per PDF Spec (3.6.2)
        pageBox = getPageBox("CropBox");
    }
    else if (inBox == "CropBox")
    {
        // If crop box is not specified then
        // default to MediaBox per PDF Spec (3.6.2)
        pageBox = getPageBox("MediaBox");
    }

    return pageBox;
}

int PdfPage::GetRotationRaw() const
{
    int rot = 0;

    auto obj = GetDictionary().FindKeyParent("Rotate");
    if (obj != nullptr && (obj->IsNumber() || obj->GetReal()))
        rot = static_cast<int>(obj->GetNumber());

    return rot;
}

void PdfPage::SetRotationRaw(int rotation)
{
    if (rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270)
        PODOFO_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    this->GetDictionary().AddKey("Rotate", PdfVariant(static_cast<int64_t>(rotation)));
}

void PdfPage::MoveAt(unsigned index)
{
    // TODO: CHECK-ME FOR CORRECT WORKING
    auto& doc = GetDocument();
    auto& pages = doc.GetPages();
    unsigned fromIndex = m_Index;
    pages.InsertDocumentPageAt(index, doc, m_Index);
    if (index < fromIndex)
    {
        // If we inserted the page before the old 
        // position we have to increment the from position
        fromIndex++;
    }

    pages.RemovePageAt(fromIndex);
    m_Index = fromIndex;
}

PdfField& PdfPage::CreateField(const string_view& name, PdfFieldType fieldType, const PdfRect& rect)
{
    auto& annotation = static_cast<PdfAnnotationWidget&>(GetAnnotations().CreateAnnot(PdfAnnotationType::Widget, rect));
    return PdfField::Create(name, annotation, fieldType);
}

PdfField& PdfPage::createField(const string_view& name, const type_info& typeInfo, const PdfRect& rect)
{
    auto& annotation = static_cast<PdfAnnotationWidget&>(GetAnnotations().CreateAnnot(PdfAnnotationType::Widget, rect));
    return PdfField::Create(name, annotation, typeInfo);
}

bool PdfPage::SetPageWidth(int newWidth)
{
    // Take advantage of inherited values - walking up the tree if necessary
    auto mediaBoxObj = GetDictionary().FindKeyParent("MediaBox");

    // assign the value of the box from the array
    if (mediaBoxObj != nullptr && mediaBoxObj->IsArray())
    {
        auto& mediaBoxArr = mediaBoxObj->GetArray();

        // in PdfRect::FromArray(), the Left value is subtracted from Width
        double dLeftMediaBox = mediaBoxArr[0].GetReal();
        mediaBoxArr[2] = PdfObject(newWidth + dLeftMediaBox);

        // Take advantage of inherited values - walking up the tree if necessary
        auto cropBoxObj = GetDictionary().FindKeyParent("CropBox");

        if (cropBoxObj != nullptr && cropBoxObj->IsArray())
        {
            auto& cropBoxArr = cropBoxObj->GetArray();
            // in PdfRect::FromArray(), the Left value is subtracted from Width
            double dLeftCropBox = cropBoxArr[0].GetReal();
            cropBoxArr[2] = PdfObject(newWidth + dLeftCropBox);
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

bool PdfPage::SetPageHeight(int newHeight)
{
    // Take advantage of inherited values - walking up the tree if necessary
    auto obj = GetDictionary().FindKeyParent("MediaBox");

    // assign the value of the box from the array
    if (obj != nullptr && obj->IsArray())
    {
        auto& mediaBoxArr = obj->GetArray();
        // in PdfRect::FromArray(), the Bottom value is subtracted from Height
        double bottom = mediaBoxArr[1].GetReal();
        mediaBoxArr[3] = PdfObject(newHeight + bottom);

        // Take advantage of inherited values - walking up the tree if necessary
        auto cropBoxObj = GetDictionary().FindKeyParent("CropBox");

        if (cropBoxObj != nullptr && cropBoxObj->IsArray())
        {
            auto& cropBoxArr = cropBoxObj->GetArray();
            // in PdfRect::FromArray(), the Bottom value is subtracted from Height
            double dBottomCropBox = cropBoxArr[1].GetReal();
            cropBoxArr[3] = PdfObject(newHeight + dBottomCropBox);
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

void PdfPage::SetMediaBox(const PdfRect& size)
{
    PdfArray mediaBox;
    size.ToArray(mediaBox);
    this->GetDictionary().AddKey("MediaBox", mediaBox);
}

void PdfPage::SetTrimBox(const PdfRect& size)
{
    PdfArray trimbox;
    size.ToArray(trimbox);
    this->GetDictionary().AddKey("TrimBox", trimbox);
}

unsigned PdfPage::GetPageNumber() const
{
    unsigned pageNumber = 0;
    auto parent = this->GetDictionary().FindKey("Parent");
    PdfReference ref = this->GetObject().GetIndirectReference();

    // CVE-2017-5852 - prevent infinite loop if Parent chain contains a loop
    // e.g. parent->GetIndirectKey( "Parent" ) == parent or parent->GetIndirectKey( "Parent" )->GetIndirectKey( "Parent" ) == parent
    constexpr unsigned maxRecursionDepth = 1000;
    unsigned depth = 0;

    while (parent != nullptr)
    {
        auto kidsObj = parent->GetDictionary().FindKey("Kids");
        if (kidsObj != nullptr)
        {
            const PdfArray& kids = kidsObj->GetArray();
            for (auto& child : kids)
            {
                if (child.GetReference() == ref)
                    break;

                auto node = this->GetDocument().GetObjects().GetObject(child.GetReference());
                if (node == nullptr)
                {
                    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NoObject,
                        "Object {} not found from Kids array {}", child.GetReference().ToString(),
                        kidsObj->GetIndirectReference().ToString());
                }

                if (node->GetDictionary().HasKey(PdfName::KeyType)
                    && node->GetDictionary().MustFindKey(PdfName::KeyType).GetName() == "Pages")
                {
                    auto count = node->GetDictionary().FindKey("Count");
                    if (count != nullptr)
                        pageNumber += static_cast<int>(count->GetNumber());
                }
                else
                {
                    // if we do not have a page tree node, 
                    // we most likely have a page object:
                    // so the page count is 1
                    pageNumber++;
                }
            }
        }

        ref = parent->GetIndirectReference();
        parent = parent->GetDictionary().FindKey("Parent");
        depth++;

        if (depth > maxRecursionDepth)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::BrokenFile, "Loop in Parent chain");
    }

    return ++pageNumber;
}

void PdfPage::SetICCProfile(const string_view& csTag, InputStream& stream,
    int64_t colorComponents, PdfColorSpace alternateColorSpace)
{
    // Check nColorComponents for a valid value
    if (colorComponents != 1 &&
        colorComponents != 3 &&
        colorComponents != 4)
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "SetICCProfile nColorComponents must be 1, 3 or 4!");
    }

    // Create a colorspace object
    auto& iccObject = this->GetDocument().GetObjects().CreateDictionaryObject();
    PdfName nameForCS = PoDoFo::ColorSpaceToNameRaw(alternateColorSpace);
    iccObject.GetDictionary().AddKey("Alternate", nameForCS);
    iccObject.GetDictionary().AddKey("N", colorComponents);
    iccObject.GetOrCreateStream().SetData(stream);

    // Add the colorspace
    PdfArray array;
    array.Add(PdfName("ICCBased"));
    array.Add(iccObject.GetIndirectReference());

    PdfDictionary iccBasedDictionary;
    iccBasedDictionary.AddKey(csTag, array);

    // Add the colorspace to resource
    GetOrCreateResources().GetDictionary().AddKey("ColorSpace", iccBasedDictionary);
}

PdfContents& PdfPage::GetOrCreateContents()
{
    ensureContentsCreated();
    return *m_Contents;
}

PdfResources* PdfPage::getResources() const
{
    return m_Resources.get();
}

PdfObject* PdfPage::getContentsObject() const
{
    if (m_Contents == nullptr)
        return nullptr;

    return &const_cast<PdfContents&>(*m_Contents).GetObject();
}

PdfElement& PdfPage::getElement() const
{
    return const_cast<PdfPage&>(*this);
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

PdfRect PdfPage::GetMediaBox() const
{
    return getPageBox("MediaBox");
}

PdfRect PdfPage::GetCropBox() const
{
    return getPageBox("CropBox");
}

PdfRect PdfPage::GetTrimBox() const
{
    return getPageBox("TrimBox");
}

PdfRect PdfPage::GetBleedBox() const
{
    return getPageBox("BleedBox");
}

PdfRect PdfPage::GetArtBox() const
{
    return getPageBox("ArtBox");
}

// https://stackoverflow.com/a/2021986/213871
int normalize(int value, int start, int end)
{
    int width = end - start;
    int offsetValue = value - start;   // value relative to 0

    // + start to reset back to start of original range
    return offsetValue - (offsetValue / width) * width + start;
}

PdfResources* getResources(PdfObject& obj, const deque<PdfObject*>& listOfParents)
{
    auto resources = obj.GetDictionary().FindKey("Resources");
    if (resources == nullptr)
    {
        // Resources might be inherited
        for (auto& parent : listOfParents)
            resources = parent->GetDictionary().FindKey("Resources");
    }

    if (resources == nullptr)
        return nullptr;

    return new PdfResources(*resources);
}
