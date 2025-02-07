/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfVariant.h"

#include <podofo/auxiliary/StreamDevice.h>
#include <podofo/private/PdfParserObject.h>

#include "PdfArray.h"
#include "PdfData.h"
#include "PdfDictionary.h"

using namespace PoDoFo;
using namespace std;

constexpr unsigned short DefaultPrecision = 6;

const PdfVariant PdfVariant::Null;

PdfVariant::PdfVariant()
    : m_Null() { }

PdfVariant::PdfVariant(bool value)
    : m_Bool(value) { }

PdfVariant::PdfVariant(int64_t value)
    : m_Number(value) { }

PdfVariant::PdfVariant(double value)
    : m_Real(value) { }

PdfVariant::PdfVariant(const PdfString& str)
    : m_String(str) { }

PdfVariant::PdfVariant(const PdfName& name)
    : m_Name(name) { }

PdfVariant::PdfVariant(const PdfReference& ref)
    : m_Reference(ref) { }

PdfVariant::PdfVariant(const PdfArray& arr)
    : m_Array(new PdfArray(arr)) { }

PdfVariant::PdfVariant(PdfArray&& arr) noexcept
    : m_Array(new PdfArray(std::move(arr))) { }

PdfVariant::PdfVariant(const PdfDictionary& dict)
    : m_Dictionary(new PdfDictionary(dict)) { }

PdfVariant::PdfVariant(PdfDictionary&& dict) noexcept
    : m_Dictionary(new PdfDictionary(std::move(dict))) { }

PdfVariant::PdfVariant(const PdfData& data)
    : m_Data(new PdfData(data)) { }

PdfVariant::PdfVariant(PdfData&& data) noexcept
    : m_Data(new PdfData(std::move(data))) { }

PdfVariant::PdfVariant(const PdfVariant& rhs)
{
    assign(rhs);
}

PdfVariant::PdfVariant(PdfVariant&& rhs) noexcept
{
    moveFrom(std::move(rhs));
}

PdfVariant::PdfVariant(PdfDictionary* dict)
    : m_Dictionary(dict)
{
}

PdfVariant::PdfVariant(PdfArray* arr)
    : m_Array(arr)
{
}

PdfVariant::~PdfVariant()
{
    switch (GetDataType())
    {
        case PdfDataType::Array:
        {
            delete m_Array.Value;
            break;
        }
        case PdfDataType::Dictionary:
        {
            delete m_Dictionary.Value;
            break;
        }
        case PdfDataType::RawData:
        {
            delete m_Data.Value;
            break;
        }
        case PdfDataType::Name:
        {
            m_Name.~PdfName();
            break;
        }
        case PdfDataType::String:
        {
            m_String.~PdfString();
            break;
        }
        case PdfDataType::Reference:
        case PdfDataType::Bool:
        case PdfDataType::Null:
        case PdfDataType::Number:
        case PdfDataType::Real:
        case PdfDataType::Unknown:
        default:
            break;
    }
}

void PdfVariant::Write(OutputStream& device, PdfWriteFlags writeMode,
    const PdfStatefulEncrypt* encrypt, charbuff& buffer) const
{
    switch (GetDataType())
    {
        case PdfDataType::Bool:
        {
            if ((writeMode & PdfWriteFlags::NoInlineLiteral) == PdfWriteFlags::None)
                device.Write(' '); // Write space before true or false

            if (m_Bool.Value)
                device.Write("true");
            else
                device.Write("false");
            break;
        }
        case PdfDataType::Number:
        {
            if ((writeMode & PdfWriteFlags::NoInlineLiteral) == PdfWriteFlags::None)
                device.Write(' '); // Write space before numbers

            utls::FormatTo(buffer, "{}", m_Number.Value);
            device.Write(buffer);
            break;
        }
        case PdfDataType::Real:
        {
            if ((writeMode & PdfWriteFlags::NoInlineLiteral) == PdfWriteFlags::None)
                device.Write(' '); // Write space before numbers

            utls::FormatTo(buffer, m_Real.Value, DefaultPrecision);
            device.Write(buffer);
            break;
        }
        case PdfDataType::Reference:
        {
            m_Reference.Write(device, writeMode, encrypt, buffer);
            break;
        }
        case PdfDataType::String:
        {
            m_String.Write(device, writeMode, encrypt, buffer);
            break;
        }
        case PdfDataType::Name:
        {
            m_Name.Write(device, writeMode, encrypt, buffer);
            break;
        }
        case PdfDataType::Array:
        {
            m_Array.Value->Write(device, writeMode, encrypt, buffer);
            break;
        }
        case PdfDataType::Dictionary:
        {
            m_Dictionary.Value->Write(device, writeMode, encrypt, buffer);
            break;
        }
        case PdfDataType::RawData:
        {
            m_Data.Value->Write(device, writeMode, encrypt, buffer);
            break;
        }
        case PdfDataType::Null:
        {
            if ((writeMode & PdfWriteFlags::NoInlineLiteral) == PdfWriteFlags::None)
                device.Write(' '); // Write space before null

            device.Write("null");
            break;
        }
        case PdfDataType::Unknown:
        default:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);
            break;
        }
    };
}

string PdfVariant::ToString(PdfWriteFlags writeFlags) const
{
    string ret;
    ToString(ret, writeFlags);
    return ret;
}

void PdfVariant::ToString(string& str, PdfWriteFlags writeFlags) const
{
    str.clear();
    switch (GetDataType())
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

    charbuff buffer;
    StringStreamDevice device(str);
    this->Write(device, writeFlags, nullptr, buffer);
}

PdfVariant& PdfVariant::operator=(const PdfVariant& rhs)
{
    this->~PdfVariant();
    assign(rhs);
    return *this;
}

PdfVariant& PdfVariant::operator=(PdfVariant&& rhs) noexcept
{
    this->~PdfVariant();
    moveFrom(std::move(rhs));
    return *this;
}

void PdfVariant::assign(const PdfVariant& rhs)
{
    switch (rhs.GetDataType())
    {
        case PdfDataType::Array:
        {
            new(&m_Array)PrimitiveMember(new PdfArray(*rhs.m_Array.Value));
            break;
        }
        case PdfDataType::Dictionary:
        {
            new(&m_Dictionary)PrimitiveMember(new PdfDictionary(*rhs.m_Dictionary.Value));
            break;
        }
        case PdfDataType::RawData:
        {
            new(&m_Data)PrimitiveMember(new PdfData(*rhs.m_Data.Value));
            break;
        }
        case PdfDataType::Name:
        {
            new(&m_Name)PdfName(rhs.m_Name);
            break;
        }
        case PdfDataType::String:
        {
            new(&m_String)PdfString(rhs.m_String);
            break;
        }
        case PdfDataType::Reference:
        {
            new(&m_Reference)PdfReference(rhs.m_Reference);
            break;
        }
        case PdfDataType::Bool:
        {
            new(&m_Bool)PrimitiveMember(rhs.m_Bool.Value);
            break;
        }
        case PdfDataType::Number:
        {
            new(&m_Number)PrimitiveMember(rhs.m_Number.Value);
            break;
        }
        case PdfDataType::Real:
        {
            new(&m_Real)PrimitiveMember(rhs.m_Real.Value);
            break;
        }
        case PdfDataType::Null:
        {
            new(&m_Null)NullMember();
            break;
        }
        case PdfDataType::Unknown:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

void PdfVariant::moveFrom(PdfVariant&& rhs)
{
    switch (rhs.GetDataType())
    {
        case PdfDataType::Array:
        {
            new(&m_Array)PrimitiveMember(rhs.m_Array);
            break;
        }
        case PdfDataType::Dictionary:
        {
            new(&m_Dictionary)PrimitiveMember(rhs.m_Dictionary);
            break;
        }
        case PdfDataType::RawData:
        {
            new(&m_Data)PrimitiveMember(rhs.m_Data);
            break;
        }
        case PdfDataType::Name:
        {
            new(&m_Name)PdfName(std::move(rhs.m_Name));
            break;
        }
        case PdfDataType::String:
        {
            new(&m_String)PdfString(std::move(rhs.m_String));
            break;
        }
        case PdfDataType::Reference:
        {
            new(&m_Reference)PdfReference(rhs.m_Reference);
            break;
        }
        case PdfDataType::Bool:
        {
            new(&m_Bool)PrimitiveMember(rhs.m_Bool.Value);
            break;
        }
        case PdfDataType::Number:
        {
            new(&m_Number)PrimitiveMember(rhs.m_Number.Value);
            break;
        }
        case PdfDataType::Real:
        {
            new(&m_Real)PrimitiveMember(rhs.m_Real.Value);
            break;
        }
        case PdfDataType::Null:
        {
            new(&m_Null)NullMember();
            break;
        }
        case PdfDataType::Unknown:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    // Reset rhs to null type
    new(&rhs.m_Null)NullMember();
}

void PdfVariant::Reset()
{
    this->~PdfVariant();
    new(&m_Null)NullMember();
}

string_view PdfVariant::GetDataTypeString() const
{
    switch (GetDataType())
    {
        case PdfDataType::Bool:
            return "Bool"sv;
        case PdfDataType::Number:
            return "Number"sv;
        case PdfDataType::Real:
            return "Real"sv;
        case PdfDataType::String:
            return "String"sv;
        case PdfDataType::Name:
            return "Name"sv;
        case PdfDataType::Array:
            return "Array"sv;
        case PdfDataType::Dictionary:
            return "Dictionary"sv;
        case PdfDataType::Null:
            return "Null"sv;
        case PdfDataType::Reference:
            return "Reference"sv;
        case PdfDataType::RawData:
            return "RawData"sv;
        case PdfDataType::Unknown:
            return "Unknown"sv;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

bool PdfVariant::operator==(const PdfVariant& rhs) const
{
    if (this == &rhs)
        return true;

    switch (GetDataType())
    {
        case PdfDataType::Bool:
        {
            bool value;
            if (rhs.TryGetBool(value))
                return m_Bool.Value == value;
            else
                return false;
        }
        case PdfDataType::Number:
        {
            int64_t value;
            if (rhs.TryGetNumber(value))
                return m_Number.Value == value;
            else
                return false;
        }
        case PdfDataType::Real:
        {
            // NOTE: Real type equality semantics is strict
            double value;
            if (rhs.TryGetRealStrict(value))
                return m_Real.Value == value;
            else
                return false;
        }
        case PdfDataType::Reference:
        {
            PdfReference value;
            if (rhs.TryGetReference(value))
                return m_Reference == value;
            else
                return false;
        }
        case PdfDataType::String:
        {
            const PdfString* value;
            if (rhs.tryGetString(value))
                return m_String == *value;
            else
                return false;
        }
        case PdfDataType::Name:
        {
            const PdfName* value;
            if (rhs.tryGetName(value))
                return m_Name == *value;
            else
                return false;
        }
        case PdfDataType::Array:
        {
            const PdfArray* value;
            if (rhs.TryGetArray(value))
                return *m_Array.Value == *value;
            else
                return false;
        }
        case PdfDataType::Dictionary:
        {
            const PdfDictionary* value;
            if (rhs.TryGetDictionary(value))
                return *m_Dictionary.Value == *value;
            else
                return false;
        }
        case PdfDataType::RawData:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Equality not yet implemented for RawData");
        case PdfDataType::Null:
            return rhs.GetDataType() == PdfDataType::Null;
        case PdfDataType::Unknown:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

bool PdfVariant::operator!=(const PdfVariant& rhs) const
{
    if (this != &rhs)
        return true;

    switch (GetDataType())
    {
        case PdfDataType::Bool:
        {
            bool value;
            if (rhs.TryGetBool(value))
                return m_Bool.Value != value;
            else
                return true;
        }
        case PdfDataType::Number:
        {
            int64_t value;
            if (rhs.TryGetNumber(value))
                return m_Number.Value != value;
            else
                return true;
        }
        case PdfDataType::Real:
        {
            // NOTE: Real type equality semantics is strict
            double value;
            if (rhs.TryGetRealStrict(value))
                return m_Real.Value != value;
            else
                return true;
        }
        case PdfDataType::Reference:
        {
            PdfReference value;
            if (rhs.TryGetReference(value))
                return m_Reference != value;
            else
                return true;
        }
        case PdfDataType::String:
        {
            const PdfString* value;
            if (rhs.tryGetString(value))
                return m_String != *value;
            else
                return true;
        }
        case PdfDataType::Name:
        {
            const PdfName* value;
            if (rhs.tryGetName(value))
                return m_Name != *value;
            else
                return true;
        }
        case PdfDataType::Array:
        {
            const PdfArray* value;
            if (rhs.TryGetArray(value))
                return *m_Array.Value != *value;
            else
                return true;
        }
        case PdfDataType::Dictionary:
        {
            const PdfDictionary* value;
            if (rhs.TryGetDictionary(value))
                return *m_Dictionary.Value != *value;
            else
                return true;
        }
        case PdfDataType::RawData:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Disequality not yet implemented for RawData");
        case PdfDataType::Null:
            return rhs.GetDataType() != PdfDataType::Null;
        case PdfDataType::Unknown:
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

PdfDataType PdfVariant::GetDataType() const
{
    return reinterpret_cast<const PdfDataMember&>(*this).GetDataType();
}

bool PdfVariant::GetBool() const
{
    bool ret;
    if (!TryGetBool(ret))
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetBool(bool& value) const
{
    if (GetDataType() != PdfDataType::Bool)
    {
        value = false;
        return false;
    }

    value = m_Bool.Value;
    return true;
}

int64_t PdfVariant::GetNumberLenient() const
{
    int64_t ret;
    if (!TryGetNumberLenient(ret))
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetNumberLenient(int64_t& value) const
{
    auto type = GetDataType();
    if (type == PdfDataType::Number)
    {
        value = m_Number.Value;
        return true;
    }
    else if (type == PdfDataType::Real)
    {
        value = static_cast<int64_t>(std::round(m_Real.Value));
        return true;
    }
    else
    {
        value = 0;
        return false;
    }
}

int64_t PdfVariant::GetNumber() const
{
    int64_t ret;
    if (!TryGetNumber(ret))
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return m_Number.Value;
}

bool PdfVariant::TryGetNumber(int64_t& value) const
{
    if (GetDataType() != PdfDataType::Number)
    {
        value = 0;
        return false;
    }

    value = m_Number.Value;
    return true;
}

double PdfVariant::GetReal() const
{
    double ret;
    if (!TryGetReal(ret))
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetReal(double& value) const
{
    auto type = GetDataType();
    if (type == PdfDataType::Real)
    {
        value = m_Real.Value;
        return true;
    }
    else if (type == PdfDataType::Number)
    {
        value = static_cast<double>(m_Number.Value);
        return true;
    }
    else
    {
        value = 0;
        return false;
    }
}

double PdfVariant::GetRealStrict() const
{
    double ret;
    if (!TryGetRealStrict(ret))
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return m_Real.Value;
}

bool PdfVariant::TryGetRealStrict(double& value) const
{
    if (GetDataType() != PdfDataType::Real)
    {
        value = 0;
        return false;
    }

    value = m_Real.Value;
    return true;
}

const PdfString& PdfVariant::GetString() const
{
    const PdfString* ret;
    if (!tryGetString(ret))
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetString(PdfString& str) const
{
    const PdfString* ret;
    if (!tryGetString(ret))
    {
        str = { };
        return false;
    }

    str = *ret;
    return true;
}

bool PdfVariant::TryGetString(const PdfString*& str) const
{
    return tryGetString(str);
}

bool PdfVariant::tryGetString(const PdfString*& str) const
{
    if (GetDataType() != PdfDataType::String)
    {
        str = nullptr;
        return false;
    }

    str = &m_String;
    return true;
}

const PdfName& PdfVariant::GetName() const
{
    const PdfName* ret;
    if (!tryGetName(ret))
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetName(PdfName& name) const
{
    const PdfName* ret;
    if (!tryGetName(ret))
    {
        name = { };
        return false;
    }

    name = *ret;
    return true;
}

bool PdfVariant::TryGetName(const PdfName*& name) const
{
    return tryGetName(name);
}

bool PdfVariant::tryGetName(const PdfName*& name) const
{
    if (GetDataType() != PdfDataType::Name)
    {
        name = nullptr;
        return false;
    }

    name = &m_Name;
    return true;
}

PdfReference PdfVariant::GetReference() const
{
    PdfReference ret;
    if (!TryGetReference(ret))
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetReference(PdfReference& ref) const
{
    if (GetDataType() != PdfDataType::Reference)
    {
        ref = PdfReference();
        return false;
    }

    ref = m_Reference;
    return true;
}

const PdfArray& PdfVariant::GetArray() const
{
    PdfArray* ret;
    if (!tryGetArray(ret))
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return *ret;
}

PdfArray& PdfVariant::GetArray()
{
    PdfArray* ret;
    if (!tryGetArray(ret))
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetArray(const PdfArray*& arr) const
{
    return tryGetArray(const_cast<PdfArray*&>(arr));
}

bool PdfVariant::TryGetArray(PdfArray*& arr)
{
    return tryGetArray(arr);
}

const PdfDictionary& PdfVariant::GetDictionary() const
{
    PdfDictionary* ret;
    if (!tryGetDictionary(ret))
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return *ret;
}

PdfDictionary& PdfVariant::GetDictionary()
{
    PdfDictionary* ret;
    if (!tryGetDictionary(ret))
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetDictionary(const PdfDictionary*& dict) const
{
    return tryGetDictionary(const_cast<PdfDictionary*&>(dict));
}

bool PdfVariant::TryGetDictionary(PdfDictionary*& dict)
{
    return tryGetDictionary(dict);
}

bool PdfVariant::tryGetDictionary(PdfDictionary*& dict) const
{
    if (GetDataType() != PdfDataType::Dictionary)
    {
        dict = nullptr;
        return false;
    }

    dict = (PdfDictionary*)m_Dictionary.Value;
    return true;
}

bool PdfVariant::tryGetArray(PdfArray*& arr) const
{
    if (GetDataType() != PdfDataType::Array)
    {
        arr = nullptr;
        return false;
    }

    arr = (PdfArray*)m_Array.Value;
    return true;
}

PdfReference PdfVariant::GetReferenceUnsafe() const
{
    return m_Reference;
}

const PdfDictionary& PdfVariant::GetDictionaryUnsafe() const
{
    return *(const PdfDictionary*)m_Dictionary.Value;
}

const PdfArray& PdfVariant::GetArrayUnsafe() const
{
    return *(const PdfArray*)m_Array.Value;
}

PdfDictionary& PdfVariant::GetDictionaryUnsafe()
{
    return *(PdfDictionary*)m_Dictionary.Value;
}

PdfArray& PdfVariant::GetArrayUnsafe()
{
    return *(PdfArray*)m_Array.Value;
}

void PdfVariant::SetBool(bool value)
{
    if (GetDataType() != PdfDataType::Bool)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    m_Bool = value;
}

void PdfVariant::SetNumber(int64_t value)
{
    if (GetDataType() != PdfDataType::Number)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    m_Number = value;
}

void PdfVariant::SetReal(double value)
{
    if (GetDataType() != PdfDataType::Real)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    m_Real = value;
}

void PdfVariant::SetName(const PdfName& name)
{
    if (GetDataType() != PdfDataType::Name)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    m_Name = name;
}

void PdfVariant::SetString(const PdfString& str)
{
    if (GetDataType() != PdfDataType::String)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    m_String = str;
}

void PdfVariant::SetReference(const PdfReference& ref)
{
    if (GetDataType() != PdfDataType::Reference)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    m_Reference = ref;
}

bool PdfVariant::IsBool() const
{
    return GetDataType() == PdfDataType::Bool;
}

bool PdfVariant::IsNumber() const
{
    return GetDataType() == PdfDataType::Number;
}

bool PdfVariant::IsRealStrict() const
{
    return GetDataType() == PdfDataType::Real;
}

bool PdfVariant::IsNumberOrReal() const
{
    auto type = GetDataType();
    return type == PdfDataType::Number || type == PdfDataType::Real;
}

bool PdfVariant::IsString() const
{
    return GetDataType() == PdfDataType::String;
}

bool PdfVariant::IsName() const
{
    return GetDataType() == PdfDataType::Name;
}

bool PdfVariant::IsArray() const
{
    return GetDataType() == PdfDataType::Array;
}

bool PdfVariant::IsDictionary() const
{
    return GetDataType() == PdfDataType::Dictionary;
}

bool PdfVariant::IsRawData() const
{
    return GetDataType() == PdfDataType::RawData;
}

bool PdfVariant::IsNull() const
{
    return GetDataType() == PdfDataType::Null;
}

bool PdfVariant::IsReference() const
{
    return GetDataType() == PdfDataType::Reference;
}

PdfVariant::NullMember::NullMember()
    : PdfDataMember(PdfDataType::Null)
{
}
