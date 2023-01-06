/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontType1.h"

#include <utfcpp/utf8.h>

#include "PdfInputDevice.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfObjectStream.h"
#include "PdfDifferenceEncoding.h"
#include "PdfDocument.h"

using namespace std;
using namespace PoDoFo;

PdfFontType1::PdfFontType1(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding) :
    PdfFontSimple(doc, metrics, encoding)
{
}

bool PdfFontType1::SupportsSubsetting() const
{
    // TODO: Re-enable me by fixing below code
    // return true;
    return false;
}

PdfFontType PdfFontType1::GetType() const
{
    return PdfFontType::Type1;
}

/*
// Helper Class needed for parsing type1-font for subsetting
class PdfType1Encrypt
{
public:
    PdfType1Encrypt();
    char Encrypt(char plain);
    char Decrypt(char cipher);

protected:
    unsigned short m_r;
    unsigned short m_c1;
    unsigned short m_c2;
};

class PdfType1EncryptEexec : public PdfType1Encrypt
{
public:
    PdfType1EncryptEexec();
};

class PdfType1EncryptCharstring : public PdfType1Encrypt
{
public:
    PdfType1EncryptCharstring();
};

void PdfFontType1::embedFontSubset()
{
    size_t size = 0;
    unsigned length1 = 0;
    unsigned length2 = 0;
    unsigned length3 = 0;
    const char* buffer;

    auto contents = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject();
    m_Descriptor->GetDictionary().AddKey("FontFile", contents->GetIndirectReference());

    if (m_Metrics->GetFontFileData().empty())
        PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);

    buffer = m_Metrics->GetFontFileData().data();
    size = m_Metrics->GetFontFileData().size();

    // Allocate buffer for subsetted font, worst case size is input size
    chars outBuff(size);
    unsigned outIndex = 0;

    // unsigned to make comparisons work
    unsigned const char* inBuff = reinterpret_cast<unsigned const char*>(buffer);
    unsigned inIndex = 0;

    // 6-Byte binary header for leading ascii-part
    PODOFO_ASSERT(inBuff[inIndex + 0] == 0x80);
    PODOFO_ASSERT(inBuff[inIndex + 1] == 0x01);
    unsigned length = inBuff[inIndex + 2] +
        (inBuff[inIndex + 3] << 8) +
        (inBuff[inIndex + 4] << 16) +
        (inBuff[inIndex + 5] << 24);				// little endian
    inIndex += 6;

    PODOFO_ASSERT(memcmp(&inBuff[inIndex], "%!PS-AdobeFont-1.", 17) == 0);

    // transfer ascii-part, modify encoding dictionary (dup ...), if present
    string line;
    bool dupFound = false;
    for (unsigned i = 0; i < length; i++)
    {
        line += static_cast<char>(inBuff[inIndex + i]);
        if (inBuff[inIndex + i] == '\r')
        {
            if (line.find("dup ") != 0)
            {
                memcpy(&outBuff[outIndex], line.c_str(), line.length());
                outIndex += (unsigned)line.length();
            }
            else
            {
                if (!dupFound)
                {
                    // if first found, replace with new dictionary according to used glyphs
                    // ignore further dup's
                    for (char32_t ch = 0; ch < 256; ch++)
                    {
                        ////
                        throw runtime_error("Untested after utf8 migration");
                        if (m_usedSet.find(ch) != m_usedSet.end())
                        {
                            // TODO: Use utls::FormatTo()
                            outIndex += sprintf(reinterpret_cast<char*>(&outBuff[outIndex]),
                                "dup %u /%s put\r", (unsigned)ch,
                                PdfDifferenceEncoding::UnicodeIDToName(ch).GetString().c_str());
                        }
                    }
                    dupFound = true;
                }
            }
            line.clear();
        }
    }
    inIndex += length;
    length1 = outIndex;


    // 6-Byte binary header for binary-part
    PODOFO_ASSERT(inBuff[inIndex + 0] == 0x80);
    PODOFO_ASSERT(inBuff[inIndex + 1] == 0x02);
    length = inBuff[inIndex + 2] +
        (inBuff[inIndex + 3] << 8) +
        (inBuff[inIndex + 4] << 16) +
        (inBuff[inIndex + 5] << 24);				// little endian
    inIndex += 6;

    // copy binary using encrpytion
    unsigned outIndexStart = outIndex;
    bool foundSeacGlyph;

    // if glyph contains seac-command, add the used base-glyphs and loop again
    do
    {
        PdfType1EncryptEexec inCrypt;

        outIndex = outIndexStart;
        line.clear();

        foundSeacGlyph = false;
        bool inCharString = false;
        for (unsigned i = 0; i < length; )
        {
            unsigned char plain = inCrypt.Decrypt(inBuff[inIndex + i]);
            i++;

            line += static_cast<char>(plain);

            // output is ssssbuild uncrypted, as parts might be skipped and cipher-engine must be unchanged
            if (inCharString && line.find("/") == 0)
            {
                // we are now inside a glyph, copy anything until RD or -| to output,
                // in case this glyph will be skipped we go back to saveOutIndex
                outBuff[outIndex++] = plain;
                while (line.find("RD ") == static_cast<size_t>(-1)
                    && line.find("-| ") == static_cast<size_t>(-1))
                {
                    plain = inCrypt.Decrypt(inBuff[inIndex + i]);
                    outBuff[outIndex++] = plain;
                    line += static_cast<char>(plain);
                    i++;
                }

                ////
                throw runtime_error("Untested after utf8 migration");

                // parse line for name and length of glyph
                string glyphName = line;
                unsigned glyphLen;
                unsigned result;
                if (line.find("RD ") != static_cast<size_t>(-1))
                    result = sscanf(line.c_str(), "%s %d RD ", glyphName.data(), &glyphLen);
                else
                    result = sscanf(line.c_str(), "%s %d -| ", glyphName.data(), &glyphLen);
                PODOFO_ASSERT(result == 2);

                glyphName = glyphName.substr(1);
                bool useGlyph = false;
                // determine if this glyph is used in normal chars
                for (char32_t ch = 0; ch < 256; ch++)
                {
                    ////
                    throw runtime_error("Untested after utf8 migration");
                    if (m_usedSet.find(ch) != m_usedSet.end() &&
                        PdfDifferenceEncoding::UnicodeIDToName(ch).GetString() == glyphName)
                    {
                        useGlyph = true;
                        break;
                    }
                }

                // always use .notdef
                if (glyphName == ".notdef")
                    useGlyph = true;

                // transfer glyph to output
                for (unsigned j = 0; j < glyphLen; j++, i++)
                    outBuff[outIndex++] = inCrypt.Decrypt(inBuff[inIndex + i]);

                // check used glyph for seac-command
                if (useGlyph && FindSeac(outBuff.data() + (outIndex - glyphLen), glyphLen))
                    foundSeacGlyph = true;

                // transfer rest until end of line to output
                do
                {
                    plain = inCrypt.Decrypt(inBuff[inIndex + i]);
                    outBuff[outIndex++] = plain;
                    line += static_cast<char>(plain);
                    i++;
                } while (plain != '\r' && plain != '\n');

                if (!useGlyph)
                {
                    // glyph is not used, go back to saved position
                    outIndex = outIndexSave;
                }
            }
            else
            {
                // copy anything outside glyph to output
                outBuff[outIndex++] = plain;
            }

            if (plain == '\r' || plain == '\n')
            {
                // parse for /CharStrings = begin of glyphs
                if (line.find("/CharStrings") != static_cast<size_t>(-1))
                    inCharString = true;
                line.clear();
            }
        }
    } while (foundSeacGlyph);

    // now encrypt resulting output-buffer
    PdfType1EncryptEexec outCrypt;
    for (unsigned i = outIndexStart; i < outIndex; i++)
        outBuff[i] = outCrypt.Encrypt(outBuff[i]);

    length2 = outIndex - outIndexStart;
    inIndex += length;


    // 6-Byte binary header for ascii-part
    PODOFO_ASSERT(inBuff[inIndex + 0] == 0x80);
    PODOFO_ASSERT(inBuff[inIndex + 1] == 0x01);
    length = inBuff[inIndex + 2] +
        (inBuff[inIndex + 3] << 8) +
        (inBuff[inIndex + 4] << 16) +
        (inBuff[inIndex + 5] << 24);				// little endian
    inIndex += 6;

    // copy ascii
    memcpy(&outBuff[outIndex], &inBuff[inIndex], length);
    length3 = length;
    inIndex += length;
    outIndex += length;

    // now embed
    contents->GetOrCreateStream().Set(outBuff.data(), (size_t)outIndex);

    // enter length in dictionary
    contents->GetDictionary().AddKey("Length1", PdfObject(static_cast<int64_t>(length1)));
    contents->GetDictionary().AddKey("Length2", PdfObject(static_cast<int64_t>(length2)));
    contents->GetDictionary().AddKey("Length3", PdfObject(static_cast<int64_t>(length3)));
}

// TODO: Understand what this code was meant to do
//       Settle all the subsetting stuff
//       For now we just rely on the PdfFontSimple generic embedding
void PdfFontType1::embedFontFile(PdfObject& descriptor)
{
    size_t size = 0;
    ptrdiff_t length1 = 0;
    ptrdiff_t length2 = 0;
    ptrdiff_t length3 = 0;
    PdfObject* contents;
    chars buffer;

    contents = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject();

    descriptor.GetDictionary().AddKey("FontFile", contents->GetIndirectReference());

    if (m_Metrics->GetFontFileData().empty())
        PODOFO_RAISE_ERROR(PdfErrorCode::InternalLogic);

    buffer = (chars)m_Metrics->GetFontFileData();
    size = m_Metrics->GetFontFileData().size();

    // Remove binary segment headers from pfb
    unsigned char* data = reinterpret_cast<unsigned char*>(buffer.data());
    while (*data == 0x80)	// binary segment header
    {
        constexpr unsigned HeaderLength = 6;
        unsigned segmentType = data[1];	// binary segment type
        unsigned segmentLength = 0;
        ptrdiff_t segmentDelta = (buffer.data() + size) - reinterpret_cast<const char*>(data);

        switch (segmentType)
        {
            case 1:									// ASCII text
            {
                segmentLength = data[2] + 		// little endian
                    data[3] * 256 +
                    data[4] * 65536 +
                    data[5] * 16777216;
                if (length1 == 0)
                    length1 = segmentLength;
                else
                    length3 = segmentLength;
                size -= HeaderLength;
                memmove(data, data + HeaderLength, segmentDelta - HeaderLength);
                data = data + segmentLength;
                break;
            }
            case 2:									// binary data
            {
                segmentLength = data[2] + 		// little endian
                    data[3] * 256 +
                    data[4] * 65536 +
                    data[5] * 16777216;
                length2 = segmentLength;
                size -= HeaderLength;
                memmove(data, data + HeaderLength, segmentDelta - HeaderLength);
                data = data + segmentLength;
                break;
            }
            case 3:									// end-of-file
            {
                // First set pContents keys before writing stream, so that PdfTFontType1 works with streamed document
                contents->GetDictionary().AddKey("Length1", PdfObject(static_cast<int64_t>(length1)));
                contents->GetDictionary().AddKey("Length2", PdfObject(static_cast<int64_t>(length2)));
                contents->GetDictionary().AddKey("Length3", PdfObject(static_cast<int64_t>(length3)));
                contents->GetOrCreateStream().Set(buffer.data(), size - 2);
                return;
            }
        }
    }

    // Parse the font data buffer to get the values for length1, length2 and length3
    length1 = FindInBuffer("eexec", buffer.data(), size);
    if (length1 > 0)
        length1 += 6; // 6 == eexec + lf
    else
        length1 = 0;

    if (length1 != 0)
    {
        length2 = FindInBuffer("cleartomark", buffer.data(), size);
        if (length2 > 0)
            length2 = size - length1 - 520; // 520 == 512 + strlen(cleartomark)
        else
            length1 = 0;
    }

    length3 = size - length2 - length1;

    // TODO: Pdf Supports only Type1 fonts with binary encrypted sections and not the hex format
    contents->GetOrCreateStream().Set(buffer.data(), size);
    contents->GetDictionary().AddKey("Length1", PdfObject(static_cast<int64_t>(length1)));
    contents->GetDictionary().AddKey("Length2", PdfObject(static_cast<int64_t>(length2)));
    contents->GetDictionary().AddKey("Length3", PdfObject(static_cast<int64_t>(length3)));
}

bool PdfFontType1::FindSeac(const char* buffer, size_t length)
{
    (void)buffer;
    (void)length;
    return false;
    ////
    throw runtime_error("Untested after utf8 migration");

    PdfType1EncryptCharstring crypt;
    auto stdEncoding = PdfEncodingFactory::CreateStandardEncoding();

    bool foundNewGlyph = false;
    int code1 = 0;
    int code2 = 0;
    for (size_t j = 0; j < length; )
    {
        unsigned char plain = crypt.Decrypt(buffer[j++]);

        if (j <= 4)
        {
            // skip first 4 bytes
        }
        else if (plain < 32)
        {
            // decode commands
            switch (plain)
            {
                case 1:		// hstem
                case 3:		// vstem
                case 4:		// rmoveto
                case 5:		// rlineto
                case 6:		// hlineto
                case 7:		// vlineto
                case 8:		// rrcurveto
                case 9:		// closepath
                case 10:	// callsubr
                case 11:	// return
                    break;

                case 12:	// escape
                {
                    plain = crypt.Decrypt(buffer[j++]);
                    switch (plain)
                    {
                        case 0:		// dotsection
                        case 1:		// vstem3
                        case 2:		// hstem3
                            break;

                        case 6:		// seac
                        {
                            // found seac command, use acquired code1 and code2 to get glyphname in standard-encoding
                            char32_t ch = stdEncoding->GetCodePoint(static_cast<unsigned>(code1));
                            foundNewGlyph = m_usedSet.insert(ch).second;

                            ch = stdEncoding->GetCodePoint(static_cast<unsigned>(code2));
                            foundNewGlyph = m_usedSet.insert(ch).second;
                        }
                        break;

                        case 7:		// sbw
                        case 12:	// div
                        case 16:	// callothersubr
                        case 17:	// pop
                        case 33:	// setcurrentpoint
                            break;

                        default:	// ???
                            break;
                    }
                }
                break;

                case 13:	// hsbw
                case 14:	// endchar
                case 21:	// rmoveto
                case 22:	// hmoveto
                case 30:	// vhcurveto
                case 31:	// hcurveto
                    break;

                default:	// ???
                    break;
            }
        }
        else if (plain >= 32) // &&  plain <= 255 )
        {
            // This is a number
            // Type1 specification 6.2 Charstring Number Encoding
            int number = 0;
            if (plain >= 32 && plain <= 246)
            {
                number = static_cast<int>(plain - 139);
            }
            else if (plain >= 247 && plain <= 250)
            {
                unsigned char next = crypt.Decrypt(buffer[j++]);

                number = (static_cast<int>(plain) - 247) * 256 + next + 108;
            }
            else if (plain >= 251 && plain <= 254)
            {
                unsigned char next = crypt.Decrypt(buffer[j++]);

                number = -((static_cast<int>(plain) - 251) * 256) - next - 108;
            }
            else if (plain == 255)
            {
                unsigned char next1 = crypt.Decrypt(buffer[j++]);
                unsigned char next2 = crypt.Decrypt(buffer[j++]);
                unsigned char next3 = crypt.Decrypt(buffer[j++]);
                unsigned char next4 = crypt.Decrypt(buffer[j++]);

                number = (static_cast<int>(next1) << 24)
                    + (static_cast<int>(next2) << 16)
                    + (static_cast<int>(next3) << 8)
                    + next4;
            }

            // TODO: Use utls::FormatTo()
            char num[32];
            sprintf(num, "%d ", number);

            code1 = code2;
            code2 = number;
        }
    }
    return foundNewGlyph;
}

ptrdiff_t PdfFontType1::FindInBuffer(const char* needle, const char* haystack, size_t len) const
{
    // if lNeedleLen is 0 the while loop will not be executed and we return -1
    size_t needleLen = needle ? strlen(needle) : 0;
    const char* end = haystack + len - needleLen;
    const char* start = haystack;

    if (needle != nullptr)
    {
        while (haystack < end)
        {
            if (strncmp(haystack, needle, needleLen) == 0)
                return haystack - start;

            haystack++;
        }
    }

    return -1;
}

PdfType1EncryptEexec::PdfType1EncryptEexec()
{
    m_r = 55665;
}

PdfType1EncryptCharstring::PdfType1EncryptCharstring()
{
    m_r = 4330;
}

PdfType1Encrypt::PdfType1Encrypt()
    : m_r(-1) // will be initialized in subclasses with real value
{
    m_c1 = 52845;
    m_c2 = 22719;
}

char PdfType1Encrypt::Encrypt(char plain)
{
    unsigned char cipher = (unsigned char)plain ^ (unsigned char)(m_r >> 8);
    m_r = ((cipher + m_r) * m_c1 + m_c2) & ((1 << 16) - 1);
    return (char)cipher;
}

char PdfType1Encrypt::Decrypt(char cipher)
{
    unsigned char plain = (unsigned char)cipher ^ (unsigned char)(m_r >> 8);
    m_r = (cipher + m_r) * m_c1 + m_c2;
    return (char)plain;
}
*/
