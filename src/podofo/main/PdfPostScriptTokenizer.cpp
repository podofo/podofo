/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPostScriptTokenizer.h"

using namespace std;
using namespace PoDoFo;

PdfPostScriptTokenizer::PdfPostScriptTokenizer()
    : PdfTokenizer(false) { }

PdfPostScriptTokenizer::PdfPostScriptTokenizer(const shared_ptr<charbuff>& buffer)
    : PdfTokenizer(buffer, false) { }

void PdfPostScriptTokenizer::ReadNextVariant(InputStreamDevice& device, PdfVariant& variant)
{
    if (!PdfTokenizer::TryReadNextVariant(device, variant, { }))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected variant");
}

bool PdfPostScriptTokenizer::TryReadNextVariant(InputStreamDevice& device, PdfVariant& variant)
{
    return PdfTokenizer::TryReadNextVariant(device, variant, { });
}

bool PdfPostScriptTokenizer::TryReadNext(InputStreamDevice& device, PdfPostScriptTokenType& psTokenType, string_view& keyword, PdfVariant& variant)
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

    PdfLiteralDataType dataType = DetermineDataType(device, token, tokenType, variant);

    // asume we read a variant unless we discover otherwise later.
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
            this->ReadDictionary(device, variant, { });
            break;
        case PdfLiteralDataType::Array:
            this->ReadArray(device, variant, { });
            break;
        case PdfLiteralDataType::String:
            this->ReadString(device, variant, { });
            break;
        case PdfLiteralDataType::HexString:
            this->ReadHexString(device, variant, { });
            break;
        case PdfLiteralDataType::Name:
            this->ReadName(device, variant);
            break;
        case PdfLiteralDataType::Reference:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported reference datatype at this context");
        default:
            // Assume we have a keyword
            keyword = token;
            psTokenType = PdfPostScriptTokenType::Keyword;
            break;
    }

    return true;
}
