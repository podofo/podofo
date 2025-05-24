/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfTokenizer.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include <podofo/auxiliary/InputDevice.h>
#include "PdfName.h"
#include "PdfString.h"
#include "PdfReference.h"
#include "PdfVariant.h"

using namespace std;
using namespace PoDoFo;

static bool tryGetEscapedCharacter(char ch, char& escapedChar);
static void readHexString(InputStreamDevice& device, charbuff& buffer);
static bool isOctalChar(char ch);

PdfTokenizer::PdfTokenizer(const PdfTokenizerOptions& options)
    : PdfTokenizer(std::in_place, std::make_shared<charbuff>(BufferSize), options)
{
}

PdfTokenizer::PdfTokenizer(shared_ptr<charbuff> buffer, const PdfTokenizerOptions& options)
    : PdfTokenizer(std::in_place, std::move(buffer), options)
{
}

PdfTokenizer::PdfTokenizer(std::in_place_t, shared_ptr<charbuff>&& buffer, const PdfTokenizerOptions& options)
    : m_buffer(std::move(buffer)), m_options(options)
{
    if (m_buffer == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);
}

bool PdfTokenizer::TryReadNextToken(InputStreamDevice& device, string_view& token)
{
    PdfTokenType tokenType;
    return TryReadNextToken(device, token, tokenType);
}

bool PdfTokenizer::TryReadNextToken(InputStreamDevice& device, string_view& token, PdfTokenType& tokenType)
{
    char* buffer = m_buffer->data();
    // NOTE: Reserve 1 byte for the null termination
    size_t bufferSize = m_buffer->size() - 1;

    // check first if there are queued tokens and return them first
    if (m_tokenQueque.size() != 0)
    {
        auto& pair = m_tokenQueque.front();
        tokenType = pair.second;

        size_t size = std::min(bufferSize, pair.first.size());
        // make sure buffer is \0 terminated
        std::memcpy(buffer, pair.first.data(), size);
        buffer[size] = '\0';
        token = string_view(buffer, size);

        m_tokenQueque.pop_front();
        return true;
    }

    tokenType = PdfTokenType::Literal;

    char ch1;
    char ch2;
    size_t count = 0;
    while (count < bufferSize)
    {
        if (!device.Peek(ch1))
            goto Eof;

        // ignore leading whitespaces
        if (count == 0 && IsCharWhitespace(ch1))
        {
            // Consume the whitespace character
            (void)device.ReadChar();
            continue;
        }
        // ignore comments
        else if (ch1 == '%')
        {
            // Consume all characters before the next line break
            do
            {
                (void)device.ReadChar();
                if (!device.Peek(ch1))
                    goto Eof;

            } while (ch1 != '\n' && ch1 != '\r');

            // If we've already read one or more chars of a token, return them, since
            // comments are treated as token-delimiting whitespace. Otherwise keep reading
            // at the start of the next line.
            if (count != 0)
                break;
        }
        // special handling for << and >> tokens
        else if (count == 0 && (ch1 == '<' || ch1 == '>'))
        {
            // Really consume character from stream
            (void)device.ReadChar();
            buffer[count] = ch1;
            count++;

            if (!device.Peek(ch2))
                goto Eof;

            // Is n another < or > , ie are we opening/closing a dictionary?
            // If so, consume that character too.
            if (ch2 == ch1)
            {
                (void)device.ReadChar();
                buffer[count++] = ch2;
                if ((int)m_options.LanguageLevel < 2)
                    continue;

                if (ch1 == '<')
                    tokenType = PdfTokenType::DoubleAngleBracketsLeft;
                else
                    tokenType = PdfTokenType::DoubleAngleBracketsRight;
            }
            else
            {
                if (ch1 == '<')
                    tokenType = PdfTokenType::AngleBracketLeft;
                else
                    tokenType = PdfTokenType::AngleBracketRight;
            }

            break;
        }
        else if (count != 0 && (IsCharWhitespace(ch1) || IsCharDelimiter(ch1)))
        {
            // Next (unconsumed) character is a token-terminating char, so
            // we have a complete token and can return it.
            break;
        }
        else
        {
            // Consume the next character and add it to the token we're building.
            (void)device.ReadChar();
            buffer[count] = ch1;
            count++;

            PdfTokenType tokenDelimiterType;
            if (IsCharTokenDelimiter(ch1, tokenDelimiterType))
            {
                // All delimiters except << and >> (handled above) are
                // one-character tokens, so if we hit one we can just return it
                // immediately.
                tokenType = tokenDelimiterType;
                break;
            }
        }
    }

Exit:
    buffer[count] = '\0';
    token = string_view(buffer, count);
    return true;

Eof:
    if (count == 0)
    {
        // No characters were read before EOF, so we're out of data.
        // Ensure the buffer points to nullptr in case someone fails to check the return value.
        token = { };
        return false;
    }

    goto Exit;
}

bool PdfTokenizer::TryPeekNextToken(InputStreamDevice& device, string_view& token)
{
    PdfTokenType tokenType;
    return TryPeekNextToken(device, token, tokenType);
}

bool PdfTokenizer::TryPeekNextToken(InputStreamDevice& device, string_view& token, PdfTokenType& tokenType)
{
    if (!this->TryReadNextToken(device, token, tokenType))
        return false;

    // Don't consume the token
    this->EnqueueToken(token, tokenType);
    return true;
}

int64_t PdfTokenizer::ReadNextNumber(InputStreamDevice& device)
{
    int64_t ret;
    if (!TryReadNextNumber(device, ret))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidNumber, "Could not read number");

    return ret;
}

bool PdfTokenizer::TryReadNextNumber(InputStreamDevice& device, int64_t& value)
{
    PdfTokenType tokenType;
    string_view token;
    if (!this->TryReadNextToken(device, token, tokenType))
        return false;

    if (!utls::TryParse(token, value))
    {
        // Don't consume the token
        this->EnqueueToken(token, tokenType);
        return false;
    }

    return true;
}

void PdfTokenizer::ReadNextVariant(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt)
{
    if (!TryReadNextVariant(device, variant, encrypt))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected variant");
}

bool PdfTokenizer::TryReadNextVariant(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt)
{
    PdfTokenType tokenType;
    string_view token;
    if (!TryReadNextToken(device, token, tokenType))
        return false;

    return PdfTokenizer::TryReadNextVariant(device, token, tokenType, variant, encrypt);
}

void PdfTokenizer::ReadNextVariant(InputStreamDevice& device, const string_view& token, PdfTokenType tokenType, PdfVariant& variant, const PdfStatefulEncrypt* encrypt)
{
    if (!TryReadNextVariant(device, token, tokenType, variant, encrypt))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Could not read a variant");
}

bool PdfTokenizer::TryReadNextVariant(InputStreamDevice& device, const string_view& token, PdfTokenType tokenType, PdfVariant& variant, const PdfStatefulEncrypt* encrypt)
{
    utls::RecursionGuard guard;
    PdfLiteralDataType dataType = DetermineDataType(device, token, tokenType, variant);
    return tryReadDataType(device, dataType, variant, encrypt);
}

PdfTokenizer::PdfLiteralDataType PdfTokenizer::DetermineDataType(InputStreamDevice& device,
    const string_view& token, PdfTokenType tokenType, PdfVariant& variant)
{
    switch (tokenType)
    {
        case PdfTokenType::Literal:
        {
            variant.~PdfVariant();

            // check for the two special datatypes
            // null and boolean.
            // check for numbers
            if (token == "null")
            {
                new(&variant.m_Null)PdfVariant::NullMember();
                return PdfLiteralDataType::Null;
            }
            else if (token == "true")
            {
                new(&variant.m_Bool)PdfVariant::PrimitiveMember(true);
                return PdfLiteralDataType::Bool;
            }
            else if (token == "false")
            {
                new(&variant.m_Bool)PdfVariant::PrimitiveMember(false);
                return PdfLiteralDataType::Bool;
            }

            PdfLiteralDataType dataType = PdfLiteralDataType::Number;
            const char* start = token.data();
            while (*start)
            {
                if (*start == '.')
                {
                    dataType = PdfLiteralDataType::Real;
                }
                else if (!(isdigit(*start) || *start == '-' || *start == '+'))
                {
                    dataType = PdfLiteralDataType::Unknown;
                    break;
                }

                start++;
            }

            if (dataType == PdfLiteralDataType::Real)
            {
                double val;
                if (!utls::TryParse(token, val))
                {
                    // Don't consume the token
                    this->EnqueueToken(token, tokenType);
                    PoDoFo::LogMessage(PdfLogSeverity::Warning, "Invalid real while parsing content");
                    return PdfLiteralDataType::Unknown;
                }

                new(&variant.m_Real)PdfVariant::PrimitiveMember(val);
                return PdfLiteralDataType::Real;
            }
            else if (dataType == PdfLiteralDataType::Number)
            {
                int64_t num1;
                if (!utls::TryParse(token, num1))
                {
                    // Don't consume the token
                    this->EnqueueToken(token, tokenType);
                    PoDoFo::LogMessage(PdfLogSeverity::Warning, "Invalid number while parsing content");
                    return PdfLiteralDataType::Unknown;
                }

                if (!m_options.ReadReferences)
                {
                    new(&variant.m_Number)PdfVariant::PrimitiveMember(num1);
                    return PdfLiteralDataType::Number;
                }

                // read another two tokens to see if it is a reference
                // we cannot be sure that there is another token
                // on the input device, so if we hit EOF just return
                // EPdfDataType::Number .
                PdfTokenType secondTokenType;
                string_view nextToken;
                bool gotToken = this->TryReadNextToken(device, nextToken, secondTokenType);
                if (!gotToken)
                {
                    // No next token, so it can't be a reference
                    new(&variant.m_Number)PdfVariant::PrimitiveMember(num1);
                    return PdfLiteralDataType::Number;
                }

                if (secondTokenType != PdfTokenType::Literal)
                {
                    this->EnqueueToken(nextToken, secondTokenType);
                    new(&variant.m_Number)PdfVariant::PrimitiveMember(num1);
                    return PdfLiteralDataType::Number;
                }

                int64_t num2;
                if (!utls::TryParse(nextToken, num2))
                {
                    // Don't consume the token
                    this->EnqueueToken(nextToken, secondTokenType);
                    new(&variant.m_Number)PdfVariant::PrimitiveMember(num1);
                    return PdfLiteralDataType::Number;
                }

                string tmp(nextToken);
                PdfTokenType thirdTokenType;
                gotToken = this->TryReadNextToken(device, nextToken, thirdTokenType);
                if (!gotToken)
                {
                    // No third token, so it can't be a reference
                    new(&variant.m_Number)PdfVariant::PrimitiveMember(num1);
                    return PdfLiteralDataType::Number;
                }
                if (thirdTokenType == PdfTokenType::Literal &&
                    nextToken.length() == 1 && nextToken[0] == 'R')
                {
                    new(&variant.m_Reference)PdfReference(static_cast<uint32_t>(num1), static_cast<uint16_t>(num2));
                    return PdfLiteralDataType::Reference;
                }
                else
                {
                    this->EnqueueToken(tmp, secondTokenType);
                    this->EnqueueToken(nextToken, thirdTokenType);
                    new(&variant.m_Number)PdfVariant::PrimitiveMember(num1);
                    return PdfLiteralDataType::Number;
                }
            }
            else
            {
                new(&variant.m_Null)PdfVariant::NullMember();
                return PdfLiteralDataType::Unknown;
            }
        }
        // Following types just reset the variant to "null",
        // they will be properly initialized later
        case PdfTokenType::DoubleAngleBracketsLeft:
            variant.Reset();
            return PdfLiteralDataType::Dictionary;
        case PdfTokenType::SquareBracketLeft:
            variant.Reset();
            return PdfLiteralDataType::Array;
        case PdfTokenType::ParenthesisLeft:
            variant.Reset();
            return PdfLiteralDataType::String;
        case PdfTokenType::AngleBracketLeft:
            variant.Reset();
            return PdfLiteralDataType::HexString;
        case PdfTokenType::Slash:
            variant.Reset();
            return PdfLiteralDataType::Name;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported token at this context");
    }
}

bool PdfTokenizer::tryReadDataType(InputStreamDevice& device, PdfLiteralDataType dataType, PdfVariant& variant, const PdfStatefulEncrypt* encrypt)
{
    switch (dataType)
    {
        case PdfLiteralDataType::Dictionary:
            this->ReadDictionary(device, variant, encrypt);
            return true;
        case PdfLiteralDataType::Array:
            this->ReadArray(device, variant, encrypt);
            return true;
        case PdfLiteralDataType::String:
            this->ReadString(device, variant, encrypt);
            return true;
        case PdfLiteralDataType::HexString:
            this->ReadHexString(device, variant, encrypt);
            return true;
        case PdfLiteralDataType::Name:
            this->ReadName(device, variant);
            return true;
        // The following datatypes are not handled by read datatype
        // but are already parsed by DetermineDatatype
        case PdfLiteralDataType::Null:
        case PdfLiteralDataType::Bool:
        case PdfLiteralDataType::Number:
        case PdfLiteralDataType::Real:
        case PdfLiteralDataType::Reference:
            return true;
        default:
            return false;
    }
}

void PdfTokenizer::ReadDictionary(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt)
{
    PODOFO_ASSERT(variant.GetDataType() == PdfDataType::Null);

    PdfVariant nameVar;
    PdfTokenType tokenType;
    string_view token;
    unique_ptr<charbuff> contentsHexBuffer;

    new(&variant.m_Dictionary)PdfVariant::PrimitiveMember(new PdfDictionary());
    auto& dict = variant.GetDictionaryUnsafe();

    while (true)
    {
        bool gotToken = this->TryReadNextToken(device, token, tokenType);
        if (!gotToken)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected dictionary key name or >> delim");

        if (tokenType == PdfTokenType::DoubleAngleBracketsRight)
            break;

        this->ReadNextVariant(device, token, tokenType, nameVar, encrypt);
        // Convert the read variant to a name; throws InvalidDataType if not a name.
        auto& key = nameVar.GetName();

        gotToken = this->TryReadNextToken(device, token, tokenType);
        if (!gotToken)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected variant");

        // Try to get the next variant
        auto& emplaced = dict.EmplaceNoDirtySet(key);
        PdfLiteralDataType dataType = DetermineDataType(device, token, tokenType, emplaced.GetVariantUnsafe());
        if (key == "Contents" && dataType == PdfLiteralDataType::HexString)
        {
            // 'Contents' key in signature dictionaries is an unencrypted Hex string:
            // save the string buffer for later check if it needed decryption
            contentsHexBuffer = std::unique_ptr<charbuff>(new charbuff());
            readHexString(device, *contentsHexBuffer);
            continue;
        }

        if (!tryReadDataType(device, dataType, emplaced.GetVariantUnsafe(), encrypt))
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Could not read variant");

        emplaced.SetParent(dict);
    }

    if (contentsHexBuffer.get() != nullptr)
    {
        auto type = dict.GetKey("Type");
        // "Contents" is unencrypted in /Type/Sig and /Type/DocTimeStamp dictionaries 
        // https://issues.apache.org/jira/browse/PDFBOX-3173
        bool contentsUnencrypted = type != nullptr && type->GetDataType() == PdfDataType::Name &&
            (type->GetName() == "Sig" || type->GetName() == "DocTimeStamp");

        const PdfStatefulEncrypt* actualEncrypt = nullptr;
        if (!contentsUnencrypted)
            actualEncrypt = encrypt;

        new(&dict.EmplaceNoDirtySet("Contents"_n).GetVariantUnsafe().m_String)PdfString(
            PdfString::FromHexData({ contentsHexBuffer->size() ? contentsHexBuffer->data() : "", contentsHexBuffer->size() }, actualEncrypt));
    }
}

void PdfTokenizer::ReadArray(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt)
{
    PODOFO_ASSERT(variant.GetDataType() == PdfDataType::Null);

    string_view token;
    PdfTokenType tokenType;
    new(&variant.m_Array)PdfVariant::PrimitiveMember(new PdfArray());
    auto& arr = variant.GetArrayUnsafe();

    while (true)
    {
        bool gotToken = this->TryReadNextToken(device, token, tokenType);
        if (!gotToken)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected array item or ] delim");

        if (tokenType == PdfTokenType::SquareBracketRight)
            break;

        auto& newobj = arr.EmplaceBackNoDirtySet();
        this->ReadNextVariant(device, token, tokenType, newobj.GetVariantUnsafe(), encrypt);
        newobj.SetParent(arr);
    }
}

void PdfTokenizer::ReadString(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt)
{
    PODOFO_ASSERT(variant.GetDataType() == PdfDataType::Null);

    char ch;
    bool escape = false;
    bool octEscape = false;
    int octCharCount = 0;
    char octValue = 0;
    int balanceCount = 0; // Balanced parenthesis do not have to be escaped in strings

    m_charBuffer.clear();
    while (device.Read(ch))
    {
        if (escape)
        {
            // Handle escape sequences
            if (octEscape)
            {
                // Handle octal escape sequences
                octCharCount++;

                if (!isOctalChar(ch))
                {
                    if (ch == ')')
                    {
                        // Handle end of string while reading octal code
                        // NOTE: The octal value is added outside of the loop
                        break;
                    }

                    // No octal character anymore,
                    // so the octal sequence must be ended
                    // and the character has to be treated as normal character!
                    m_charBuffer.push_back(octValue);

                    if (ch != '\\')
                    {
                        m_charBuffer.push_back(ch);
                        escape = false;
                    }

                    octEscape = false;
                    octCharCount = 0;
                    octValue = 0;
                    continue;
                }

                octValue <<= 3;
                octValue |= ((ch - '0') & 0x07);

                if (octCharCount == 3)
                {
                    m_charBuffer.push_back(octValue);
                    escape = false;
                    octEscape = false;
                    octCharCount = 0;
                    octValue = 0;
                }
            }
            else if (isOctalChar(ch))
            {
                // The last character we have read was a '\\',
                // so we check now for a digit to find stuff like \005
                octValue = (ch - '0') & 0x07;
                octEscape = true;
                octCharCount = 1;
            }
            else
            {
                // Handle plain escape sequences
                char escapedCh;
                if (tryGetEscapedCharacter(ch, escapedCh))
                    m_charBuffer.push_back(escapedCh);

                escape = false;
            }
        }
        else
        {
            // Handle raw characters
            if (balanceCount == 0 && ch == ')')
                break;

            if (ch == '(')
                balanceCount++;
            else if (ch == ')')
                balanceCount--;

            escape = ch == '\\';
            if (!escape)
                m_charBuffer.push_back(static_cast<char>(ch));
        }
    }

    // In case the string ends with a octal escape sequence
    if (octEscape)
        m_charBuffer.push_back(octValue);

    if (m_charBuffer.size() != 0)
    {
        if (encrypt != nullptr)
        {
            charbuff decrypted;
            encrypt->DecryptTo(decrypted, { m_charBuffer.data(), m_charBuffer.size() });
            new(&variant.m_String)PdfString(std::move(decrypted), false);
        }
        else
        {
            new(&variant.m_String)PdfString(charbuff(m_charBuffer.data(), m_charBuffer.size()), false);
        }
    }
    else
    {
        // NOTE: The string is empty but ensure it will be
        // initialized as a raw buffer first
        new(&variant.m_String)PdfString(charbuff(), false);
    }
}

void PdfTokenizer::ReadHexString(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt* encrypt)
{
    PODOFO_ASSERT(variant.GetDataType() == PdfDataType::Null);
    readHexString(device, m_charBuffer);
    new(&variant.m_String)PdfString(PdfString::FromHexData({ m_charBuffer.size() ? m_charBuffer.data() : "", m_charBuffer.size() }, encrypt));
}

void PdfTokenizer::ReadName(InputStreamDevice& device, PdfVariant& variant)
{
    PODOFO_ASSERT(variant.GetDataType() == PdfDataType::Null);

    // Do special checking for empty names
    // as tryReadNextToken will ignore white spaces
    // and we have to take care for stuff like:
    // 10 0 obj / endobj
    // which stupid but legal PDF
    char ch;
    if (!device.Peek(ch) || IsCharWhitespace(ch))
    {
        // We have an empty PdfName
        // NOTE: Delimiters are handled correctly by tryReadNextToken
        new(&variant.m_Name)PdfName();
        return;
    }

    PdfTokenType tokenType;
    string_view token;
    bool gotToken = this->TryReadNextToken(device, token, tokenType);
    if (!gotToken || tokenType != PdfTokenType::Literal)
    {
        // We got an empty name which is legal according to the PDF specification
        // Some weird PDFs even use them.
        new(&variant.m_Name)PdfName();

        // Enqueue the token again
        if (gotToken)
            EnqueueToken(token, tokenType);
    }
    else
    {
        new(&variant.m_Name)PdfName(PdfName::FromEscaped(token));
    }
}

void PdfTokenizer::EnqueueToken(const string_view& token, PdfTokenType tokenType)
{
    m_tokenQueque.push_back(TokenizerPair(string(token), tokenType));
}

bool tryGetEscapedCharacter(char ch, char& escapedChar)
{
    switch (ch)
    {
        case '\n':          // Ignore newline characters when reading escaped sequences
            escapedChar = '\0';
            return false;
        case '\r':          // Ignore newline characters when reading escaped sequences
            escapedChar = '\0';
            return false;
        case 'n':           // Line feed (LF)
            escapedChar = '\n';
            return true;
        case 'r':           // Carriage return (CR)
            escapedChar = '\r';
            return true;
        case 't':           // Horizontal tab (HT)
            escapedChar = '\t';
            return true;
        case 'b':           // Backspace (BS)
            escapedChar = '\b';
            return true;
        case 'f':           // Form feed (FF)
            escapedChar = '\f';
            return true;
        default:
            escapedChar = ch;
            return true;
    }
}

void readHexString(InputStreamDevice& device, charbuff& buffer)
{
    buffer.clear();
    char ch;
    while (device.Read(ch))
    {
        // end of stream reached
        if (ch == '>')
            break;

        // only a hex digits
        if (isdigit(ch) ||
            (ch >= 'A' && ch <= 'F') ||
            (ch >= 'a' && ch <= 'f'))
            buffer.push_back(ch);
    }

    // pad to an even length if necessary
    if (buffer.size() % 2)
        buffer.push_back('0');
}

bool isOctalChar(char ch)
{
    switch (ch)
    {
        case '0':
            return true;
        case '1':
            return true;
        case '2':
            return true;
        case '3':
            return true;
        case '4':
            return true;
        case '5':
            return true;
        case '6':
            return true;
        case '7':
            return true;
        default:
            return false;
    }
}
