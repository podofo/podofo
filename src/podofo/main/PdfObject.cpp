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
#include "PdfStatefulEncrypt.h"
#include "PdfMemoryObjectStream.h"

#include <podofo/auxiliary/StreamDevice.h>
#include <podofo/private/PdfStreamedObjectStream.h>

using namespace std;
using namespace PoDoFo;

const PdfObject PdfObject::Null = PdfObject(nullptr);

PdfObject::PdfObject()
    : m_Variant(new PdfDictionary()), m_IsDirty(false), m_IsImmutable(false)
{
    initObject();
    m_Variant.GetDictionaryUnsafe().SetOwner(*this);
}

PdfObject::PdfObject(nullptr_t)
    : m_IsDirty(false), m_IsImmutable(false)
{
    initObject();
}

PdfObject::~PdfObject() { }

PdfObject::PdfObject(const PdfVariant& var)
    : PdfObject(PdfVariant(var), PdfReference(), false) { }

PdfObject::PdfObject(PdfVariant&& var) noexcept
    : m_Variant(std::move(var)), m_IsDirty(false), m_IsImmutable(false)
{
    initObject();
    SetVariantOwner();
}

PdfObject::PdfObject(bool b)
    : m_Variant(b), m_IsDirty(false), m_IsImmutable(false)
{
    initObject();
}

PdfObject::PdfObject(int64_t l)
    : m_Variant(l), m_IsDirty(false), m_IsImmutable(false)
{
    initObject();
}

PdfObject::PdfObject(double d)
    : m_Variant(d), m_IsDirty(false), m_IsImmutable(false)
{
    initObject();
}

PdfObject::PdfObject(const PdfString& str)
    : m_Variant(str), m_IsDirty(false), m_IsImmutable(false)
{
    initObject();
}
PdfObject::PdfObject(const PdfName& name)
    : m_Variant(name), m_IsDirty(false), m_IsImmutable(false)
{
    initObject();
}

PdfObject::PdfObject(const PdfReference& ref)
    : m_Variant(ref), m_IsDirty(false), m_IsImmutable(false)
{
    initObject();
}

PdfObject::PdfObject(const PdfArray& arr)
    : m_Variant(arr), m_IsDirty(false), m_IsImmutable(false)
{
    initObject();
    m_Variant.GetArray().SetOwner(*this);
}

PdfObject::PdfObject(PdfArray&& arr) noexcept
    : m_Variant(std::move(arr)), m_IsDirty(false), m_IsImmutable(false)
{
    initObject();
    m_Variant.GetArray().SetOwner(*this);
}

PdfObject::PdfObject(const PdfDictionary& dict)
    : m_Variant(dict), m_IsDirty(false), m_IsImmutable(false)
{
    initObject();
    m_Variant.GetDictionaryUnsafe().SetOwner(*this);
}

PdfObject::PdfObject(PdfDictionary&& dict) noexcept
    : m_Variant(std::move(dict)), m_IsDirty(false), m_IsImmutable(false)
{
    initObject();
    m_Variant.GetDictionaryUnsafe().SetOwner(*this);
}

// NOTE: Don't copy parent document/container/indirect reference.
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
PdfObject::PdfObject(PdfObject&& rhs) noexcept
    : m_Document(nullptr), m_Parent(nullptr), m_IsDirty(false), m_IsImmutable(false)
{
    moveFrom(std::move(rhs));
    rhs.SetDirty();
}

// NOTE: Dirty objects are those who are supposed to be serialized
// or deserialized.
PdfObject::PdfObject(PdfVariant&& var, const PdfReference& indirectReference, bool isDirty)
    : m_Variant(std::move(var)), m_IndirectReference(indirectReference), m_IsDirty(isDirty), m_IsImmutable(false)
{
    initObject();
    SetVariantOwner();
}

PdfObject::PdfObject(PdfArray* arr)
    : m_Variant(arr), m_IsDirty(false), m_IsImmutable(false)
{
    initObject();
    m_Variant.GetDictionaryUnsafe().SetOwner(*this);
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

    const_cast<PdfObject&>(*this).delayedLoad();
    m_IsDelayedLoadDone = true;
    const_cast<PdfObject&>(*this).SetVariantOwner();
}

void PdfObject::delayedLoad()
{
    // Default implementation of virtual void delayedLoad() throws, since delayed
    // loading should not be enabled except by types that support it.
    PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
}

void PdfObject::SetVariantOwner()
{
    auto dataType = m_Variant.GetDataType();
    switch (dataType)
    {
        case PdfDataType::Dictionary:
            m_Variant.GetDictionaryUnsafe().SetOwner(*this);
            break;
        case PdfDataType::Array:
            m_Variant.GetArrayUnsafe().SetOwner(*this);
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

void PdfObject::Write(OutputStream& stream, PdfWriteFlags writeMode,
    const PdfStatefulEncrypt* encrypt, charbuff& buffer) const
{
    DelayedLoadStream();
    write(stream, true, writeMode, encrypt, buffer);
}

void PdfObject::WriteFinal(OutputStream& stream, PdfWriteFlags writeMode, const PdfStatefulEncrypt* encrypt, charbuff& buffer)
{
    DelayedLoadStream();
    write(stream, false, writeMode, encrypt, buffer);

    // After writing we can reset the dirty flag
    ResetDirty();
}

void PdfObject::write(OutputStream& stream, bool skipLengthFix,
    PdfWriteFlags writeMode, const PdfStatefulEncrypt* encrypt, charbuff& buffer) const
{
    if (m_IndirectReference.IsIndirect())
        WriteHeader(stream, writeMode, buffer);

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
            auto& objStream = object.GetOrCreateStream();
            {
                auto output = objStream.GetOutputStream({ PdfFilterType::FlateDecode });
                auto input = m_Stream->GetInputStream();
                input.CopyTo(output);
            }

            m_Stream->MoveFrom(objStream);
        }

        // Set length if it's not handled by the underlying provider
        if (!skipLengthFix)
        {
            size_t length = m_Stream->GetLength();
            if (encrypt != nullptr)
                length = encrypt->CalculateStreamLength(length);

            // Add the key without triggering SetDirty
            const_cast<PdfObject&>(*this).m_Variant.GetDictionaryUnsafe()
                .AddKeyNoDirtySet("Length"_n, PdfVariant(static_cast<int64_t>(length)));
        }
    }

    m_Variant.Write(stream, writeMode, encrypt, buffer);
    stream.Write('\n');

    if (m_Stream != nullptr)
        m_Stream->Write(stream, encrypt);

    if (m_IndirectReference.IsIndirect())
        stream.Write("endobj\n");
}

void PdfObject::WriteHeader(OutputStream& stream, PdfWriteFlags writeMode, charbuff& buffer) const
{
    if ((writeMode & PdfWriteFlags::Clean) != PdfWriteFlags::None
        || (writeMode & PdfWriteFlags::PdfAPreserve) != PdfWriteFlags::None)
    {
        // PDF/A compliance requires all objects to be written in a clean way
        utls::FormatTo(buffer, "{} {} obj\n", m_IndirectReference.ObjectNumber(), m_IndirectReference.GenerationNumber());
        stream.Write(buffer);
    }
    else
    {
        utls::FormatTo(buffer, "{} {} obj", m_IndirectReference.ObjectNumber(), m_IndirectReference.GenerationNumber());
        stream.Write(buffer);
    }
}

PdfObjectStream& PdfObject::GetOrCreateStream()
{
    DelayedLoadStream();
    return getOrCreateStream();
}

void PdfObject::RemoveStream()
{
    DelayedLoad();
    // Unconditionally set the stream as already loaded,
    // then just remove it
    m_IsDelayedLoadStreamDone = true;
    bool hasStream = m_Stream != nullptr || removeStream();
    m_Stream = nullptr;
    if (hasStream)
        SetDirty();
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

bool PdfObject::TryUnload()
{
    // Do nothing on base PdfObject class
    return false;
}

bool PdfObject::HasStream() const
{
    DelayedLoad();
    return m_Stream != nullptr || HasStreamToParse();
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

    const_cast<PdfObject&>(*this).delayedLoadStream();
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
    moveFrom(std::move(rhs));
    SetDirty();
    rhs.SetDirty();
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
        stream.CopyFrom(*obj.m_Stream);
    }
}

void PdfObject::moveStreamFrom(PdfObject& obj)
{
    obj.DelayedLoadStream();
    m_Stream = std::move(obj.m_Stream);
    if (m_Stream != nullptr)
        m_Stream->SetParent(*this);
}

void PdfObject::EnableDelayedLoading()
{
    m_IsDelayedLoadDone = false;
}

void PdfObject::EnableDelayedLoadingStream()
{
    m_IsDelayedLoadStreamDone = false;
}

void PdfObject::SetRevised()
{
    // Do nothing on base PdfObject class
}

void PdfObject::delayedLoadStream()
{
    // Default implementation throws, since delayed loading of
    // streams should not be enabled except by types that support it.
    PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);
}

bool PdfObject::removeStream()
{
    // Do nothing for regular object
    return false;
}

bool PdfObject::HasStreamToParse() const
{
    return false;
}

void PdfObject::AssignNoDirtySet(const PdfObject& rhs)
{
    PODOFO_ASSERT(&rhs != this);
    assign(rhs);
}

void PdfObject::AssignNoDirtySet(PdfObject&& rhs)
{
    PODOFO_ASSERT(&rhs != this);
    moveFrom(std::move(rhs));
}

void PdfObject::AssignNoDirtySet(PdfVariant&& rhs)
{
    m_Variant = std::move(rhs);
    m_IsDelayedLoadDone = true;
    SetVariantOwner();
    m_Stream = nullptr;
    m_IsDelayedLoadStreamDone = true;
}

void PdfObject::SetParent(PdfDataContainer& parent)
{
    m_Parent = &parent;
    auto document = parent.GetObjectDocument();
    SetDocument(document);
}

void PdfObject::assertMutable() const
{
    if (m_IsImmutable)
        PODOFO_RAISE_ERROR(PdfErrorCode::ChangeOnImmutable);
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
void PdfObject::moveFrom(PdfObject&& rhs)
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
            m_Variant.GetArrayUnsafe().ResetDirty();
            break;
        case PdfDataType::Dictionary:
            m_Variant.GetDictionaryUnsafe().ResetDirty();
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
        // first indirect ancestor
        m_Parent->SetDirty();
    }
}

void PdfObject::setDirty()
{
    m_IsDirty = true;
    SetRevised();
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

string PdfObject::ToString(PdfWriteFlags writeFlags) const
{
    string ret;
    ToString(ret, writeFlags);
    return ret;
}

void PdfObject::ToString(string& ret, PdfWriteFlags writeFlags) const
{
    DelayedLoadStream();
    ret.clear();
    switch (m_Variant.GetDataType())
    {
        case PdfDataType::Null:
        case PdfDataType::Bool:
        case PdfDataType::Number:
        case PdfDataType::Real:
        case PdfDataType::Reference:
            // We enforce the literals to not be spaced
            writeFlags |= PdfWriteFlags::NoInlineLiteral;
            break;
        default:
            // Do nothing
            break;
    }

    StringStreamDevice device(ret);
    charbuff buffer;
    write(device, true, writeFlags, nullptr, buffer);
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

bool PdfObject::TryGetArray(PdfArray& arr) const
{
    DelayedLoad();
    const PdfArray* val;
    if (m_Variant.TryGetArray(val))
    {
        arr = *val;
        return true;
    }
    else
    {
        arr.Clear();
        return false;
    }
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

bool PdfObject::TryGetDictionary(PdfDictionary& dict) const
{
    DelayedLoad();
    const PdfDictionary* val;
    if (m_Variant.TryGetDictionary(val))
    {
        dict = *val;
        return true;
    }
    else
    {
        dict.Clear();
        return false;
    }
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
    assertMutable();
    DelayedLoad();
    m_Variant.SetBool(b);
    SetDirty();
}

void PdfObject::SetNumber(int64_t l)
{
    assertMutable();
    DelayedLoad();
    m_Variant.SetNumber(l);
    SetDirty();
}

void PdfObject::SetReal(double d)
{
    assertMutable();
    DelayedLoad();
    m_Variant.SetReal(d);
    SetDirty();
}

void PdfObject::SetName(const PdfName& name)
{
    assertMutable();
    DelayedLoad();
    m_Variant.SetName(name);
    SetDirty();
}

void PdfObject::SetString(const PdfString& str)
{
    assertMutable();
    DelayedLoad();
    m_Variant.SetString(str);
    SetDirty();
}

void PdfObject::SetReference(const PdfReference& ref)
{
    assertMutable();
    DelayedLoad();
    m_Variant.SetReference(ref);
    SetDirty();
}

void PdfObject::SetNumberNoDirtySet(int64_t l)
{
    PODOFO_ASSERT(m_IsDelayedLoadDone);
    m_Variant.SetNumber(l);
}

void PdfObject::SetImmutable()
{
    PODOFO_ASSERT(m_IsDelayedLoadDone);
    m_IsImmutable = true;
}

string_view PdfObject::GetDataTypeString() const
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
