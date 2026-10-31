// SPDX-FileCopyrightText: 2011 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2011 Petr Pytelka
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfSignature.h"

#include <numerics/checked_math.h>
#include <podofo/private/PdfParser.h>

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfData.h"

#include "PdfXObject.h"
#include "PdfPage.h"

using namespace std;
using namespace PoDoFo;
using namespace chromium::base;

PdfSignature::PdfSignature(PdfAcroForm& acroform, shared_ptr<PdfField>&& parent) :
    PdfField(acroform, PdfFieldType::Signature, std::move(parent))
{
    init(acroform);
}

PdfSignature::PdfSignature(PdfAnnotationWidget& widget, shared_ptr<PdfField>&& parent) :
    PdfField(widget, PdfFieldType::Signature, std::move(parent))
{
    init(widget.GetDocument().GetOrCreateAcroForm());
}

PdfSignature::PdfSignature(PdfObject& obj, PdfAcroForm* acroform) :
    PdfField(obj, acroform, PdfFieldType::Signature)
{
    // NOTE: Do not call init() here
}

void PdfSignature::init(PdfAcroForm& acroForm)
{
    // TABLE 8.68 Signature flags: SignaturesExist (1)
    // This will open signature panel when inspecting PDF with acrobat,
    // even if the signature is unsigned
    // NOTE: Don't touch the form if the flag is already set,
    // and preserve the other flags, such as AppendOnly
    auto sigFlags = acroForm.GetSigFlags();
    if ((sigFlags & PdfAcroFormSigFlags::SignaturesExist) == PdfAcroFormSigFlags::None)
        acroForm.SetSigFlags(sigFlags | PdfAcroFormSigFlags::SignaturesExist);
}

void PdfSignature::SetSignerName(nullable<const PdfString&> text)
{
    auto& dict = ensureValueDictionary();
    if (text.has_value())
        dict.AddKey("Name"_n, *text);
    else
        dict.RemoveKey("Name");
}

void PdfSignature::SetSignatureReason(nullable<const PdfString&> text)
{
    auto& dict = ensureValueDictionary();
    if (text.has_value())
        dict.AddKey("Reason"_n, *text);
    else
        dict.RemoveKey("Reason");
}

void PdfSignature::SetSignatureDate(nullable<const PdfDate&> sigDate)
{
    auto& dict = ensureValueDictionary();
    if (sigDate.has_value())
    {
        PdfString dateStr = sigDate->ToString();
        dict.AddKey("M"_n, dateStr);
    }
    else
    {
        dict.RemoveKey("M");
    }
}

void PdfSignature::PrepareForSigning(const string_view& filter,
    const string_view& subFilter, const string_view& type,
    const PdfSignatureBeacons& beacons)
{
    auto dict = &ensureValueDictionary();
    // NOTE: If we are repeating the signature, we must create a newer object
    if (dict->HasKey("Contents"))
    {
        auto& valueObj = this->GetDocument().GetObjects().CreateObject(*dict);
        GetDictionary().AddKey("V"_n, valueObj.GetIndirectReference());
        dict = &valueObj.GetDictionary();
    }

    // This must be ensured before any signing operation
    dict->AddKey("Filter"_n, PdfName(filter));
    dict->AddKey("SubFilter"_n, PdfName(subFilter));
    dict->AddKey("Type"_n, PdfName(type));

    // Prepare contents data
    PdfData contentsData = PdfData(beacons.ContentsBeacon, beacons.ContentsOffset);
    dict->AddKey("Contents"_n, PdfVariant(std::move(contentsData)));

    // Prepare byte range data
    PdfData byteRangeData = PdfData(beacons.ByteRangeBeacon, beacons.ByteRangeOffset);
    dict->AddKey("ByteRange"_n, PdfVariant(std::move(byteRangeData)));
}

void PdfSignature::SetSignatureLocation(nullable<const PdfString&> text)
{
    auto& dict = ensureValueDictionary();
    if (text.has_value())
        dict.AddKey("Location"_n, *text);
    else
        dict.RemoveKey("Location");
}

void PdfSignature::SetSignatureCreator(nullable<const PdfString&> creator)
{
    if (creator == nullptr)
        SetCreatingApplication(nullptr);
    else
        SetCreatingApplication(PdfName(creator->GetString()));
}

void PdfSignature::SetCreatingApplication(nullable<const PdfName&> application)
{
    auto& valueDict = ensureValueDictionary();
    PdfDictionary* dict;
    if (application.has_value())
    {
        if (!valueDict.TryFindKeyAs("Prop_Build", dict))
            dict = &valueDict.AddKey("Prop_Build"_n, PdfDictionary()).GetDictionary();

        PdfDictionary* appDict;
        if (!dict->TryFindKeyAs("Prop_Build", appDict))
            appDict = &dict->AddKey("App"_n, PdfDictionary()).GetDictionary();

        appDict->AddKey("Name"_n, *application);
    }
    else
    {
        dict = valueDict.FindKeyAs<PdfDictionary*>("Prop_Build");
        if (dict != nullptr)
            dict->RemoveKey("App");
    }
}

void PdfSignature::AddCertificationReference(PdfCertPermission perm)
{
    auto& dict = ensureValueDictionary();
    dict.RemoveKey("Reference");

    auto& sigRef = this->GetDocument().GetObjects().CreateDictionaryObject("SigRef"_n);
    sigRef.GetDictionary().AddKey("TransformMethod"_n, "DocMDP"_n);

    auto& transParams = this->GetDocument().GetObjects().CreateDictionaryObject("TransformParams"_n);
    transParams.GetDictionary().AddKey("V"_n, "1.2"_n);
    transParams.GetDictionary().AddKey("P"_n, (int64_t)perm);
    sigRef.GetDictionary().AddKey("TransformParams"_n, transParams);

    auto& catalog = GetDocument().GetCatalog();
    PdfObject permObject;
    permObject.GetDictionary().AddKey("DocMDP"_n, this->GetDictionary().GetKey("V")->GetReference());
    catalog.GetDictionary().AddKey("Perms"_n, permObject);

    PdfArray refers;
    refers.Add(sigRef);

    dict.AddKey("Reference"_n, PdfVariant(refers));
}

nullable<const PdfString&> PdfSignature::GetSignerName() const
{
    auto valueObj = getValueObject();
    if (valueObj == nullptr)
        return nullptr;

    auto obj = valueObj->GetDictionary().FindKey("Name");
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return nullptr;

    return *str;
}

nullable<const PdfString&> PdfSignature::GetSignatureReason() const
{
    auto valueObj = getValueObject();
    if (valueObj == nullptr)
        return nullptr;

    auto obj = valueObj->GetDictionary().FindKey("Reason");
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return nullptr;

    return *str;
}

nullable<const PdfString&> PdfSignature::GetSignatureLocation() const
{
    auto valueObj = getValueObject();
    if (valueObj == nullptr)
        return nullptr;

    auto obj = valueObj->GetDictionary().FindKey("Location");
    const PdfString* str;
    if (obj == nullptr || !obj->TryGetString(str))
        return nullptr;

    return *str;
}

nullable<PdfDate> PdfSignature::GetSignatureDate() const
{
    auto valueObj = getValueObject();
    if (valueObj == nullptr)
        return nullptr;

    auto obj = valueObj->GetDictionary().FindKey("M");
    PdfDate date;
    const PdfString* str;
    if (obj == nullptr
        || !obj->TryGetString(str)
        || !PdfDate::TryParse(str->GetString(), date))
    {
        return nullptr;
    }

    return date;
}

bool PdfSignature::TryGetPreviousRevision(InputStreamDevice& input, OutputStreamDevice& output) const
{
    // The field may be unsigned, or carry an invalid value
    auto valueObj = getValueObject();
    const PdfArray* byteRange;
    if (valueObj == nullptr || !valueObj->GetDictionary().TryFindKeyAs("ByteRange", byteRange)
        || byteRange->GetSize() < 4)
    {
        return false;
    }

    int64_t lastRangeOffset;
    int64_t lastRangeLength;
    if (!byteRange->TryGetAtAs(byteRange->GetSize() - 2, lastRangeOffset)
        || !byteRange->TryGetAtAs(byteRange->GetSize() - 1, lastRangeLength)
        || lastRangeOffset < 0 || lastRangeLength < 0)
    {
        return false;
    }

    size_t signedRevisionOffset;
    if (!(CheckedNumeric((size_t)lastRangeOffset) + CheckedNumeric((size_t)lastRangeLength)).AssignIfValid(&signedRevisionOffset))
        return false;

    size_t previousRevisionOffset;
    if (!PdfParser::TryGetPreviousRevisionOffset(input, signedRevisionOffset, previousRevisionOffset))
        return false;

    input.Seek(0);
    input.CopyTo(output, previousRevisionOffset);
    return true;
}

PdfObject* PdfSignature::getValueObject() const
{
    // NOTE: A /V that is not a dictionary is treated as missing
    auto ret = PdfField::getValueObject();
    if (ret == nullptr || !ret->IsDictionary())
        return nullptr;

    return ret;
}

void PdfSignature::SetContentsByteRangeNoDirtySet(const bufferview& contents, PdfArray&& byteRange)
{
    // The field of a restored signing context may have lost its value
    auto valueObj = getValueObject();
    if (valueObj == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidObject, "The signature field has no value object");

    auto& dict = valueObj->GetDictionary();
    dict.AddKeyNoDirtySet("ByteRange"_n, PdfVariant(std::move(byteRange)));
    dict.AddKeyNoDirtySet("Contents"_n, PdfVariant(PdfString::FromRaw(contents, true)));
}

void PdfSignature::EnsureValueObject()
{
    (void)ensureValueDictionary();
}

PdfDictionary& PdfSignature::ensureValueDictionary()
{
    auto valueObj = getValueObject();
    if (valueObj != nullptr)
        return valueObj->GetDictionary();

    auto& ret = this->GetDocument().GetObjects().CreateDictionaryObject("Sig"_n);
    GetDictionary().AddKey("V"_n, ret.GetIndirectReference());
    return ret.GetDictionary();
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
