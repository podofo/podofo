/**
 * SPDX-FileCopyrightText: (C) 2011 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2011 Petr Pytelka
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfSignature.h"

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfData.h"

#include "PdfDocument.h"
#include "PdfXObject.h"
#include "PdfPage.h"

using namespace std;
using namespace PoDoFo;

PdfSignature::PdfSignature(PdfAcroForm& acroform, const shared_ptr<PdfField>& parent) :
    PdfField(acroform, PdfFieldType::Signature, parent),
    m_ValueObj(nullptr)
{
    init(acroform);
}

PdfSignature::PdfSignature(PdfAnnotationWidget& widget, const shared_ptr<PdfField>& parent) :
    PdfField(widget, PdfFieldType::Signature, parent),
    m_ValueObj(nullptr)
{
    init(widget.GetDocument().GetOrCreateAcroForm());
}

PdfSignature::PdfSignature(PdfObject& obj, PdfAcroForm* acroform) :
    PdfField(obj, acroform, PdfFieldType::Signature),
    m_ValueObj(this->GetObject().GetDictionary().FindKey("V"))
{
    // NOTE: Do not call init() here
}

void PdfSignature::SetAppearanceStream(PdfXObjectForm& obj, PdfAppearanceType appearance, const PdfName& state)
{
    GetWidget()->SetAppearanceStream(obj, appearance, state);
    (void)this->GetWidget()->GetOrCreateAppearanceCharacteristics();
}

void PdfSignature::init(PdfAcroForm& acroForm)
{
    // TABLE 8.68 Signature flags: SignaturesExist (1) | AppendOnly (2)
    // This will open signature panel when inspecting PDF with acrobat,
    // even if the signature is unsigned
    acroForm.GetObject().GetDictionary().AddKey("SigFlags", (int64_t)3);
}

void PdfSignature::SetSignerName(nullable<const PdfString&> text)
{
    if (m_ValueObj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    if (text.has_value())
        m_ValueObj->GetDictionary().AddKey("Name", *text);
    else
        m_ValueObj->GetDictionary().RemoveKey("Name");
}

void PdfSignature::SetSignatureReason(nullable<const PdfString&> text)
{
    if (m_ValueObj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    if (text.has_value())
        m_ValueObj->GetDictionary().AddKey("Reason", *text);
    else
        m_ValueObj->GetDictionary().RemoveKey("Reason");
}

void PdfSignature::SetSignatureDate(nullable<const PdfDate&> sigDate)
{
    if (m_ValueObj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);


    if (sigDate.has_value())
    {
        PdfString dateStr = sigDate->ToString();
        m_ValueObj->GetDictionary().AddKey("M", dateStr);
    }
    else
    {
        m_ValueObj->GetDictionary().RemoveKey("M");
    }
}

void PdfSignature::PrepareForSigning(const string_view& filter,
    const string_view& subFilter, const std::string_view& type,
    const PdfSignatureBeacons& beacons)
{
    EnsureValueObject();
    auto& dict = m_ValueObj->GetDictionary();
    // This must be ensured before any signing operation
    dict.AddKey(PdfName::KeyFilter, PdfName(filter));
    dict.AddKey("SubFilter", PdfName(subFilter));
    dict.AddKey(PdfName::KeyType, PdfName(type));

    // Prepare contents data
    PdfData contentsData = PdfData(beacons.ContentsBeacon, beacons.ContentsOffset);
    m_ValueObj->GetDictionary().AddKey(PdfName::KeyContents, PdfVariant(std::move(contentsData)));

    // Prepare byte range data
    PdfData byteRangeData = PdfData(beacons.ByteRangeBeacon, beacons.ByteRangeOffset);
    m_ValueObj->GetDictionary().AddKey("ByteRange", PdfVariant(std::move(byteRangeData)));
}

void PdfSignature::SetSignatureLocation(nullable<const PdfString&> text)
{
    if (m_ValueObj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    if (text.has_value())
        m_ValueObj->GetDictionary().AddKey("Location", *text);
    else
        m_ValueObj->GetDictionary().RemoveKey("Location");
}

void PdfSignature::SetSignatureCreator(nullable<const PdfString&> creator)
{
    if (m_ValueObj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    // TODO: Make it less brutal, preserving /Prop_Build
    if (creator.has_value())
    {
        m_ValueObj->GetDictionary().AddKey("Prop_Build", PdfDictionary());
        PdfObject* propBuild = m_ValueObj->GetDictionary().GetKey("Prop_Build");
        propBuild->GetDictionary().AddKey("App", PdfDictionary());
        PdfObject* app = propBuild->GetDictionary().GetKey("App");
        app->GetDictionary().AddKey("Name", *creator);
    }
    else
    {
        m_ValueObj->GetDictionary().RemoveKey("Prop_Build");
    }
}

void PdfSignature::AddCertificationReference(PdfCertPermission perm)
{
    if (m_ValueObj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_ValueObj->GetDictionary().RemoveKey("Reference");

    auto& sigRef = this->GetDocument().GetObjects().CreateDictionaryObject("SigRef");
    sigRef.GetDictionary().AddKey("TransformMethod", PdfName("DocMDP"));

    auto& transParams = this->GetDocument().GetObjects().CreateDictionaryObject("TransformParams");
    transParams.GetDictionary().AddKey("V", PdfName("1.2"));
    transParams.GetDictionary().AddKey("P", (int64_t)perm);
    sigRef.GetDictionary().AddKey("TransformParams", transParams);

    auto& catalog = GetDocument().GetCatalog();
    PdfObject permObject;
    permObject.GetDictionary().AddKey("DocMDP", this->GetObject().GetDictionary().GetKey("V")->GetReference());
    catalog.GetDictionary().AddKey("Perms", permObject);

    PdfArray refers;
    refers.Add(sigRef);

    m_ValueObj->GetDictionary().AddKey("Reference", PdfVariant(refers));
}

nullable<const PdfString&> PdfSignature::GetSignerName() const
{
    if (m_ValueObj == nullptr)
        return nullptr;

    auto obj = m_ValueObj->GetDictionary().FindKey("Name");
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return nullptr;

    return *str;
}

nullable<const PdfString&> PdfSignature::GetSignatureReason() const
{
    if (m_ValueObj == nullptr)
        return nullptr;

    auto obj = m_ValueObj->GetDictionary().FindKey("Reason");
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return nullptr;

    return *str;
}

nullable<const PdfString&> PdfSignature::GetSignatureLocation() const
{
    if (m_ValueObj == nullptr)
        return nullptr;

    auto obj = m_ValueObj->GetDictionary().FindKey("Location");
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return nullptr;

    return *str;
}

nullable<PdfDate> PdfSignature::GetSignatureDate() const
{
    if (m_ValueObj == nullptr)
        return nullptr;

    auto obj = m_ValueObj->GetDictionary().FindKey("M");
    PdfDate date;
    const PdfString* str;
    if (obj == nullptr
        || !obj->TryGetString(str)
        || !PdfDate::TryParse(str->GetString(), date))
        return nullptr;

    return date;
}

PdfObject* PdfSignature::getValueObject() const
{
    return m_ValueObj;
}

void PdfSignature::EnsureValueObject()
{
    if (m_ValueObj != nullptr)
        return;

    m_ValueObj = &this->GetDocument().GetObjects().CreateDictionaryObject("Sig");
    if (m_ValueObj == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    GetObject().GetDictionary().AddKey("V", m_ValueObj->GetIndirectReference());
}

PdfSignature* PdfSignature::GetParent()
{
    return GetParentTyped<PdfSignature>(PdfFieldType::Signature);
}

const PdfSignature* PdfSignature::GetParent() const
{
    return GetParentTyped<PdfSignature>(PdfFieldType::Signature);
}

PdfSignatureBeacons::PdfSignatureBeacons()
{
    ContentsOffset = std::make_shared<size_t>();
    ByteRangeOffset = std::make_shared<size_t>();
}
