// SPDX-FileCopyrightText: 2021 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPostScriptTokenizer.h"

using namespace std;
using namespace PoDoFo;

static PdfTokenizerParams getPostScriptParams(PdfPostScriptLanguageLevel level);

PdfPostScriptTokenizer::PdfPostScriptTokenizer()
{
    // Enforce SkipReferences for PostScript tokenizer, as PostScript has no indirect references
    m_Params.Flags |= PdfTokenizerFlags::SkipReferences;
}

PdfPostScriptTokenizer::PdfPostScriptTokenizer(std::shared_ptr<charbuff> buffer)
    : PdfTokenizer(std::in_place, std::move(buffer))
{
    // Enforce SkipReferences for PostScript tokenizer, as PostScript has no indirect references
    m_Params.Flags |= PdfTokenizerFlags::SkipReferences;
}

PdfPostScriptTokenizer::PdfPostScriptTokenizer(PdfPostScriptLanguageLevel level)
{
    m_Params = getPostScriptParams(level);
}

PdfPostScriptTokenizer::PdfPostScriptTokenizer(shared_ptr<charbuff> buffer, PdfPostScriptLanguageLevel level)
    : PdfTokenizer(std::in_place, std::move(buffer))
{
    m_Params = getPostScriptParams(level);
}

void PdfPostScriptTokenizer::ReadNextVariant(InputStreamDevice& device, PdfVariant& variant)
{
    PdfTokenizer::ReadNextVariant(device, variant, { });
}

bool PdfPostScriptTokenizer::TryReadNextVariant(InputStreamDevice& device, PdfVariant& variant)
{
    return PdfTokenizer::TryReadNextVariant(device, variant, { });
}

void PdfPostScriptTokenizer::ReadNext(InputStreamDevice& device, PdfPostScriptTokenType& tokenType, std::string_view& keyword, PdfVariant& variant)
{
    ParsingOptions opts{ true, (GetParameters().Flags & PdfTokenizerFlags::StrictParsing) != PdfTokenizerFlags::None };
    (void)readNext(device, tokenType, keyword, opts, variant);
}

bool PdfPostScriptTokenizer::TryReadNext(InputStreamDevice& device, PdfPostScriptTokenType& tokenType, string_view& keyword, PdfVariant& variant)
{
    ParsingOptions opts{ false, (GetParameters().Flags & PdfTokenizerFlags::StrictParsing) != PdfTokenizerFlags::None };
    return readNext(device, tokenType, keyword, opts, variant);
}

void PdfPostScriptTokenizer::SetParameters(const PdfTokenizerParams& params)
{
    m_Params = params;
    // Enforce SkipReferences for PostScript tokenizer, as PostScript has no indirect references
    m_Params.Flags |= PdfTokenizerFlags::SkipReferences;
}

bool PdfPostScriptTokenizer::readNext(InputStreamDevice& device, PdfPostScriptTokenType& psTokenType,
    string_view& keyword, ParsingOptions opts, PdfVariant& variant)
{
    PdfTokenType tokenType;
    string_view token;
    keyword = { };
    bool gotToken = PdfTokenizer::TryReadNextToken(device, token, tokenType);
    if (!gotToken)
    {
        psTokenType = PdfPostScriptTokenType::Unknown;
        return false;
    }

    // Try first to detect PS procedures delimiters
    switch (tokenType)
    {
        case PdfTokenType::BraceLeft:
            psTokenType = PdfPostScriptTokenType::ProcedureEnter;
            return true;
        case PdfTokenType::BraceRight:
            psTokenType = PdfPostScriptTokenType::ProcedureExit;
            return true;
        default:
            // Continue evaluating data type
            break;
    }

    PdfLiteralDataType dataType = DetermineDataType(device, token, tokenType, variant, opts);

    // assume we read a variant unless we discover otherwise later.
    psTokenType = PdfPostScriptTokenType::Variant;
    switch (dataType)
    {
        case PdfLiteralDataType::Null:
        case PdfLiteralDataType::Bool:
        case PdfLiteralDataType::Number:
        case PdfLiteralDataType::Real:
            // the data was already read into variant by the DetermineDataType function
            break;

        case PdfLiteralDataType::Dictionary:
            if (!this->ReadDictionary(device, variant, { }, opts))
                return false;
            break;
        case PdfLiteralDataType::Array:
            if (!this->ReadArray(device, variant, { }, opts))
                return false;
            break;
        case PdfLiteralDataType::String:
            if (!this->ReadString(device, variant, { }, opts))
                return false;
            break;
        case PdfLiteralDataType::HexString:
            if (!this->ReadHexString(device, variant, { }, opts))
                return false;
            break;
        case PdfLiteralDataType::Name:
            if (!this->ReadName(device, variant, opts))
                return false;
            break;
        case PdfLiteralDataType::Reference:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported reference datatype at this context");
        default:
            // Assume we have a keyword. Drain any token that
            // DetermineDataType re-enqueued for TryReadNextVariant
            // callers, otherwise we'd re-process it infinitely.
            this->Reset();
            keyword = token;
            psTokenType = PdfPostScriptTokenType::Keyword;
            break;
    }

    return true;
}

PdfTokenizerParams getPostScriptParams(PdfPostScriptLanguageLevel level)
{
    PdfTokenizerParams tokenizerParams;
    tokenizerParams.LanguageLevel = level;
    tokenizerParams.Flags = PdfTokenizerFlags::SkipReferences;
    return tokenizerParams;
}
