/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfParserObject.h"

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfInputDevice.h"
#include "PdfInputStream.h"
#include "PdfParser.h"
#include "PdfObjectStream.h"
#include "PdfVariant.h"

using namespace PoDoFo;
using namespace std;

PdfParserObject::PdfParserObject(PdfDocument& doc, const PdfReference& indirectReference, InputStreamDevice& device, ssize_t offset)
    : PdfParserObject(&doc, indirectReference, device, offset)
{
    if (!indirectReference.IsIndirect())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Indirect reference must be valid");
}

PdfParserObject::PdfParserObject(PdfDocument& doc, InputStreamDevice& device, ssize_t offset)
    : PdfParserObject(&doc, PdfReference(), device, offset)
{
}

PdfParserObject::PdfParserObject(InputStreamDevice& device, ssize_t offset)
    : PdfParserObject(nullptr, PdfReference(), device, offset) { }

PdfParserObject::PdfParserObject(PdfDocument* doc, const PdfReference& indirectReference,
    InputStreamDevice& device, ssize_t offset) :
    PdfObject(PdfVariant(), indirectReference, true),
    m_device(&device),
    m_Encrypt(nullptr),
    m_IsTrailer(false),
    m_Offset(offset < 0 ? device.GetPosition() : offset),
    m_HasStream(false),
    m_StreamOffset(0)
{
    // Parsed objects by definition are initially not dirty
    resetDirty();
    SetDocument(doc);

    // We rely heavily on the demand loading infrastructure whether or not
    // we *actually* delay loading.
    EnableDelayedLoading();
    EnableDelayedLoadingStream();
}


void PdfParserObject::Parse()
{
    // It's really just a call to DelayedLoad
    DelayedLoad();
}

void PdfParserObject::ParseStream()
{
    // It's really just a call to DelayedLoad
    DelayedLoadStream();
}

void PdfParserObject::DelayedLoadImpl()
{
    PdfTokenizer tokenizer;
    m_device->Seek(m_Offset);
    if (!m_IsTrailer)
        checkReference(tokenizer);

    Parse(tokenizer);
}

void PdfParserObject::DelayedLoadStreamImpl()
{
    PODOFO_ASSERT(getStream() == nullptr);

    // Note: we can't use HasStream() here because it'll call DelayedLoad()
    if (HasStreamToParse())
    {
        try
        {
            parseStream();
        }
        catch (PdfError& e)
        {
            PODOFO_PUSH_FRAME_INFO(e, "Unable to parse the stream for object {} {} R",
                GetIndirectReference().ObjectNumber(),
                GetIndirectReference().GenerationNumber());
            throw;
        }
    }
}

PdfReference PdfParserObject::ReadReference(PdfTokenizer& tokenizer)
{
    m_device->Seek(m_Offset);
    return readReference(tokenizer);
}

// Only called via the demand loading mechanism
// Be very careful to avoid recursive demand loads via PdfVariant
// or PdfObject method calls here.
void PdfParserObject::Parse(PdfTokenizer& tokenizer)
{
    PdfStatefulEncrypt encrypt;
    if (m_Encrypt != nullptr)
        encrypt = PdfStatefulEncrypt(*m_Encrypt, GetIndirectReference());

    // Do not call ReadNextVariant directly,
    // but TryReadNextToken, to handle empty objects like:
    // 13 0 obj
    // endobj

    PdfTokenType tokenType;
    string_view token;
    bool gotToken = tokenizer.TryReadNextToken(*m_device, token, tokenType);
    if (!gotToken)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected variant");

    // Check if we have an empty object or data
    if (token != "endobj")
    {
        tokenizer.ReadNextVariant(*m_device, token, tokenType, m_Variant, encrypt);

        if (!m_IsTrailer)
        {
            bool gotToken = tokenizer.TryReadNextToken(*m_device, token);
            if (!gotToken)
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected 'endobj' or (if dict) 'stream', got EOF");

            if (token == "endobj")
            {
                // nothing to do, just validate that the PDF is correct
                // If it's a dictionary, it might have a stream, so check for that
            }
            else if (m_Variant.IsDictionary() && token == "stream")
            {
                m_HasStream = true;
                m_StreamOffset = m_device->GetPosition(); // NOTE: whitespace after "stream" handle in stream parser!
            }
            else
            {
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NoObject, token);
            }
        }
    }
}


// Only called during delayed loading. Must be careful to avoid
// triggering recursive delay loading due to use of accessors of
// PdfVariant or PdfObject.
void PdfParserObject::parseStream()
{
    PODOFO_ASSERT(IsDelayedLoadDone());

    int64_t size = -1;
    char ch;

    auto& lengthObj = this->m_Variant.GetDictionary().MustFindKey(PdfName::KeyLength);
    if (!lengthObj.TryGetNumber(size))
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidStreamLength);

    m_device->Seek(m_StreamOffset);

    size_t streamOffset;
    while (true)
    {
        if (!m_device->Peek(ch))
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Unexpected EOF when reading stream");

        switch (ch)
        {
            // Skip spaces between the stream keyword and the carriage return/line
            // feed or line feed. Actually, this is not required by PDF Reference,
            // but certain PDFs have additionals whitespaces
            case ' ':
            case '\t':
                (void)m_device->ReadChar();
                break;
            // From PDF 32000:2008 7.3.8.1 General
            // "The keyword stream that follows the stream dictionary shall be
            // followed by an end-of-line marker consisting of either a CARRIAGE
            // RETURN and a LINE FEED or just a LINE FEED, and not by a CARRIAGE
            // RETURN alone"
            case '\r':
                streamOffset = m_device->GetPosition();
                (void)m_device->ReadChar();
                if (!m_device->Peek(ch))
                    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Unexpected EOF when reading stream");

                if (ch == '\n')
                {
                    (void)m_device->ReadChar();
                    streamOffset = m_device->GetPosition();
                }
                goto ReadStream;
            case '\n':
                (void)m_device->ReadChar();
                streamOffset = m_device->GetPosition();
                goto ReadStream;
            // Assume malformed PDF with no whitespaces after the stream keyword
            default:
                streamOffset = m_device->GetPosition();
                goto ReadStream;
        }
    }

ReadStream:
    m_device->Seek(streamOffset);	// reset it before reading!
    if (m_Encrypt != nullptr && !m_Encrypt->IsMetadataEncrypted())
    {
        // If metadata is not encrypted the Filter is set to "Crypt"
        auto filterObj = this->m_Variant.GetDictionary().FindKey(PdfName::KeyFilter);
        if (filterObj != nullptr && filterObj->IsArray())
        {
            auto& filters = filterObj->GetArray();
            for (unsigned i = 0; i < filters.GetSize(); i++)
            {
                auto& obj = filters.MustFindAt(i);
                if (obj.IsName() && obj.GetName() == "Crypt")
                    m_Encrypt = nullptr;
            }
        }
    }

    // Set stream raw data without marking the object dirty
    if (m_Encrypt != nullptr)
    {
        auto input = m_Encrypt->CreateEncryptionInputStream(*m_device, static_cast<size_t>(size), GetIndirectReference());
        getOrCreateStream().InitData(*input, static_cast<ssize_t>(size), PdfFilterFactory::CreateFilterList(*this));
    }
    else
    {
        getOrCreateStream().InitData(*m_device, static_cast<ssize_t>(size), PdfFilterFactory::CreateFilterList(*this));
    }
}

void PdfParserObject::checkReference(PdfTokenizer& tokenizer)
{
    auto reference = readReference(tokenizer);
    if (GetIndirectReference() != reference)
    {
        PoDoFo::LogMessage(PdfLogSeverity::Warning,
            "Found object with reference {} different than reported {} in XRef sections",
            GetIndirectReference().ToString(), reference.ToString());
    }
}

PdfReference PdfParserObject::readReference(PdfTokenizer& tokenizer)
{
    PdfReference reference;
    try
    {
        int64_t obj = tokenizer.ReadNextNumber(*m_device);
        int64_t gen = tokenizer.ReadNextNumber(*m_device);
        reference = PdfReference(static_cast<uint32_t>(obj), static_cast<uint16_t>(gen));

    }
    catch (PdfError& e)
    {
        PODOFO_PUSH_FRAME_INFO(e, "Object and generation number cannot be read");
        throw e;
    }

    if (!tokenizer.IsNextToken(*m_device, "obj"))
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NoObject, "Error while reading object {} {} R: Next token is not 'obj'",
            reference.ObjectNumber(), reference.GenerationNumber());
    }

    return reference;
}

void PdfParserObject::FreeObjectMemory(bool force)
{
    if (!this->IsDirty() || force)
    {
        if (IsDelayedLoadDone())
            m_Variant = PdfVariant();

        FreeStream();
        EnableDelayedLoading();
        EnableDelayedLoadingStream();
    }
}
