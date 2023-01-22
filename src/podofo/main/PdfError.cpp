/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// PdfError.h doesn't, and can't, include PdfDeclarations.h so we do so here.
// PdfDeclarationsPrivate.h will include PdfError.h for us.
#include <podofo/private/PdfDeclarationsPrivate.h>

#include <podofo/private/FileSystem.h>
#include <podofo/private/outstringstream.h>

using namespace std;
using namespace cmn;
using namespace PoDoFo;

// Retrieve the basepath of the source directory
struct SourcePathOffset
{
    SourcePathOffset()
        : Value(fs::u8path(__FILE__).parent_path().parent_path()
            .u8string().length() + 1) { }
    const size_t Value;
};

static SourcePathOffset s_PathOffset;

PdfErrorInfo::PdfErrorInfo(string filepath, unsigned line, string info)
    : m_Line(line), m_FilePath(std::move(filepath)), m_Info(std::move(info)) { }

PdfError::PdfError(PdfErrorCode code, string filepath, unsigned line,
    string information)
{
    m_Code = code;
    this->AddToCallStack(std::move(filepath), line, std::move(information));
}

PdfError& PdfError::operator=(const PdfErrorCode& code)
{
    m_Code = code;
    m_CallStack.clear();
    return *this;
}

bool PdfError::operator==(PdfErrorCode code)
{
    return m_Code == code;
}

bool PdfError::operator!=(PdfErrorCode code)
{
    return m_Code != code;
}

string_view PdfError::GetName() const
{
    return ErrorName(m_Code);
}

void PdfError::PrintErrorMsg() const
{
    const_cast<PdfError&>(*this).initFullDescription();
    PoDoFo::LogMessage(PdfLogSeverity::Error, m_FullDescription);
}

const char* PdfError::what() const noexcept
{
    const_cast<PdfError&>(*this).initFullDescription();
    return m_FullDescription.c_str();
}

string_view PdfError::ErrorName(PdfErrorCode code)
{
    switch (code)
    {
        case PdfErrorCode::InvalidHandle:
            return "PdfErrorCode::InvalidHandle"sv;
        case PdfErrorCode::FileNotFound:
            return "PdfErrorCode::FileNotFound"sv;
        case PdfErrorCode::InvalidDeviceOperation:
            return "PdfErrorCode::InvalidDeviceOperation"sv;
        case PdfErrorCode::UnexpectedEOF:
            return "PdfErrorCode::UnexpectedEOF"sv;
        case PdfErrorCode::OutOfMemory:
            return "PdfErrorCode::OutOfMemory"sv;
        case PdfErrorCode::ValueOutOfRange:
            return "PdfErrorCode::ValueOutOfRange"sv;
        case PdfErrorCode::InternalLogic:
            return "PdfErrorCode::InternalLogic"sv;
        case PdfErrorCode::InvalidEnumValue:
            return "PdfErrorCode::InvalidEnumValue"sv;
        case PdfErrorCode::BrokenFile:
            return "PdfErrorCode::BrokenFile"sv;
        case PdfErrorCode::PageNotFound:
            return "PdfErrorCode::PageNotFound"sv;
        case PdfErrorCode::NoPdfFile:
            return "PdfErrorCode::NoPdfFile"sv;
        case PdfErrorCode::NoXRef:
            return "PdfErrorCode::NoXRef"sv;
        case PdfErrorCode::NoTrailer:
            return "PdfErrorCode::NoTrailer"sv;
        case PdfErrorCode::NoNumber:
            return "PdfErrorCode::NoNumber"sv;
        case PdfErrorCode::NoObject:
            return "PdfErrorCode::NoObject"sv;
        case PdfErrorCode::NoEOFToken:
            return "PdfErrorCode::NoEOFToken"sv;
        case PdfErrorCode::InvalidTrailerSize:
            return "PdfErrorCode::InvalidTrailerSize"sv;
        case PdfErrorCode::InvalidDataType:
            return "PdfErrorCode::InvalidDataType"sv;
        case PdfErrorCode::InvalidXRef:
            return "PdfErrorCode::InvalidXRef"sv;
        case PdfErrorCode::InvalidXRefStream:
            return "PdfErrorCode::InvalidXRefStream"sv;
        case PdfErrorCode::InvalidXRefType:
            return "PdfErrorCode::InvalidXRefType"sv;
        case PdfErrorCode::InvalidPredictor:
            return "PdfErrorCode::InvalidPredictor"sv;
        case PdfErrorCode::InvalidStrokeStyle:
            return "PdfErrorCode::InvalidStrokeStyle"sv;
        case PdfErrorCode::InvalidHexString:
            return "PdfErrorCode::InvalidHexString"sv;
        case PdfErrorCode::InvalidStream:
            return "PdfErrorCode::InvalidStream"sv;
        case PdfErrorCode::InvalidStreamLength:
            return "PdfErrorCode::InvalidStream"sv;
        case PdfErrorCode::InvalidKey:
            return "PdfErrorCode::InvalidKey"sv;
        case PdfErrorCode::InvalidName:
            return "PdfErrorCode::InvalidName"sv;
        case PdfErrorCode::InvalidEncryptionDict:              ///< The encryption dictionary is invalid or misses a required key
            return "PdfErrorCode::InvalidEncryptionDict"sv;
        case PdfErrorCode::InvalidPassword:                    ///< The password used to open the PDF file was invalid
            return "PdfErrorCode::InvalidPassword"sv;
        case PdfErrorCode::InvalidFontData:
            return "PdfErrorCode::InvalidFontData"sv;
        case PdfErrorCode::InvalidContentStream:
            return "PdfErrorCode::InvalidContentStream"sv;
        case PdfErrorCode::UnsupportedFilter:
            return "PdfErrorCode::UnsupportedFilter"sv;
        case PdfErrorCode::UnsupportedFontFormat:               ///< This font format is not supported by PoDoFo.
            return "PdfErrorCode::UnsupportedFontFormat"sv;
        case PdfErrorCode::ActionAlreadyPresent:
            return "PdfErrorCode::ActionAlreadyPresent"sv;
        case PdfErrorCode::WrongDestinationType:
            return "PdfErrorCode::WrongDestinationType"sv;
        case PdfErrorCode::MissingEndStream:
            return "PdfErrorCode::MissingEndStream"sv;
        case PdfErrorCode::Date:
            return "PdfErrorCode::Date"sv;
        case PdfErrorCode::Flate:
            return "PdfErrorCode::Flate"sv;
        case PdfErrorCode::FreeType:
            return "PdfErrorCode::FreeType"sv;
        case PdfErrorCode::SignatureError:
            return "PdfErrorCode::SignatureError"sv;
        case PdfErrorCode::UnsupportedImageFormat:              ///< This image format is not supported by PoDoFo.
            return "PdfErrorCode::UnsupportedImageFormat"sv;
        case PdfErrorCode::CannotConvertColor:                  ///< This color format cannot be converted.
            return "PdfErrorCode::CannotConvertColor"sv;
        case PdfErrorCode::NotImplemented:
            return "PdfErrorCode::NotImplemented"sv;
        case PdfErrorCode::NotCompiled:
            return "PdfErrorCode::NotCompiled"sv;
        case PdfErrorCode::DestinationAlreadyPresent:
            return "PdfErrorCode::DestinationAlreadyPresent"sv;
        case PdfErrorCode::ChangeOnImmutable:
            return "PdfErrorCode::ChangeOnImmutable"sv;
        case PdfErrorCode::OutlineItemAlreadyPresent:
            return "PdfErrorCode::OutlineItemAlreadyPresent"sv;
        case PdfErrorCode::NotLoadedForUpdate:
            return "PdfErrorCode::NotLoadedForUpdate"sv;
        case PdfErrorCode::CannotEncryptedForUpdate:
            return "PdfErrorCode::CannotEncryptedForUpdate"sv;
        case PdfErrorCode::Unknown:
            return "PdfErrorCode::Unknown"sv;
        default:
            break;
    }

    return { };
}

string_view PdfError::ErrorMessage(PdfErrorCode code)
{
    switch (code)
    {
        case PdfErrorCode::InvalidHandle:
            return "An invalid handle was passed or returned, but initialized data was expected."sv;
        case PdfErrorCode::FileNotFound:
            return "The specified file was not found."sv;
        case PdfErrorCode::InvalidDeviceOperation:
            return "Tried to do something unsupported to an I/O device like seek a non-seekable input device"sv;
        case PdfErrorCode::UnexpectedEOF:
            return "End of file was reached unxexpectedly."sv;
        case PdfErrorCode::OutOfMemory:
            return "PoDoFo is out of memory."sv;
        case PdfErrorCode::ValueOutOfRange:
            return "The passed value is out of range."sv;
        case PdfErrorCode::InternalLogic:
            return "An internal error occurred."sv;
        case PdfErrorCode::InvalidEnumValue:
            return "An invalid enum value was specified."sv;
        case PdfErrorCode::BrokenFile:
            return "The file content is broken."sv;
        case PdfErrorCode::PageNotFound:
            return "The requested page could not be found in the PDF."sv;
        case PdfErrorCode::NoPdfFile:
            return "This is not a PDF file."sv;
        case PdfErrorCode::NoXRef:
            return "No XRef table was found in the PDF file."sv;
        case PdfErrorCode::NoTrailer:
            return "No trailer was found in the PDF file."sv;
        case PdfErrorCode::NoNumber:
            return "A number was expected but not found."sv;
        case PdfErrorCode::NoObject:
            return "A object was expected but not found."sv;
        case PdfErrorCode::NoEOFToken:
            return "No EOF Marker was found in the PDF file."sv;
        case PdfErrorCode::InvalidTrailerSize:
        case PdfErrorCode::InvalidDataType:
        case PdfErrorCode::InvalidXRef:
        case PdfErrorCode::InvalidXRefStream:
        case PdfErrorCode::InvalidXRefType:
        case PdfErrorCode::InvalidPredictor:
        case PdfErrorCode::InvalidStrokeStyle:
        case PdfErrorCode::InvalidHexString:
        case PdfErrorCode::InvalidStream:
        case PdfErrorCode::InvalidStreamLength:
        case PdfErrorCode::InvalidKey:
        case PdfErrorCode::InvalidName:
            break;
        case PdfErrorCode::InvalidEncryptionDict:
            return "The encryption dictionary is invalid or misses a required key."sv;
        case PdfErrorCode::InvalidPassword:
            return "The password used to open the PDF file was invalid."sv;
        case PdfErrorCode::InvalidFontData:
            return "The font data is invalid."sv;
        case PdfErrorCode::InvalidContentStream:
            return "The content stream is invalid due to mismatched context pairing or other problems."sv;
        case PdfErrorCode::UnsupportedFilter:
            break;
        case PdfErrorCode::UnsupportedFontFormat:
            return "This font format is not supported by PoDoFo."sv;
        case PdfErrorCode::DestinationAlreadyPresent:
        case PdfErrorCode::ActionAlreadyPresent:
            return "Outlines can have either destinations or actions."sv;
        case PdfErrorCode::WrongDestinationType:
            return "The requested field is not available for the given destination type"sv;
        case PdfErrorCode::MissingEndStream:
        case PdfErrorCode::Date:
            break;
        case PdfErrorCode::Flate:
            return "ZLib returned an error."sv;
        case PdfErrorCode::FreeType:
            return "FreeType returned an error."sv;
        case PdfErrorCode::SignatureError:
            return "The signature contains an error."sv;
        case PdfErrorCode::UnsupportedImageFormat:
            return "This image format is not supported by PoDoFo."sv;
        case PdfErrorCode::CannotConvertColor:
            return "This color format cannot be converted."sv;
        case PdfErrorCode::ChangeOnImmutable:
            return "Changing values on immutable objects is not allowed."sv;
        case PdfErrorCode::NotImplemented:
            return "This feature is currently not implemented."sv;
        case PdfErrorCode::NotCompiled:
            return "This feature was disabled during compile time."sv;
        case PdfErrorCode::OutlineItemAlreadyPresent:
            return "Given OutlineItem already present in destination tree."sv;
        case PdfErrorCode::NotLoadedForUpdate:
            return "The document had not been loaded for update."sv;
        case PdfErrorCode::CannotEncryptedForUpdate:
            return "Cannot load encrypted documents for update."sv;
        case PdfErrorCode::XmpMetadata:
            return "Error while reading or writing XMP metadata"sv;
        case PdfErrorCode::Unknown:
            return "Error code unknown."sv;
        default:
            break;
    }

    return { };
}

void PdfError::initFullDescription()
{
    if (m_FullDescription.length() != 0)
        return;

    auto msg = PdfError::ErrorMessage(m_Code);
    auto name = PdfError::ErrorName(m_Code);

    outstringstream stream;
    stream << endl << endl << "PoDoFo encountered an error. Error: " << (int)m_Code << name;

    if (msg.length() != 0)
        stream << "\tError Description: " << msg;

    if (m_CallStack.size() != 0)
        stream << "\tCallstack:";

    unsigned i = 0;
    for (auto& info : m_CallStack)
    {
        auto filepath = info.GetFilePath();
        if (!filepath.empty())
            stream << "\t#" << i << " Error Source : " << filepath << ": " << info.GetLine();

        if (!info.GetInformation().empty())
            stream << "\t\t" << "Information: " << info.GetInformation();

        stream << endl;
        i++;
    }

    m_FullDescription = stream.take_str();
}

void PdfError::AddToCallStack(string filepath, unsigned line, string information)
{
    m_CallStack.push_front(PdfErrorInfo(std::move(filepath), line, information));
}

string_view PdfErrorInfo::GetFilePath() const
{
    return ((string_view)m_FilePath).substr(s_PathOffset.Value);
}
