/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfTokenizer.h"

#include <podofo/private/charconv_compat.h>

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfInputDevice.h"
#include "PdfName.h"
#include "PdfString.h"
#include "PdfReference.h"
#include "PdfVariant.h"

using namespace std;
using namespace PoDoFo;

static char getEscapedCharacter(char ch);
static void readHexString(InputStreamDevice& device, charbuff& buffer);
static bool isOctalChar(char ch);

PdfTokenizer::PdfTokenizer(bool readReferences)
    : PdfTokenizer(std::make_shared<charbuff>(BufferSize), readReferences)
{
}

PdfTokenizer::PdfTokenizer(const shared_ptr<charbuff>& buffer, bool readReferences)
    : m_buffer(buffer), m_readReferences(readReferences)
{
    if (buffer == nullptr)
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
        if (count == 0 && IsWhitespace(ch1))
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
                buffer[count] = ch2;
                if (ch1 == '<')
                    tokenType = PdfTokenType::DoubleAngleBracketsLeft;
                else
                    tokenType = PdfTokenType::DoubleAngleBracketsRight;
                count++;
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
        else if (count != 0 && (IsWhitespace(ch1) || IsDelimiter(ch1)))
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
            if (IsTokenDelimiter(ch1, tokenDelimiterType))
            {
                // All delimeters except << and >> (handled above) are
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

bool PdfTokenizer::IsNextToken(InputStreamDevice& device, const string_view& token)
{
    if (token.length() == 0)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    string_view readToken;
    bool gotToken = this->TryReadNextToken(device, readToken);
    if (!gotToken)
        PODOFO_RAISE_ERROR(PdfErrorCode::UnexpectedEOF);

    return token == readToken;
}

int64_t PdfTokenizer::ReadNextNumber(InputStreamDevice& device)
{
    PdfTokenType tokenType;
    string_view token;
    bool gotToken = this->TryReadNextToken(device, token, tokenType);
    if (!gotToken)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected number");

    int64_t num;
    if (std::from_chars(token.data(), token.data() + token.size(), num).ec != std::errc())
    {
        // Don't consume the token
        this->EnqueueToken(token, tokenType);
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NoNumber, "Could not read number");
    }

    return static_cast<int64_t>(num);
}

void PdfTokenizer::ReadNextVariant(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt& encrypt)
{
    if (!TryReadNextVariant(device, variant, encrypt))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected variant");
}

bool PdfTokenizer::TryReadNextVariant(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt& encrypt)
{
    PdfTokenType tokenType;
    string_view token;
    if (!TryReadNextToken(device, token, tokenType))
        return false;

    return PdfTokenizer::TryReadNextVariant(device, token, tokenType, variant, encrypt);
}

void PdfTokenizer::ReadNextVariant(InputStreamDevice& device, const string_view& token, PdfTokenType tokenType, PdfVariant& variant, const PdfStatefulEncrypt& encrypt)
{
    if (!TryReadNextVariant(device, token, tokenType, variant, encrypt))
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Could not read a variant");
}

bool PdfTokenizer::TryReadNextVariant(InputStreamDevice& device, const string_view& token, PdfTokenType tokenType, PdfVariant& variant, const PdfStatefulEncrypt& encrypt)
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
            // check for the two special datatypes
            // null and boolean.
            // check for numbers
            if (token == "null")
            {
                variant = PdfVariant();
                return PdfLiteralDataType::Null;
            }
            else if (token == "true")
            {
                variant = PdfVariant(true);
                return PdfLiteralDataType::Bool;
            }
            else if (token == "false")
            {
                variant = PdfVariant(false);
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
                if (std::from_chars(token.data(), token.data() + token.length(), val, chars_format::fixed).ec != std::errc())
                {
                    // Don't consume the token
                    this->EnqueueToken(token, tokenType);
                    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NoNumber, token);
                }

                variant = PdfVariant(val);
                return PdfLiteralDataType::Real;
            }
            else if (dataType == PdfLiteralDataType::Number)
            {
                int64_t num;
                if (std::from_chars(token.data(), token.data() + token.size(), num).ec != std::errc())
                {
                    // Don't consume the token
                    this->EnqueueToken(token, tokenType);
                    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NoNumber, token);
                }

                variant = PdfVariant(num);
                if (!m_readReferences)
                    return PdfLiteralDataType::Number;

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
                    return PdfLiteralDataType::Number;
                }
                if (secondTokenType != PdfTokenType::Literal)
                {
                    this->EnqueueToken(nextToken, secondTokenType);
                    return PdfLiteralDataType::Number;
                }

                if (std::from_chars(nextToken.data(), nextToken.data() + nextToken.length(), num).ec != std::errc())
                {
                    // Don't consume the token
                    this->EnqueueToken(nextToken, secondTokenType);
                    return PdfLiteralDataType::Number;
                }

                string tmp(nextToken);
                PdfTokenType thirdTokenType;
                gotToken = this->TryReadNextToken(device, nextToken, thirdTokenType);
                if (!gotToken)
                {
                    // No third token, so it can't be a reference
                    return PdfLiteralDataType::Number;
                }
                if (thirdTokenType == PdfTokenType::Literal &&
                    nextToken.length() == 1 && nextToken[0] == 'R')
                {
                    variant = PdfReference(static_cast<uint32_t>(variant.GetNumber()), static_cast<uint16_t>(num));
                    return PdfLiteralDataType::Reference;
                }
                else
                {
                    this->EnqueueToken(tmp, secondTokenType);
                    this->EnqueueToken(nextToken, thirdTokenType);
                    return PdfLiteralDataType::Number;
                }
            }
            else
                return PdfLiteralDataType::Unknown;
        }
        case PdfTokenType::DoubleAngleBracketsLeft:
            return PdfLiteralDataType::Dictionary;
        case PdfTokenType::SquareBracketLeft:
            return PdfLiteralDataType::Array;
        case PdfTokenType::ParenthesisLeft:
            return PdfLiteralDataType::String;
        case PdfTokenType::AngleBracketLeft:
            return PdfLiteralDataType::HexString;
        case PdfTokenType::Slash:
            return PdfLiteralDataType::Name;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported token at this context");
    }
}

bool PdfTokenizer::tryReadDataType(InputStreamDevice& device, PdfLiteralDataType dataType, PdfVariant& variant, const PdfStatefulEncrypt& encrypt)
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

void PdfTokenizer::ReadDictionary(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt& encrypt)
{
    PdfVariant val;
    PdfName key;
    PdfTokenType tokenType;
    string_view token;
    unique_ptr<charbuff> contentsHexBuffer;

    variant = PdfDictionary();
    PdfDictionary& dict = variant.GetDictionary();

    while (true)
    {
        bool gotToken = this->TryReadNextToken(device, token, tokenType);
        if (!gotToken)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected dictionary key name or >> delim");

        if (tokenType == PdfTokenType::DoubleAngleBracketsRight)
            break;

        this->ReadNextVariant(device, token, tokenType, val, encrypt);
        // Convert the read variant to a name; throws InvalidDataType if not a name.
        key = val.GetName();

        // Try to get the next variant
        gotToken = this->TryReadNextToken(device, token, tokenType);
        if (!gotToken)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected variant");

        PdfLiteralDataType dataType = DetermineDataType(device, token, tokenType, val);
        if (key == "Contents" && dataType == PdfLiteralDataType::HexString)
        {
            // 'Contents' key in signature dictionaries is an unencrypted Hex string:
            // save the string buffer for later check if it needed decryption
            contentsHexBuffer = std::unique_ptr<charbuff>(new charbuff());
            readHexString(device, *contentsHexBuffer);
            continue;
        }

        if (!tryReadDataType(device, dataType, val, encrypt))
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Could not read variant");

        // Add the key without triggering SetDirty
        dict.AddKey(key, std::move(val), true);
    }

    if (contentsHexBuffer.get() != nullptr)
    {
        PdfObject* type = dict.GetKey("Type");
        // "Contents" is unencrypted in /Type/Sig and /Type/DocTimeStamp dictionaries 
        // https://issues.apache.org/jira/browse/PDFBOX-3173
        bool contentsUnencrypted = type != nullptr && type->GetDataType() == PdfDataType::Name &&
            (type->GetName() == "Sig" || type->GetName() == "DocTimeStamp");

        PdfStatefulEncrypt actualEncrypt;
        if (!contentsUnencrypted)
            actualEncrypt = encrypt;

        val = PdfString::FromHexData({ contentsHexBuffer->size() ? contentsHexBuffer->data() : "", contentsHexBuffer->size() }, actualEncrypt);
        dict.AddKey("Contents", std::move(val));
    }
}

void PdfTokenizer::ReadArray(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt& encrypt)
{
    string_view token;
    PdfTokenType tokenType;
    PdfVariant var;
    variant = PdfArray();
    PdfArray& arr = variant.GetArray();

    while (true)
    {
        bool gotToken = this->TryReadNextToken(device, token, tokenType);
        if (!gotToken)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected array item or ] delim");

        if (tokenType == PdfTokenType::SquareBracketRight)
            break;

        this->ReadNextVariant(device, token, tokenType, var, encrypt);
        arr.Add(std::move(var));
    }
}

void PdfTokenizer::ReadString(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt& encrypt)
{
    char ch;
    bool escape = false;
    bool octEscape = false;
    int octCharCount = 0;
    char octValue = 0;
    int balanceCount = 0; // Balanced parathesis do not have to be escaped in strings

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
                // Ignore end of line characters when reading escaped sequences
                if (ch != '\n' && ch != '\r')
                {
                    // Handle plain escape sequences
                    char escapedCh = getEscapedCharacter(ch);
                    if (escapedCh != '\0')
                        m_charBuffer.push_back(escapedCh);
                }

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
        if (encrypt.HasEncrypt())
        {
            charbuff decrypted;
            encrypt.DecryptTo(decrypted, { m_charBuffer.data(), m_charBuffer.size() });
            variant = PdfString(std::move(decrypted), false);
        }
        else
        {
            variant = PdfString::FromRaw({ m_charBuffer.data(), m_charBuffer.size() }, false);
        }
    }
    else
    {
        // NOTE: The string is empty but ensure it will be
        // initialized as a raw buffer first
        variant = PdfString::FromRaw({ }, false);
    }
}

void PdfTokenizer::ReadHexString(InputStreamDevice& device, PdfVariant& variant, const PdfStatefulEncrypt& encrypt)
{
    readHexString(device, m_charBuffer);
    variant = PdfString::FromHexData({ m_charBuffer.size() ? m_charBuffer.data() : "", m_charBuffer.size() }, encrypt);
}

void PdfTokenizer::ReadName(InputStreamDevice& device, PdfVariant& variant)
{
    // Do special checking for empty names
    // as tryReadNextToken will ignore white spaces
    // and we have to take care for stuff like:
    // 10 0 obj / endobj
    // which stupid but legal PDF
    char ch;
    if (!device.Peek(ch) || IsWhitespace(ch))
    {
        // We have an empty PdfName
        // NOTE: Delimeters are handled correctly by tryReadNextToken
        variant = PdfName();
        return;
    }

    PdfTokenType tokenType;
    string_view token;
    bool gotToken = this->TryReadNextToken(device, token, tokenType);
    if (!gotToken || tokenType != PdfTokenType::Literal)
    {
        // We got an empty name which is legal according to the PDF specification
        // Some weird PDFs even use them.
        variant = PdfName();

        // Enqueue the token again
        if (gotToken)
            EnqueueToken(token, tokenType);
    }
    else
        variant = PdfName::FromEscaped(token);
}

void PdfTokenizer::EnqueueToken(const string_view& token, PdfTokenType tokenType)
{
    m_tokenQueque.push_back(TokenizerPair(string(token), tokenType));
}

bool PdfTokenizer::IsWhitespace(char ch)
{
    switch (ch)
    {
        case '\0': // NULL
            return true;
        case '\t': // TAB
            return true;
        case '\n': // Line Feed
            return true;
        case '\f': // Form Feed
            return true;
        case '\r': // Carriage Return
            return true;
        case ' ': // White space
            return true;
        default:
            return false;
    }
}

bool PdfTokenizer::IsDelimiter(char ch)
{
    switch (ch)
    {
        case '(':
            return true;
        case ')':
            return true;
        case '<':
            return true;
        case '>':
            return true;
        case '[':
            return true;
        case ']':
            return true;
        case '{':
            return true;
        case '}':
            return true;
        case '/':
            return true;
        case '%':
            return true;
        default:
            return false;
    }
}

bool PdfTokenizer::IsTokenDelimiter(char ch, PdfTokenType& tokenType)
{
    switch (ch)
    {
        case '(':
            tokenType = PdfTokenType::ParenthesisLeft;
            return true;
        case ')':
            tokenType = PdfTokenType::ParenthesisRight;
            return true;
        case '[':
            tokenType = PdfTokenType::SquareBracketLeft;
            return true;
        case ']':
            tokenType = PdfTokenType::SquareBracketRight;
            return true;
        case '{':
            tokenType = PdfTokenType::BraceLeft;
            return true;
        case '}':
            tokenType = PdfTokenType::BraceRight;
            return true;
        case '/':
            tokenType = PdfTokenType::Slash;
            return true;
        default:
            tokenType = PdfTokenType::Unknown;
            return false;
    }
}

bool PdfTokenizer::IsRegular(char ch)
{
    return !IsWhitespace(ch) && !IsDelimiter(ch);
}

bool PdfTokenizer::IsPrintable(char ch)
{
    return ch > 32 && ch < 125;
}

char getEscapedCharacter(char ch)
{
    switch (ch)
    {
        case 'n':           // Line feed (LF)
            return '\n';
        case 'r':           // Carriage return (CR)
            return '\r';
        case 't':           // Horizontal tab (HT)
            return '\t';
        case 'b':           // Backspace (BS)
            return '\b';
        case 'f':           // Form feed (FF)
            return '\f';
        case '(':
            return '(';
        case ')':
            return ')';
        case '\\':
            return '\\';
        default:
            return '\0';
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
