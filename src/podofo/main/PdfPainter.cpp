/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPainter.h"

#include <utfcpp/utf8.h>

#include "PdfExtGState.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfImage.h"

using namespace std;
using namespace PoDoFo;

constexpr unsigned BEZIER_POINTS = 13;

// 4/3 * (1-cos 45<-,A0<-(B)/sin 45<-,A0<-(B = 4/3 * sqrt(2) - 1
constexpr double ARC_MAGIC = 0.552284749;

static void convertRectToBezier(double x, double y, double width, double height, double pointsX[], double pointsY[]);
static string expandTabs(const string_view& str, unsigned tabWidth, unsigned tabCount);

// TODO: Remove me after cleaning getMultiLineTextAsLines()
inline bool IsNewLineChar(char32_t ch)
{
    return ch == U'\n' || ch == U'\r';
}

// TODO: Remove me after cleaning getMultiLineTextAsLines()
inline bool IsSpaceChar(char32_t ch)
{
    if (ch > 255)
        return false;

    return std::isspace((int)ch) != 0;
}

PdfPainter::PdfPainter(PdfPainterFlags flags) :
    m_flags(flags),
    m_painterStatus(StatusDefault),
    m_textStackCount(0),
    GraphicsState(*this, m_StateStack.Current->GraphicsState),
    TextState(*this, m_StateStack.Current->TextState),
    Path(*this),
    Text(*this),
    m_objStream(nullptr),
    m_canvas(nullptr),
    m_TabWidth(4)
{
}

PdfPainter::~PdfPainter() noexcept(false)
{
    try
    {
        finishDrawing();
    }
    catch (...)
    {
        if (!std::uncaught_exceptions())
            throw;
    }
}

void PdfPainter::SetCanvas(PdfCanvas& canvas)
{
    // Ignore setting the same canvas twice
    if (m_canvas == &canvas)
        return;

    finishDrawing();
    reset();
    m_canvas = &canvas;
    m_objStream = nullptr;
}

void PdfPainter::FinishDrawing()
{
    finishDrawing();
    reset();
}

void PdfPainter::finishDrawing()
{
    if (m_subPaths.size() != 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "{} paths are unbalanced. Call painter.Path.End()", m_subPaths.size());

    if (m_textStackCount != 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "{} text objects are unbalanced. Call painter.Text.End()", m_textStackCount);

    if (m_objStream != nullptr)
    {
        PdfObjectOutputStream output;
        if ((m_flags & PdfPainterFlags::NoSaveRestorePrior) == PdfPainterFlags::NoSaveRestorePrior)
        {
            // GetLength() must be called before BeginAppend()
            if (m_objStream->GetLength() == 0)
            {
                output = m_objStream->GetOutputStream();
            }
            else
            {
                output = m_objStream->GetOutputStream();
                // there is already content here - so let's assume we are appending
                // as such, we MUST put in a "space" to separate whatever we do.
                output.Write("\n");
            }
        }
        else
        {
            charbuff buffer;
            if (m_objStream->GetLength() != 0)
                m_objStream->CopyTo(buffer);

            if (buffer.size() == 0)
            {
                output = m_objStream->GetOutputStream();
            }
            else
            {
                output = m_objStream->GetOutputStream(true);
                output.Write("q\n");
                output.Write(buffer);
                output.Write("Q\n");
            }
        }

        if ((m_flags & PdfPainterFlags::NoSaveRestore) == PdfPainterFlags::NoSaveRestore)
        {
            output.Write(m_stream.GetString());
        }
        else
        {
            output.Write("q\n");
            output.Write(m_stream.GetString());
            output.Write("Q\n");
        }
    }
}


void PdfPainter::reset()
{
    m_StateStack.Clear();
    m_stream.Clear();
    m_painterStatus = StatusDefault;
    m_objStream = nullptr;
    m_canvas = nullptr;
}

void PdfPainter::SetStrokingShadingPattern(const PdfShadingPattern& pattern)
{
    checkStream();
    checkStatus(StatusDefault);
    this->addToPageResources("Pattern", pattern.GetIdentifier(), pattern.GetObject());
    m_stream << "/Pattern CS /" << pattern.GetIdentifier().GetString() << " SCN" << endl;
}

void PdfPainter::SetShadingPattern(const PdfShadingPattern& pattern)
{
    checkStream();
    checkStatus(StatusDefault);
    this->addToPageResources("Pattern", pattern.GetIdentifier(), pattern.GetObject());
    m_stream << "/Pattern cs /" << pattern.GetIdentifier().GetString() << " scn" << endl;
}

void PdfPainter::SetStrokingTilingPattern(const PdfTilingPattern& pattern)
{
    checkStream();
    checkStatus(StatusDefault);
    this->addToPageResources("Pattern", pattern.GetIdentifier(), pattern.GetObject());
    m_stream << "/Pattern CS /" << pattern.GetIdentifier().GetString() << " SCN" << endl;
}

void PdfPainter::SetStrokingTilingPattern(const string_view& patternName)
{
    checkStream();
    checkStatus(StatusDefault);
    m_stream << "/Pattern CS /" << patternName << " SCN" << endl;
}

void PdfPainter::SetTilingPattern(const PdfTilingPattern& pattern)
{
    checkStream();
    checkStatus(StatusDefault);
    this->addToPageResources("Pattern", pattern.GetIdentifier(), pattern.GetObject());
    m_stream << "/Pattern cs /" << pattern.GetIdentifier().GetString() << " scn" << endl;
}

void PdfPainter::SetTilingPattern(const string_view& patternName)
{
    checkStream();
    checkStatus(StatusDefault);
    m_stream << "/Pattern cs /" << patternName << " scn" << endl;
}

void PdfPainter::SetStrokeStyle(PdfStrokeStyle strokeStyle, const string_view& custom, bool inverted, double scale, bool subtractJoinCap)
{
    bool have = false;
    checkStream();
    checkStatus(StatusDefault);

    if (strokeStyle != PdfStrokeStyle::Custom)
        m_stream << "[";

    if (inverted && strokeStyle != PdfStrokeStyle::Solid && strokeStyle != PdfStrokeStyle::Custom)
        m_stream << "0 ";

    switch (strokeStyle)
    {
        case PdfStrokeStyle::Solid:
        {
            have = true;
            break;
        }
        case PdfStrokeStyle::Dash:
        {
            have = true;
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5)
            {
                m_stream << "6 2";
            }
            else
            {
                if (subtractJoinCap)
                    m_stream << scale * 2.0 << " " << scale * 2.0;
                else
                    m_stream << scale * 3.0 << " " << scale * 1.0;
            }
            break;
        }
        case PdfStrokeStyle::Dot:
        {
            have = true;
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5)
            {
                m_stream << "2 2";
            }
            else
            {
                if (subtractJoinCap)
                {
                    // zero length segments are drawn anyway here
                    m_stream << 0.001 << " " << 2.0 * scale << " " << 0 << " " << 2.0 * scale;
                }
                else
                {
                    m_stream << scale * 1.0 << " " << scale * 1.0;
                }
            }
            break;
        }
        case PdfStrokeStyle::DashDot:
        {
            have = true;
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5)
            {
                m_stream << "3 2 1 2";
            }
            else
            {
                if (subtractJoinCap)
                {
                    // zero length segments are drawn anyway here
                    m_stream << scale * 2.0 << " " << scale * 2.0 << " " << 0 << " " << scale * 2.0;
                }
                else
                {
                    m_stream << scale * 3.0 << " " << scale * 1.0 << " " << scale * 1.0 << " " << scale * 1.0;
                }
            }
            break;
        }
        case PdfStrokeStyle::DashDotDot:
        {
            have = true;
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5)
            {
                m_stream << "3 1 1 1 1 1";
            }
            else
            {
                if (subtractJoinCap)
                {
                    // zero length segments are drawn anyway here
                    m_stream << scale * 2.0 << " " << scale * 2.0 << " " << 0 << " " << scale * 2.0 << " " << 0 << " " << scale * 2.0;
                }
                else {
                    m_stream << scale * 3.0 << " " << scale * 1.0 << " " << scale * 1.0 << " " << scale * 1.0 << " " << scale * 1.0 << " " << scale * 1.0;
                }
            }
            break;
        }
        case PdfStrokeStyle::Custom:
        {
            have = !custom.empty();
            if (have)
                m_stream << custom;
            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidStrokeStyle);
        }
    }

    if (!have)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidStrokeStyle);

    if (inverted && strokeStyle != PdfStrokeStyle::Solid && strokeStyle != PdfStrokeStyle::Custom)
        m_stream << " 0";

    if (strokeStyle != PdfStrokeStyle::Custom)
        m_stream << "] 0";

    m_stream << " d" << endl;
}

void PdfPainter::SetClipRect(const PdfRect& rect)
{
    this->SetClipRect(rect.GetLeft(), rect.GetBottom(), rect.GetWidth(), rect.GetHeight());
}

void PdfPainter::SetClipRect(double x, double y, double width, double height)
{
    checkStream();
    checkStatus(StatusDefault);
    m_stream    << x     << " "  << y      << " "
                << width << " "  << height << " re W n" << endl;
}

void PdfPainter::DrawLine(double x1, double y1, double x2, double y2)
{
    checkStream();
    checkStatus(StatusDefault);
    pathMoveTo(x1, y1);
    addLineTo(x2, y2);
    stroke();
}

void PdfPainter::DrawCubicBezier(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4)
{
    checkStream();
    checkStatus(StatusDefault);
    pathMoveTo(x1, y1);
    addCubicBezierTo(x2, y2, x3, y3, x4, y4);
    stroke();
}

void PdfPainter::DrawArc(double x, double y, double radius, double angle1, double angle2, bool counterclockwise)
{
    checkStream();
    checkStatus(StatusDefault);
    addArc(x, y, radius, angle1, angle2, counterclockwise);
    stroke();
}

void PdfPainter::DrawCircle(double x, double y, double radius, PdfPathDrawMode mode)
{
    checkStream();
    checkStatus(StatusDefault);
    addCircle(x, y, radius);
    drawPath(mode);
}

void PdfPainter::DrawEllipse(double x, double y, double width, double height, PdfPathDrawMode mode)
{
    checkStream();
    checkStatus(StatusDefault);
    addEllipse(x, y, width, height);
    drawPath(mode);
}

void PdfPainter::DrawRectangle(double x, double y, double width, double height, PdfPathDrawMode mode, double roundX, double roundY)
{
    checkStream();
    checkStatus(StatusDefault);
    addRectangle(x, y, width, height, roundX, roundY);
    drawPath(mode);
}

void PdfPainter::DrawRectangle(const PdfRect& rect, PdfPathDrawMode mode, double roundX, double roundY)
{
    checkStream();
    checkStatus(StatusDefault);
    addRectangle(rect.GetLeft(), rect.GetBottom(),
        rect.GetWidth(), rect.GetHeight(), roundX, roundY);
    drawPath(mode);
}

void PdfPainter::DrawText(const string_view& str, double x, double y,
    PdfDrawTextStyle style)
{
    checkStream();
    checkStatus(StatusDefault);
    checkFont();

    m_stream << "BT" << endl;
    writeTextState();
    drawText(str, x, y,
        (style & PdfDrawTextStyle::Underline) != PdfDrawTextStyle::Regular,
        (style & PdfDrawTextStyle::StrikeOut) != PdfDrawTextStyle::Regular);
    m_stream << "ET" << endl;
}

void PdfPainter::drawText(const string_view& str, double x, double y, bool isUnderline, bool isStrikeOut)
{
    m_stream << x << " " << y <<  " Td ";

    auto& textState = m_StateStack.Current->TextState;
    auto& font = *textState.Font;
    auto expStr = this->expandTabs(str);

    if (isUnderline || isStrikeOut)
    {
        this->save();

        // Draw underline
        this->setLineWidth(font.GetUnderlineThickness(textState));
        if (isUnderline)
        {
            this->DrawLine(x,
                y + font.GetUnderlinePosition(textState),
                x + font.GetStringLength(expStr, textState),
                y + font.GetUnderlinePosition(textState));
        }

        // Draw strikeout
        this->setLineWidth(font.GetStrikeOutThickness(textState));
        if (isStrikeOut)
        {
            this->DrawLine(x,
                y + font.GetStrikeOutPosition(textState),
                x + font.GetStringLength(expStr, textState),
                y + font.GetStrikeOutPosition(textState));
        }

        this->restore();
    }

    font.WriteStringToStream(m_stream, expStr);
    m_stream << " Tj" << endl;
}

void PdfPainter::DrawTextMultiLine(const string_view& str, const PdfRect& rect,
    const PdfDrawTextMultiLineParams& params)
{
    this->DrawTextMultiLine(str, rect.GetLeft(), rect.GetBottom(), rect.GetWidth(), rect.GetHeight(),
        params);
}

void PdfPainter::DrawTextMultiLine(const string_view& str, double x, double y, double width, double height,
    const PdfDrawTextMultiLineParams& params)
{
    checkStream();
    checkStatus(StatusDefault | StatusText);
    checkFont();

    if (width <= 0.0 || height <= 0.0) // nonsense arguments
        return;

    m_stream << "BT" << endl;
    writeTextState();
    drawMultiLineText(str, x, y, width, height,
        params.HorizontalAlignment, params.VerticalAlignment,
        params.Clip, params.SkipSpaces, params.Style);
    m_stream << "ET" << endl;
}

void PdfPainter::DrawTextAligned(const string_view& str, double x, double y, double width,
    PdfHorizontalAlignment hAlignment, PdfDrawTextStyle style)
{
    if (width <= 0.0) // nonsense arguments
        return;

    checkStream();
    checkStatus(StatusDefault | StatusText);
    checkFont();

    m_stream << "BT" << endl;
    writeTextState();
    drawTextAligned(str, x, y, width, hAlignment, style);
    m_stream << "ET" << endl;
}

void PdfPainter::drawMultiLineText(const string_view& str, double x, double y, double width, double height,
    PdfHorizontalAlignment hAlignment, PdfVerticalAlignment vAlignment, bool clip, bool skipSpaces,
    PdfDrawTextStyle style)
{
    auto& textState = m_StateStack.Current->TextState;
    auto& font = *textState.Font;

    this->save();
    if (clip)
        this->SetClipRect(x, y, width, height);

    auto expanded = this->expandTabs(str);

    vector<string> lines = getMultiLineTextAsLines(expanded, width, skipSpaces);
    double dLineGap = font.GetLineSpacing(textState) - font.GetAscent(textState) + font.GetDescent(textState);
    // Do vertical alignment
    switch (vAlignment)
    {
        default:
        case PdfVerticalAlignment::Top:
            y += height;
            break;
        case PdfVerticalAlignment::Bottom:
            y += font.GetLineSpacing(textState) * lines.size();
            break;
        case PdfVerticalAlignment::Center:
            y += (height -
                ((height - (font.GetLineSpacing(textState) * lines.size())) / 2.0));
            break;
    }

    y -= (font.GetAscent(textState) + dLineGap / (2.0));

    vector<string>::const_iterator it = lines.begin();
    while (it != lines.end())
    {
        if (it->length() != 0)
            this->drawTextAligned(*it, x, y, width, hAlignment, style);

        y -= font.GetLineSpacing(textState);
        it++;
    }
    this->restore();
}

// FIX-ME/CLEAN-ME: The following was found
// in this deprecable state while cleaning the code
vector<string> PdfPainter::getMultiLineTextAsLines(const string_view& str, double width, bool skipSpaces)
{
    (void)str;
    (void)width;
    (void)skipSpaces;

    // FIX-ME: This method is currently broken, just rewrite it later
    vector<string> ret = { (string)str };
    return ret;
    /*
    if (width <= 0.0) // nonsense arguments
        return vector<string>();

    if (str.length() == 0) // empty string
        return vector<string>(1, (string)str);

    auto& textState = m_StateStack.Current->TextState;
    auto& font = *textState.Font;
    const char* const stringBegin = str.data();
    const char* lineBegin = stringBegin;
    const char* currentCharacter = stringBegin;
    const char* startOfCurrentWord = stringBegin;
    bool startOfWord = true;
    double curWidthOfLine = 0.0;
    vector<string> lines;

    // do simple word wrapping
    while (*currentCharacter)
    {
        if (IsNewLineChar(*currentCharacter)) // hard-break!
        {
            lines.push_back(string(lineBegin, (size_t)(currentCharacter - lineBegin)));

            lineBegin = currentCharacter + 1; // skip the line feed
            startOfWord = true;
            curWidthOfLine = 0.0;
        }
        else if (IsSpaceChar(*currentCharacter))
        {
            if (curWidthOfLine > width)
            {
                // The previous word does not fit in the current line.
                // -> Move it to the next one.
                if (startOfCurrentWord > lineBegin)
                {
                    lines.push_back(string(lineBegin, (size_t)(startOfCurrentWord - lineBegin)));
                }
                else
                {
                    lines.push_back(string(lineBegin, (size_t)(currentCharacter - lineBegin)));
                    if (skipSpaces)
                    {
                        // Skip all spaces at the end of the line
                        while (IsSpaceChar(*(currentCharacter + 1)))
                            currentCharacter++;

                        startOfCurrentWord = currentCharacter + 1;
                    }
                    else
                    {
                        startOfCurrentWord = currentCharacter;
                    }
                    startOfWord = true;
                }
                lineBegin = startOfCurrentWord;

                if (!startOfWord)
                {
                    curWidthOfLine = font.GetStringLength(
                        { startOfCurrentWord, (size_t)(currentCharacter - startOfCurrentWord) },
                        textState);
                }
                else
                {
                    curWidthOfLine = 0.0;
                }
            }
            ////else if( ( dCurWidthOfLine + font.GetCharWidth( *currentCharacter, textState) ) > width )
            {
                lines.push_back(string(lineBegin, (size_t)(currentCharacter - lineBegin)));
                if (skipSpaces)
                {
                    // Skip all spaces at the end of the line
                    while (IsSpaceChar(*(currentCharacter + 1)))
                        currentCharacter++;

                    startOfCurrentWord = currentCharacter + 1;
                }
                else
                {
                    startOfCurrentWord = currentCharacter;
                }
                lineBegin = startOfCurrentWord;
                startOfWord = true;
                curWidthOfLine = 0.0;
            }
            ////else
            {
                ////    dCurWidthOfLine += font.GetCharWidth( *currentCharacter, textState);
            }

            startOfWord = true;
        }
        else
        {
            if (startOfWord)
            {
                startOfCurrentWord = currentCharacter;
                startOfWord = false;
            }
            //else do nothing

            ////if ((dCurWidthOfLine + font.GetCharWidth(*currentCharacter, textState)) > width)
            {
                if (lineBegin == startOfCurrentWord)
                {
                    // This word takes up the whole line.
                    // Put as much as possible on this line.
                    if (lineBegin == currentCharacter)
                    {
                        lines.push_back(string(currentCharacter, 1));
                        lineBegin = currentCharacter + 1;
                        startOfCurrentWord = currentCharacter + 1;
                        curWidthOfLine = 0;
                    }
                    else
                    {
                        lines.push_back(string(lineBegin, (size_t)(currentCharacter - lineBegin)));
                        lineBegin = currentCharacter;
                        startOfCurrentWord = currentCharacter;
                        ////dCurWidthOfLine = font.GetCharWidth(*currentCharacter, textState);
                    }
                }
                else
                {
                    // The current word does not fit in the current line.
                    // -> Move it to the next one.
                    lines.push_back(string(lineBegin, (size_t)(startOfCurrentWord - lineBegin)));
                    lineBegin = startOfCurrentWord;
                    curWidthOfLine = font.GetStringLength({ startOfCurrentWord, (size_t)((currentCharacter - startOfCurrentWord) + 1) }, textState);
                }
            }
            ////else
            {
                ////    dCurWidthOfLine += font.GetCharWidth(*currentCharacter, textState);
            }
        }
        currentCharacter++;
    }

    if ((currentCharacter - lineBegin) > 0)
    {
        if (curWidthOfLine > width && startOfCurrentWord > lineBegin)
        {
            // The previous word does not fit in the current line.
            // -> Move it to the next one.
            lines.push_back(string(lineBegin, (size_t)(startOfCurrentWord - lineBegin)));
            lineBegin = startOfCurrentWord;
        }
        //else do nothing

        if (currentCharacter - lineBegin > 0)
        {
            lines.push_back(string(lineBegin, (size_t)(currentCharacter - lineBegin)));
        }
        //else do nothing
    }

    return lines;
    */
}

void PdfPainter::drawTextAligned(const string_view& str, double x, double y, double width,
    PdfHorizontalAlignment hAlignment, PdfDrawTextStyle style)
{
    auto& textState = m_StateStack.Current->TextState;
    switch (hAlignment)
    {
        default:
        case PdfHorizontalAlignment::Left:
            break;
        case PdfHorizontalAlignment::Center:
            x += (width - textState.Font->GetStringLength(str, textState)) / 2.0;
            break;
        case PdfHorizontalAlignment::Right:
            x += (width - textState.Font->GetStringLength(str, textState));
            break;
    }

    this->drawText(str, x, y,
        (style & PdfDrawTextStyle::Underline) != PdfDrawTextStyle::Regular,
        (style & PdfDrawTextStyle::StrikeOut) != PdfDrawTextStyle::Regular);
}

void PdfPainter::DrawImage(const PdfImage& obj, double x, double y, double scaleX, double scaleY)
{
    this->DrawXObject(obj, x, y,
        scaleX * obj.GetRect().GetWidth(),
        scaleY * obj.GetRect().GetHeight());
}

void PdfPainter::DrawXObject(const PdfXObject& obj, double x, double y, double scaleX, double scaleY)
{
    checkStream();
    checkStatus(StatusDefault);

    // use OriginalReference() as the XObject might have been written to disk
    // already and is not in memory anymore in this case.
    this->addToPageResources("XObject", obj.GetIdentifier(), obj.GetObject());

    m_stream << "q" << endl << scaleX << " 0 0 " << scaleY << " "
        << x << " " << y << " cm" << endl
        << "/" << obj.GetIdentifier().GetString() << " Do" << endl << "Q" << endl;
}

void PdfPainter::Save()
{
    checkStream();
    checkStatus(StatusDefault);
    save();
}

void PdfPainter::save()
{
    m_stream << "q" << endl;
    m_StateStack.Push();
    auto& current = *m_StateStack.Current;
    GraphicsState.SetState(current.GraphicsState);
    TextState.SetState(current.TextState);
}

void PdfPainter::Restore()
{
    checkStream();
    checkStatus(StatusDefault);

    if (m_StateStack.GetSize() == 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Can't restore the state when only default state is opened");

    restore();
}

void PdfPainter::restore()
{
    m_stream << "Q" << endl;
    m_StateStack.Pop();
    auto& current = *m_StateStack.Current;
    GraphicsState.SetState(current.GraphicsState);
    TextState.SetState(current.TextState);
}

void PdfPainter::SetExtGState(const PdfExtGState& inGState)
{
    checkStream();
    checkStatus(StatusDefault);
    this->addToPageResources("ExtGState", inGState.GetIdentifier(), inGState.GetObject());
    m_stream << "/" << inGState.GetIdentifier().GetString() << " gs" << endl;
}

// TODO: Validate when marked content can be put
void PdfPainter::BeginMarkedContent(const string_view& tag)
{
    checkStatus(StatusDefault);
    m_stream << '/' << tag << " BMC" << endl;
}

void PdfPainter::EndMarkedContent()
{
    checkStatus(StatusDefault);
    m_stream << "EMC" << endl;
}

void PdfPainter::SetTransformationMatrix(const Matrix& matrix)
{
    checkStream();
    checkStatus(StatusDefault);
    m_stream
        << matrix[0] << " "
        << matrix[1] << " "
        << matrix[2] << " "
        << matrix[3] << " "
        << matrix[4] << " "
        << matrix[5] << " cm" << endl;
}

void PdfPainter::SetPrecision(unsigned short precision)
{
    m_stream.SetPrecision(precision);
}

unsigned short PdfPainter::GetPrecision() const
{
    return static_cast<unsigned char>(m_stream.GetPrecision());
}

void PdfPainter::BeginPath(double x, double y)
{
    checkStream();
    checkStatus(StatusDefault | StatusPath);
    pathMoveTo(x, y);
    m_subPaths.push_back(Vector2(x, y));
    m_painterStatus = StatusPath;
}

void PdfPainter::ClosePath()
{
    checkStream();
    checkStatus(StatusPath);
    closePath();
}

void PdfPainter::closePath()
{
    m_stream << "h" << endl;
    // TODO: should update last point with last point in 'x y m' call
    // or something similar, needs testing
}

void PdfPainter::EndPath()
{
    checkStream();
    checkStatus(StatusPath);
    m_stream << "n" << endl;
    endPath();
}

void PdfPainter::endPath()
{
    PODOFO_INVARIANT(m_subPaths.size() != 0);
    m_subPaths.clear();
    m_painterStatus = StatusDefault;
}

void PdfPainter::BeginText()
{
    checkStream();
    checkStatus(StatusDefault | StatusText);
    m_stream << "BT" << endl;
    m_textStackCount++;
    m_painterStatus = StatusText;
    writeTextState();
}

void PdfPainter::TextMoveTo(double x, double y)
{
    checkStream();
    checkStatus(StatusText);
    m_stream << x << " " << y << " Td" << endl;
}

void PdfPainter::AddText(const string_view& str)
{
    checkStream();
    checkStatus(StatusText);
    checkFont();
    auto expStr = this->expandTabs(str);
    m_StateStack.Current->TextState.Font->WriteStringToStream(m_stream, expStr);
    m_stream << " Tj" << endl;
}

void PdfPainter::EndText()
{
    checkStream();
    checkStatus(StatusText);
    m_stream << "ET" << endl;
    PODOFO_ASSERT(m_textStackCount != 0);
    m_textStackCount--;
    if (m_textStackCount == 0)
        m_painterStatus = StatusDefault;
}

void PdfPainter::SetLineWidth(double value)
{
    checkStream();
    setLineWidth(value);
}

void PdfPainter::setLineWidth(double width)
{
    m_stream << width << " w" << endl;
}

void PdfPainter::SetMiterLimit(double value)
{
    checkStream();
    m_stream << value << " M" << endl;
}

void PdfPainter::SetLineCapStyle(PdfLineCapStyle style)
{
    checkStream();
    m_stream << static_cast<int>(style) << " J" << endl;
}

void PdfPainter::SetLineJoinStyle(PdfLineJoinStyle style)
{
    checkStream();
    m_stream << static_cast<int>(style) << " j" << endl;
}

void PdfPainter::SetRenderingIntent(const string_view& intent)
{
    checkStream();
    m_stream << "/" << intent << " ri" << endl;
}

void PdfPainter::SetFillColor(const PdfColor& color)
{
    checkStream();
    switch (color.GetColorSpace())
    {
        default:
        case PdfColorSpace::DeviceRGB:
        {
            m_stream << color.GetRed() << " "
                << color.GetGreen() << " "
                << color.GetBlue()
                << " rg" << endl;
            break;
        }
        case PdfColorSpace::DeviceCMYK:
        {
            m_stream << color.GetCyan() << " "
                << color.GetMagenta() << " "
                << color.GetYellow() << " "
                << color.GetBlack()
                << " k" << endl;
            break;
        }
        case PdfColorSpace::DeviceGray:
        {
            m_stream << color.GetGrayScale() << " g" << endl;
            break;
        }
        case PdfColorSpace::Separation:
        {
            m_canvas->GetOrCreateResources().AddColorResource(color);
            m_stream << "/ColorSpace" << PdfName(color.GetName()).GetEscapedName() << " cs " << color.GetDensity() << " scn" << endl;
            break;
        }
        case PdfColorSpace::Lab:
        {
            m_canvas->GetOrCreateResources().AddColorResource(color);
            m_stream << "/ColorSpaceCieLab" << " cs "
                << color.GetCieL() << " "
                << color.GetCieA() << " "
                << color.GetCieB() <<
                " scn" << endl;
            break;
        }
        case PdfColorSpace::Unknown:
        case PdfColorSpace::Indexed:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
        }
    }
}

void PdfPainter::SetStrokeColor(const PdfColor& color)
{
    checkStream();
    switch (color.GetColorSpace())
    {
        default:
        case PdfColorSpace::DeviceRGB:
        {
            m_stream << color.GetRed() << " "
                << color.GetGreen() << " "
                << color.GetBlue()
                << " RG" << endl;
            break;
        }
        case PdfColorSpace::DeviceCMYK:
        {
            m_stream << color.GetCyan() << " "
                << color.GetMagenta() << " "
                << color.GetYellow() << " "
                << color.GetBlack()
                << " K" << endl;
            break;
        }
        case PdfColorSpace::DeviceGray:
        {
            m_stream << color.GetGrayScale() << " G" << endl;
            break;
        }
        case PdfColorSpace::Separation:
        {
            m_canvas->GetOrCreateResources().AddColorResource(color);
            m_stream << "/ColorSpace" << PdfName(color.GetName()).GetEscapedName() << " CS " << color.GetDensity() << " SCN" << endl;
            break;
        }
        case PdfColorSpace::Lab:
        {
            m_canvas->GetOrCreateResources().AddColorResource(color);
            m_stream << "/ColorSpaceCieLab" << " CS "
                << color.GetCieL() << " "
                << color.GetCieA() << " "
                << color.GetCieB() <<
                " SCN" << endl;
            break;
        }
        case PdfColorSpace::Unknown:
        case PdfColorSpace::Indexed:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
        }
    }
}

void PdfPainter::SetFont(const PdfFont* font, double fontSize)
{
    if (font == nullptr)
        return;

    checkStream();
    this->addToPageResources("Font", font->GetIdentifier(), font->GetObject());
    if (m_painterStatus == StatusText)
        setFont(font, fontSize);
}

void PdfPainter::setFont(const PdfFont* font, double fontSize)
{
    auto& textState = m_StateStack.Current->EmittedTextState;
    if (textState.Font == font
        && textState.FontSize == fontSize)
    {
        return;
    }

    m_stream << "/" << font->GetIdentifier().GetString()
        << " " << fontSize
        << " Tf" << endl;

    textState.Font = font;
    textState.FontSize = fontSize;
}

void PdfPainter::SetFontScale(double value)
{
    checkStream();
    if (m_painterStatus == StatusText)
        setFontScale(value);
}

void PdfPainter::setFontScale(double value)
{
    auto& textState = m_StateStack.Current->EmittedTextState;
    if (textState.FontScale == value)
        return;

    m_stream << value * 100 << " Tz" << endl;
    textState.FontScale = value;
}

void PdfPainter::SetCharSpacing(double value)
{
    checkStream();
    if (m_painterStatus == StatusText)
        setCharSpacing(value);
}

void PdfPainter::setCharSpacing(double value)
{
    auto& textState = m_StateStack.Current->EmittedTextState;
    if (textState.CharSpacing == value)
        return;

    m_stream << value << " Tc" << endl;
    textState.CharSpacing = value;
}

void PdfPainter::SetWordSpacing(double value)
{
    checkStream();
    if (m_painterStatus == StatusText)
        setWordSpacing(value);
}

void PdfPainter::setWordSpacing(double value)
{
    auto& textState = m_StateStack.Current->EmittedTextState;
    if (textState.WordSpacing == value)
        return;

    m_stream << value << " Tw" << endl;
    textState.WordSpacing = value;
}

void PdfPainter::SetTextRenderingMode(PdfTextRenderingMode value)
{
    checkStream();
    if (m_painterStatus == StatusText)
        setTextRenderingMode(value);
}

void PdfPainter::setTextRenderingMode(PdfTextRenderingMode value)
{
    auto& textState = m_StateStack.Current->EmittedTextState;
    if (textState.RenderingMode == value)
        return;

    m_stream << (int)value << " Tr" << endl;
    textState.RenderingMode = value;
}

void PdfPainter::writeTextState()
{
    auto& textState = m_StateStack.Current->TextState;
    if (textState.Font != nullptr)
        setFont(textState.Font, textState.FontSize);

    if (textState.FontScale != 1)
        setFontScale(textState.FontScale);

    if (textState.CharSpacing != 0)
        setCharSpacing(textState.CharSpacing);

    if (textState.WordSpacing != 0)
        setWordSpacing(textState.WordSpacing);

    if (textState.RenderingMode != PdfTextRenderingMode::Fill)
        setTextRenderingMode(textState.RenderingMode);
}

void PdfPainter::addToPageResources(const PdfName& type, const PdfName& identifier, const PdfObject& obj)
{
    if (m_canvas == nullptr)
        PODOFO_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_canvas->GetOrCreateResources().AddResource(type, identifier, obj);
}

string PdfPainter::expandTabs(const string_view& str) const
{
    unsigned tabCount = 0;
    auto it = str.begin();
    auto end = str.end();
    while (it != end)
    {
        char32_t ch = (char32_t)utf8::next(it, end);
        if (ch == U'\t')
            tabCount++;
    }

    // if no tabs are found: bail out!
    if (tabCount == 0)
        return (string)str;

    return ::expandTabs(str, m_TabWidth, tabCount);
}

void PdfPainter::checkStream()
{
    if (m_objStream != nullptr)
        return;

    PODOFO_RAISE_LOGIC_IF(m_canvas == nullptr, "Call SetCanvas() first before doing drawing operations");
    m_objStream = &m_canvas->GetStreamForAppending((PdfStreamAppendFlags)(m_flags & (~PdfPainterFlags::NoSaveRestore)));
}

void PdfPainter::checkFont()
{
    auto& textState = m_StateStack.Current->TextState;
    if (textState.Font == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Font should be set prior calling the method");
}

void PdfPainter::checkStatus(int expectedStatus)
{
    if ((expectedStatus & m_painterStatus) == 0)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported operation at this time");
}

PdfPainterPathContext::PdfPainterPathContext(PdfPainter& painter) :
    m_painter(&painter)
{
}

void PdfPainterPathContext::Begin(double x, double y)
{
    m_painter->BeginPath(x, y);
}

void PdfPainterPathContext::AddLineTo(double x, double y)
{
    m_painter->AddLineTo(x, y);
}

void PdfPainterPathContext::AddLine(double x1, double y1, double x2, double y2)
{
    m_painter->BeginPath(x1, y1);
    m_painter->addLineTo(x2, y2);
}

void PdfPainterPathContext::AddCubicBezierTo(double x1, double y1, double x2, double y2, double x3, double y3)
{
    m_painter->AddCubicBezierTo(x1, y1, x2, y2, x3, y3);
}

void PdfPainterPathContext::AddCubicBezier(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4)
{
    m_painter->BeginPath(x1, y1);
    m_painter->addCubicBezierTo(x2, y2, x3, y3, x4, y4);
}

void PdfPainterPathContext::AddCircle(double x, double y, double radius)
{
    m_painter->AddCircle(x, y, radius);
}

void PdfPainterPathContext::AddRectangle(const PdfRect& rect, double roundX, double roundY)
{
    m_painter->AddRectangle(rect.GetLeft(), rect.GetBottom(),
        rect.GetWidth(), rect.GetHeight(), roundX, roundY);
}

void PdfPainterPathContext::AddRectangle(double x, double y, double width, double height,
    double roundX, double roundY)
{
    m_painter->AddRectangle(x, y, width, height, roundX, roundY);
}

void PdfPainterPathContext::AddEllipse(double x, double y, double width, double height)
{
    m_painter->AddEllipse(x, y, width, height);
}

void PdfPainterPathContext::AddArc(double x, double y, double radius,
    double angle1, double angle2, bool counterclockwise)
{
    m_painter->AddArc(x, y, radius, angle1, angle2, counterclockwise);
}

void PdfPainterPathContext::AddArcTo(double x1, double y1, double x2, double y2, double radius)
{
    m_painter->AddArcTo(x1, y1, x2, y2, radius);
}

void PdfPainterPathContext::Discard()
{
    m_painter->EndPath();
}

void PdfPainterPathContext::Close()
{
    m_painter->ClosePath();
}

void PdfPainterPathContext::Draw(PdfPathDrawMode drawMode)
{
    m_painter->DrawPath(drawMode);
}

void PdfPainterPathContext::Clip(bool useEvenOddRule)
{
    m_painter->Clip(useEvenOddRule);
}

PdfPainterTextContext::PdfPainterTextContext(PdfPainter& painter)
    : m_painter(&painter)
{
}

void PdfPainterTextContext::Begin()
{
    m_painter->BeginText();
}

void PdfPainterTextContext::MoveTo(double x, double y)
{
    m_painter->TextMoveTo(x, y);
}

void PdfPainterTextContext::AddText(const string_view& str)
{
    m_painter->AddText(str);
}

void PdfPainterTextContext::End()
{
    m_painter->EndText();
}

PdfGraphicsStateWrapper::PdfGraphicsStateWrapper(PdfPainter& painter, PdfGraphicsState& state)
    : m_painter(&painter), m_state(&state) { }

void PdfGraphicsStateWrapper::SetCurrentMatrix(const Matrix& matrix)
{
    if (m_state->CTM == matrix)
        return;

    m_state->CTM = matrix;
    m_painter->SetTransformationMatrix(m_state->CTM);
}

void PdfGraphicsStateWrapper::SetLineWidth(double lineWidth)
{
    if (m_state->LineWidth == lineWidth)
        return;

    m_state->LineWidth = lineWidth;
    m_painter->SetLineWidth(m_state->LineWidth);
}

void PdfGraphicsStateWrapper::SetMiterLevel(double value)
{
    if (m_state->MiterLimit == value)
        return;

    m_state->MiterLimit = value;
    m_painter->SetMiterLimit(m_state->MiterLimit);
}

void PdfGraphicsStateWrapper::SetLineCapStyle(PdfLineCapStyle capStyle)
{
    if (m_state->LineCapStyle == capStyle)
        return;

    m_state->LineCapStyle = capStyle;
    m_painter->SetLineCapStyle(m_state->LineCapStyle);
}

void PdfGraphicsStateWrapper::SetLineJoinStyle(PdfLineJoinStyle joinStyle)
{
    if (m_state->LineJoinStyle == joinStyle)
        return;

    m_state->LineJoinStyle = joinStyle;
    m_painter->SetLineJoinStyle(m_state->LineJoinStyle);
}

void PdfGraphicsStateWrapper::SetRenderingIntent(const string_view& intent)
{
    if (m_state->RenderingIntent == intent)
        return;

    m_state->RenderingIntent = intent;
    m_painter->SetRenderingIntent(m_state->RenderingIntent);
}

void PdfGraphicsStateWrapper::SetFillColor(const PdfColor& color)
{
    if (m_state->FillColor == color)
        return;

    m_state->FillColor = color;
    m_painter->SetFillColor(m_state->FillColor);
}

void PdfGraphicsStateWrapper::SetStrokeColor(const PdfColor& color)
{
    if (m_state->StrokeColor == color)
        return;

    m_state->StrokeColor = color;
    m_painter->SetStrokeColor(m_state->StrokeColor);
}

PdfTextStateWrapper::PdfTextStateWrapper(PdfPainter& painter, PdfTextState& state)
    : m_painter(&painter), m_state(&state) { }

void PdfTextStateWrapper::SetFont(const PdfFont& font, double fontSize)
{
    if (m_state->Font == &font && m_state->FontSize == fontSize)
        return;

    m_state->Font = &font;
    m_state->FontSize = fontSize;
    m_painter->SetFont(m_state->Font, m_state->FontSize);
}

void PdfTextStateWrapper::SetFontScale(double scale)
{
    if (m_state->FontScale == scale)
        return;

    m_state->FontScale = scale;
    m_painter->SetFontScale(m_state->FontScale);
}

void PdfTextStateWrapper::SetCharSpacing(double charSpacing)
{
    if (m_state->CharSpacing == charSpacing)
        return;

    m_state->CharSpacing = charSpacing;
    m_painter->SetCharSpacing(m_state->CharSpacing);
}

void PdfTextStateWrapper::SetWordSpacing(double wordSpacing)
{
    if (m_state->WordSpacing == wordSpacing)
        return;

    m_state->WordSpacing = wordSpacing;
    m_painter->SetWordSpacing(m_state->WordSpacing);
}

void PdfTextStateWrapper::SetRenderingMode(PdfTextRenderingMode mode)
{
    if (m_state->RenderingMode == mode)
        return;

    m_state->RenderingMode = mode;
    m_painter->SetTextRenderingMode(m_state->RenderingMode);
}

void PdfPainter::AddLineTo(double x, double y)
{
    checkStream();
    checkStatus(StatusPath);
    addLineTo(x, y);
}

void PdfPainter::addLineTo(double x, double y)
{
    m_stream << x << " " << y << " l" << endl;
    m_StateStack.Current->CurrentPoint = Vector2(x, y);
}

void PdfPainter::AddCubicBezierTo(double x1, double y1, double x2, double y2, double x3, double y3)
{
    checkStream();
    checkStatus(StatusPath);
    addCubicBezierTo(x1, y1, x2, y2, x3, y3);
}

void PdfPainter::addCubicBezierTo(double x1, double y1, double x2, double y2, double x3, double y3)
{
    m_stream << x1 << " " << y1 << " "
             << x2 << " " << y2 << " "
             << x3 << " " << y3 << " c" << endl;
    m_StateStack.Current->CurrentPoint = Vector2(x3, y3);
}

void PdfPainter::PathMoveTo(double x, double y)
{
    checkStream();
    checkStatus(StatusPath);
    pathMoveTo(x, y);
}

void PdfPainter::pathMoveTo(double x, double y)
{
    m_stream << x << " " << y << " m" << endl;
    m_StateStack.Current->CurrentPoint = Vector2(x, y);
}

void PdfPainter::AddArcTo(double x1, double y1, double x2, double y2, double r)
{
    checkStream();
    checkStatus(StatusPath);
    addArcTo(x1, y1, x2, y2, r);
}

void PdfPainter::addArcTo(double x1, double y1, double x2, double y2, double r)
{
    double x0 = m_StateStack.Current->CurrentPoint.X;
    double y0 = m_StateStack.Current->CurrentPoint.Y;

    // Reference https://math.stackexchange.com/questions/191942/find-arc-center-from-tangent-lines-and-rounding-value

    double x1_0 = x0 - x1;
    double y1_0 = y0 - y1;
    double x1_2 = x2 - x1;
    double y1_2 = y2 - y1;

    // Compute the tagent points
    double norm1 = std::sqrt(x1_0 * x1_0 + y1_0 * y1_0);
    double norm2 = std::sqrt(x1_2 * x1_2 + y1_2 * y1_2);

    double x1t = x1 + x1_0 / norm1 * r;
    double y1t = y1 + y1_0 / norm1 * r;
    double x2t = x1 + x1_2 / norm2 * r;
    double y2t = y1 + y1_2 / norm2 * r;

    // Compute a two-point form -(y2–y1)*(x-x1) + (x2-x1)*(y-y1) = 0 and
    // then find the equation of perpendicular on the point (x1,y1) with b*x - a*y + a * y1 − b * x1 = 0

    // Compute the coefficientes of a line passing through (x1t, y1t) and perpendicular to the arc tangent
    double a0t = x1_0;
    double b0t = y1_0;
    double c0t = -x1_0 * x1t - y1_0 * y1t;

    // Compute the coefficientes of a line passing through (x2t, y2t) and perpendicular to the arc tangent
    double a2t = x1_2;
    double b2t = y1_2;
    double c2t = -x1_2 * x2t - y1_2 * y2t;

    // https://www.cuemath.com/geometry/intersection-of-two-lines/
    double xc = (b0t * c2t - b2t * c0t) / (a0t * b2t - a2t * b0t);
    double yc = (c0t * a2t - c2t * a0t) / (a0t * b2t - a2t * b0t);

    double ax = x1t - xc;
    double ay = y1t - yc;
    double bx = x2t - xc;
    double by = y2t - yc;
    double q1 = ax * ax + ay * ay;
    double q2 = q1 + ax * bx + ay * by;
    double k2 = (4 / 3.) * (sqrt(2 * q1 * q2) - q2) / (ax * by - ay * bx);

    double c1x = xc + ax - k2 * ay;
    double c1y = yc + ay + k2 * ax;
    double c2x = xc + bx + k2 * by;
    double c2y = yc + by - k2 * bx;

    // Draw the advancement to the first tangent point
    addLineTo(x1t, y1t);
    // Draw the bezier curve
    addCubicBezierTo(c1x, c1y, c2x, c2y, x2t, y2t);

    // According to testing of HTML5 canvas, it's not
    // needed to continue the line to the point (x2, y2)
    m_StateStack.Current->CurrentPoint = Vector2(x2t, y2t);
}

void PdfPainter::AddArc(double x, double y, double radius, double startAngle, double endAngle, double counterclockwise)
{
    checkStream();
    checkStatus(StatusPath);
    addArc(x, y, radius, startAngle, endAngle, counterclockwise);
}

void PdfPainter::addArc(double x, double y, double radius, double startAngle, double endAngle, double counterclockwise)
{
    (void)x;
    (void)y;
    (void)radius;
    (void)startAngle;
    (void)endAngle;
    (void)counterclockwise;
    pathMoveTo(x, y);
    // https://www.w3.org/2015/04/2dcontext-lc-sample.html#dom-context-2d-arc
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

void PdfPainter::AddCircle(double x, double y, double radius)
{
    checkStream();
    checkStatus(StatusPath);
    addCircle(x, y, radius);
}

void PdfPainter::addCircle(double x, double y, double radius)
{
    // draw four Bezier curves to approximate a circle
    pathMoveTo(x + radius, y);
    addCubicBezierTo(x + radius, y + radius * ARC_MAGIC,
        x + radius * ARC_MAGIC, y + radius,
        x, y + radius);
    addCubicBezierTo(x - radius * ARC_MAGIC, y + radius,
        x - radius, y + radius * ARC_MAGIC,
        x - radius, y);
    addCubicBezierTo(x - radius, y - radius * ARC_MAGIC,
        x - radius * ARC_MAGIC, y - radius,
        x, y - radius);
    addCubicBezierTo(x + radius * ARC_MAGIC, y - radius,
        x + radius, y - radius * ARC_MAGIC,
        x + radius, y);
    closePath();
}

void PdfPainter::AddEllipse(double x, double y, double width, double height)
{
    checkStream();
    checkStatus(StatusPath);
    addEllipse(x, y, width, height);
}

void PdfPainter::addEllipse(double x, double y, double width, double height)
{
    double pointsX[BEZIER_POINTS];
    double pointsY[BEZIER_POINTS];
    convertRectToBezier(x, y, width, height, pointsX, pointsY);

    pathMoveTo(x, y);
    for (unsigned i = 1; i < BEZIER_POINTS; i += 3)
    {
        m_stream << pointsX[i]     << " " << pointsY[i]     << " "
                 << pointsX[i + 1] << " " << pointsY[i + 1] << " "
                 << pointsX[i + 2] << " " << pointsY[i + 2] << " c" << endl;
    }
    closePath();
}

void PdfPainter::AddRectangle(double x, double y, double width, double height, double roundX, double roundY)
{
    checkStream();
    checkStatus(StatusPath);
    addRectangle(x, y, width, height, roundX, roundY);
}

void PdfPainter::addRectangle(double x, double y, double width, double height, double roundX, double roundY)
{
    if (static_cast<int>(roundX) || static_cast<int>(roundY))
    {
        double w = width;
        double h = height;
        double rx = roundX;
        double ry = roundY;
        double b = 0.4477f;

        pathMoveTo(x + rx, y);
        addLineTo(x + w - rx, y);
        addCubicBezierTo(x + w - rx * b, y, x + w, y + ry * b, x + w, y + ry);
        addLineTo(x + w, y + h - ry);
        addCubicBezierTo(x + w, y + h - ry * b, x + w - rx * b, y + h, x + w - rx, y + h);
        addLineTo(x + rx, y + h);
        addCubicBezierTo(x + rx * b, y + h, x, y + h - ry * b, x, y + h - ry);
        addLineTo(x, y + ry);
        addCubicBezierTo(x, y + ry * b, x + rx * b, y, x + rx, y);
        closePath();
    }
    else
    {
        m_stream << x     << " " << y      << " "
                 << width << " " << height << " re" << endl;
        m_StateStack.Current->CurrentPoint = Vector2(x + width, y + width);
    }
}

void PdfPainter::Clip(bool useEvenOddRule)
{
    checkStream();
    checkStatus(StatusPath);
    if (useEvenOddRule)
        m_stream << "W* n" << endl;
    else
        m_stream << "W n" << endl;
    endPath();
}

void PdfPainter::DrawPath(PdfPathDrawMode mode)
{
    // ISO 32000-2:2020, 8.5.3.1 General "Attempting to execute
    // a painting operator when the current path is undefined
    // (at the beginning of a new page or immediately after a
    // painting operator has been executed) shall generate an error"

    checkStream();
    checkStatus(StatusPath);
    drawPath(mode);
    endPath();
}

void PdfPainter::drawPath(PdfPathDrawMode mode)
{
    switch (mode)
    {
        case PdfPathDrawMode::Stroke:
            stroke();
            break;
        case PdfPathDrawMode::Fill:
            fill(false);
            break;
        case PdfPathDrawMode::StrokeFill:
            strokeAndFill(false);
            break;
        case PdfPathDrawMode::FillEvenOdd:
            fill(true);
            break;
        case PdfPathDrawMode::StrokeFillEvenOdd:
            strokeAndFill(true);
            break;
        default:
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

void PdfPainter::stroke()
{
    m_stream << "S" << endl;
}

void PdfPainter::fill(bool useEvenOddRule)
{
    if (useEvenOddRule)
        m_stream << "f*" << endl;
    else
        m_stream << "f" << endl;
}

void PdfPainter::strokeAndFill(bool useEvenOddRule)
{
    if (useEvenOddRule)
        m_stream << "B*" << endl;
    else
        m_stream << "B" << endl;
}

/** Coverts a rectangle to an array of points which can be used
 *  to draw an ellipse using 4 bezier curves.
 *
 *  The arrays plPointX and plPointY need space for at least 12 longs
 *  to be stored.
 *
 *  \param x x position of the bounding rectangle
 *  \param y y position of the bounding rectangle
 *  \param width width of the bounding rectangle
 *  \param height height of the bounding rectangle
 *  \param pointsX pointer to an array were the x coordinates
 *                  of the resulting points will be stored
 *  \param pointsY pointer to an array were the y coordinates
 *                  of the resulting points will be stored
 */
void convertRectToBezier(double x, double y, double width, double height, double pointsX[], double pointsY[])
{
    // this function is based on code from:
    // http://www.codeguru.com/Cpp/G-M/gdi/article.php/c131/
    // (Llew Goodstadt)

    // MAGICAL CONSTANT to map ellipse to beziers = 2/3*(sqrt(2)-1)
    const double dConvert = 0.2761423749154;

    double offX = width * dConvert;
    double offY = height * dConvert;
    double centerX = x + (width / 2.0);
    double centerY = y + (height / 2.0);

    //------------------------//
    //                        //
    //        2___3___4       //
    //     1             5    //
    //     |             |    //
    //     |             |    //
    //     0,12          6    //
    //     |             |    //
    //     |             |    //
    //    11             7    //
    //       10___9___8       //
    //                        //
    //------------------------//

    pointsX[0] = pointsX[1] = pointsX[11] = pointsX[12] = x;
    pointsX[5] = pointsX[6] = pointsX[7] = x + width;
    pointsX[2] = pointsX[10] = centerX - offX;
    pointsX[4] = pointsX[8] = centerX + offX;
    pointsX[3] = pointsX[9] = centerX;

    pointsY[2] = pointsY[3] = pointsY[4] = y;
    pointsY[8] = pointsY[9] = pointsY[10] = y + height;
    pointsY[7] = pointsY[11] = centerY + offY;
    pointsY[1] = pointsY[5] = centerY - offY;
    pointsY[0] = pointsY[12] = pointsY[6] = centerY;
}

string expandTabs(const string_view& str, unsigned tabWidth, unsigned tabCount)
{
    auto it = str.begin();
    auto end = str.end();

    string ret;
    ret.reserve(str.length() + tabCount * (tabWidth - 1));
    while (it != end)
    {
        char32_t ch = (char32_t)utf8::next(it, end);
        if (ch == U'\t')
            ret.append(tabWidth, ' ');

        utf8::append(ch, ret);
    }

    return ret;
}
