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

PdfError::PdfError(PdfErrorCode code, string filepath, unsigned line,
    string information)
{
    m_Code = code;
    this->AddToCallStack(std::move(filepath), line, std::move(information));
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
        case PdfErrorCode::IOError:
            return "PdfErrorCode::IOError"sv;
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
        case PdfErrorCode::ObjectNotFound:
            return "PdfErrorCode::ObjectNotFound"sv;
        case PdfErrorCode::MaxRecursionReached:
            return "PdfErrorCode::MaxRecursionReached"sv;
        case PdfErrorCode::BrokenFile:
            return "PdfErrorCode::BrokenFile"sv;
        case PdfErrorCode::InvalidPDF:
            return "PdfErrorCode::InvalidPDF"sv;
        case PdfErrorCode::InvalidXRef:
            return "PdfErrorCode::InvalidXRef"sv;
        case PdfErrorCode::InvalidTrailer:
            return "PdfErrorCode::InvalidTrailer"sv;
        case PdfErrorCode::InvalidNumber:
            return "PdfErrorCode::InvalidNumber"sv;
        case PdfErrorCode::InvalidEncoding:
            return "PdfErrorCode::InvalidEncoding"sv;
        case PdfErrorCode::InvalidObject:
            return "PdfErrorCode::InvalidObject"sv;
        case PdfErrorCode::InvalidEOFToken:
            return "PdfErrorCode::InvalidEOFToken"sv;
        case PdfErrorCode::InvalidDataType:
            return "PdfErrorCode::InvalidDataType"sv;
        case PdfErrorCode::InvalidXRefStream:
            return "PdfErrorCode::InvalidXRefStream"sv;
        case PdfErrorCode::InvalidPredictor:
            return "PdfErrorCode::InvalidPredictor"sv;
        case PdfErrorCode::InvalidStrokeStyle:
            return "PdfErrorCode::InvalidStrokeStyle"sv;
        case PdfErrorCode::InvalidStream:
            return "PdfErrorCode::InvalidStream"sv;
        case PdfErrorCode::InvalidKey:
            return "PdfErrorCode::InvalidKey"sv;
        case PdfErrorCode::InvalidName:
            return "PdfErrorCode::InvalidName"sv;
        case PdfErrorCode::InvalidEncryptionDict:
            return "PdfErrorCode::InvalidEncryptionDict"sv;
        case PdfErrorCode::InvalidPassword:
            return "PdfErrorCode::InvalidPassword"sv;
        case PdfErrorCode::InvalidFontData:
            return "PdfErrorCode::InvalidFontData"sv;
        case PdfErrorCode::InvalidContentStream:
            return "PdfErrorCode::InvalidContentStream"sv;
        case PdfErrorCode::InvalidInput:
            return "PdfErrorCode::InvalidInput"sv;
        case PdfErrorCode::UnsupportedFilter:
            return "PdfErrorCode::UnsupportedFilter"sv;
        case PdfErrorCode::UnsupportedFontFormat:
            return "PdfErrorCode::UnsupportedFontFormat"sv;
        case PdfErrorCode::WrongDestinationType:
            return "PdfErrorCode::WrongDestinationType"sv;
        case PdfErrorCode::FlateError:
            return "PdfErrorCode::FlateError"sv;
        case PdfErrorCode::FreeTypeError:
            return "PdfErrorCode::FreeTypeError"sv;
        case PdfErrorCode::UnsupportedPixelFormat:
            return "PdfErrorCode::UnsupportedPixelFormat"sv;
        case PdfErrorCode::UnsupportedImageFormat:
            return "PdfErrorCode::UnsupportedImageFormat"sv;
        case PdfErrorCode::CannotConvertColor:
            return "PdfErrorCode::CannotConvertColor"sv;
        case PdfErrorCode::NotImplemented:
            return "PdfErrorCode::NotImplemented"sv;
        case PdfErrorCode::ChangeOnImmutable:
            return "PdfErrorCode::ChangeOnImmutable"sv;
        case PdfErrorCode::ItemAlreadyPresent:
            return "PdfErrorCode::ItemAlreadyPresent"sv;
        case PdfErrorCode::OpenSSLError:
            return "PdfErrorCode::OpenSSLError"sv;
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
            return "Unexpected null pointer or invalid state."sv;
        case PdfErrorCode::FileNotFound:
            return "The specified file was not found."sv;
        case PdfErrorCode::IOError:
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
        case PdfErrorCode::ObjectNotFound:
            return "An object was requested but was not found."sv;
        case PdfErrorCode::MaxRecursionReached:
            return "Reached maximum recursion depth."sv;
        case PdfErrorCode::BrokenFile:
            return "The file content is broken."sv;
        case PdfErrorCode::InvalidPDF:
            return "This is not a PDF file."sv;
        case PdfErrorCode::InvalidXRef:
            return "No XRef table was found in the PDF file."sv;
        case PdfErrorCode::InvalidTrailer:
            return "No trailer was found in the PDF file."sv;
        case PdfErrorCode::InvalidNumber:
            return "A number was expected but not found."sv;
        case PdfErrorCode::InvalidObject:
            return "A object was expected but not found."sv;
        case PdfErrorCode::InvalidEncoding:
            return "Invalid encoding information."sv;
        case PdfErrorCode::InvalidEOFToken:
            return "No EOF Marker was found in the PDF file."sv;
        case PdfErrorCode::InvalidDataType:
        case PdfErrorCode::InvalidXRefStream:
        case PdfErrorCode::InvalidPredictor:
        case PdfErrorCode::InvalidStrokeStyle:
        case PdfErrorCode::InvalidStream:
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
        case PdfErrorCode::InvalidInput:
            return "The supplied input value is incorrect/unsupported."sv;
        case PdfErrorCode::UnsupportedFilter:
            break;
        case PdfErrorCode::UnsupportedFontFormat:
            return "This font format is not supported by PoDoFo."sv;
        case PdfErrorCode::WrongDestinationType:
            return "The requested field is not available for the given destination type"sv;
        case PdfErrorCode::FlateError:
            return "ZLib returned an error."sv;
        case PdfErrorCode::FreeTypeError:
            return "FreeType returned an error."sv;
        case PdfErrorCode::UnsupportedPixelFormat:
            return "This pixel format is not supported by PoDoFo."sv;
        case PdfErrorCode::UnsupportedImageFormat:
            return "This image format is not supported by PoDoFo."sv;
        case PdfErrorCode::CannotConvertColor:
            return "This color format cannot be converted."sv;
        case PdfErrorCode::ChangeOnImmutable:
            return "Changing values on immutable objects is not allowed."sv;
        case PdfErrorCode::NotImplemented:
            return "This feature is currently not implemented."sv;
        case PdfErrorCode::ItemAlreadyPresent:
            return "An item to be inserted is already in this container."sv;
        case PdfErrorCode::XmpMetadataError:
            return "Error while reading or writing XMP metadata"sv;
        case PdfErrorCode::OpenSSLError:
            return "OpenSSL error"sv;
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
    stream << name;

    if (msg.length() != 0)
        stream << ", " << msg;

    if (m_CallStack.size() != 0)
        stream << endl << "Callstack:";

    unsigned i = 0;
    for (auto& info : m_CallStack)
    {
        if (i > 0)
            stream << endl;

        auto filepath = info.GetFilePath();
        if (filepath.empty())
        {
            if (!info.GetInformation().empty())
                stream << "t#" << i << ", Information: " << info.GetInformation();
        }
        else
        {
            stream << "t#" << i << " Error Source: " << filepath << '(' << info.GetLine() << ')';

            if (!info.GetInformation().empty())
                stream << ", Information: " << info.GetInformation();
        }

        i++;
    }

    m_FullDescription = stream.take_str();
}

void PdfError::AddToCallStack(string&& filepath, unsigned line, string&& information)
{
    m_CallStack.push_front(PdfErrorInfo(std::move(filepath), line, std::move(information)));
}

PdfErrorInfo::PdfErrorInfo(string filepath, unsigned line, string info)
    : m_Line(line), m_FilePath(std::move(filepath)), m_Info(std::move(info)) { }

string_view PdfErrorInfo::GetFilePath() const
{
    return ((string_view)m_FilePath).substr(s_PathOffset.Value);
}
