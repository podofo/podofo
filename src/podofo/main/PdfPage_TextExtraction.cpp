/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPage.h"

#include <regex>
#include <list>

#include <utf8cpp/utf8.h>

#include "PdfDocument.h"
#include "PdfTextState.h"
#include "PdfMath.h"
#include "PdfXObjectForm.h"
#include "PdfContentStreamReader.h"
#include "PdfFont.h"

#include <podofo/private/outstringstream.h>
#include <podofo/auxiliary/StateStack.h>

using namespace std;
using namespace cmn;
using namespace PoDoFo;

constexpr double SAME_LINE_THRESHOLD = 0.01;
constexpr double SEPARATION_EPSILON = 0.0000001;
// Inferred empirically on Adobe Acrobat Pro
constexpr unsigned HARD_SEPARATION_SPACING_MULTIPLIER = 6;
#define ASSERT(condition, message, ...) if (!condition)\
    PoDoFo::LogMessage(PdfLogSeverity::Warning, message, ##__VA_ARGS__);

static constexpr float NaN = numeric_limits<float>::quiet_NaN();

// 5.2 Text State Parameters and Operators
// 5.3 Text Objects
struct TextState
{
    TextState()
    {
        // Reset font size
        PdfState.FontSize = -1;
    }

    Matrix T_rm;  // Current T_rm
    Matrix CTM;   // Current CTM
    Matrix T_m;   // Current T_m
    Matrix T_lm;  // Current T_lm
    double T_l = 0;             // Leading text Tl
    PdfTextState PdfState;
    Vector2 WordSpacingVectorRaw;
    Vector2 SpaceCharVectorRaw;
    double WordSpacingLength = 0;
    double CharSpaceLength = 0;
    void ComputeDependentState();
    void ComputeSpaceDescriptors();
    void ComputeT_rm();
    double GetWordSpacingLength() const;
    double GetSpaceCharLength() const;
    void ScanString(const PdfString& encodedStr, string& decoded, vector<double>& lengths, vector<unsigned>& positions);
};

class StatefulString
{
public:
    StatefulString(string&& str, const TextState& state, vector<double>&& rawLengths, vector<unsigned>&& positions);
public:
    bool BeginsWithWhiteSpace() const;
    bool EndsWithWhiteSpace() const;
    StatefulString GetTrimmedBegin() const;
    StatefulString GetTrimmedEnd() const;
    double GetLengthRaw() const;
    double GetLength() const;
private:
    vector<double> computeLengths(const vector<double>& rawLengths);
public:
    const string String;
    const TextState State;
    const vector<double> RawLengths;
    const vector<double> Lengths;
    // Glyph position in the string
    const vector<unsigned> StringPositions;
    const Vector2 Position;
    const bool IsWhiteSpace;
};

struct EntryOptions
{
    bool IgnoreCase;
    bool TrimSpaces;
    bool TokenizeWords;
    bool MatchWholeWord;
    bool RegexPattern;
    bool ComputeBoundingBox;
    bool RawCoordinates;
    bool ExtractSubstring;
};

using StringChunk = list<StatefulString>;
using StringChunkPtr = unique_ptr<StringChunk>;
using StringChunkList = list<StringChunkPtr>;
using StringChunkListPtr = unique_ptr<StringChunkList>;
using TextStateStack = StateStack<TextState>;

struct XObjectState
{
    const PdfXObjectForm* Form;
    unsigned TextStateIndex;
};

struct ExtractionContext
{
public:
    ExtractionContext(vector<PdfTextEntry> &entries, const PdfPage &page, const string_view &pattern,
        PdfTextExtractFlags flags, const nullable<Rect> &clipRect);
public:
    void BeginText();
    void EndText();
    void Tf_Operator(const PdfName &fontname, double fontsize);
    void cm_Operator(double a, double b, double c, double d, double e, double f);
    void Tm_Operator(double a, double b, double c, double d, double e, double f);
    void TdTD_Operator(double tx, double ty);
    void AdvanceSpace(double ty);
    void TStar_Operator();
public:
    void PushString(const StatefulString &str, bool pushchunk = false);
    void TryPushChunk();
    void TryAddLastEntry();
private:
    bool areChunksSpaced(double& distance);
    void pushChunk();
    void addEntry();
    void tryAddEntry(const StatefulString& currStr);
    const PdfCanvas& getActualCanvas();
    const StatefulString& getPreviouString() const;
private:
    const PdfPage& m_page;
public:
    const int PageIndex;
    const string Pattern;
    const EntryOptions Options;
    const nullable<Rect> ClipRect;
    unique_ptr<Matrix> Rotation;
    vector<PdfTextEntry> &Entries;
    StringChunkPtr Chunk = std::make_unique<StringChunk>();
    StringChunkList Chunks;
    TextStateStack States;
    vector<XObjectState> XObjectStateIndices;
    double CurrentEntryT_rm_y = NaN;    // Tracks line changing
    Vector2 PrevChunkT_rm_Pos;          // Tracks space separation
    bool BlockOpen = false;
};

struct GlyphAddress
{
    unsigned StringIndex;
    unsigned GlyphIndex;
};

static bool decodeString(const PdfString &str, TextState &state, string &decoded,
    vector<double> &lengths, vector<unsigned>& positions);
static bool areEqual(double lhs, double rhs);
static bool isWhiteSpaceChunk(const StringChunk &chunk);
static void splitChunkBySpaces(vector<StringChunkPtr> &splittedChunks, const StringChunk &chunk);
static void splitStringBySpaces(vector<StatefulString> &separatedStrings, const StatefulString &string);
static void trimSpacesBegin(StringChunk &chunk);
static void trimSpacesEnd(StringChunk &chunk);
static void addEntry(vector<PdfTextEntry> &textEntries, StringChunkList &strings,
    const string_view &pattern, const EntryOptions &options, const nullable<Rect> &clipRect,
    int pageIndex, const Matrix* rotation);
static void addEntryChunk(vector<PdfTextEntry> &textEntries, StringChunkList &strings,
    const string_view &pattern, const EntryOptions& options, const nullable<Rect> &clipRect,
    int pageIndex, const Matrix* rotation);
static void processChunks(const StringChunkList& chunks, string& destString,
    vector<unsigned>& positions, vector<const StatefulString*>& strings,
    vector<GlyphAddress>& glyphAddresses);
static double computeLength(const vector<const StatefulString*>& strings, const vector<GlyphAddress>& glyphAddresses,
    unsigned lowerIndex, unsigned upperIndex);
static bool isMatchWholeWordSubstring(const string_view& str, const string_view& pattern, size_t& matchPos);
static Rect computeBoundingBox(const TextState& textState, double boxWidth);
static void read(const PdfVariantStack& stack, double &tx, double &ty);
static void read(const PdfVariantStack& stack, double &a, double &b, double &c, double &d, double &e, double &f);
static void getSubstringIndices(const vector<unsigned>& positions, unsigned lowerPos, unsigned upperLimitPos,
    unsigned& lowerIndex, unsigned& upperLimitIndex);
static EntryOptions optionsFromFlags(PdfTextExtractFlags flags);

void PdfPage::ExtractTextTo(vector<PdfTextEntry>& entries, const PdfTextExtractParams& params) const
{
    ExtractTextTo(entries, { }, params);
}

void PdfPage::ExtractTextTo(vector<PdfTextEntry>& entries, const string_view& pattern,
    const PdfTextExtractParams& params) const
{
    ExtractionContext context(entries, *this, pattern, params.Flags, params.ClipRect);

    // Look FIGURE 4.1 Graphics objects
    PdfContentReaderArgs args;
    args.Flags = PdfContentReaderFlags::SkipHandleNonFormXObjects; // Images are not needed for text extraction
    PdfContentStreamReader reader(*this, args);
    PdfContent content;
    vector<double> lengths;
    vector<unsigned> positions;
    string decoded;
    AbortCheckInfo info;
    while (reader.TryReadNext(content))
    {
        // Check for an abort
        if (++info.ReadCount % 100 == 0)
        {
            if (params.AbortCheck && params.AbortCheck(info))
                break;
        }

        switch (content.GetType())
        {
            case PdfContentType::Operator:
            {
                if (content.HasErrors())
                {
                    // Ignore invalid operators
                    continue;
                }

                // T_l TL: Set the text leading, T_l
                switch (content->Operator)
                {
                    case PdfOperator::TL:
                    {
                        context.States.Current->T_l = content->Stack[0].GetReal();
                        break;
                    }
                    case PdfOperator::cm:
                    {
                        double a, b, c, d, e, f;
                        read(content->Stack, a, b, c, d, e, f);
                        context.cm_Operator(a, b, c, d, e, f);
                        break;
                    }
                    // t_x t_y Td     : Move to the start of the next line
                    // t_x t_y TD     : Move to the start of the next line
                    // a b c d e f Tm : Set the text matrix, T_m , and the text line matrix, T_lm
                    case PdfOperator::Td:
                    case PdfOperator::TD:
                    case PdfOperator::Tm:
                    {
                        if (content->Operator == PdfOperator::Td || content->Operator == PdfOperator::TD)
                        {
                            double tx, ty;
                            read(content->Stack, tx, ty);
                            context.TdTD_Operator(tx, ty);

                            if (content->Operator == PdfOperator::TD)
                                context.States.Current->T_l = -ty;
                        }
                        else if (content->Operator == PdfOperator::Tm)
                        {
                            double a, b, c, d, e, f;
                            read(content->Stack, a, b, c, d, e, f);
                            context.Tm_Operator(a, b, c, d, e, f);
                        }
                        else
                        {
                            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Invalid flow");
                        }

                        break;
                    }
                    // T*: Move to the start of the next line
                    case PdfOperator::T_Star:
                    {
                        // NOTE: Errata for the PDF Reference, sixth edition, version 1.7
                        // Section 5.3, Text Objects:
                        // This operator has the same effect as the code
                        //    0 -Tl Td
                        context.TStar_Operator();
                        break;
                    }
                    // BT: Begin a text object
                    case PdfOperator::BT:
                    {
                        context.BeginText();
                        break;
                    }
                    // ET: End a text object
                    case PdfOperator::ET:
                    {
                        context.EndText();
                        break;
                    }
                    // font size Tf : Set the text font, T_f
                    case PdfOperator::Tf:
                    {
                        double fontSize = content->Stack[0].GetReal();
                        auto& fontName = content->Stack[1].GetName();
                        context.Tf_Operator(fontName, fontSize);
                        break;
                    }
                    // string Tj : Show a text string
                    // string '  : Move to the next line and show a text string
                    // a_w a_c " : Move to the next line and show a text string,
                    //             using a_w as the word spacing and a_c as the
                    //             character spacing
                    case PdfOperator::Tj:
                    case PdfOperator::Quote:
                    case PdfOperator::DoubleQuote:
                    {
                        ASSERT(context.BlockOpen, "No text block open");

                        auto& str = content->Stack[0].GetString();
                        if (content->Operator == PdfOperator::DoubleQuote)
                        {
                            // Operator " arguments: aw ac string "
                            context.States.Current->PdfState.CharSpacing = content->Stack[1].GetReal();
                            context.States.Current->PdfState.WordSpacing = content->Stack[2].GetReal();
                        }

                        if (decodeString(str, *context.States.Current, decoded, lengths, positions)
                            && decoded.length() != 0)
                        {
                            context.PushString(StatefulString(std::move(decoded), *context.States.Current,
                                std::move(lengths), std::move(positions)), true);
                        }

                        if (content->Operator == PdfOperator::Quote
                            || content->Operator == PdfOperator::DoubleQuote)
                        {
                            context.TStar_Operator();
                        }

                        break;
                    }
                    // array TJ : Show one or more text strings
                    case PdfOperator::TJ:
                    {
                        ASSERT(context.BlockOpen, "No text block open");

                        auto& array = content->Stack[0].GetArray();
                        for (unsigned i = 0; i < array.GetSize(); i++)
                        {
                            const PdfString* str;
                            double real;
                            auto& obj = array[i];
                            if (obj.TryGetString(str))
                            {
                                if (decodeString(*str, *context.States.Current, decoded, lengths, positions)
                                    && decoded.length() != 0)
                                {
                                    context.PushString(StatefulString(std::move(decoded), *context.States.Current,
                                        std::move(lengths), std::move(positions)));
                                }
                            }
                            else if (obj.TryGetReal(real))
                            {
                                // pg. 408, Pdf Reference 1.7: "The number is expressed in thousandths of a unit
                                // of text space. [...] This amount is subtracted from from the current horizontal or
                                // vertical coordinate, depending on the writing mode"
                                // It must be scaled by the font size
                                double space = (-real / 1000) * context.States.Current->PdfState.FontSize;
                                context.AdvanceSpace(space);
                            }
                            else
                            {
                                PoDoFo::LogMessage(PdfLogSeverity::Warning, "Invalid array object type {}", obj.GetDataTypeString());
                            }
                        }

                        context.TryPushChunk();
                        break;
                    }
                    // Tc : word spacing
                    case PdfOperator::Tc:
                    {
                        context.States.Current->PdfState.CharSpacing = content->Stack[0].GetReal();
                        break;
                    }
                    case PdfOperator::Tw:
                    {
                        context.States.Current->PdfState.WordSpacing = content->Stack[0].GetReal();
                        break;
                    }
                    // q : Save the current graphics state
                    case PdfOperator::q:
                    {
                        ASSERT(!context.BlockOpen, "Text block must be not open");
                        context.States.Push();
                        break;
                    }
                    // Q : Restore the graphics state by removing
                    // the most recently saved state from the stack
                    case PdfOperator::Q:
                    {
                        ASSERT(!context.BlockOpen, "Text block must be not open");
                        ASSERT(context.States.PopLenient(), "Save/restore must be balanced");
                        break;
                    }
                    default:
                    {
                        // Ignore all the other operators
                        break;
                    }
                }

                break;
            }
            case PdfContentType::ImageDictionary:
            case PdfContentType::ImageData:
            {
                // Ignore image data token
                break;
            }
            case PdfContentType::BeginFormXObject:
            {
                context.XObjectStateIndices.push_back({
                    (const PdfXObjectForm*)content->XObject.get(),
                    context.States.GetSize()
                    });
                context.States.Push();

                // The form XObject matrix concatenates to
                // the CTM like a 'cm' operator
                auto& matrix = content->XObject->GetMatrix();
                context.cm_Operator(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5]);
                break;
            }
            case PdfContentType::EndFormXObject:
            {
                PODOFO_ASSERT(context.XObjectStateIndices.size() != 0);
                context.States.Pop(context.States.GetSize() - context.XObjectStateIndices.back().TextStateIndex);
                context.XObjectStateIndices.pop_back();
                break;
            }
            case PdfContentType::DoXObject:
            {
                // Ignore handling of non Form XObjects
                break;
            }
            default:
            {
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Unsupported PdfContentType");
            }
        }
    }

    // After finishing processing tokens, one entry may still
    // be inside the chunks
    context.TryAddLastEntry();
}

void addEntry(vector<PdfTextEntry> &textEntries, StringChunkList &chunks, const string_view &pattern,
    const EntryOptions &options, const nullable<Rect> &clipRect, int pageIndex, const Matrix* rotation)
{
    if (options.TokenizeWords)
    {
        // Split lines into chunks separated by at char space
        // NOTE: It doesn't trim empty strings, leading and trailing,
        // white characters yet!
        vector<StringChunkListPtr> batches;
        vector<StringChunkPtr> previousWhiteChunks;
        vector<StringChunkPtr> sepratedChunks;
        auto currentBatch = std::make_unique<StringChunkList>();
        while (true)
        {
            if (chunks.size() == 0)
            {
                // Chunks analysis finished. Try to push last batch
                if (currentBatch->size() != 0)
                    batches.push_back(std::move(currentBatch));

                break;
            }

            auto chunk = std::move(chunks.front());
            chunks.pop_front();

            splitChunkBySpaces(sepratedChunks, *chunk);
            for (auto &sepratedChunk : sepratedChunks)
            {
                if (isWhiteSpaceChunk(*sepratedChunk))
                {
                    // A white space chunk is separating words. Try to push a batch
                    if (currentBatch->size() != 0)
                    {
                        batches.push_back(std::move(currentBatch));
                        currentBatch = std::make_unique<StringChunkList>();
                    }

                    previousWhiteChunks.push_back(std::move(sepratedChunk));
                }
                else
                {
                    // Reinsert previous white space chunks, they won't be trimmed yet
                    for (auto &whiteChunk : previousWhiteChunks)
                        currentBatch->push_back(std::move(whiteChunk));

                    previousWhiteChunks.clear();
                    currentBatch->push_back(std::move(sepratedChunk));
                }
            }
        }

        for (auto& batch : batches)
        {
            addEntryChunk(textEntries, *batch, pattern, options,
                clipRect, pageIndex, rotation);
        }
    }
    else
    {
        addEntryChunk(textEntries, chunks, pattern, options,
            clipRect, pageIndex, rotation);
    }
}

void addEntryChunk(vector<PdfTextEntry> &textEntries, StringChunkList &chunks, const string_view &pattern,
    const EntryOptions& options, const nullable<Rect> &clipRect, int pageIndex, const Matrix* rotation)
{
    if (options.TrimSpaces)
    {
        // Trim spaces at the begin of the string
        while (true)
        {
            if (chunks.size() == 0)
                return;

            auto &front = chunks.front();
            if (isWhiteSpaceChunk(*front))
            {
                chunks.pop_front();
                continue;
            }

            trimSpacesBegin(*front);
            break;
        }

        // Trim spaces at the end of the string
        while (true)
        {
            auto &back = chunks.back();
            if (isWhiteSpaceChunk(*back))
            {
                chunks.pop_back();
                continue;
            }

            trimSpacesEnd(*back);
            break;
        }
    }

    PODOFO_ASSERT(chunks.size() != 0);
    auto &firstChunk = *chunks.front();
    PODOFO_ASSERT(firstChunk.size() != 0);
    auto &firstStr = firstChunk.front();
    if (clipRect.has_value() && !clipRect->Contains(firstStr.Position.X, firstStr.Position.Y))
    {
        chunks.clear();
        return;
    }

    string str;
    vector<unsigned> positions;
    vector<const StatefulString*> strings;
    vector<GlyphAddress> glyphAddresses;
    processChunks(chunks, str, positions, strings, glyphAddresses);
    unsigned lowerIndex = 0;
    unsigned upperIndexLimit = (unsigned)glyphAddresses.size();
    auto textState = firstStr.State;
    if (pattern.length() != 0)
    {
        PODOFO_INVARIANT(utls::IsValidUtf8String(pattern));
        bool match;
        if (options.RegexPattern)
        {
            PODOFO_ASSERT(!(options.MatchWholeWord || options.ExtractSubstring));
            auto flags = regex_constants::ECMAScript;
            if (options.IgnoreCase)
                flags |= regex_constants::icase;

            // NOTE: regex_search returns true when a sub-part of the string
            // matches the regex
            regex pieces_regex((string)pattern, flags);
            match = std::regex_search(str, pieces_regex);
        }
        else
        {
            if (options.ExtractSubstring)
            {
                size_t pos;
                if (options.MatchWholeWord)
                {
                    if (options.IgnoreCase)
                        match = isMatchWholeWordSubstring(utls::ToLower(str), utls::ToLower(pattern), pos);
                    else
                        match = isMatchWholeWordSubstring(str, pattern, pos);
                }
                else
                {
                    if (options.IgnoreCase)
                        pos = utls::ToLower(str).find(utls::ToLower(pattern));
                    else
                        pos = str.find(pattern);
                    match = pos != string::npos;
                }

                if (match)
                {
                    getSubstringIndices(positions, (unsigned)pos, (unsigned)(pos + pattern.size()),
                        lowerIndex, upperIndexLimit);

                    // Assign actual found matched substring
                    if (pos != 0 || str.size() != pattern.size())
                        str = str.substr(pos, pattern.size());

                    if (lowerIndex != 0)
                    {
                        // Compute substring translation and apply it
                        // TODO: Handle vertical scripts
                        double substringTx = computeLength(strings, glyphAddresses, 0, lowerIndex - 1);
                        textState.T_rm.Apply<Tx>(substringTx);
                    }
                }
            }
            else
            {
                if (options.MatchWholeWord)
                {
                    if (options.IgnoreCase)
                        match = utls::ToLower(str) == utls::ToLower(pattern);
                    else
                        match = str == pattern;
                }
                else
                {
                    if (options.IgnoreCase)
                        match = utls::ToLower(str).find(utls::ToLower(pattern)) != string::npos;
                    else
                        match = str.find(pattern) != string::npos;
                }
            }
        }

        if (!match)
        {
            chunks.clear();
            return;
        }
    }

    double strLength = computeLength(strings, glyphAddresses, lowerIndex, upperIndexLimit - 1);
    nullable<Rect> bbox;
    if (options.ComputeBoundingBox)
        bbox = computeBoundingBox(textState, strLength);

    // Rotate to canonical frame
    auto strPosition = textState.T_rm.GetTranslationVector();
    if (rotation == nullptr || options.RawCoordinates)
    {
        textEntries.push_back(PdfTextEntry{ str, pageIndex,
            strPosition.X, strPosition.Y, strLength, bbox });
    }
    else
    {
        Vector2 rawp(strPosition.X, strPosition.Y);
        auto p_1 = rawp * (*rotation);
        textEntries.push_back(PdfTextEntry{ str, pageIndex,
            p_1.X, p_1.Y, strLength, bbox });
    }

    chunks.clear();
}

void read(const PdfVariantStack& tokens, double & tx, double & ty)
{
    ty = tokens[0].GetReal();
    tx = tokens[1].GetReal();
}

void read(const PdfVariantStack& tokens, double & a, double & b, double & c, double & d, double & e, double & f)
{
    f = tokens[0].GetReal();
    e = tokens[1].GetReal();
    d = tokens[2].GetReal();
    c = tokens[3].GetReal();
    b = tokens[4].GetReal();
    a = tokens[5].GetReal();
}

bool decodeString(const PdfString &str, TextState &state, string &decoded,
    vector<double>& lengths, vector<unsigned>& positions)
{
    if (state.PdfState.Font == nullptr)
    {
        if (!str.IsHex())
        {
            // As a fallback try to retrieve the raw string
            // CHECK-ME: Maybe intrepret them as PdfDocEncoding?
            decoded = str.GetString();
            lengths.resize(decoded.length());
            positions.resize(decoded.length());
            for (unsigned i = 0; i < decoded.length(); i++)
                positions[i] = i;

            return true;
        }

        decoded.clear();
        lengths.clear();
        positions.clear();
        return false;
    }

    state.ScanString(str, decoded, lengths, positions);
    return true;
}

StatefulString::StatefulString(string&& str, const TextState& state,
        vector<double>&& lengths, vector<unsigned>&& positions) :
    String(std::move(str)),
    State(state),
    RawLengths(std::move(lengths)),
    Lengths(computeLengths(RawLengths)),
    StringPositions(std::move(positions)),
    Position(state.T_rm.GetTranslationVector()),
    IsWhiteSpace(utls::IsStringEmptyOrWhiteSpace(String))
{
    PODOFO_ASSERT(String.length() != 0);
    PODOFO_ASSERT(RawLengths.size() != 0);
    PODOFO_ASSERT(StringPositions.size() != 0);
}

StatefulString StatefulString::GetTrimmedBegin() const
{
    auto &str = String;
    auto it = str.begin();
    auto end = str.end();
    auto prev = it;
    while (it != end)
    {
        char32_t cp = utf8::next(it, end);
        if (!utls::IsWhiteSpace(cp))
            break;

        prev = it;
    }

    // First, advance the x position by finding the space string size with the current font
    auto state = State;
    unsigned trimmedLen = (unsigned)(prev - str.begin());
    unsigned lowerIndex = 0;
    if (trimmedLen != 0)
    {
        auto spacestr = str.substr(0, trimmedLen);
        double length = 0;
        for (unsigned i = 0; i < StringPositions.size(); i++)
        {
            if (StringPositions[i] >= trimmedLen)
            {
                lowerIndex = i;
                break;
            }

            length += Lengths[i];
        }

        state.T_m.Apply<Tx>(length);
        state.ComputeT_rm();
    }

    // Fixed string positions after trim
    auto positions = vector<unsigned>(StringPositions.begin() + lowerIndex, StringPositions.end());
    for (unsigned i = 0; i < positions.size(); i++)
        positions[i] -= trimmedLen;

    // After, rewrite the string without spaces
    return StatefulString(str.substr(trimmedLen), state,
        { RawLengths.begin() + lowerIndex, RawLengths.end() },
        std::move(positions));
}

bool StatefulString::BeginsWithWhiteSpace() const
{
    auto& str = String;
    auto it = str.begin();
    auto end = str.end();
    while (it != end)
    {
        char32_t cp = utf8::next(it, end);
        if (utls::IsWhiteSpace(cp))
            return true;
    }

    return false;
}

bool StatefulString::EndsWithWhiteSpace() const
{
    bool isPrevWhiteSpace = false;
    auto& str = String;
    auto it = str.begin();
    auto end = str.end();
    while (it != end)
    {
        char32_t cp = utf8::next(it, end);
        isPrevWhiteSpace = utls::IsWhiteSpace(cp);
    }

    return isPrevWhiteSpace;
}

StatefulString StatefulString::GetTrimmedEnd() const
{
    string trimmedStr = utls::TrimSpacesEnd(String);
    unsigned positionIndexLimit = (unsigned)StringPositions.size();
    if (trimmedStr.length() != String.length())
    {
        for (int i = (int)StringPositions.size() - 1; i >= 0; i--)
        {
            if (StringPositions[i] < (unsigned)trimmedStr.length())
            {
                positionIndexLimit = i + 1;
                break;
            }
        }
    }
    return StatefulString(std::move(trimmedStr), State,
        { Lengths.begin(), Lengths.begin() + positionIndexLimit },
        { StringPositions.begin(), StringPositions.begin() + positionIndexLimit });
}

double StatefulString::GetLengthRaw() const
{
    double length = 0;
    for (unsigned i = 0; i < RawLengths.size(); i++)
        length += RawLengths[i];

    return length;
}

double StatefulString::GetLength() const
{
    double length = 0;
    for (unsigned i = 0; i < Lengths.size(); i++)
        length += Lengths[i];

    return length;
}

vector<double> StatefulString::computeLengths(const vector<double>& rawLengths)
{
    // NOTE: the lengths are transformed accordingly to text state but
    // are not CTM transformed
    vector<double> ret;
    ret.reserve(rawLengths.size());
    for (unsigned i = 0; i < rawLengths.size(); i++)
        ret.push_back((Vector2(rawLengths[i], 0) * State.CTM.GetScalingRotation()).GetLength());

    return ret;
}

ExtractionContext::ExtractionContext(vector<PdfTextEntry>& entries, const PdfPage& page, const string_view& pattern,
    PdfTextExtractFlags flags , const nullable<Rect>& clipRect) :
    m_page(page),
    PageIndex(page.GetPageNumber() - 1),
    Pattern(pattern),
    Options(optionsFromFlags(flags)),
    ClipRect(clipRect),
    Entries(entries)
{
    if (Options.ExtractSubstring && pattern.empty())
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Unsupported ExtractSubstring flag with empty pattern");

    // Determine page rotation transformation
    double teta;
    if (page.TryGetRotationRadians(teta))
        Rotation = std::make_unique<Matrix>(PoDoFo::GetFrameRotationTransform((Rect)page.GetRectRaw(), teta));
}

void ExtractionContext::BeginText()
{
    ASSERT(!BlockOpen, "Text block already open");

    // NOTE: BT doesn't reset font
    BlockOpen = true;
}

void ExtractionContext::EndText()
{
    ASSERT(BlockOpen, "No text block open");
    States.Current->T_m = Matrix();
    States.Current->T_lm = Matrix();
    States.Current->ComputeDependentState();
    BlockOpen = false;
}

void ExtractionContext::Tf_Operator(const PdfName &fontname, double fontsize)
{
    auto resources = getActualCanvas().GetResources();
    double spacingLengthRaw = 0;
    double spaceCharLengthRaw = 0;
    States.Current->PdfState.FontSize = fontsize;
    if (resources == nullptr || (States.Current->PdfState.Font = resources->GetFont(fontname)) == nullptr)
    {
        PoDoFo::LogMessage(PdfLogSeverity::Warning, "Unable to find font object {}", fontname.GetString());
    }
    else
    {
        spacingLengthRaw = States.Current->GetWordSpacingLength();
        spaceCharLengthRaw = States.Current->GetSpaceCharLength();
    }

    States.Current->WordSpacingVectorRaw = Vector2(spacingLengthRaw, 0);
    if (spacingLengthRaw == 0)
    {
        PoDoFo::LogMessage(PdfLogSeverity::Warning, "Unable to provide a word spacing length, setting default font size");
        States.Current->WordSpacingVectorRaw = Vector2(fontsize, 0);
    }

    States.Current->SpaceCharVectorRaw = Vector2(spaceCharLengthRaw, 0);
    if (spaceCharLengthRaw == 0)
    {
        PoDoFo::LogMessage(PdfLogSeverity::Warning, "Unable to provide a space char length, setting default font size");
        States.Current->SpaceCharVectorRaw = Vector2(fontsize, 0);
    }

    States.Current->ComputeSpaceDescriptors();
}

void ExtractionContext::cm_Operator(double a, double b, double c, double d, double e, double f)
{
    // TABLE 4.7: "cm" Modify the current transformation
    // matrix (CTM) by concatenating the specified matrix
    Matrix cm(a, b, c, d, e, f);
    States.Current->CTM = cm * States.Current->CTM;
    States.Current->ComputeT_rm();
}

void ExtractionContext::Tm_Operator(double a, double b, double c, double d, double e, double f)
{
    States.Current->T_lm = Matrix(a, b, c, d, e, f);
    States.Current->T_m = States.Current->T_lm;
    States.Current->ComputeDependentState();
}

void ExtractionContext::TdTD_Operator(double tx, double ty)
{
    // 5.5 Text-positioning operators, Td/TD
    States.Current->T_lm = States.Current->T_lm.Translate(Vector2(tx, ty));
    States.Current->T_m = States.Current->T_lm;
    States.Current->ComputeDependentState();
}

void ExtractionContext::TStar_Operator()
{
    States.Current->T_lm.Apply<Ty>(-States.Current->T_l);
    States.Current->T_m = States.Current->T_lm;
    States.Current->ComputeDependentState();
}

void ExtractionContext::AdvanceSpace(double tx)
{
    States.Current->T_m.Apply<Tx>(tx);
    States.Current->ComputeDependentState();
}

void ExtractionContext::PushString(const StatefulString &str, bool pushchunk)
{
    PODOFO_ASSERT(str.String.length() != 0);
    if (std::isnan(CurrentEntryT_rm_y))
    {
        // Initialize tracking for line
        CurrentEntryT_rm_y = States.Current->T_rm.Get<Ty>();
    }

    tryAddEntry(str);

    // Set current line tracking
    CurrentEntryT_rm_y = States.Current->T_rm.Get<Ty>();
    Chunk->push_back(str);
    if (pushchunk)
        pushChunk();

    States.Current->T_m.Apply<Tx>(str.GetLengthRaw());
    States.Current->ComputeT_rm();
    PrevChunkT_rm_Pos = States.Current->T_rm.GetTranslationVector();
}

void ExtractionContext::TryPushChunk()
{
    if (Chunk->size() == 0)
        return;

    pushChunk();
}

void ExtractionContext::pushChunk()
{
    Chunks.push_back(std::move(Chunk));
    Chunk = std::make_unique<StringChunk>();
}

void ExtractionContext::TryAddLastEntry()
{
    if (Chunks.size() > 0)
        addEntry();
}

const PdfCanvas& ExtractionContext::getActualCanvas()
{
    if (XObjectStateIndices.size() == 0)
        return m_page;

    return *XObjectStateIndices.back().Form;
}

const StatefulString& ExtractionContext::getPreviouString() const
{
    const StatefulString* prevString;
    if (Chunk->size() > 0)
    {
        prevString = &Chunk->back();
    }
    else
    {
        PODOFO_INVARIANT(Chunks.back()->size() != 0);
        prevString = &Chunks.back()->back();
    }

    return *prevString;
}

void ExtractionContext::addEntry()
{
    ::addEntry(Entries, Chunks, Pattern, Options, ClipRect, PageIndex, Rotation.get());
}

void ExtractionContext::tryAddEntry(const StatefulString& currStr)
{
    PODOFO_INVARIANT(Chunk != nullptr);
    if (Chunks.size() > 0 || Chunk->size() > 0)
    {
        if (areEqual(States.Current->T_rm.Get<Ty>(), CurrentEntryT_rm_y))
        {
            double distance;
            if (areChunksSpaced(distance))
            {
                if (Options.TokenizeWords
                    || distance + SEPARATION_EPSILON >
                        States.Current->CharSpaceLength * HARD_SEPARATION_SPACING_MULTIPLIER)
                {
                    // Current entry is space separated and either we
                    //  tokenize words, or it's an hard entry separation
                    TryPushChunk();
                    addEntry();
                }
                else
                {
                    // Add "fake" space
                    auto& prevString = getPreviouString();
                    if (!(prevString.EndsWithWhiteSpace() || currStr.BeginsWithWhiteSpace()))
                        Chunk->push_back(StatefulString(" ", prevString.State, { distance }, { 0 }));
                }
            }
        }
        else
        {
            // Current entry is not on same line
            TryPushChunk();
            addEntry();
        }
    }
}

bool ExtractionContext::areChunksSpaced(double& distance)
{
    // TODO
    // 1) Handle the word spacing Tw state
    // 2) Handle the char spacing Tc state (is it actually needed?)
    // 3) Handle arbitrary rotations
    // 4) Handle vertical scripts (HARD)
    // 5) Try to avoid computing GetLength() and use only dot product
    auto curr = States.Current->T_rm.GetTranslationVector();
    auto prev_curr = curr - PrevChunkT_rm_Pos;
    distance = prev_curr.GetLength();
    double dot1 = prev_curr.Dot(Vector2(1, 0)); // Hardcoded for horizontal text
    bool spaced = distance + SEPARATION_EPSILON >= States.Current->WordSpacingLength;
    if (dot1 < 0 && spaced)
    {
        auto& prevString = getPreviouString();
        auto prev_init = prevString.Position - PrevChunkT_rm_Pos;
        double dot2 = prev_init.Dot(Vector2(1, 0));
        return dot1 < dot2;
    }

    return spaced;
}

// Separate chunk words by spaces
void splitChunkBySpaces(vector<StringChunkPtr> &splittedChunks, const StringChunk &chunk)
{
    PODOFO_ASSERT(chunk.size() != 0);
    splittedChunks.clear();

    vector<StatefulString> separatedStrings;
    for (auto &str : chunk)
    {
        auto separatedChunk = std::make_unique<StringChunk>();
        splitStringBySpaces(separatedStrings, str);
        bool previousWhiteSpace = true;
        for (auto &separatedStr : separatedStrings)
        {
            if (separatedChunk->size() != 0 && separatedStr.IsWhiteSpace != previousWhiteSpace)
            {
                splittedChunks.push_back(std::move(separatedChunk));
                separatedChunk = std::make_unique<StringChunk>();
            }

            separatedChunk->push_back(separatedStr);
            previousWhiteSpace = separatedStr.IsWhiteSpace;
        }

        // Push back last chunk, if present
        if (separatedChunk->size() != 0)
            splittedChunks.push_back(std::move(separatedChunk));
    }
}

// Separate string words by spaces
void splitStringBySpaces(vector<StatefulString>& separatedStrings, const StatefulString& str)
{
    PODOFO_ASSERT(str.String.length() != 0);
    separatedStrings.clear();

    string separatedStr;
    auto state = str.State;
    unsigned previousPos = 0;
    unsigned lowerPos = previousPos;
    unsigned upperPosLim = (unsigned)str.String.length();
    unsigned lowerPosIndex;
    unsigned upperPosLimIndex;

    auto pushString = [&]() {
        getSubstringIndices(str.StringPositions, lowerPos, upperPosLim, lowerPosIndex, upperPosLimIndex);
        double length = 0;
        for (unsigned i = lowerPosIndex; i < upperPosLimIndex; i++)
            length += str.Lengths[i];

        // Fixed string positions after split
        auto positions = vector<unsigned>(str.StringPositions.begin() + lowerPosIndex, str.StringPositions.begin() + upperPosLimIndex);
        for (unsigned i = 0; i < positions.size(); i++)
            positions[i] -= lowerPos;

        separatedStrings.push_back(StatefulString(std::move(separatedStr), state,
            { str.Lengths.begin() + lowerPosIndex, str.Lengths.begin() + upperPosLimIndex },
            std::move(positions)));
        lowerPos = previousPos;
        upperPosLim = (unsigned)str.String.length();

        state.T_m.Apply<Tx>(length);
        state.ComputeT_rm();
    };

    // NOTE: This function shall NOT trim spaces, just split
    bool isPreviousWhiteSpace = true;
    auto it = str.String.begin();
    auto end = str.String.end();
    do
    {
        char32_t cp = utf8::next(it, end);
        bool isCurrentWhiteSpace = utls::IsWhiteSpace(cp);
        if (separatedStr.length() != 0 && isCurrentWhiteSpace != isPreviousWhiteSpace)
        {
            upperPosLim = previousPos;
            pushString();
        }

        utf8::unchecked::append((uint32_t)cp, std::back_inserter(separatedStr));

        previousPos = (unsigned)(it - str.String.begin());
        isPreviousWhiteSpace = isCurrentWhiteSpace;
    } while (it != end);

    // Push back last string, if present
    if (separatedStr.length() != 0)
        pushString();
}

void trimSpacesBegin(StringChunk &chunk)
{
    while (true)
    {
        if (chunk.size() == 0)
            break;

        auto &front = chunk.front();
        if (!front.IsWhiteSpace)
        {
            auto trimmed = front.GetTrimmedBegin();

            chunk.pop_front();
            chunk.push_front(trimmed);
            break;
        }

        chunk.pop_front();
    }
}

void trimSpacesEnd(StringChunk &chunk)
{
    while (true)
    {
        if (chunk.size() == 0)
            break;

        auto &back = chunk.back();
        if (!back.IsWhiteSpace)
        {
            auto trimmed = back.GetTrimmedEnd();
            chunk.pop_back();
            chunk.push_back(trimmed);
            break;
        }

        chunk.pop_back();
    }
}

bool isWhiteSpaceChunk(const StringChunk &chunk)
{
    for (auto &str : chunk)
    {
        if (!str.IsWhiteSpace)
            return false;
    }

    return true;
}

bool areEqual(double lhs, double rhs)
{
    return std::abs(lhs - rhs) < SAME_LINE_THRESHOLD;
}

void TextState::ComputeDependentState()
{
    ComputeSpaceDescriptors();
    ComputeT_rm();
}

void TextState::ComputeSpaceDescriptors()
{
    WordSpacingLength = (WordSpacingVectorRaw * T_m.GetScalingRotation()).GetLength();
    CharSpaceLength = (SpaceCharVectorRaw * T_m.GetScalingRotation()).GetLength();
}

void TextState::ComputeT_rm()
{
    T_rm = T_m * CTM;
}

double TextState::GetWordSpacingLength() const
{
    return PdfState.Font->GetWordSpacingLength(PdfState);
}

double TextState::GetSpaceCharLength() const
{
    return PdfState.Font->GetSpaceCharLength(PdfState);
}

void TextState::ScanString(const PdfString& encodedStr, string& decoded, vector<double>& lengths, vector<unsigned>& positions)
{
    (void)PdfState.Font->TryScanEncodedString(encodedStr, PdfState, decoded, lengths, positions);
}

// Concatenate all strings, lengths and string positions
void processChunks(const StringChunkList& chunks, string& destString,
    vector<unsigned>& positions, vector<const StatefulString*>& strings,
    vector<GlyphAddress>& glyphAddresses)
{
    unsigned offsetPosition = 0;
    unsigned stringIndex;
    for (auto& chunk : chunks)
    {
        for (auto& str : *chunk)
        {
            destString.append(str.String.data(), str.String.length());
            stringIndex = (unsigned)strings.size();
            strings.push_back(&str);
            for (unsigned i = 0; i < str.StringPositions.size(); i++)
            {
                glyphAddresses.push_back(GlyphAddress{ stringIndex, i });
                positions.push_back(offsetPosition + str.StringPositions[i]);
            }

            offsetPosition += (unsigned)str.String.length();
        }
    }
}

// TODO: Handle vertical scripts
double computeLength(const vector<const StatefulString*>& strings, const vector<GlyphAddress>& glyphAddresses,
    unsigned lowerIndex, unsigned upperIndex)
{
    PODOFO_ASSERT(lowerIndex <= upperIndex);
    auto& fromAddr = glyphAddresses[lowerIndex];
    auto& toAddr = glyphAddresses[upperIndex];
    if (fromAddr.StringIndex == toAddr.StringIndex)
    {
        // NOTE: Include the last glyph
        auto str = strings[fromAddr.StringIndex];
        double length = 0;
        for (unsigned i = 0; i <= toAddr.GlyphIndex; i++)
            length += str->Lengths[i];

        return length;
    }
    else
    {
        auto fromStr = strings[fromAddr.StringIndex];
        auto toStr = strings[toAddr.StringIndex];

        // Advance the position before the first glyph
        auto fromPosition = fromStr->Position;
        for (unsigned i = 0; i < fromAddr.GlyphIndex; i++)
            fromPosition += Vector2(fromStr->Lengths[i], 0);

        // NOTE: Include the last glyph
        auto toPosition = toStr->Position;
        for (unsigned i = 0; i <= toAddr.GlyphIndex; i++)
            toPosition += Vector2(toStr->Lengths[i], 0);

        return (fromPosition - toPosition).GetLength();
    }
}

// Verify if the string matches the pattern and verify
// presence of delimiters for whole word match
bool isMatchWholeWordSubstring(const string_view& str, const string_view& pattern, size_t& matchPos)
{
    bool prevDelimiter;
    auto found = str.find(pattern);
    string_view::const_iterator it;
    string_view::const_iterator end;
    char32_t cp;
    if (found == string_view::npos)
        goto NoMatch;

    it = str.begin();
    end = str.begin() + found;

    prevDelimiter = true;
    while (it != end)
    {
        cp = utf8::unchecked::next(it);
        prevDelimiter = utls::IsStringDelimiter(cp);
    }

    if (!prevDelimiter)
        goto NoMatch;

    it = str.begin() + found + pattern.length();
    end = str.end();
    if (it != end)
    {
        cp = utf8::unchecked::next(it);
        if (!utls::IsStringDelimiter(cp))
            goto NoMatch;
    }

    matchPos = found;
    return true;

NoMatch:
    matchPos = string_view::npos;
    return false;
}

Rect computeBoundingBox(const TextState& textState, double boxWidth)
{
    // NOTE: This is very inaccurate
    // TODO1: Handle multiple text/pdf states
    // TODO2: Handle actual font glyphs (HARD)
    // TODO3: Handle vertical scripts
    double descend = 0;
    double ascent = 0;
    auto& pdfState = textState.PdfState;
    auto font = pdfState.Font;
    auto transform = textState.T_rm.GetScalingRotation();
    if (font != nullptr)
    {
        descend = (Vector2(0, font->GetMetrics().GetDescent() * pdfState.FontSize * pdfState.FontScale)
            * transform).GetLength();
        ascent = (Vector2(0, font->GetMetrics().GetAscent() * pdfState.FontSize * pdfState.FontScale)
            * transform).GetLength();
    }

    auto position = textState.T_rm.GetTranslationVector();
    return Rect(position.X, position.Y - descend, boxWidth, descend + ascent);
}

void getSubstringIndices(const vector<unsigned>& positions, unsigned lowerPos, unsigned upperPosLim,
    unsigned& lowerPosIndex, unsigned& upperPosLimIndex)
{
    // CHECK-ME: Evaluate optimize with bisection as positions is sorted
    lowerPosIndex = std::numeric_limits<unsigned>::max();
    upperPosLimIndex = 0;
    for (unsigned i = 0; i < positions.size(); i++)
    {
        if (positions[i] >= lowerPos)
        {
            lowerPosIndex = i;
            break;
        }
    }

    for (int i = (int)positions.size() - 1; i >= 0; i--)
    {
        if (positions[i] < upperPosLim)
        {
            upperPosLimIndex = i + 1;
            break;
        }
    }
}

EntryOptions optionsFromFlags(PdfTextExtractFlags flags)
{
    EntryOptions ret;
    ret.IgnoreCase = (flags & PdfTextExtractFlags::IgnoreCase) != PdfTextExtractFlags::None;
    ret.MatchWholeWord = (flags & PdfTextExtractFlags::MatchWholeWord) != PdfTextExtractFlags::None;
    ret.RegexPattern = (flags & PdfTextExtractFlags::RegexPattern) != PdfTextExtractFlags::None;
    ret.TokenizeWords = (flags & PdfTextExtractFlags::TokenizeWords) != PdfTextExtractFlags::None;
    ret.TrimSpaces = (flags & PdfTextExtractFlags::KeepWhiteTokens) == PdfTextExtractFlags::None || ret.TokenizeWords;
    ret.ComputeBoundingBox = (flags & PdfTextExtractFlags::ComputeBoundingBox) != PdfTextExtractFlags::None;
    ret.RawCoordinates = (flags & PdfTextExtractFlags::RawCoordinates) != PdfTextExtractFlags::None;
    ret.ExtractSubstring = (flags & PdfTextExtractFlags::ExtractSubstring) != PdfTextExtractFlags::None;

    if (ret.RegexPattern)
    {
        if (ret.MatchWholeWord)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "RegexPattern is incompatible with MatchWholeWord flag");

        if (ret.ExtractSubstring)
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "RegexPattern is currently unsupported with ExtractSubstring");
    }

    return ret;
}
