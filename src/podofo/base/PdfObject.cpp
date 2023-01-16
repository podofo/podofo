/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfObject.h"

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfStreamedObjectStream.h"
#include "PdfMemoryObjectStream.h"
#include "PdfStreamDevice.h"

using namespace std;
using namespace PoDoFo;

PdfObject PdfObject::Null;

PdfObject::PdfObject()
    : PdfObject(PdfDictionary(), PdfReference(), false) { }

PdfObject::~PdfObject() { }

PdfObject::PdfObject(const PdfVariant& var)
    : PdfObject(PdfVariant(var), PdfReference(), false) { }

PdfObject::PdfObject(PdfVariant&& var) noexcept
    : m_Variant(std::move(var)), m_IsDirty(false)
{
    initObject();
    SetVariantOwner();
}

PdfObject::PdfObject(bool b)
    : m_Variant(b), m_IsDirty(false)
{
    initObject();
}

PdfObject::PdfObject(int64_t l)
    : m_Variant(l), m_IsDirty(false)
{
    initObject();
}

PdfObject::PdfObject(double d)
    : m_Variant(d), m_IsDirty(false)
{
    initObject();
}

PdfObject::PdfObject(const PdfString& str)
    : m_Variant(str), m_IsDirty(false)
{
    initObject();
}
PdfObject::PdfObject(const PdfName& name)
    : m_Variant(name), m_IsDirty(false)
{
    initObject();
}

PdfObject::PdfObject(const PdfReference& ref)
    : m_Variant(ref), m_IsDirty(false)
{
    initObject();
}

PdfObject::PdfObject(const PdfArray& arr)
    : m_Variant(arr), m_IsDirty(false)
{
    initObject();
    m_Variant.GetArray().SetOwner(*this);
}

PdfObject::PdfObject(PdfArray&& arr) noexcept
    : m_Variant(std::move(arr)), m_IsDirty(false)
{
    initObject();
    m_Variant.GetArray().SetOwner(*this);
}

PdfObject::PdfObject(const PdfDictionary& dict)
    : m_Variant(dict), m_IsDirty(false)
{
    initObject();
    m_Variant.GetDictionary().SetOwner(*this);
}

PdfObject::PdfObject(PdfDictionary&& dict) noexcept
    : m_Variant(std::move(dict)), m_IsDirty(false)
{
    initObject();
    m_Variant.GetDictionary().SetOwner(*this);
}

// NOTE: Don't copy parent document/container/indirect refernce.
// Copied objects must be always detached. Ownership will be set
// automatically elsewhere
PdfObject::PdfObject(const PdfObject& rhs)
    : PdfObject(PdfVariant(rhs.GetVariant()), PdfReference(), false)
{
    copyStreamFrom(rhs);
}

// NOTE: Don't move parent document/container. Copied objects must be
// always detached. Ownership will be set automatically elsewhere.
// Also don't move reference
PdfObject::PdfObject(PdfObject&& rhs) noexcept :
    m_Variant(std::move(rhs.m_Variant)),
    m_IsDirty(false)
{
    initObject();
    SetVariantOwner();
    moveStreamFrom(rhs);
}

const PdfObjectStream* PdfObject::GetStream() const
{
    DelayedLoadStream();
    return m_Stream.get();
}

PdfObjectStream* PdfObject::GetStream()
{
    DelayedLoadStream();
    return m_Stream.get();
}

// NOTE: Dirty objects are those who are supposed to be serialized
// or deserialized.
PdfObject::PdfObject(PdfVariant&& var, const PdfReference& indirectReference, bool isDirty)
    : m_Variant(std::move(var)), m_IndirectReference(indirectReference), m_IsDirty(isDirty)
{
    initObject();
    SetVariantOwner();
}

void PdfObject::ForceCreateStream()
{
    DelayedLoadStream();
    forceCreateStream();
}

void PdfObject::SetDocument(PdfDocument* document)
{
    if (m_Document == document)
    {
        // The inner document for variant data objects is guaranteed to be same
        return;
    }

    m_Document = document;
    SetVariantOwner();
}

void PdfObject::DelayedLoad() const
{
    if (m_IsDelayedLoadDone)
        return;

    const_cast<PdfObject&>(*this).DelayedLoadImpl();
    m_IsDelayedLoadDone = true;
    const_cast<PdfObject&>(*this).SetVariantOwner();
}

void PdfObject::DelayedLoadImpl()
{
    // Default implementation of virtual void DelayedLoadImpl() throws, since delayed
    // loading should not be enabled except by types that support it.
    PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
}

void PdfObject::SetVariantOwner()
{
    auto dataType = m_Variant.GetDataType();
    switch (dataType)
    {
        case PdfDataType::Dictionary:
            m_Variant.GetDictionary().SetOwner(*this);
            break;
        case PdfDataType::Array:
            m_Variant.GetArray().SetOwner(*this);
            break;
        default:
            break;
    }
}

void PdfObject::FreeStream()
{
    m_Stream = nullptr;
}

void PdfObject::initObject()
{
    m_Document = nullptr;
    m_Parent = nullptr;
    // By default delayed load is disabled
    m_IsDelayedLoadDone = true;
    m_IsDelayedLoadStreamDone = true;
}

void PdfObject::Write(OutputStreamDevice& device, PdfWriteFlags writeMode,
    const PdfEncrypt* encrypt_, charbuff& buffer) const
{
    DelayedLoad();
    DelayedLoadStream();

    PdfStatefulEncrypt encrypt;
    if (encrypt_ != nullptr)
        encrypt = PdfStatefulEncrypt(*encrypt_, m_IndirectReference);

    if (m_IndirectReference.IsIndirect())
    {
        if ((writeMode & PdfWriteFlags::Clean) == PdfWriteFlags::None
            && (writeMode & PdfWriteFlags::NoPDFAPreserve) != PdfWriteFlags::None)
        {
            utls::FormatTo(buffer, "{} {} obj", m_IndirectReference.ObjectNumber(), m_IndirectReference.GenerationNumber());
            device.Write(buffer);
        }
        else
        {
            // PDF/A compliance requires all objects to be written in a clean way
            utls::FormatTo(buffer, "{} {} obj\n", m_IndirectReference.ObjectNumber(), m_IndirectReference.GenerationNumber());
            device.Write(buffer);
        }
    }

    if (m_Stream != nullptr)
    {
        // Try to compress the flate compress the stream if it has no filters,
        // the compression is not disabled and it's not the /MetaData object,
        // which must be unfiltered as per PDF/A
        const PdfObject* metadataObj;
        if ((writeMode & PdfWriteFlags::NoFlateCompress) == PdfWriteFlags::None
            && m_Stream->GetFilters().size() == 0
            && (m_Document == nullptr 
                || (metadataObj = m_Document->GetCatalog().GetMetadataObject()) == nullptr
                || m_IndirectReference != metadataObj->GetIndirectReference()))
        {
            PdfObject object;
            auto& stream = object.GetOrCreateStream();
            {
                auto output = stream.GetOutputStream({ PdfFilterType::FlateDecode });
                auto input = m_Stream->GetInputStream();
                input.CopyTo(output);
            }

            m_Stream->MoveFrom(stream);
        }

        // Set length if it's not handled by the underlying provider
        if (!m_Stream->GetProvider().IsLengthHandled())
        {
            size_t length = m_Stream->GetLength();
            if (encrypt.HasEncrypt())
                length = encrypt.CalculateStreamLength(length);

            // Add the key without triggering SetDirty
            const_cast<PdfObject&>(*this).m_Variant.GetDictionary()
                .AddKey(PdfName::KeyLength, static_cast<int64_t>(length), true);
        }
    }

    m_Variant.Write(device, writeMode, encrypt, buffer);
    device.Write('\n');

    if (m_Stream != nullptr)
        m_Stream->Write(device, encrypt);

    if (m_IndirectReference.IsIndirect())
        device.Write("endobj\n");

    // After write we ca reset the dirty flag
    const_cast<PdfObject&>(*this).ResetDirty();
}

PdfObjectStream& PdfObject::GetOrCreateStream()
{
    DelayedLoadStream();
    return getOrCreateStream();
}

const PdfObjectStream& PdfObject::MustGetStream() const
{
    DelayedLoadStream();
    if (m_Stream == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The object doesn't have a stream");

    return *m_Stream.get();
}

PdfObjectStream& PdfObject::MustGetStream()
{
    DelayedLoadStream();
    if (m_Stream == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The object doesn't have a stream");

    return *m_Stream.get();
}

bool PdfObject::IsIndirect() const
{
    return m_IndirectReference.IsIndirect();
}

bool PdfObject::HasStream() const
{
    DelayedLoadStream();
    return m_Stream != nullptr;
}

PdfObjectStream& PdfObject::getOrCreateStream()
{
    forceCreateStream();
    return *m_Stream;
}

void PdfObject::forceCreateStream()
{
    if (m_Stream != nullptr)
        return;

    if (m_Variant.GetDataType() != PdfDataType::Dictionary)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Tried to get stream of non-dictionary object");

    if (m_Document == nullptr)
    {
        m_Stream.reset(new PdfObjectStream(*this,
            unique_ptr<PdfObjectStreamProvider>(new PdfMemoryObjectStream())));
    }
    else
    {
        m_Stream.reset(new PdfObjectStream(*this, m_Document->GetObjects().CreateStream()));
    }
}

PdfObjectStream* PdfObject::getStream()
{
    return m_Stream.get();
}

void PdfObject::DelayedLoadStream() const
{
    DelayedLoad();
    delayedLoadStream();
}

void PdfObject::delayedLoadStream() const
{
    if (m_IsDelayedLoadStreamDone)
        return;

    const_cast<PdfObject&>(*this).DelayedLoadStreamImpl();
    m_IsDelayedLoadStreamDone = true;
}

// TODO2: SetDirty only if the value to be added is different
//        For value (numbers) types this is trivial.
//        For dictionaries/lists maybe we can rely on auomatic dirty set
PdfObject& PdfObject::operator=(const PdfObject& rhs)
{
    assign(rhs);
    SetDirty();
    return *this;
}

PdfObject& PdfObject::operator=(PdfObject&& rhs) noexcept
{
    moveFrom(rhs);
    SetDirty();
    return *this;
}

void PdfObject::copyStreamFrom(const PdfObject& obj)
{
    // NOTE: Don't call rhs.DelayedLoad() here. It's implicitly
    // called in PdfVariant assignment or copy constructor
    obj.delayedLoadStream();
    if (obj.m_Stream != nullptr)
    {
        auto& stream = getOrCreateStream();
        stream = *obj.m_Stream;
    }
}

void PdfObject::moveStreamFrom(PdfObject& obj)
{
    obj.DelayedLoadStream();
    m_Stream = std::move(obj.m_Stream);
}

void PdfObject::EnableDelayedLoading()
{
    m_IsDelayedLoadDone = false;
}

void PdfObject::EnableDelayedLoadingStream()
{
    m_IsDelayedLoadStreamDone = false;
}

void PdfObject::DelayedLoadStreamImpl()
{
    // Default implementation throws, since delayed loading of
    // steams should not be enabled except by types that support it.
    PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
}

void PdfObject::Assign(const PdfObject& rhs)
{
    if (&rhs == this)
        return;

    assign(rhs);
}

void PdfObject::SetParent(PdfDataContainer& parent)
{
    m_Parent = &parent;
    auto document = parent.GetObjectDocument();
    SetDocument(document);
}

// NOTE: Don't copy parent document/container and indirect reference.
// Objects being assigned always keep current ownership
void PdfObject::assign(const PdfObject& rhs)
{
    rhs.DelayedLoad();
    m_Variant = rhs.m_Variant;
    m_IsDelayedLoadDone = true;
    SetVariantOwner();
    copyStreamFrom(rhs);
    m_IsDelayedLoadStreamDone = true;
}

// NOTE: Don't move parent document/container and indirect reference.
// Objects being assigned always keep current ownership
void PdfObject::moveFrom(PdfObject& rhs)
{
    rhs.DelayedLoad();
    m_Variant = std::move(rhs.m_Variant);
    m_IsDelayedLoadDone = true;
    SetVariantOwner();
    moveStreamFrom(rhs);
    m_IsDelayedLoadStreamDone = true;
}

void PdfObject::ResetDirty()
{
    PODOFO_ASSERT(m_IsDelayedLoadDone);
    // Propagate new dirty state to subclasses
    switch (m_Variant.GetDataType())
    {
        // Arrays and Dictionaries
        // handle dirty status by themselves
        case PdfDataType::Array:
            static_cast<PdfDataContainer&>(m_Variant.GetArray()).ResetDirty();
            break;
        case PdfDataType::Dictionary:
            static_cast<PdfDataContainer&>(m_Variant.GetDictionary()).ResetDirty();
            break;
        case PdfDataType::Bool:
        case PdfDataType::Number:
        case PdfDataType::Real:
        case PdfDataType::String:
        case PdfDataType::Name:
        case PdfDataType::RawData:
        case PdfDataType::Reference:
        case PdfDataType::Null:
        case PdfDataType::Unknown:
        default:
            break;
    }

    resetDirty();
}

void PdfObject::SetDirty()
{
    if (IsIndirect())
    {
        // Set dirty only if is indirect object
        setDirty();
    }
    else if (m_Parent != nullptr)
    {
        // Reset parent if not indirect. Resetting will stop at
        // first indirect anchestor
        m_Parent->SetDirty();
    }
}

void PdfObject::setDirty()
{
    m_IsDirty = true;
}

void PdfObject::resetDirty()
{
    m_IsDirty = false;
}

PdfObject::operator const PdfVariant& () const
{
    DelayedLoad();
    return m_Variant;
}

PdfDocument& PdfObject::MustGetDocument() const
{
    if (m_Document == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    return *m_Document;
}

const PdfVariant& PdfObject::GetVariant() const
{
    DelayedLoad();
    return m_Variant;
}

PdfDataType PdfObject::GetDataType() const
{
    DelayedLoad();
    return m_Variant.GetDataType();
}

string PdfObject::ToString() const
{
    string ret;
    ToString(ret);
    return ret;
}

void PdfObject::ToString(string& ret) const
{
    ret.clear();
    StringStreamDevice device(ret);
    charbuff buffer;
    Write(device, PdfWriteFlags::None, nullptr, buffer);
}

bool PdfObject::GetBool() const
{
    DelayedLoad();
    return m_Variant.GetBool();
}

bool PdfObject::TryGetBool(bool& value) const
{
    DelayedLoad();
    return m_Variant.TryGetBool(value);
}

int64_t PdfObject::GetNumberLenient() const
{
    DelayedLoad();
    return m_Variant.GetNumberLenient();
}

bool PdfObject::TryGetNumberLenient(int64_t& value) const
{
    DelayedLoad();
    return m_Variant.TryGetNumberLenient(value);
}

int64_t PdfObject::GetNumber() const
{
    DelayedLoad();
    return m_Variant.GetNumber();
}

bool PdfObject::TryGetNumber(int64_t& value) const
{
    DelayedLoad();
    return m_Variant.TryGetNumber(value);
}

double PdfObject::GetReal() const
{
    DelayedLoad();
    return m_Variant.GetReal();
}

bool PdfObject::TryGetReal(double& value) const
{
    DelayedLoad();
    return m_Variant.TryGetReal(value);
}

double PdfObject::GetRealStrict() const
{
    DelayedLoad();
    return m_Variant.GetRealStrict();
}

bool PdfObject::TryGetRealStrict(double& value) const
{
    DelayedLoad();
    return m_Variant.TryGetRealStrict(value);
}

const PdfString& PdfObject::GetString() const
{
    DelayedLoad();
    return m_Variant.GetString();
}

bool PdfObject::TryGetString(PdfString& str) const
{
    DelayedLoad();
    return m_Variant.TryGetString(str);
}

bool PdfObject::TryGetString(const PdfString*& str) const
{
    DelayedLoad();
    return m_Variant.TryGetString(str);
}

const PdfName& PdfObject::GetName() const
{
    DelayedLoad();
    return m_Variant.GetName();
}

bool PdfObject::TryGetName(PdfName& name) const
{
    DelayedLoad();
    return m_Variant.TryGetName(name);
}

bool PdfObject::TryGetName(const PdfName*& name) const
{
    DelayedLoad();
    return m_Variant.TryGetName(name);
}

const PdfArray& PdfObject::GetArray() const
{
    DelayedLoad();
    return m_Variant.GetArray();
}

PdfArray& PdfObject::GetArray()
{
    DelayedLoad();
    return m_Variant.GetArray();
}

bool PdfObject::TryGetArray(const PdfArray*& arr) const
{
    DelayedLoad();
    return m_Variant.TryGetArray(arr);
}

bool PdfObject::TryGetArray(PdfArray*& arr)
{
    DelayedLoad();
    return m_Variant.TryGetArray(arr);
}

const PdfDictionary& PdfObject::GetDictionary() const
{
    DelayedLoad();
    return m_Variant.GetDictionary();
}

PdfDictionary& PdfObject::GetDictionary()
{
    DelayedLoad();
    return m_Variant.GetDictionary();
}

bool PdfObject::TryGetDictionary(const PdfDictionary*& dict) const
{
    DelayedLoad();
    return m_Variant.TryGetDictionary(dict);
}

bool PdfObject::TryGetDictionary(PdfDictionary*& dict)
{
    DelayedLoad();
    return m_Variant.TryGetDictionary(dict);
}

PdfReference PdfObject::GetReference() const
{
    DelayedLoad();
    return m_Variant.GetReference();
}

bool PdfObject::TryGetReference(PdfReference& ref) const
{
    DelayedLoad();
    return m_Variant.TryGetReference(ref);
}

void PdfObject::SetBool(bool b)
{
    DelayedLoad();
    m_Variant.SetBool(b);
    SetDirty();
}

void PdfObject::SetNumber(int64_t l)
{
    DelayedLoad();
    m_Variant.SetNumber(l);
    SetDirty();
}

void PdfObject::SetReal(double d)
{
    DelayedLoad();
    m_Variant.SetReal(d);
    SetDirty();
}

void PdfObject::SetName(const PdfName& name)
{
    DelayedLoad();
    m_Variant.SetName(name);
    SetDirty();
}

void PdfObject::SetString(const PdfString& str)
{
    DelayedLoad();
    m_Variant.SetString(str);
    SetDirty();
}

void PdfObject::SetReference(const PdfReference& ref)
{
    DelayedLoad();
    m_Variant.SetReference(ref);
    SetDirty();
}

const char* PdfObject::GetDataTypeString() const
{
    DelayedLoad();
    return m_Variant.GetDataTypeString();
}

bool PdfObject::IsBool() const
{
    return GetDataType() == PdfDataType::Bool;
}

bool PdfObject::IsNumber() const
{
    return GetDataType() == PdfDataType::Number;
}

bool PdfObject::IsRealStrict() const
{
    return GetDataType() == PdfDataType::Real;
}

bool PdfObject::IsNumberOrReal() const
{
    PdfDataType dataType = GetDataType();
    return dataType == PdfDataType::Number || dataType == PdfDataType::Real;
}

bool PdfObject::IsString() const
{
    return GetDataType() == PdfDataType::String;
}

bool PdfObject::IsName() const
{
    return GetDataType() == PdfDataType::Name;
}

bool PdfObject::IsArray() const
{
    return GetDataType() == PdfDataType::Array;
}

bool PdfObject::IsDictionary() const
{
    return GetDataType() == PdfDataType::Dictionary;
}

bool PdfObject::IsRawData() const
{
    return GetDataType() == PdfDataType::RawData;
}

bool PdfObject::IsNull() const
{
    return GetDataType() == PdfDataType::Null;
}

bool PdfObject::IsReference() const
{
    return GetDataType() == PdfDataType::Reference;
}

bool PdfObject::operator<(const PdfObject& rhs) const
{
    if (m_Document != rhs.m_Document)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Can't compare objects with different parent document");

    return m_IndirectReference < rhs.m_IndirectReference;
}

bool PdfObject::operator==(const PdfObject& rhs) const
{
    if (this == &rhs)
        return true;

    if (m_IndirectReference.IsIndirect())
    {
        // If lhs is indirect, just check document and reference
        return m_Document == rhs.m_Document &&
            m_IndirectReference == rhs.m_IndirectReference;
    }
    else
    {
        // Otherwise check variant
        DelayedLoad();
        rhs.DelayedLoad();
        return m_Variant == rhs.m_Variant;
    }
}

bool PdfObject::operator!=(const PdfObject& rhs) const
{
    if (this != &rhs)
        return true;

    if (m_IndirectReference.IsIndirect())
    {
        // If lhs is indirect, just check document and reference
        return m_Document != rhs.m_Document ||
            m_IndirectReference != rhs.m_IndirectReference;
    }
    else
    {
        // Otherwise check variant
        DelayedLoad();
        rhs.DelayedLoad();
        return m_Variant != rhs.m_Variant;
    }
}

bool PdfObject::operator==(const PdfVariant& rhs) const
{
    DelayedLoad();
    return m_Variant == rhs;
}

bool PdfObject::operator!=(const PdfVariant& rhs) const
{
    DelayedLoad();
    return m_Variant != rhs;
}
