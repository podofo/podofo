/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_TOKENIZER_H
#define PDF_TOKENIZER_H

#include "PdfDeclarations.h"
#include <podofo/auxiliary/InputDevice.h>
#include "PdfStatefulEncrypt.h"

#include <deque>

namespace PoDoFo {

class PdfVariant;

enum class PdfPostScriptLanguageLevel : uint8_t
{
    L1 = 1,
    L2 = 2,
};

struct PODOFO_API PdfTokenizerOptions final
{
    PdfPostScriptLanguageLevel LanguageLevel = PdfPostScriptLanguageLevel::L2;
    bool ReadReferences = true;
};

/**
 * A simple tokenizer for PDF files and PDF content streams
 */
class PODOFO_API PdfTokenizer
{
    friend class PdfPostScriptTokenizer;
    PODOFO_PRIVATE_FRIEND(class PdfParserObject);

public:
    static constexpr unsigned BufferSize = 4096;

public:
    PdfTokenizer(const PdfTokenizerOptions& options = { });
    PdfTokenizer(std::shared_ptr<charbuff> buffer, const PdfTokenizerOptions& options = { });

    /** Reads the next token from the current file position
     *  ignoring all comments.
     *
     *  \param[out] token On true return, set to a pointer to the read
     *                     token (a nullptr-terminated C string). The pointer is
     *                     to memory owned by PdfTokenizer and must NOT be
     *                     freed.  The contents are invalidated on the next
     *                     call to tryReadNextToken(..) and by the destruction of
     *                     the PdfTokenizer. Undefined on false return.
     *
     *  \param[out] tokenType On true return, if not nullptr the type of the read token
     *                     will be stored into this parameter. Undefined on false
     *                     return.
     *
     *  \returns           True if a token was read, false if there are no
     *                     more tokens to read.
     */
    bool TryReadNextToken(InputStreamDevice& device, std::string_view& token);
    bool TryReadNextToken(InputStreamDevice& device, std::string_view& token, PdfTokenType& tokenType);

    /** Try peek the next token from the current file position
     * ignoring all comments, without actually consuming it
     *
     * \returns false if EOF
     */
    bool TryPeekNextToken(InputStreamDevice& device, std::string_view& token);
    bool TryPeekNextToken(InputStreamDevice& device, std::string_view& token, PdfTokenType& tokenType);

    /** Read the next number from the current file position
     *  ignoring all comments.
     *
     *  Raises NoNumber exception if the next token is no number, and
     *  UnexpectedEOF if no token could be read. No token is consumed if
     *  NoNumber is thrown.
     *
     *  \returns a number read from the input device.
     */
    int64_t ReadNextNumber(InputStreamDevice& device);
    bool TryReadNextNumber(InputStreamDevice& device, int64_t& value);

    /** Read the next variant from the current file position
     *  ignoring all comments.
     *
     *  Raises an UnexpectedEOF exception if there is no variant left in the
     *  file.
     *
     *  \param variant write the read variant to this value
     *  \param encrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadNextVariant(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt = { });
    bool TryReadNextVariant(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt = { });

protected:
    // This enum differs from regular PdfDataType in the sense
    // it enumerates only data types that can be determined literally
    // by the tokenization and specify better if the strings literals
    // are regular or hex strings
    enum class PdfLiteralDataType : uint8_t
    {
        Unknown = 0,
        Bool,
        Number,
        Real,
        String,
        HexString,
        Name,
        Array,
        Dictionary,
        Null,
        Reference,
    };

protected:
    /** Read the next variant from the current file position
     *  ignoring all comments.
     *
     *  Raises an exception if there is no variant left in the file.
     *
     *  \param token a token that has already been read
     *  \param type type of the passed token
     *  \param variant write the read variant to this value
     *  \param encrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadNextVariant(InputStreamDevice& device, const std::string_view& token, PdfTokenType tokenType, PdfVariant& variant, const PdfStatefulEncrypt* encrypt);
    bool TryReadNextVariant(InputStreamDevice& device, const std::string_view& token, PdfTokenType tokenType, PdfVariant& variant, const PdfStatefulEncrypt* encrypt);

    /** Add a token to the queue of tokens.
     *  tryReadNextToken() will return all enqueued tokens first before
     *  reading new tokens from the input device.
     *
     *  \param token string of the token
     *  \param type type of the token
     *
     *  \see tryReadNextToken
     */
    void EnqueueToken(const std::string_view& token, PdfTokenType type);

    /** Read a dictionary from the input device
     *  and store it into a variant.
     *
     *  \param variant store the dictionary into this variable
     *  \param encrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadDictionary(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt);

    /** Read an array from the input device
     *  and store it into a variant.
     *
     *  \param variant store the array into this variable
     *  \param encrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadArray(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt);

    /** Read a string from the input device
     *  and store it into a variant.
     *
     *  \param variant store the string into this variable
     *  \param encrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadString(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt);

    /** Read a hex string from the input device
     *  and store it into a variant.
     *
     *  \param variant store the hex string into this variable
     *  \param encrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadHexString(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt);

    /** Read a name from the input device
     *  and store it into a variant.
     *
     *  Throws UnexpectedEOF if there is nothing to read.
     *
     *  \param variant store the name into this variable
     */
    void ReadName(InputStreamDevice& device, PdfVariant& variant);

    /** Determine the possible datatype of a token.
     *  Numbers, reals, bools or nullptr values are parsed directly by this function
     *  and saved to a variant.
     *
     *  \returns the expected datatype
     */
    PdfLiteralDataType DetermineDataType(InputStreamDevice& device, const std::string_view& token, PdfTokenType tokenType, PdfVariant& variant);

private:
    PdfTokenizer(std::in_place_t, std::shared_ptr<charbuff>&& buffer, const PdfTokenizerOptions& options);
    bool tryReadDataType(InputStreamDevice& device, PdfLiteralDataType dataType, PdfVariant& variant, const PdfStatefulEncrypt* encrypt);

private:
    using TokenizerPair = std::pair<std::string, PdfTokenType>;
    using TokenizerQueque = std::deque<TokenizerPair>;

private:
    std::shared_ptr<charbuff> m_buffer;
    PdfTokenizerOptions m_options;
    TokenizerQueque m_tokenQueque;
    charbuff m_charBuffer;
};

};

#endif // PDF_TOKENIZER_H
