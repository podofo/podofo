/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>

#include "PdfAnnotation.h"

#include <podofo/private/PdfDrawingOperations.h>

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfPage.h"
#include "PdfXObjectForm.h"
#include "PdfAnnotationWidget.h"
#include "PdfAnnotation_Types.h"
#include "PdfMath.h"

using namespace std;
using namespace PoDoFo;

static PdfName getAppearanceName(PdfAppearanceType appearance);

PdfAnnotation::PdfAnnotation(PdfPage& page, PdfAnnotationType annotType, const Rect& rect)
    : PdfDictionaryElement(page.GetDocument(), "Annot"_n), m_AnnotationType(annotType), m_Page(&page)
{
    const PdfName name(PoDoFo::ToString(annotType));

    if (name.IsNull())
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    PdfArray arr;
    rect.ToArray(arr);

    GetDictionary().AddKey("Subtype"_n, name);
    GetDictionary().AddKey("Rect"_n, arr);
    GetDictionary().AddKey("P"_n, page.GetObject().GetIndirectReference());

    // Default set print flag
    auto flags = GetFlags();
    SetFlags(flags | PdfAnnotationFlags::Print);

    if (annotType != PdfAnnotationType::Widget
        && (GetDocument().GetMetadata().GetPdfUALevel() != PdfUALevel::Unknown
        || PoDoFo::IsAccessibiltyProfile(GetDocument().GetMetadata().GetPdfALevel())))
    {
        // Ensure PDF/UA compliance. NOTE: /Widget annotations wants
        // a /Form structure element
        SetContents(PdfString(string(PoDoFo::ToString(annotType)).append(" annotation")));
        PoDoFo::CreateObjectStructElement(*this, page, "Annot"_n);
    }
}

PdfAnnotation::PdfAnnotation(PdfObject& obj, PdfAnnotationType annotType)
    : PdfDictionaryElement(obj), m_AnnotationType(annotType), m_Page(nullptr)
{
}

Corners PdfAnnotation::GetRectRaw() const
{
    const PdfArray* arr;
    if (!GetDictionary().TryFindKeyAs("Rect", arr))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::ObjectNotFound, "Missing /Rect element");

    return Corners::FromArray(*arr);
}

void PdfAnnotation::SetRectRaw(const Corners& rect)
{
    PdfArray arr;
    rect.ToArray(arr);
    GetDictionary().AddKey("Rect"_n, arr);
}

Rect PdfAnnotation::GetRect() const
{
    return PoDoFo::TransformCornersPage(GetRectRaw(), MustGetPage());
}

void PdfAnnotation::SetRect(const Rect& rect)
{
    PdfArray arr;
    auto transformed = PoDoFo::TransformRectPage(rect, MustGetPage());
    transformed.ToArray(arr);
    GetDictionary().AddKey("Rect"_n, arr);
}

PdfPage& PdfAnnotation::MustGetPage()
{
    if (m_Page == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    return *m_Page;
}

const PdfPage& PdfAnnotation::MustGetPage() const
{
    if (m_Page == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    return *m_Page;
}

unique_ptr<PdfAnnotation> PdfAnnotation::Create(PdfPage& page, PdfAnnotationType annotType, const Rect& rect)
{
    switch (annotType)
    {
        case PdfAnnotationType::Text:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationText(page, rect));
        case PdfAnnotationType::Link:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationLink(page, rect));
        case PdfAnnotationType::FreeText:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationFreeText(page, rect));
        case PdfAnnotationType::Line:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationLine(page, rect));
        case PdfAnnotationType::Square:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationSquare(page, rect));
        case PdfAnnotationType::Circle:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationCircle(page, rect));
        case PdfAnnotationType::Polygon:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationPolygon(page, rect));
        case PdfAnnotationType::PolyLine:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationPolyLine(page, rect));
        case PdfAnnotationType::Highlight:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationHighlight(page, rect));
        case PdfAnnotationType::Underline:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationUnderline(page, rect));
        case PdfAnnotationType::Squiggly:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationSquiggly(page, rect));
        case PdfAnnotationType::StrikeOut:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationStrikeOut(page, rect));
        case PdfAnnotationType::Stamp:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationStamp(page, rect));
        case PdfAnnotationType::Caret:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationCaret(page, rect));
        case PdfAnnotationType::Ink:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationInk(page, rect));
        case PdfAnnotationType::Popup:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationPopup(page, rect));
        case PdfAnnotationType::FileAttachement:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationFileAttachment(page, rect));
        case PdfAnnotationType::Sound:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationSound(page, rect));
        case PdfAnnotationType::Movie:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationMovie(page, rect));
        case PdfAnnotationType::Widget:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationWidget(page, rect));
        case PdfAnnotationType::Screen:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationScreen(page, rect));
        case PdfAnnotationType::PrinterMark:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationPrinterMark(page, rect));
        case PdfAnnotationType::TrapNet:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationTrapNet(page, rect));
        case PdfAnnotationType::Watermark:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationWatermark(page, rect));
        case PdfAnnotationType::Model3D:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationModel3D(page, rect));
        case PdfAnnotationType::RichMedia:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationRichMedia(page, rect));
        case PdfAnnotationType::WebMedia:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationWebMedia(page, rect));
        case PdfAnnotationType::Redact:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationRedact(page, rect));
        case PdfAnnotationType::Projection:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationProjection(page, rect));
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

void PdfAnnotation::SetAppearanceStream(const PdfXObject& xobj, PdfAppearanceType appearance,
    const PdfName& state, bool skipSelectedState)
{
    PushAppearanceStream(xobj, appearance, state, false);
    if (!state.IsNull() && !skipSelectedState)
        GetDictionary().AddKey("AS"_n, state);
}

void PdfAnnotation::SetAppearanceStreamRaw(const PdfXObject& xobj, PdfAppearanceType appearance,
    const PdfName& state, bool skipSelectedState)
{
    PushAppearanceStream(xobj, appearance, state, true);
    if (!state.IsNull() && !skipSelectedState)
        GetDictionary().AddKey("AS"_n, state);
}

void PdfAnnotation::GetAppearanceStreams(vector<PdfAppearanceStream>& states) const
{
    states.clear();
    auto apDict = getAppearanceDictionary();
    if (apDict == nullptr)
        return;

    PdfReference reference;
    for (auto& pair1 : apDict->GetIndirectIterator())
    {
        PdfAppearanceType apType;
        if (pair1.first == "R")
            apType = PdfAppearanceType::Rollover;
        else if (pair1.first == "D")
            apType = PdfAppearanceType::Down;
        else if (pair1.first == "N")
            apType = PdfAppearanceType::Normal;
        else
            continue;

        PdfDictionary* apStateDict;
        if (pair1.second->HasStream())
        {
            states.push_back({ pair1.second, apType, PdfName::Null });
        }
        else if (pair1.second->TryGetDictionary(apStateDict))
        {
            for (auto& pair2 : apStateDict->GetIndirectIterator())
            {
                if (pair2.second->HasStream())
                    states.push_back({ pair2.second, apType, pair2.first });
            }
        }
    }
}

void PdfAnnotation::ClearAppearances()
{
    GetDictionary().AddKey("AP"_n, PdfDictionary());
}

PdfObject* PdfAnnotation::GetAppearanceDictionaryObject()
{
    return GetDictionary().FindKey("AP");
}

const PdfObject* PdfAnnotation::GetAppearanceDictionaryObject() const
{
    return GetDictionary().FindKey("AP");
}

PdfObject* PdfAnnotation::GetAppearanceStream(PdfAppearanceType appearance, const string_view& state)
{
    return getAppearanceStream(appearance, state);
}

const PdfObject* PdfAnnotation::GetAppearanceStream(PdfAppearanceType appearance, const string_view& state) const
{
    return getAppearanceStream(appearance, state);
}

PdfObject* PdfAnnotation::getAppearanceStream(PdfAppearanceType appearance, const string_view& state) const
{
    auto apDict = getAppearanceDictionary();
    if (apDict == nullptr)
        return nullptr;

    PdfName apName = getAppearanceName(appearance);
    PdfObject* apObjInn = apDict->FindKey(apName);
    if (apObjInn == nullptr)
        return nullptr;

    if (state.size() == 0)
        return apObjInn;

    return apObjInn->GetDictionary().FindKey(state);
}

PdfDictionary* PdfAnnotation::getAppearanceDictionary() const
{
    auto apObj = const_cast<PdfAnnotation&>(*this).GetAppearanceDictionaryObject();
    if (apObj == nullptr)
        return nullptr;

    PdfDictionary* apDict;
    (void)apObj->TryGetDictionary(apDict);
    return apDict;
}

void PdfAnnotation::SetFlags(PdfAnnotationFlags flags)
{
    GetDictionary().AddKey("F"_n, PdfVariant(static_cast<int64_t>(flags)));
}

PdfAnnotationFlags PdfAnnotation::GetFlags() const
{
    auto obj = GetDictionary().FindKeyParent("F");
    int64_t number;
    if (obj == nullptr || !obj->TryGetNumber(number))
        return PdfAnnotationFlags::None;

    return static_cast<PdfAnnotationFlags>(number);
}

void PdfAnnotation::SetBorderStyle(double dHCorner, double dVCorner, double width)
{
    this->SetBorderStyle(dHCorner, dVCorner, width, PdfArray());
}

void PdfAnnotation::SetBorderStyle(double dHCorner, double dVCorner, double width, const PdfArray& strokeStyle)
{
    // TODO : Support for Border style for PDF Vers > 1.0
    PdfArray values;

    values.Add(dHCorner);
    values.Add(dVCorner);
    values.Add(width);
    if (strokeStyle.size() != 0)
        values.Add(strokeStyle);

    GetDictionary().AddKey("Border"_n, values);
}

void PdfAnnotation::SetTitle(nullable<const PdfString&> title)
{
    if (title.has_value())
        GetDictionary().AddKey("T"_n, *title);
    else
        GetDictionary().RemoveKey("T");
}

nullable<const PdfString&> PdfAnnotation::GetTitle() const
{
    auto obj = GetDictionary().FindKeyParent("T");
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return { };

    return *str;
}

void PdfAnnotation::SetContents(nullable<const PdfString&> contents)
{
    if (contents.has_value())
        GetDictionary().AddKey("Contents"_n, *contents);
    else
        GetDictionary().RemoveKey("Contents");
}

nullable<const PdfString&> PdfAnnotation::GetContents() const
{
    auto obj = GetDictionary().FindKeyParent("Contents");
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return { };

    return *str;
}

PdfColor PdfAnnotation::GetColor() const
{
    PdfColor color;
    auto colorObj = GetDictionary().FindKeyParent("C");
    if (colorObj == nullptr
        || !PdfColor::TryCreateFromObject(*colorObj, color))
    {
        return { };
    }

    return color;
}

void PdfAnnotation::SetColor(nullable<const PdfColor&> color)
{
    if (color.has_value())
        GetDictionary().AddKey("C"_n, color->ToArray());
    else
        GetDictionary().RemoveKey("C");
}

bool PdfAnnotation::TryCreateFromObject(PdfObject& obj, unique_ptr<PdfAnnotation>& xobj)
{
    PdfAnnotation* xobj_;
    if (!tryCreateFromObject(obj, PdfAnnotationType::Unknown, xobj_))
        return false;

    xobj.reset(xobj_);
    return true;
}

bool PdfAnnotation::TryCreateFromObject(const PdfObject& obj, unique_ptr<const PdfAnnotation>& xobj)
{
    PdfAnnotation* xobj_;
    if (!tryCreateFromObject(obj, PdfAnnotationType::Unknown, xobj_))
        return false;

    xobj.reset(xobj_);
    return true;
}

void PdfAnnotation::PushAppearanceStream(const PdfXObject& xobj, PdfAppearanceType appearance, const PdfName& state, bool raw)
{
    auto form = xobj.GetForm();

    const PdfObject* apObj;
    double teta;
    if (raw || !MustGetPage().TryGetRotationRadians(teta))
    {
        if (form == nullptr)
        {
            // Create a preamble form that just draw the xobject
            auto actualXobj = GetDocument().CreateXObjectForm(xobj.GetRect());
            static_cast<PdfResourceOperations&>(actualXobj->GetOrCreateResources())
                .AddResource(PdfResourceType::XObject, "XOb1"_n, xobj.GetObject());
            PdfStringStream sstream;
            PoDoFo::WriteOperator_Do(sstream, "XOb1");
            actualXobj->GetObject().GetOrCreateStream().SetData(sstream.GetString());
            form = actualXobj.get();
        }

        apObj = &form->GetObject();
    }
    else
    {
        // If the page has a rotation, add a preamble from that
        // will transform the input xobject and adjust the orientation
        unique_ptr<PdfXObjectForm> actualXobj;
        Matrix newMat;
        if (form == nullptr)
        {
            actualXobj = GetDocument().CreateXObjectForm(xobj.GetRect());
            static_cast<PdfResourceOperations&>(actualXobj->GetOrCreateResources())
                .AddResource(PdfResourceType::XObject, "XOb1"_n, xobj.GetObject());
            newMat = PoDoFo::GetFrameRotationTransform(xobj.GetRect(), -teta);
        }
        else
        {
            actualXobj = GetDocument().CreateXObjectForm(form->GetRect());
            static_cast<PdfResourceOperations&>(actualXobj->GetOrCreateResources())
                .AddResource(PdfResourceType::XObject, "XOb1"_n, form->GetObject());
            newMat = PoDoFo::GetFrameRotationTransform(form->GetRect(), -teta);
        }

        PdfStringStream sstream;
        PoDoFo::WriteOperator_Do(sstream, "XOb1");
        actualXobj->GetObject().GetOrCreateStream().SetData(sstream.GetString());
        actualXobj->SetMatrix(newMat);
        apObj = &actualXobj->GetObject();
    }

    PdfName name;
    if (appearance == PdfAppearanceType::Rollover)
        name = "R";
    else if (appearance == PdfAppearanceType::Down)
        name = "D";
    else // PdfAnnotationAppearance::Normal
        name = "N";

    auto apDictObj = GetDictionary().FindKey("AP");
    if (apDictObj == nullptr || !apDictObj->IsDictionary())
        apDictObj = &GetDictionary().AddKey("AP"_n, PdfDictionary());

    if (state.IsNull())
    {
        apDictObj->GetDictionary().AddKeyIndirectSafe(name, *apObj);
    }
    else
    {
        // when the state is defined, then the appearance is expected to be a dictionary
        auto apInnerObj = apDictObj->GetDictionary().FindKey(name);
        if (apInnerObj == nullptr || !apInnerObj->IsDictionary())
            apInnerObj = &apDictObj->GetDictionary().AddKey(name, PdfDictionary());

        apInnerObj->GetDictionary().AddKeyIndirectSafe(state, *apObj);
    }
}

bool PdfAnnotation::tryCreateFromObject(const PdfObject& obj, PdfAnnotationType targetType, PdfAnnotation*& xobj)
{
    auto type = getAnnotationType(obj);
    if (targetType != PdfAnnotationType::Unknown && type != targetType)
    {
        xobj = nullptr;
        return false;
    }

    switch (type)
    {
        case PdfAnnotationType::Text:
            xobj = new PdfAnnotationText(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Link:
            xobj = new PdfAnnotationLink(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::FreeText:
            xobj = new PdfAnnotationFreeText(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Line:
            xobj = new PdfAnnotationLine(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Square:
            xobj = new PdfAnnotationSquare(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Circle:
            xobj = new PdfAnnotationCircle(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Polygon:
            xobj = new PdfAnnotationPolygon(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::PolyLine:
            xobj = new PdfAnnotationPolyLine(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Highlight:
            xobj = new PdfAnnotationHighlight(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Underline:
            xobj = new PdfAnnotationUnderline(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Squiggly:
            xobj = new PdfAnnotationSquiggly(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::StrikeOut:
            xobj = new PdfAnnotationStrikeOut(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Stamp:
            xobj = new PdfAnnotationStamp(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Caret:
            xobj = new PdfAnnotationCaret(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Ink:
            xobj = new PdfAnnotationInk(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Popup:
            xobj = new PdfAnnotationPopup(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::FileAttachement:
            xobj = new PdfAnnotationFileAttachment(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Sound:
            xobj = new PdfAnnotationSound(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Movie:
            xobj = new PdfAnnotationMovie(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Widget:
            xobj = new PdfAnnotationWidget(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Screen:
            xobj = new PdfAnnotationScreen(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::PrinterMark:
            xobj = new PdfAnnotationPrinterMark(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::TrapNet:
            xobj = new PdfAnnotationTrapNet(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Watermark:
            xobj = new PdfAnnotationWatermark(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Model3D:
            xobj = new PdfAnnotationModel3D(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::RichMedia:
            xobj = new PdfAnnotationRichMedia(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::WebMedia:
            xobj = new PdfAnnotationWebMedia(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Redact:
            xobj = new PdfAnnotationRedact(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Projection:
            xobj = new PdfAnnotationProjection(const_cast<PdfObject&>(obj));
            return true;
        default:
            xobj = nullptr;
            return false;
    }
}

PdfAnnotationType PdfAnnotation::getAnnotationType(const PdfObject& obj)
{
    const PdfName* name;
    auto subTypeObj = obj.GetDictionary().FindKey("Subtype");
    if (subTypeObj == nullptr || !subTypeObj->TryGetName(name))
        return PdfAnnotationType::Unknown;

    PdfAnnotationType ret;
    (void)PoDoFo::TryConvertTo(name->GetString(), ret);
    return ret;
}

PdfName getAppearanceName(PdfAppearanceType appearance)
{
    switch (appearance)
    {
        case PdfAppearanceType::Normal:
            return "N"_n;
        case PdfAppearanceType::Rollover:
            return "R"_n;
        case PdfAppearanceType::Down:
            return "D"_n;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Invalid appearance type");
    }
}
