// SPDX-FileCopyrightText: 2006 Dominik Seichter <domseichter@web.de>
// SPDX-FileCopyrightText: 2020 Francesco Pretto <ceztko@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later OR MPL-2.0

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

enum class PdfTokenizerFlags : uint32_t
{
    None = 0,
    StrictParsing = 1,
    SkipReferences = 2,     ///< Don't read indirect references in the form "1 0 R".
                            ///< Implied for PdfPostScriptTokenizer: PostScript has no indirect references.
};

struct PODOFO_API PdfTokenizerParams final
{
    PdfPostScriptLanguageLevel LanguageLevel = PdfPostScriptLanguageLevel::L2;
    PdfTokenizerFlags Flags = PdfTokenizerFlags::None;
};

/// @deprecated Use PdfTokenizerParams instead
struct PODOFO_API PdfTokenizerOptions final
{
    PdfPostScriptLanguageLevel LanguageLevel = PdfPostScriptLanguageLevel::L2;
    bool ReadReferences = true;
};

/// A simple tokenizer for PDF files and PDF content streams
class PODOFO_API PdfTokenizer
{
    friend class PdfParser;
    friend class PdfPostScriptTokenizer;
    PODOFO_PRIVATE_FRIEND(class PdfParserObject);

public:
    static constexpr unsigned BufferSize = 4096;
    static constexpr size_t MaxStringLength = 64 * 1024 * 1024; // 64 MiB

public:
    PdfTokenizer();
    PdfTokenizer(std::shared_ptr<charbuff> buffer);

    [[deprecated("Use the overloads with PdfTokenizerParams instead")]]
    PdfTokenizer(const PdfTokenizerOptions& options);

    [[deprecated("Use the overloads with PdfTokenizerParams instead")]]
    PdfTokenizer(std::shared_ptr<charbuff> buffer, const PdfTokenizerOptions& options);

    /// Reads the next token from the current file position
    /// ignoring all comments.
    ///
    /// @param[out] token On true return, set to a pointer to the read
    ///                     token (a nullptr-terminated C string). The pointer is
    ///                     to memory owned by PdfTokenizer and must NOT be
    ///                     freed.  The contents are invalidated on the next
    ///                     call to tryReadNextToken(..) and by the destruction of
    ///                     the PdfTokenizer. Undefined on false return.
    ///
    /// @param[out] tokenType On true return, if not nullptr the type of the read token
    ///                     will be stored into this parameter. Undefined on false
    ///                     return.
    ///
    /// @returns           True if a token was read, false if there are no
    ///                     more tokens to read.
    bool TryReadNextToken(InputStreamDevice& device, std::string_view& token);
    bool TryReadNextToken(InputStreamDevice& device, std::string_view& token, PdfTokenType& tokenType);

    /// Try peek the next token from the current file position
    /// ignoring all comments, without actually consuming it
    ///
    /// @returns false if EOF
    bool TryPeekNextToken(InputStreamDevice& device, std::string_view& token);
    bool TryPeekNextToken(InputStreamDevice& device, std::string_view& token, PdfTokenType& tokenType);

    /// Read the next number from the current file position
    /// ignoring all comments.
    ///
    /// Raises NoNumber exception if the next token is no number, and
    /// UnexpectedEOF if no token could be read. No token is consumed if
    /// NoNumber is thrown.
    ///
    /// @returns a number read from the input device.
    int64_t ReadNextNumber(InputStreamDevice& device);
    bool TryReadNextNumber(InputStreamDevice& device, int64_t& value);

    /// Read the next variant from the current file position
    /// ignoring all comments.
    ///
    /// Raises an UnexpectedEOF exception if there is no variant left in the
    /// file.
    ///
    /// @param variant write the read variant to this value
    /// @param encrypt an encryption object which is used to decrypt strings during parsing
    void ReadNextVariant(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt = { });
    bool TryReadNextVariant(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt = { });

    void Reset();

    void SetParameters(const PdfTokenizerParams& params) { m_Params = params; }

    const PdfTokenizerParams& GetParameters() const { return m_Params; }

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

    /// @remarks Default initialization for bitfields is not available in C++17
    /// (it was introduced in C++20), so always use full aggregate initialization
    struct ParsingOptions final
    {
        /// If set, malformed content raises an exception in the current call.
        /// Otherwise the offending method logs a message and returns false so
        /// the caller can bail out early.
        bool ThrowOnError : 1;
        /// PdfTokenizerFlags::StrictParsing at the time of the
        /// entry-point call so internal paths can react to strict mode
        /// without touching the tokenizer state again.
        bool Strict : 1;
    };

protected:
    /// Read the next variant from the current file position
    /// ignoring all comments.
    ///
    /// Raises an exception if there is no variant left in the file.
    ///
    /// @param token a token that has already been read
    /// @param tokenType type of the passed token
    /// @param variant write the read variant to this value
    /// @param encrypt an encryption object which is used to decrypt strings during parsing
    void ReadNextVariant(InputStreamDevice& device, const std::string_view& token, PdfTokenType tokenType, PdfVariant& variant, const PdfStatefulEncrypt* encrypt);
    bool TryReadNextVariant(InputStreamDevice& device, const std::string_view& token, PdfTokenType tokenType, PdfVariant& variant, const PdfStatefulEncrypt* encrypt);

    /// Add a token to the queue of tokens.
    /// tryReadNextToken() will return all enqueued tokens first before
    /// reading new tokens from the input device.
    ///
    /// @param token string of the token
    /// @param type type of the token
    ///
    /// @see tryReadNextToken
    void EnqueueToken(const std::string_view& token, PdfTokenType type);

    /// Read a dictionary from the input device
    /// and store it into a variant.
    ///
    /// @param variant store the dictionary into this variable
    /// @param encrypt an encryption object which is used to decrypt strings during parsing
    /// @param opts per-call flags controlling throw-vs-log-and-return-false semantics
    /// @returns true on success, false only when ThrowOnError is unset and parsing failed
    bool ReadDictionary(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt, ParsingOptions opts);

    /// Read an array from the input device
    /// and store it into a variant.
    ///
    /// @param variant store the array into this variable
    /// @param encrypt an encryption object which is used to decrypt strings during parsing
    /// @param opts per-call flags controlling throw-vs-log-and-return-false semantics
    /// @returns true on success, false only when ThrowOnError is unset and parsing failed
    bool ReadArray(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt, ParsingOptions opts);

    /// Read a string from the input device
    /// and store it into a variant.
    ///
    /// @param variant store the string into this variable
    /// @param encrypt an encryption object which is used to decrypt strings during parsing
    /// @param opts per-call flags controlling throw-vs-log-and-return-false semantics
    /// @returns true on success, false only when ThrowOnError is unset and parsing failed
    bool ReadString(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt, ParsingOptions opts);

    /// Read a hex string from the input device
    /// and store it into a variant.
    ///
    /// @param variant store the hex string into this variable
    /// @param encrypt an encryption object which is used to decrypt strings during parsing
    /// @param opts per-call flags controlling throw-vs-log-and-return-false semantics
    /// @returns true on success, false only when ThrowOnError is unset and parsing failed
    bool ReadHexString(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt, ParsingOptions opts);

    /// Read a name from the input device
    /// and store it into a variant.
    ///
    /// Throws UnexpectedEOF if there is nothing to read.
    ///
    /// @param variant store the name into this variable
    /// @param device the input device to read from
    /// @param opts per-call flags controlling throw-vs-log-and-return-false semantics
    /// @returns true on success, false only when ThrowOnError is unset and parsing failed
    bool ReadName(InputStreamDevice& device, PdfVariant& variant, ParsingOptions opts);

    /// Determine the possible datatype of a token.
    /// Numbers, reals, bools or nullptr values are parsed directly by this function
    /// and saved to a variant.
    ///
    /// @param opts per-call flags controlling throw-vs-log-and-return-false semantics
    /// @returns the expected datatype, or PdfLiteralDataType::Unknown when
    ///          ThrowOnError is unset and the token could not be classified
    PdfLiteralDataType DetermineDataType(InputStreamDevice& device, const std::string_view& token, PdfTokenType tokenType, PdfVariant& variant, ParsingOptions opts);

private:
    PdfTokenizer(std::in_place_t, std::shared_ptr<charbuff>&& buffer);
    bool readDataType(InputStreamDevice& device, PdfLiteralDataType dataType, PdfVariant& variant, const PdfStatefulEncrypt* encrypt, ParsingOptions opts);
    /// Consume tokens up to and including the specified end delimiter,
    /// balancing nested containers. Used for lenient recovery when an
    /// inner element of a dictionary or array fails to parse.
    ///
    /// @returns true if endToken was found, false on EOF (truncated container)
    bool drainContainer(InputStreamDevice& device, PdfTokenType endToken);

    /// Structural truncation: EOF hit inside the container. No draining is
    /// possible so we just report the failure and bail out.
    /// @returns always false
    bool handleContainerTruncation(ParsingOptions opts, PdfErrorCode code, std::string_view msg);

    /// Inner pair/element failed to parse. In strict mode this propagates;
    /// in non-strict mode we drain to the closing delimiter and recover.
    /// If the drain reaches EOF the container is truncated.
    /// @returns true if drained successfully, false otherwise
    bool handleContainerInnerError(InputStreamDevice& device, PdfTokenType endDelim, ParsingOptions opts,
        PdfErrorCode code, std::string_view msg);

private:
    using TokenizerPair = std::pair<std::string, PdfTokenType>;
    using TokenizerQueue = std::deque<TokenizerPair>;

private:
    std::shared_ptr<charbuff> m_buffer;
    PdfTokenizerParams m_Params;
    TokenizerQueue m_tokenQueue;
    charbuff m_charBuffer;
};

};

ENABLE_BITMASK_OPERATORS(PoDoFo::PdfTokenizerFlags);

#endif // PDF_TOKENIZER_H
