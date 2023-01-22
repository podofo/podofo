/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfVariant.h"

#include "PdfArray.h"
#include "PdfData.h"
#include "PdfDictionary.h"
#include "PdfParserObject.h"
#include "PdfStreamDevice.h"

using namespace PoDoFo;
using namespace std;

constexpr unsigned short DefaultPrecision = 6;

PdfVariant PdfVariant::Null;

PdfVariant::PdfVariant(PdfDataType type)
    : m_Data{ }, m_DataType(type) { }

PdfVariant::PdfVariant()
    : PdfVariant(PdfDataType::Null) { }

PdfVariant::PdfVariant(bool value)
    : PdfVariant(PdfDataType::Bool)
{
    m_Data.Bool = value;
}

PdfVariant::PdfVariant(int64_t value)
    : PdfVariant(PdfDataType::Number)
{
    m_Data.Number = value;
}

PdfVariant::PdfVariant(double value)
    : PdfVariant(PdfDataType::Real)
{
    m_Data.Real = value;
}

PdfVariant::PdfVariant(const PdfString& str)
    : PdfVariant(PdfDataType::String)
{
    m_Data.Data = new PdfString(str);
}

PdfVariant::PdfVariant(const PdfName& name)
    : PdfVariant(PdfDataType::Name)
{
    m_Data.Data = new PdfName(name);
}

PdfVariant::PdfVariant(const PdfReference& ref)
    : PdfVariant(PdfDataType::Reference)
{
    m_Data.Reference = ref;
}

PdfVariant::PdfVariant(const PdfArray& arr)
    : PdfVariant(PdfDataType::Array)
{
    m_Data.Data = new PdfArray(arr);
}

PdfVariant::PdfVariant(PdfArray&& arr) noexcept
    : PdfVariant(PdfDataType::Array)
{
    m_Data.Data = new PdfArray(std::move(arr));
}

PdfVariant::PdfVariant(const PdfDictionary& dict)
    : PdfVariant(PdfDataType::Dictionary)
{
    m_Data.Data = new PdfDictionary(dict);
}

PdfVariant::PdfVariant(PdfDictionary&& dict) noexcept
    : PdfVariant(PdfDataType::Dictionary)
{
    m_Data.Data = new PdfDictionary(std::move(dict));
}

PdfVariant::PdfVariant(const PdfData& data)
    : PdfVariant(PdfDataType::RawData)
{
    m_Data.Data = new PdfData(data);
}

PdfVariant::PdfVariant(PdfData&& data) noexcept
    : PdfVariant(PdfDataType::RawData)
{
    m_Data.Data = new PdfData(std::move(data));
}


PdfVariant::PdfVariant(const PdfVariant& rhs)
    : m_Data{ }
{
    assign(rhs);
}

PdfVariant::PdfVariant(PdfVariant&& rhs) noexcept
    : m_Data(rhs.m_Data), m_DataType(rhs.m_DataType)
{
    rhs.m_Data = { };
    rhs.m_DataType = PdfDataType::Null;
}

PdfVariant::~PdfVariant()
{
    clear();
}

void PdfVariant::clear()
{
    switch (m_DataType)
    {
        case PdfDataType::Array:
        case PdfDataType::Dictionary:
        case PdfDataType::Name:
        case PdfDataType::String:
        case PdfDataType::RawData:
        {
            delete m_Data.Data;
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

void PdfVariant::Write(OutputStreamDevice& device, PdfWriteFlags writeMode,
    const PdfStatefulEncrypt& encrypt, charbuff& buffer) const
{
    switch (m_DataType)
    {
        case PdfDataType::Bool:
        {
            if ((writeMode & PdfWriteFlags::NoInlineLiteral) == PdfWriteFlags::None)
                device.Write(' '); // Write space before true or false

            if (m_Data.Bool)
                device.Write("true");
            else
                device.Write("false");
            break;
        }
        case PdfDataType::Number:
        {
            if ((writeMode & PdfWriteFlags::NoInlineLiteral) == PdfWriteFlags::None)
                device.Write(' '); // Write space before numbers

            utls::FormatTo(buffer, "{}", m_Data.Number);
            device.Write(buffer);
            break;
        }
        case PdfDataType::Real:
        {
            if ((writeMode & PdfWriteFlags::NoInlineLiteral) == PdfWriteFlags::None)
                device.Write(' '); // Write space before numbers

            utls::FormatTo(buffer, m_Data.Real, DefaultPrecision);
            device.Write(buffer);
            break;
        }
        case PdfDataType::Reference:
            m_Data.Reference.Write(device, writeMode, buffer);
            break;
        case PdfDataType::String:
        case PdfDataType::Name:
        case PdfDataType::Array:
        case PdfDataType::Dictionary:
        case PdfDataType::RawData:
            if (m_Data.Data == nullptr)
                PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

            m_Data.Data->Write(device, writeMode, encrypt, buffer);
            break;
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

string PdfVariant::ToString() const
{
    string ret;
    ToString(ret);
    return ret;
}

void PdfVariant::ToString(string& str) const
{
    str.clear();
    PdfWriteFlags writeFlags;
    switch (m_DataType)
    {
        case PdfDataType::Null:
        case PdfDataType::Bool:
        case PdfDataType::Number:
        case PdfDataType::Real:
        case PdfDataType::Reference:
            // By default we want the literals to not be spaced
            writeFlags = PdfWriteFlags::NoInlineLiteral;
            break;
        default:
            writeFlags = PdfWriteFlags::None;
            break;
    }

    charbuff buffer;
    StringStreamDevice device(str);
    this->Write(device, writeFlags, PdfStatefulEncrypt(), buffer);
}

PdfVariant& PdfVariant::operator=(const PdfVariant& rhs)
{
    clear();
    assign(rhs);
    return *this;
}

PdfVariant& PdfVariant::operator=(PdfVariant&& rhs) noexcept
{
    clear();
    m_DataType = rhs.m_DataType;
    m_Data = rhs.m_Data;
    rhs.m_DataType = PdfDataType::Null;
    rhs.m_Data = { };
    return *this;
}

void PdfVariant::assign(const PdfVariant& rhs)
{
    m_DataType = rhs.m_DataType;
    switch (m_DataType)
    {
        case PdfDataType::Array:
        {
            m_Data.Data = new PdfArray(*static_cast<const PdfArray*>(rhs.m_Data.Data));
            break;
        }
        case PdfDataType::Dictionary:
        {
            m_Data.Data = new PdfDictionary(*static_cast<const PdfDictionary*>(rhs.m_Data.Data));
            break;
        }
        case PdfDataType::Name:
        {
            m_Data.Data = new PdfName(*static_cast<const PdfName*>(rhs.m_Data.Data));
            break;
        }
        case PdfDataType::String:
        {
            m_Data.Data = new PdfString(*static_cast<const PdfString*>(rhs.m_Data.Data));
            break;
        }

        case PdfDataType::RawData:
        {
            m_Data.Data = new PdfData((*static_cast<const PdfData*>(rhs.m_Data.Data)));
            break;
        }
        case PdfDataType::Reference:
        case PdfDataType::Bool:
        case PdfDataType::Null:
        case PdfDataType::Number:
        case PdfDataType::Real:
            m_Data = rhs.m_Data;
            break;

        case PdfDataType::Unknown:
        default:
            break;
    };
}

const char* PdfVariant::GetDataTypeString() const
{
    switch (m_DataType)
    {
        case PdfDataType::Bool:
            return "Bool";
        case PdfDataType::Number:
            return "Number";
        case PdfDataType::Real:
            return "Real";
        case PdfDataType::String:
            return "String";
        case PdfDataType::Name:
            return "Name";
        case PdfDataType::Array:
            return "Array";
        case PdfDataType::Dictionary:
            return "Dictionary";
        case PdfDataType::Null:
            return "Null";
        case PdfDataType::Reference:
            return "Reference";
        case PdfDataType::RawData:
            return "RawData";
        case PdfDataType::Unknown:
            return "Unknown";
        default:
            return "INVALID_TYPE_ENUM";
    }
}

bool PdfVariant::operator==(const PdfVariant& rhs) const
{
    if (this == &rhs)
        return true;

    switch (m_DataType)
    {
        case PdfDataType::Bool:
        {
            bool value;
            if (rhs.TryGetBool(value))
                return m_Data.Bool == value;
            else
                return false;
        }
        case PdfDataType::Number:
        {
            int64_t value;
            if (rhs.TryGetNumber(value))
                return m_Data.Number == value;
            else
                return false;
        }
        case PdfDataType::Real:
        {
            // NOTE: Real type equality semantics is strict
            double value;
            if (rhs.TryGetRealStrict(value))
                return m_Data.Real == value;
            else
                return false;
        }
        case PdfDataType::Reference:
        {
            PdfReference value;
            if (rhs.TryGetReference(value))
                return m_Data.Reference == value;
            else
                return false;
        }
        case PdfDataType::String:
        {
            const PdfString* value;
            if (rhs.tryGetString(value))
                return *(PdfString*)m_Data.Data == *value;
            else
                return false;
        }
        case PdfDataType::Name:
        {
            const PdfName* value;
            if (rhs.tryGetName(value))
                return *(PdfName*)m_Data.Data == *value;
            else
                return false;
        }
        case PdfDataType::Array:
        {
            const PdfArray* value;
            if (rhs.TryGetArray(value))
                return *(PdfArray*)m_Data.Data == *value;
            else
                return false;
        }
        case PdfDataType::Dictionary:
        {
            const PdfDictionary* value;
            if (rhs.TryGetDictionary(value))
                return *(PdfDictionary*)m_Data.Data == *value;
            else
                return false;
        }
        case PdfDataType::RawData:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Equality not yet implemented for RawData");
        case PdfDataType::Null:
            return m_DataType == PdfDataType::Null;
        case PdfDataType::Unknown:
            return false;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
    }
}

bool PdfVariant::operator!=(const PdfVariant& rhs) const
{
    if (this != &rhs)
        return true;

    switch (m_DataType)
    {
        case PdfDataType::Bool:
        {
            bool value;
            if (rhs.TryGetBool(value))
                return m_Data.Bool != value;
            else
                return true;
        }
        case PdfDataType::Number:
        {
            int64_t value;
            if (rhs.TryGetNumber(value))
                return m_Data.Number != value;
            else
                return true;
        }
        case PdfDataType::Real:
        {
            // NOTE: Real type equality semantics is strict
            double value;
            if (rhs.TryGetRealStrict(value))
                return m_Data.Real != value;
            else
                return true;
        }
        case PdfDataType::Reference:
        {
            PdfReference value;
            if (rhs.TryGetReference(value))
                return m_Data.Reference != value;
            else
                return true;
        }
        case PdfDataType::String:
        {
            const PdfString* value;
            if (rhs.tryGetString(value))
                return *(PdfString*)m_Data.Data != *value;
            else
                return true;
        }
        case PdfDataType::Name:
        {
            const PdfName* value;
            if (rhs.tryGetName(value))
                return *(PdfName*)m_Data.Data != *value;
            else
                return true;
        }
        case PdfDataType::Array:
        {
            const PdfArray* value;
            if (rhs.TryGetArray(value))
                return *(PdfArray*)m_Data.Data != *value;
            else
                return true;
        }
        case PdfDataType::Dictionary:
        {
            const PdfDictionary* value;
            if (rhs.TryGetDictionary(value))
                return *(PdfDictionary*)m_Data.Data != *value;
            else
                return true;
        }
        case PdfDataType::RawData:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Disequality not yet implemented for RawData");
        case PdfDataType::Null:
            return m_DataType != PdfDataType::Null;
        case PdfDataType::Unknown:
            return true;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
    }
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
    if (m_DataType != PdfDataType::Bool)
    {
        value = false;
        return false;
    }

    value = m_Data.Bool;
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
    if (!(m_DataType == PdfDataType::Number
        || m_DataType == PdfDataType::Real))
    {
        value = 0;
        return false;
    }

    if (m_DataType == PdfDataType::Real)
        value = static_cast<int64_t>(std::round(m_Data.Real));
    else
        value = m_Data.Number;

    return true;
}

int64_t PdfVariant::GetNumber() const
{
    int64_t ret;
    if (!TryGetNumber(ret))
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return m_Data.Number;
}

bool PdfVariant::TryGetNumber(int64_t& value) const
{
    if (m_DataType != PdfDataType::Number)
    {
        value = 0;
        return false;
    }

    value = m_Data.Number;
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
    if (!(m_DataType == PdfDataType::Real
        || m_DataType == PdfDataType::Number))
    {
        value = 0;
        return false;
    }

    if (m_DataType == PdfDataType::Number)
        value = static_cast<double>(m_Data.Number);
    else
        value = m_Data.Real;

    return true;
}

double PdfVariant::GetRealStrict() const
{
    double ret;
    if (!TryGetRealStrict(ret))
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    return m_Data.Real;
}

bool PdfVariant::TryGetRealStrict(double& value) const
{
    if (m_DataType != PdfDataType::Real)
    {
        value = 0;
        return false;
    }

    value = m_Data.Real;
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
    if (m_DataType != PdfDataType::String)
    {
        str = nullptr;
        return false;
    }

    str = (PdfString*)m_Data.Data;
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
    if (m_DataType != PdfDataType::Name)
    {
        name = nullptr;
        return false;
    }

    name = (PdfName*)m_Data.Data;
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
    if (m_DataType != PdfDataType::Reference)
    {
        ref = PdfReference();
        return false;
    }

    ref = m_Data.Reference;
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
    if (m_DataType != PdfDataType::Dictionary)
    {
        dict = nullptr;
        return false;
    }

    dict = (PdfDictionary*)m_Data.Data;
    return true;
}

bool PdfVariant::tryGetArray(PdfArray*& arr) const
{
    if (m_DataType != PdfDataType::Array)
    {
        arr = nullptr;
        return false;
    }

    arr = (PdfArray*)m_Data.Data;
    return true;
}

void PdfVariant::SetBool(bool value)
{
    if (m_DataType != PdfDataType::Bool)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    m_Data.Bool = value;
}

void PdfVariant::SetNumber(int64_t value)
{
    if (!(m_DataType == PdfDataType::Number
        || m_DataType == PdfDataType::Real))
    {
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);
    }

    if (m_DataType == PdfDataType::Real)
        m_Data.Real = static_cast<double>(value);
    else
        m_Data.Number = value;
}

void PdfVariant::SetReal(double value)
{
    if (!(m_DataType == PdfDataType::Real
        || m_DataType == PdfDataType::Number))
    {
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);
    }

    if (m_DataType == PdfDataType::Number)
        m_Data.Number = static_cast<int64_t>(std::round(value));
    else
        m_Data.Real = value;
}

void PdfVariant::SetName(const PdfName& name)
{
    if (m_DataType != PdfDataType::Name)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    *((PdfName*)m_Data.Data) = name;
}

void PdfVariant::SetString(const PdfString& str)
{
    if (m_DataType != PdfDataType::String)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    *((PdfString*)m_Data.Data) = str;
}

void PdfVariant::SetReference(const PdfReference& ref)
{
    if (m_DataType != PdfDataType::Reference)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    m_Data.Reference = ref;
}

bool PdfVariant::IsBool() const
{
    return m_DataType == PdfDataType::Bool;
}

bool PdfVariant::IsNumber() const
{
    return m_DataType == PdfDataType::Number;
}

bool PdfVariant::IsRealStrict() const
{
    return m_DataType == PdfDataType::Real;
}

bool PdfVariant::IsNumberOrReal() const
{
    return m_DataType == PdfDataType::Number || m_DataType == PdfDataType::Real;
}

bool PdfVariant::IsString() const
{
    return m_DataType == PdfDataType::String;
}

bool PdfVariant::IsName() const
{
    return m_DataType == PdfDataType::Name;
}

bool PdfVariant::IsArray() const
{
    return m_DataType == PdfDataType::Array;
}

bool PdfVariant::IsDictionary() const
{
    return m_DataType == PdfDataType::Dictionary;
}

bool PdfVariant::IsRawData() const
{
    return m_DataType == PdfDataType::RawData;
}

bool PdfVariant::IsNull() const
{
    return m_DataType == PdfDataType::Null;
}

bool PdfVariant::IsReference() const
{
    return m_DataType == PdfDataType::Reference;
}
