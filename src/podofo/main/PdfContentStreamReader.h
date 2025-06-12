/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_CONTENT_READER_H
#define PDF_CONTENT_READER_H

#include "PdfXObject.h"
#include "PdfCanvas.h"
#include "PdfData.h"
#include "PdfDictionary.h"
#include "PdfVariantStack.h"
#include "PdfPostScriptTokenizer.h"

namespace PoDoFo {

/** Type of the content read from a content stream
 */
enum class PdfContentType : uint8_t
{
    Unknown = 0,
    Operator,          ///< The token is a PDF operator
    ImageDictionary,   ///< Inline image dictionary
    ImageData,         ///< Raw inline image data found between ID and EI tags (see PDF ref section 4.8.6)
    DoXObject,         ///< Issued when a Do operator is found and it is handled by the reader. NOTE: for Form XObjects BeginFormXObject is issued instead, unless PdfContentReaderFlags::SkipFollowFormXObject is used. 
    BeginFormXObject,  ///< Issued when a Form XObject is being followed
    EndFormXObject,    ///< Issued when a Form XObject has just been followed
    UnexpectedKeyword, ///< An unexpected keyword that can be a custom operator or invalid PostScript content   
};

enum class PdfContentWarnings : uint16_t
{
    None = 0,
    SpuriousStackContent = 1,           ///< Operand count for the operator are more than necessary
    RecursiveXObject = 2,               ///< Recursive XObject call detected. Applies to DoXObject
    InvalidImageDictionaryContent = 4,  ///< Found invalid content while reading inline image dictionary. Applies to ImageDictionary
    MissingEndImage = 8,                ///< Missing end inline image EI operator
};

enum class PdfContentErrors : uint16_t
{
    None = 0,
    InvalidOperator = 1,                ///< Unknown operator or insufficient operand count. Applies to Operator
    InvalidXObject = 2,                 ///< Invalid or not found XObject
};

/** Content as read from content streams
 */
class PODOFO_API PdfContent final
{
    friend class PdfContentStreamReader;
public:
    PdfContent();
public:
    struct Data
    {
        PdfVariantStack Stack;
        PdfOperator Operator = PdfOperator::Unknown;
        std::string_view Keyword;
        PdfDictionary InlineImageDictionary;
        charbuff InlineImageData;
        const PdfName* Name = nullptr;
        std::shared_ptr<PdfXObject> XObject;
    };

    const PdfVariantStack& GetStack() const;
    PdfOperator GetOperator() const;
    const std::string_view& GetKeyword() const;
    const PdfDictionary& GetInlineImageDictionary() const;
    const charbuff& GetInlineImageData() const;
    const std::shared_ptr<const PdfXObject>& GetXObject() const;

    bool HasWarnings() const;
    bool HasErrors() const;

    PdfContentType GetType() const { return Type; }
    PdfContentWarnings GetWarnings() const { return Warnings; }
    PdfContentErrors GetErrors() const { return Errors; }

    /** Unchecked access to content data 
     */
    const Data& operator*() const { return Data; }

    /** Unchecked and mutable access to content data
     */
    Data& operator*() { return Data; }

    /** Unchecked access to content data
     */
    const Data* operator->() const { return &Data; }

    /** Unchecked and mutable access to content data
     */
    Data* operator->() { return &Data; }

private:
    void checkAccess(PdfContentType type) const;

private:
    PdfContentType Type;
    bool ThrowOnWarnings;
    PdfContentWarnings Warnings;
    PdfContentErrors Errors;
    struct Data Data;
};

enum class PdfContentReaderFlags
{
    None = 0,
    ThrowOnWarnings = 1,
    SkipFollowFormXObjects = 2,     ///< Don't follow Form XObject 
    SkipHandleNonFormXObjects = 4,  ///< Don't handle non Form XObjects (PdfImage, PdfXObjectPostScript). Doesn't influence traversing of Form XObject(s)
};

/** Custom handler for inline images
 * \param imageDict dictionary for the inline image
 * \returns false if EOF 
 */
using PdfInlineImageHandler = std::function<bool(const PdfDictionary& imageDict, InputStreamDevice& device)>;

struct PODOFO_API PdfContentReaderArgs final
{
    PdfContentReaderFlags Flags = PdfContentReaderFlags::None;
    PdfInlineImageHandler InlineImageHandler;
};

/** Reader class to read content streams
 */
class PODOFO_API PdfContentStreamReader final
{
public:
    PdfContentStreamReader(const PdfCanvas& canvas, nullable<const PdfContentReaderArgs&> args = { });

    PdfContentStreamReader(std::shared_ptr<InputStreamDevice> device, nullable<const PdfContentReaderArgs&> args = { });

private:
    PdfContentStreamReader(std::shared_ptr<InputStreamDevice>&& device, const PdfCanvas* canvas,
        nullable<const PdfContentReaderArgs&> args);

public:
    bool TryReadNext(PdfContent& data);

private:
    void beforeReadReset(PdfContent& content);

    void afterReadClear(PdfContent& content);

    bool tryReadNextContent(PdfContent& content);

    bool tryHandleOperator(PdfContent& content, bool& eof);

    bool tryReadInlineImgDict(PdfContent& content);

    bool tryReadInlineImgData(charbuff& data);

    bool tryHandleXObject(PdfContent& content);

    bool isCalledRecursively(const PdfObject* xobj);

private:
    struct Storage
    {
        PdfPostScriptTokenType PsType;
        std::string_view Keyword;
        PdfVariant Variant;
        PdfName Name;
    };

    struct Input
    {
        std::shared_ptr<const PdfXObject> Form;
        std::shared_ptr<InputStreamDevice> Device;
        const PdfCanvas* Canvas;
    };

private:
    std::vector<Input> m_inputs;
    PdfContentReaderArgs m_args;
    std::shared_ptr<charbuff> m_buffer;
    PdfPostScriptTokenizer m_tokenizer;
    bool m_readingInlineImgData;  // A state of reading inline image data

    // Temp storage
    Storage m_temp;
};

};

ENABLE_BITMASK_OPERATORS(PoDoFo::PdfContentReaderFlags);
ENABLE_BITMASK_OPERATORS(PoDoFo::PdfContentWarnings);
ENABLE_BITMASK_OPERATORS(PoDoFo::PdfContentErrors);

#endif // PDF_CONTENT_READER_H
