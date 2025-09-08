/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfParserObject.h"

#include <podofo/main/PdfArray.h>
#include <podofo/main/PdfDictionary.h>

#include "PdfFilterFactory.h"

namespace
{
    enum class EndStreamToken : uint8_t
    {
        Undetermined = 0,
        Endstream,
        Endobj,
    };
}

using namespace PoDoFo;
using namespace std;

static size_t determineStreamSize(InputStreamDevice& device, size_t streamOffset);
static bool readObjectStreamEnd(int ch, unsigned& cursoridx, EndStreamToken& endStreamWord);

PdfParserObject::PdfParserObject(PdfDocument& doc, const PdfReference& indirectReference, InputStreamDevice& device, ssize_t offset)
    : PdfParserObject(&doc, indirectReference, device, offset, false)
{
    if (!indirectReference.IsIndirect())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Indirect reference must be valid");
}

PdfParserObject::PdfParserObject(PdfDocument& doc, InputStreamDevice& device, ssize_t offset)
    : PdfParserObject(&doc, PdfReference(), device, offset, true)
{
}

PdfParserObject::PdfParserObject(InputStreamDevice& device,
        const PdfReference& indirectReference, ssize_t offset)
    : PdfParserObject(nullptr, indirectReference, device, offset, false) { }

PdfParserObject::PdfParserObject(InputStreamDevice& device, ssize_t offset)
    : PdfParserObject(nullptr, PdfReference(), device, offset, false) { }

PdfParserObject::PdfParserObject(PdfDocument* doc, const PdfReference& indirectReference,
        InputStreamDevice& device, ssize_t offset, bool isLegacyTrailer) :
    PdfObject(PdfVariant(), indirectReference, true),
    m_device(&device),
    m_Offset(offset < 0 ? device.GetPosition() : offset),
    m_StreamOffset(0),
    m_isLegacyTrailer(isLegacyTrailer),
    m_HasStream(false),
    m_IsRevised(false)
{
    // Parsed objects by definition are initially not dirty
    resetDirty();
    SetDocument(doc);

    // We rely heavily on the demand loading infrastructure whether or not
    // we *actually* delay loading.
    EnableDelayedLoading();
    EnableDelayedLoadingStream();
}


void PdfParserObject::ParseData()
{
    // It's really just a call to DelayedLoad
    DelayedLoad();
}

void PdfParserObject::ParseFull()
{
    DelayedLoad();
    ParseStream(false);
}

void PdfParserObject::ParseStreamDryRun()
{
    PODOFO_ASSERT(IsDelayedLoadDone());
    PODOFO_ASSERT(!IsDelayedLoadStreamDone());
    if (m_HasStream)
        parseStream(true, true);
}

void PdfParserObject::ParseStream(bool shallow)
{
    PODOFO_ASSERT(IsDelayedLoadDone());
    if (IsDelayedLoadStreamDone())
        return;

    if (m_HasStream)
        parseStream(shallow, false);

    MakeDelayedLoadingStreamDone();
}

void PdfParserObject::delayedLoad()
{
    PdfTokenizer tokenizer;
    m_device->Seek(m_Offset);
    if (!m_isLegacyTrailer)
        checkReference(tokenizer);

    ParseData(tokenizer);
}

void PdfParserObject::delayedLoadStream()
{
    PODOFO_ASSERT(getStream() == nullptr);

    // Note: we can't use HasStream() here because it'll call DelayedLoad()
    if (!m_HasStream)
        return;
    
    try
    {
        parseStream(false, false);
    }
    catch (PdfError& e)
    {
        PODOFO_PUSH_FRAME_INFO(e, "Unable to parse the stream for object {} {} R",
            GetIndirectReference().ObjectNumber(),
            GetIndirectReference().GenerationNumber());
        throw;
    }
}

bool PdfParserObject::removeStream()
{
    bool hasStream = m_HasStream;
    m_HasStream = false;
    m_StreamOffset = 0;
    return hasStream;
}

void PdfParserObject::SetRevised()
{
    m_IsRevised = true;
}

PdfReference PdfParserObject::ReadReference(PdfTokenizer& tokenizer)
{
    m_device->Seek(m_Offset);
    return readReference(tokenizer);
}

// Only called via the demand loading mechanism
// Be very careful to avoid recursive demand loads via PdfVariant
// or PdfObject method calls here.
void PdfParserObject::ParseData(PdfTokenizer& tokenizer)
{
    unique_ptr<PdfStatefulEncrypt> encrypt;
    if (m_Encrypt != nullptr)
        encrypt.reset(new PdfStatefulEncrypt(m_Encrypt->GetEncrypt(), m_Encrypt->GetContext(), GetIndirectReference()));

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
        tokenizer.ReadNextVariant(*m_device, token, tokenType, m_Variant, encrypt.get());

        if (!m_isLegacyTrailer)
        {
            gotToken = tokenizer.TryReadNextToken(*m_device, token);
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
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidObject, token);
            }
        }
    }
}

bool PdfParserObject::HasStreamToParse() const
{
    return m_HasStream;
}

// Only called during delayed loading. Must be careful to avoid
// triggering recursive delay loading due to use of accessors of
// PdfVariant or PdfObject.
void PdfParserObject::parseStream(bool shallow, bool dryRun)
{
    PODOFO_ASSERT(IsDelayedLoadDone());

    char ch;
    ssize_t size;
    if (shallow)
        size = (ssize_t)this->m_Variant.GetDictionaryUnsafe().GetKeyAsSafe<int64_t>("Length", -1);
    else
        size = (ssize_t)this->m_Variant.GetDictionaryUnsafe().FindKeyAsSafe<int64_t>("Length", -1);

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
            // but certain PDFs have additional whitespaces
            case ' ':
            case '\t':
            {
                (void)m_device->ReadChar();
                break;
            }
            // From PDF 32000:2008 7.3.8.1 General
            // "The keyword stream that follows the stream dictionary shall be
            // followed by an end-of-line marker consisting of either a CARRIAGE
            // RETURN and a LINE FEED or just a LINE FEED, and not by a CARRIAGE
            // RETURN alone". Still, all implementations drop a single carriage return
            // followed by a non-newline character, see the discussion in
            // https://github.com/qpdf/qpdf/discussions/1413
            case '\r':
            {
                (void)m_device->ReadChar();
                streamOffset = m_device->GetPosition();
                if (!m_device->Peek(ch))
                    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Unexpected EOF when reading stream");

                if (ch == '\n')
                    streamOffset++;

                goto ReadStream;
            }
            case '\n':
            {
                (void)m_device->ReadChar();
                streamOffset = m_device->GetPosition();
                goto ReadStream;
            }
            // Assume malformed PDF with no whitespaces after the stream keyword
            default:
            {
                streamOffset = m_device->GetPosition();
                goto ReadStream;
            }
        }
    }

ReadStream:
    // NOTE: Retrieve the first list before seeking, otherwise
    // the following operation may also adjust the position
    auto filters = PdfFilterFactory::CreateFilterList(*this);

    m_device->Seek(streamOffset);
    if (size < 0)
    {
        if (!shallow)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidStream, "Invalid stream length");

        size = (ssize_t)determineStreamSize(*m_device, streamOffset);
    }

    if (dryRun)
    {
        m_device->Seek(streamOffset + size);
    }
    else
    {
        // Set stream raw data without marking the object dirty
        // NOTE: /Metadata objects may be unencrypted even if the
        // whole document is encrypted
        const PdfName* type;
        if (m_Encrypt != nullptr && (m_Encrypt->GetEncrypt().IsMetadataEncrypted()
            || !this->m_Variant.GetDictionaryUnsafe().TryFindKeyAs("Type", type)
            || *type != "Metadata"))
        {
            auto input = m_Encrypt->GetEncrypt().CreateEncryptionInputStream(*m_device, static_cast<size_t>(size), m_Encrypt->GetContext(), GetIndirectReference());
            getOrCreateStream().InitData(*input, static_cast<ssize_t>(size), std::move(filters));
            // Release the encrypt object after loading the stream.
            // It's not needed for serialization here
            m_Encrypt = nullptr;
        }
        else
        {
            getOrCreateStream().InitData(*m_device, static_cast<ssize_t>(size), std::move(filters));
        }
    }
}

void PdfParserObject::checkReference(PdfTokenizer& tokenizer)
{
    auto reference = readReference(tokenizer);
    if (GetIndirectReference() != reference)
    {
        PoDoFo::LogMessage(PdfLogSeverity::Warning,
            "Found object with reference {} different than reported {} in XRef sections",
            reference.ToString(), GetIndirectReference().ToString());
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
        throw;
    }

    string_view token;
    if (!tokenizer.TryReadNextToken(*m_device, token) || token != "obj")
    {
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidObject, "Error while reading object {} {} R: Next token is not 'obj'",
            reference.ObjectNumber(), reference.GenerationNumber());
    }

    return reference;
}

bool PdfParserObject::TryUnload()
{
    if (!IsDelayedLoadDone() || m_IsRevised || m_device == nullptr)
        return false;

    m_Variant = PdfVariant();
    FreeStream();
    EnableDelayedLoading();
    EnableDelayedLoadingStream();
    return true;
}

size_t determineStreamSize(InputStreamDevice& device, size_t streamOffset)
{
    char ch;
    unsigned i = 0;
    EndStreamToken endStreamToken;
    while (true)
    {
        if (device.Read(ch))
        {
            if (readObjectStreamEnd(ch, i, endStreamToken))
                goto AdjustSize;
        }
        else
        {
            if (readObjectStreamEnd(-1, i, endStreamToken))
                goto AdjustSize;

            goto Fail;
        }
    }

AdjustSize:
    // NOTE: Ignore newline characters before end stream token. We assume
    // they will either being skipped/ignored by the stream filter
    switch (endStreamToken)
    {
        case EndStreamToken::Endstream:
            return device.GetPosition() - streamOffset - 10;
        case EndStreamToken::Endobj:
            return device.GetPosition() - streamOffset - 7;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unexpected flow");
    }

Fail:
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidStream, "Unable to determine size of the stream");
}

// Try to read "endstream" or "endobj", followed by a delimiter or EOF
bool readObjectStreamEnd(int ch, unsigned& cursoridx, EndStreamToken& endStreamToken)
{
    switch (cursoridx)
    {
        case 0:
            if (ch == 'e') goto Advance; break;
        case 1:
            if (ch == 'n') goto Advance; break;
        case 2:
            if (ch == 'd') goto Advance; break;
        case 3:
            if (ch == 's')
            {
                endStreamToken = EndStreamToken::Endstream;
                goto Advance;
            }
            else if (ch == 'o')
            {
                endStreamToken = EndStreamToken::Endobj;
                goto Advance;
            }

            break;
        case 4:
            switch (endStreamToken)
            {
                case EndStreamToken::Endstream:
                    if (ch == 't') goto Advance; break;
                case EndStreamToken::Endobj:
                    if (ch == 'b') goto Advance; break;
                default:
                    goto Unexpected;
            }

            break;
        case 5:
            switch (endStreamToken)
            {
                case EndStreamToken::Endstream:
                    if (ch == 'r') goto Advance; break;
                case EndStreamToken::Endobj:
                    if (ch == 'j') goto Advance; break;
                default:
                    goto Unexpected;
            }

            break;
        case 6:
            switch (endStreamToken)
            {
                case EndStreamToken::Endstream:
                    if (ch == 'e') goto Advance; break;
                case EndStreamToken::Endobj:
                    if (ch == -1 || PoDoFo::IsCharWhitespace((char)ch))
                        return true;

                    break;
                default:
                    goto Unexpected;
            }

            break;
        case 7:
            switch (endStreamToken)
            {
                case EndStreamToken::Endstream:
                    if (ch == 'a') goto Advance; break;
                default:
                    goto Unexpected;
            }

            break;
        case 8:
            switch (endStreamToken)
            {
                case EndStreamToken::Endstream:
                    if (ch == 'm') goto Advance; break;
                default:
                    goto Unexpected;
            }

            break;
        case 9:
            switch (endStreamToken)
            {
                case EndStreamToken::Endstream:
                    if (ch == -1 || PoDoFo::IsCharWhitespace((char)ch))
                        return true;

                    break;
                default:
                    goto Unexpected;
            }

            break;
        default:
            goto Unexpected;
    }

    // Reset cursors
    cursoridx = 0;
    endStreamToken = EndStreamToken::Undetermined;
    return false;

Advance:
    // Advance cursor
    cursoridx++;
    return false;

Unexpected:
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unexpected flow");
}
