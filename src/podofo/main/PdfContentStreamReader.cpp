/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfContentStreamReader.h"

#include "PdfXObjectForm.h"
#include "PdfCanvasInputDevice.h"
#include "PdfData.h"
#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

PdfContentStreamReader::PdfContentStreamReader(const PdfCanvas& canvas,
        nullable<const PdfContentReaderArgs&> args) :
    PdfContentStreamReader(std::make_shared<PdfCanvasInputDevice>(canvas),
        &canvas, args) { }

PdfContentStreamReader::PdfContentStreamReader(const shared_ptr<InputStreamDevice>& device,
        nullable<const PdfContentReaderArgs&> args) :
    PdfContentStreamReader(device, nullptr, args) { }

PdfContentStreamReader::PdfContentStreamReader(const shared_ptr<InputStreamDevice>& device,
    const PdfCanvas* canvas, nullable<const PdfContentReaderArgs&> args) :
    m_args(args.has_value() ? *args : PdfContentReaderArgs()),
    m_buffer(std::make_shared<charbuff>(PdfTokenizer::BufferSize)),
    m_tokenizer(m_buffer),
    m_readingInlineImgData(false),
    m_temp{ }
{
    if (device == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Device must be non null");

    m_inputs.push_back({ nullptr, device, canvas });
}

bool PdfContentStreamReader::TryReadNext(PdfContent& content)
{
    beforeReadReset(content);

    while (true)
    {
        if (m_inputs.size() == 0)
            goto Eof;

        if (m_readingInlineImgData)
        {
            if (m_args.InlineImageHandler == nullptr)
            {
                if (!tryReadInlineImgData(content.InlineImageData))
                    goto PopDevice;

                content.Type = PdfContentType::ImageData;
                m_readingInlineImgData = false;
                afterReadClear(content);
                return true;
            }
            else
            {
                bool eof = !m_args.InlineImageHandler(content.InlineImageDictionary, *m_inputs.back().Device);
                m_readingInlineImgData = false;

                // Try to consume the EI end image operator
                if (eof || !tryReadNextContent(content))
                {
                    content.Warnings = PdfContentWarnings::MissingEndImage;
                    goto PopDevice;
                }

                if (content.Operator != PdfOperator::EI)
                {
                    content.Warnings = PdfContentWarnings::MissingEndImage;
                    goto HandleContent;
                }

                beforeReadReset(content);
            }
        }

        if (!tryReadNextContent(content))
            goto PopDevice;

    HandleContent:
        afterReadClear(content);
        handleWarnings();
        return true;

    PopDevice:
        PODOFO_INVARIANT(m_inputs.size() != 0);
        m_inputs.pop_back();
        if (m_inputs.size() == 0)
            goto Eof;

        // Unless the device stack is empty, popping a devices
        // means that we finished processing an XObject form
        content.Type = PdfContentType::EndFormXObject;
        if (content.Stack.GetSize() != 0)
            content.Warnings |= PdfContentWarnings::SpuriousStackContent;

        goto HandleContent;

    Eof:
        content.Type = PdfContentType::Unknown;
        afterReadClear(content);
        return false;
    }
}

// Returns false in case of EOF
bool PdfContentStreamReader::tryReadNextContent(PdfContent& content)
{
    while (true)
    {
        bool gotToken = m_tokenizer.TryReadNext(*m_inputs.back().Device, m_temp.PsType, content.Keyword, m_temp.Variant);
        if (!gotToken)
        {
            content.Type = PdfContentType::Unknown;
            return false;
        }

        switch (m_temp.PsType)
        {
            case PdfPostScriptTokenType::Keyword:
            {
                if (!TryConvertTo(content.Keyword, content.Operator))
                {
                    content.Type = PdfContentType::UnexpectedKeyword;
                    return true;
                }

                int operandCount = PoDoFo::GetOperandCount(content.Operator);
                if (operandCount != -1 && content.Stack.GetSize() != (unsigned)operandCount)
                {
                    if (content.Stack.GetSize() < (unsigned)operandCount)
                        content.Warnings |= PdfContentWarnings::InvalidOperator;
                    else // Stack.GetSize() > operandCount
                        content.Warnings |= PdfContentWarnings::SpuriousStackContent;
                }

                bool eof;
                if (!tryHandleOperator(content, eof))
                    content.Type = PdfContentType::Operator;

                return !eof;
            }
            case PdfPostScriptTokenType::Variant:
            {
                content.Stack.Push(std::move(m_temp.Variant));
                continue;
            }
            case PdfPostScriptTokenType::ProcedureEnter:
            case PdfPostScriptTokenType::ProcedureExit:
            {
                content.Type = PdfContentType::UnexpectedKeyword;
                return true;
            }
            default:
            {
                PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            }
        }
    }
}

void PdfContentStreamReader::beforeReadReset(PdfContent& content)
{
    content.Stack.Clear();
    content.Warnings = PdfContentWarnings::None;
}

void PdfContentStreamReader::afterReadClear(PdfContent& content)
{
    // Do some cleaning
    switch (content.Type)
    {
        case PdfContentType::Operator:
        {
            content.InlineImageDictionary.Clear();
            content.InlineImageData.clear();
            content.XObject = nullptr;
            content.Name = nullptr;
            break;
        }
        case PdfContentType::ImageDictionary:
        {
            content.Operator = PdfOperator::Unknown;
            content.Keyword = string_view();
            content.InlineImageData.clear();
            content.XObject = nullptr;
            content.Name = nullptr;
            break;
        }
        case PdfContentType::ImageData:
        {
            content.Operator = PdfOperator::Unknown;
            content.Keyword = string_view();
            content.InlineImageDictionary.Clear();
            content.XObject = nullptr;
            content.Name = nullptr;
            break;
        }
        case PdfContentType::DoXObject:
        case PdfContentType::BeginFormXObject:
        {
            content.Operator = PdfOperator::Unknown;
            content.Keyword = string_view();
            content.InlineImageDictionary.Clear();
            content.InlineImageData.clear();
            break;
        }
        case PdfContentType::UnexpectedKeyword:
        {
            content.Operator = PdfOperator::Unknown;
            content.InlineImageDictionary.Clear();
            content.InlineImageData.clear();
            content.XObject = nullptr;
            content.Name = nullptr;
            break;
        }
        case PdfContentType::Unknown:
        case PdfContentType::EndFormXObject:
        {
            // Used when it is reached the EOF
            content.Operator = PdfOperator::Unknown;
            content.Keyword = string_view();
            content.InlineImageDictionary.Clear();
            content.InlineImageData.clear();
            content.XObject = nullptr;
            content.Name = nullptr;
            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported flow");
        }
    }
}

// Try to handle the operator in a more specific way. Return false
// if the operator wasn't handled
bool PdfContentStreamReader::tryHandleOperator(PdfContent& content, bool& eof)
{
    // By default it's not handled
    eof = false;
    switch (content.Operator)
    {
        case PdfOperator::Do:
        {
            if (m_inputs.back().Canvas == nullptr
                || ((m_args.Flags & PdfContentReaderFlags::SkipHandleNonFormXObjects) != PdfContentReaderFlags::None
                    && (m_args.Flags & PdfContentReaderFlags::SkipFollowFormXObjects) != PdfContentReaderFlags::None))
            {
                // Don't try to handle XObject if there's no canvas or
                // if the reader is marked to not handle XObjects at all
                return false;
            }

            return tryHandleXObject(content);
        }
        case PdfOperator::BI:
        {
            if (!tryReadInlineImgDict(content))
            {
                eof = true;
                return false;
            }

            content.Type = PdfContentType::ImageDictionary;
            m_readingInlineImgData = true;
            return true;
        }
        default:
        {
            // Not handled operator
            return false;
        }
    }
}

// Returns false in case of EOF
bool PdfContentStreamReader::tryReadInlineImgDict(PdfContent& content)
{
    while (true)
    {
        if (!m_tokenizer.TryReadNext(*m_inputs.back().Device, m_temp.PsType, m_temp.Keyword, m_temp.Variant))
            return false;

        switch (m_temp.PsType)
        {
            case PdfPostScriptTokenType::Keyword:
            {
                // Try to find end of dictionary
                if (m_temp.Keyword == "ID")
                    return true;

                content.Warnings |= PdfContentWarnings::InvalidImageDictionaryContent;
                continue;
            }
            case PdfPostScriptTokenType::Variant:
            {
                if (m_temp.Variant.TryGetName(m_temp.Name))
                    break;

                content.Warnings |= PdfContentWarnings::InvalidImageDictionaryContent;
                continue;
            }
            default:
            {
                content.Warnings |= PdfContentWarnings::InvalidImageDictionaryContent;
                continue;
            }
        }

        if (m_tokenizer.TryReadNextVariant(*m_inputs.back().Device, m_temp.Variant))
            content.InlineImageDictionary.AddKey(m_temp.Name, std::move(m_temp.Variant));
        else
            return false;
    }
}

bool PdfContentStreamReader::tryHandleXObject(PdfContent& content)
{
    PODOFO_ASSERT(m_inputs.back().Canvas != nullptr);
    const PdfResources* resources;
    const PdfObject* xobjraw = nullptr;
    PdfXObjectType detectedType;
    bool followFormXObjecs = (m_args.Flags & PdfContentReaderFlags::SkipFollowFormXObjects) == PdfContentReaderFlags::None;
    bool handleXObjects = (m_args.Flags & PdfContentReaderFlags::SkipHandleNonFormXObjects) == PdfContentReaderFlags::None;
    if (content.Stack.GetSize() != 1
        || !content.Stack[0].TryGetName(content.Name)
        || (resources = m_inputs.back().Canvas->GetResources()) == nullptr
        || (xobjraw = resources->GetResource(PdfResourceType::XObject, *content.Name)) == nullptr)
    {
        goto InvalidXObj;
    }

    if (handleXObjects)
    {
        // Try to handle any XObject type
        content.XObject = PdfXObject::CreateFromObject(*xobjraw, PdfXObjectType::Unknown, detectedType);
        if (content.XObject == nullptr)
            goto InvalidXObj;
    }
    else
    {
        PODOFO_ASSERT(followFormXObjecs);

        // Limit handling to Form XObjects only
        content.XObject = PdfXObject::CreateFromObject(*xobjraw, PdfXObjectType::Form, detectedType);
        if (content.XObject == nullptr)
        {
            if (detectedType == PdfXObjectType::Unknown)
            {
                // Fallback on PdfContentType::DoXObject
                handleXObjects = true;
                goto InvalidXObj;
            }
            else
            {
                // It's not a Form XObject, but we won't handle it
                return false;
            }
        }
    }

    if (followFormXObjecs && content.XObject->GetType() == PdfXObjectType::Form)
    {
        // Select the Form XObject for next input source
        content.Type = PdfContentType::BeginFormXObject;

        if (isCalledRecursively(xobjraw))
        {
            content.Warnings |= PdfContentWarnings::RecursiveXObject;
            return true;
        }

        m_inputs.push_back({
            content.XObject,
            std::make_shared<PdfCanvasInputDevice>(static_cast<const PdfXObjectForm&>(*content.XObject)),
            dynamic_cast<const PdfCanvas*>(content.XObject.get()) });
    }
    else
    {
        // Generically signal a "Do" XObject operator
        content.Type = PdfContentType::DoXObject;
    }

    return true;

InvalidXObj:
    content.Warnings |= PdfContentWarnings::InvalidXObject;
    if (handleXObjects)
    {
        content.Type = PdfContentType::DoXObject;
        return true;
    }

    return false;
}

// Returns false in case of EOF
bool PdfContentStreamReader::tryReadInlineImgData(charbuff& data)
{
    // Consume one whitespace between ID and data
    char ch;
    if (!m_inputs.back().Device->Read(ch))
        return false;

    // Read "EI"
    enum class ReadEIStatus
    {
        ReadE,
        ReadI,
        ReadWhiteSpace
    };

    // NOTE: This is a better version of the previous approach
    // and still is wrong since the Pdf specification is broken
    // with this regard. The dictionary should have a /Length
    // key with the length of the data, and it's a requirement
    // in Pdf 2.0 specification (ISO 32000-2). To handle better
    // the situation the only approach would be to use more
    // comprehensive heuristic, similarly to what pdf.js does
    ReadEIStatus status = ReadEIStatus::ReadE;
    unsigned readCount = 0;
    while (m_inputs.back().Device->Read(ch))
    {
        switch (status)
        {
            case ReadEIStatus::ReadE:
            {
                if (ch == 'E')
                    status = ReadEIStatus::ReadI;

                break;
            }
            case ReadEIStatus::ReadI:
            {
                if (ch == 'I')
                    status = ReadEIStatus::ReadWhiteSpace;
                else
                    status = ReadEIStatus::ReadE;

                break;
            }
            case ReadEIStatus::ReadWhiteSpace:
            {
                if (PdfTokenizer::IsWhitespace(ch))
                {
                    data = bufferview(m_buffer->data(), readCount - 2);
                    return true;
                }
                else
                    status = ReadEIStatus::ReadE;

                break;
            }
        }

        if (m_buffer->size() == readCount)
        {
            // image is larger than buffer => resize buffer
            m_buffer->resize(m_buffer->size() * 2);
        }

        m_buffer->data()[readCount] = ch;
        readCount++;
    }

    return false;
}

void PdfContentStreamReader::handleWarnings()
{
    if ((m_args.Flags & PdfContentReaderFlags::ThrowOnWarnings) != PdfContentReaderFlags::None)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidContentStream, "Unsupported PostScript content");
}

bool PdfContentStreamReader::isCalledRecursively(const PdfObject* xobj)
{
    // Determines if the given object is called recursively
    for (auto& input : m_inputs)
    {
        if (input.Canvas->GetContentsObject() == xobj)
            return true;
    }

    return false;
}
