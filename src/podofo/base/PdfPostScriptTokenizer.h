/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_POSTSCRIPT_TOKENIZER_H
#define PDF_POSTSCRIPT_TOKENIZER_H

#include "PdfTokenizer.h"
#include "PdfVariant.h"
#include "PdfInputDevice.h"

namespace PoDoFo {

class PdfDocument;
class PdfCanvas;
class PdfObject;


/** An enum describing the type of a read token
 */
enum class PdfPostScriptTokenType
{
    Unknown = 0,
    Keyword, ///< The token is a PDF keyword.
    Variant, ///< The token is a PDF variant. A variant is usually a parameter to a keyword
    ProcedureEnter, ///< Procedure enter delimiter
    ProcedureExit, ///< Procedure enter delimiter
};

/** This class is a parser for general PostScript content in PDF documents.
 */
class PODOFO_API PdfPostScriptTokenizer final : private PdfTokenizer
{
public:
    PdfPostScriptTokenizer();
    PdfPostScriptTokenizer(const std::shared_ptr<charbuff>& buffer);
public:
    bool TryReadNext(InputStreamDevice& device, PdfPostScriptTokenType& tokenType, std::string_view& keyword, PdfVariant& variant);
    void ReadNextVariant(InputStreamDevice& device, PdfVariant& variant);
    bool TryReadNextVariant(InputStreamDevice& device, PdfVariant& variant);
};

};

#endif // PDF_POSTSCRIPT_TOKENIZER_H
