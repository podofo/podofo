// SPDX-FileCopyrightText: 2011 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2011 Petr Pytelka
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfSignature.h"

#include <numerics/checked_math.h>
#include <podofo/private/PdfParser.h>
#include <podofo/private/CmsVerifyContext.h>

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfData.h"

#include "PdfXObject.h"
#include "PdfPage.h"

using namespace std;
using namespace PoDoFo;
using namespace chromium::base;

constexpr size_t BufferSize = 65536;

static void appendRange(InputStreamDevice& input, CmsVerifyContext& context,
    size_t offset, size_t length, charbuff& buffer);

PdfSignature::PdfSignature(PdfAcroForm& acroform, shared_ptr<PdfField>&& parent) :
    PdfField(acroform, PdfFieldType::Signature, std::move(parent)),
    m_ValueObj(nullptr)
{
    init(acroform);
}

PdfSignature::PdfSignature(PdfAnnotationWidget& widget, shared_ptr<PdfField>&& parent) :
    PdfField(widget, PdfFieldType::Signature, std::move(parent)),
    m_ValueObj(nullptr)
{
    init(widget.GetDocument().GetOrCreateAcroForm());
}

PdfSignature::PdfSignature(PdfObject& obj, PdfAcroForm* acroform) :
    PdfField(obj, acroform, PdfFieldType::Signature),
    m_ValueObj(this->GetDictionary().FindKey("V"))
{
    // NOTE: Do not call init() here
}

bool PdfSignature::TryVerifySignature(InputStreamDevice& input, PdfSignatureVerifyStatus& status) const
{
    status = PdfSignatureVerifyStatus::Indeterminate;
    if (m_ValueObj == nullptr)
        return false;

    auto& dict = m_ValueObj->GetDictionary();
    const PdfString* contents;
    const PdfArray* byteRange;
    if (!dict.TryFindKeyAs("Contents", contents) || !dict.TryFindKeyAs("ByteRange", byteRange))
        return false;

    // A PDF signature always has a single pair of ranges, the second
    // one starting right after the /Contents string
    if (byteRange->GetSize() != 4)
        return false;

    int64_t offsets[4];
    for (unsigned i = 0; i < 4; i++)
    {
        if (!byteRange->TryGetAtAs(i, offsets[i]) || offsets[i] < 0)
            return false;
    }

    size_t firstEnd;
    size_t secondEnd;
    if (offsets[0] != 0
        || !(CheckedNumeric((size_t)offsets[0]) + CheckedNumeric((size_t)offsets[1])).AssignIfValid(&firstEnd)
        || !(CheckedNumeric((size_t)offsets[2]) + CheckedNumeric((size_t)offsets[3])).AssignIfValid(&secondEnd))
    {
        return false;
    }

    // The ranges must not overlap and must be within the input
    auto inputLength = input.GetLength();
    if (firstEnd > (size_t)offsets[2] || secondEnd > inputLength)
        return false;

    CmsVerifyContext context;
    if (!context.TryReset(contents->GetRawData()))
        return false;

    // Accordingly to current interpretation of the specification,
    // a PDF signature has exactly one signer
    if (context.GetSignerCount() != 1 || !context.TryLoadSigner(0))
        return false;

    charbuff buffer(BufferSize);
    appendRange(input, context, 0, (size_t)offsets[1], buffer);
    appendRange(input, context, (size_t)offsets[2], (size_t)offsets[3], buffer);

    if (!context.VerifySignature())
    {
        status = PdfSignatureVerifyStatus::Invalid;
        return false;
    }

    // NOTE: The attribute is not mandatory here, but it must match when present.
    // Requiring it is a conformance concern of the caller, as it depends on /SubFilter
    bool attrMissing;
    if (!context.TryVerifySigningCertificateV2(attrMissing))
    {
        status = PdfSignatureVerifyStatus::Invalid;
        return false;
    }

    status = secondEnd == inputLength
        ? PdfSignatureVerifyStatus::CryptoVerified
        : PdfSignatureVerifyStatus::CryptoVerifiedPartialCoverage;
    return true;
}

void PdfSignature::init(PdfAcroForm& acroForm)
{
    // TABLE 8.68 Signature flags: SignaturesExist (1)
    // This will open signature panel when inspecting PDF with acrobat,
    // even if the signature is unsigned
    acroForm.SetSigFlags(PdfAcroFormSigFlags::SignaturesExist);
}

void PdfSignature::SetSignerName(nullable<const PdfString&> text)
{
    EnsureValueObject();
    if (text.has_value())
        m_ValueObj->GetDictionary().AddKey("Name"_n, *text);
    else
        m_ValueObj->GetDictionary().RemoveKey("Name");
}

void PdfSignature::SetSignatureReason(nullable<const PdfString&> text)
{
    EnsureValueObject();
    if (text.has_value())
        m_ValueObj->GetDictionary().AddKey("Reason"_n, *text);
    else
        m_ValueObj->GetDictionary().RemoveKey("Reason");
}

void PdfSignature::SetSignatureDate(nullable<const PdfDate&> sigDate)
{
    EnsureValueObject();
    if (sigDate.has_value())
    {
        PdfString dateStr = sigDate->ToString();
        m_ValueObj->GetDictionary().AddKey("M"_n, dateStr);
    }
    else
    {
        m_ValueObj->GetDictionary().RemoveKey("M");
    }
}

void PdfSignature::PrepareForSigning(const string_view& filter,
    const string_view& subFilter, const string_view& type,
    const PdfSignatureBeacons& beacons)
{
    if (m_ValueObj == nullptr)
    {
        ensureValueObject();
    }
    else
    {
        // NOTE: If we are repeating the signature, we must create a newer object
        if (m_ValueObj->GetDictionary().HasKey("Contents"))
        {
            m_ValueObj = &this->GetDocument().GetObjects().CreateObject(*m_ValueObj);
            GetDictionary().AddKey("V"_n, m_ValueObj->GetIndirectReference());
        }
    }

    auto& dict = m_ValueObj->GetDictionary();
    // This must be ensured before any signing operation
    dict.AddKey("Filter"_n, PdfName(filter));
    dict.AddKey("SubFilter"_n, PdfName(subFilter));
    dict.AddKey("Type"_n, PdfName(type));

    // Prepare contents data
    PdfData contentsData = PdfData(beacons.ContentsBeacon, beacons.ContentsOffset);
    m_ValueObj->GetDictionary().AddKey("Contents"_n, PdfVariant(std::move(contentsData)));

    // Prepare byte range data
    PdfData byteRangeData = PdfData(beacons.ByteRangeBeacon, beacons.ByteRangeOffset);
    m_ValueObj->GetDictionary().AddKey("ByteRange"_n, PdfVariant(std::move(byteRangeData)));
}

void PdfSignature::SetSignatureLocation(nullable<const PdfString&> text)
{
    EnsureValueObject();
    if (text.has_value())
        m_ValueObj->GetDictionary().AddKey("Location"_n, *text);
    else
        m_ValueObj->GetDictionary().RemoveKey("Location");
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
    EnsureValueObject();
    PdfDictionary* dict;
    if (application.has_value())
    {
        if (!m_ValueObj->GetDictionary().TryFindKeyAs("Prop_Build", dict))
            dict = &m_ValueObj->GetDictionary().AddKey("Prop_Build"_n, PdfDictionary()).GetDictionary();

        PdfDictionary* appDict;
        if (!dict->TryFindKeyAs("Prop_Build", appDict))
            appDict = &dict->AddKey("App"_n, PdfDictionary()).GetDictionary();

        appDict->AddKey("Name"_n, *application);
    }
    else
    {
        dict = m_ValueObj->GetDictionary().FindKeyAs<PdfDictionary*>("Prop_Build");
        if (dict != nullptr)
            dict->RemoveKey("App");
    }
}

void PdfSignature::AddCertificationReference(PdfCertPermission perm)
{
    EnsureValueObject();
    m_ValueObj->GetDictionary().RemoveKey("Reference");

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

    m_ValueObj->GetDictionary().AddKey("Reference"_n, PdfVariant(refers));
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
    {
        return nullptr;
    }

    return date;
}

bool PdfSignature::TryGetPreviousRevision(InputStreamDevice& input, OutputStreamDevice& output) const
{
    const PdfArray* byteRange = nullptr;
    m_ValueObj->GetDictionary().TryFindKeyAs("ByteRange", byteRange);
    if (byteRange == nullptr || byteRange->GetSize() < 4)
        return false;

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
    return m_ValueObj;
}

void PdfSignature::SetContentsByteRangeNoDirtySet(const bufferview& contents, PdfArray&& byteRange)
{
    m_ValueObj->GetDictionary().AddKeyNoDirtySet("ByteRange"_n, PdfVariant(std::move(byteRange)));
    m_ValueObj->GetDictionary().AddKeyNoDirtySet("Contents"_n, PdfVariant(PdfString::FromRaw(contents, true)));
}

void PdfSignature::EnsureValueObject()
{
    if (m_ValueObj != nullptr)
        return;

    ensureValueObject();
}

void PdfSignature::ensureValueObject()
{
    m_ValueObj = &this->GetDocument().GetObjects().CreateDictionaryObject("Sig"_n);
    GetDictionary().AddKey("V"_n, m_ValueObj->GetIndirectReference());
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

void appendRange(InputStreamDevice& input, CmsVerifyContext& context,
    size_t offset, size_t length, charbuff& buffer)
{
    input.Seek(offset);
    while (length != 0)
    {
        size_t readSize = std::min(length, buffer.size());
        input.Read(buffer.data(), readSize);
        context.AppendData({ buffer.data(), readSize });
        length -= readSize;
    }
}
