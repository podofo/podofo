/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPainter.h"

#include <podofo/private/PdfDrawingOperations.h>

#include <utfcpp/utf8.h>

#include "PdfExtGState.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfImage.h"

using namespace std;
using namespace PoDoFo;

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
    m_objStream = nullptr;
    m_canvas = nullptr;
}

void PdfPainter::SetStrokingShadingPattern(const PdfShadingPattern& pattern)
{
    checkStream();
    checkStatus(StatusDefault);
    this->addToPageResources("Pattern", pattern.GetIdentifier(), pattern.GetObject());
    PoDoFo::WriteOperator_CS(m_stream, PdfColorSpace::Pattern);
    PoDoFo::WriteOperator_SCN(m_stream, pattern.GetIdentifier().GetString());
}

void PdfPainter::SetShadingPattern(const PdfShadingPattern& pattern)
{
    checkStream();
    checkStatus(StatusDefault);
    this->addToPageResources("Pattern", pattern.GetIdentifier(), pattern.GetObject());
    PoDoFo::WriteOperator_cs(m_stream, PdfColorSpace::Pattern);
    PoDoFo::WriteOperator_scn(m_stream, pattern.GetIdentifier().GetString());
}

void PdfPainter::SetStrokingTilingPattern(const PdfTilingPattern& pattern)
{
    checkStream();
    checkStatus(StatusDefault);
    this->addToPageResources("Pattern", pattern.GetIdentifier(), pattern.GetObject());
    PoDoFo::WriteOperator_CS(m_stream, PdfColorSpace::Pattern);
    PoDoFo::WriteOperator_SCN(m_stream, pattern.GetIdentifier().GetString());
}

void PdfPainter::SetTilingPattern(const PdfTilingPattern& pattern)
{
    checkStream();
    checkStatus(StatusDefault);
    this->addToPageResources("Pattern", pattern.GetIdentifier(), pattern.GetObject());
    PoDoFo::WriteOperator_cs(m_stream, PdfColorSpace::Pattern);
    PoDoFo::WriteOperator_scn(m_stream, pattern.GetIdentifier().GetString());
}

void PdfPainter::SetStrokeStyle(PdfStrokeStyle strokeStyle, bool inverted, double scale, bool subtractJoinCap)
{
    checkStream();
    checkStatus(StatusDefault);

    vector<double> dashArray;
    if (inverted && strokeStyle != PdfStrokeStyle::Solid)
        dashArray.push_back(0);

    switch (strokeStyle)
    {
        case PdfStrokeStyle::Solid:
        {
            break;
        }
        case PdfStrokeStyle::Dash:
        {
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5)
            {
                dashArray.insert(dashArray.end(), { 6, 2 });
            }
            else
            {
                if (subtractJoinCap)
                    dashArray.insert(dashArray.end(), { scale * 2, scale * 2 });
                else
                    dashArray.insert(dashArray.end(), { scale * 3, scale * 1 });
            }
            break;
        }
        case PdfStrokeStyle::Dot:
        {
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5)
            {
                dashArray.insert(dashArray.end(), { 2, 2 });
            }
            else
            {
                if (subtractJoinCap)
                {
                    // zero length segments are drawn anyway here
                    dashArray.insert(dashArray.end(), { 0.001, scale * 2, 0, scale * 2 });
                }
                else
                {
                    dashArray.insert(dashArray.end(), { scale, scale });
                }
            }
            break;
        }
        case PdfStrokeStyle::DashDot:
        {
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5)
            {
                dashArray.insert(dashArray.end(), { 3, 2, 1, 2 });
            }
            else
            {
                if (subtractJoinCap)
                {
                    // zero length segments are drawn anyway here
                    dashArray.insert(dashArray.end(), { scale * 3, scale * 2, 0, scale * 2 });
                }
                else
                {
                    dashArray.insert(dashArray.end(), { scale * 3, scale, scale, scale });
                }
            }
            break;
        }
        case PdfStrokeStyle::DashDotDot:
        {
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5)
            {
                dashArray.insert(dashArray.end(), { 3, 1, 1, 1, 1, 1 });
            }
            else
            {
                if (subtractJoinCap)
                {
                    // zero length segments are drawn anyway here
                    dashArray.insert(dashArray.end(), { scale * 2, scale * 2, 0, scale * 2, 0, scale * 2 });
                }
                else
                {
                    dashArray.insert(dashArray.end(), { scale * 3, scale, scale, scale, scale, scale });
                }
            }
            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidStrokeStyle);
        }
    }

    if (inverted && strokeStyle != PdfStrokeStyle::Solid)
        dashArray.push_back(0);

    PoDoFo::WriteOperator_d(m_stream, dashArray, 0);
}

void PdfPainter::SetStrokeStyle(const cspan<double>& dashArray, double phase)
{
    checkStream();
    checkStatus(StatusDefault);
    PoDoFo::WriteOperator_d(m_stream, dashArray, phase);
}

void PdfPainter::SetClipRect(const PdfRect& rect)
{
    this->SetClipRect(rect.GetLeft(), rect.GetBottom(), rect.GetWidth(), rect.GetHeight());
}

void PdfPainter::SetClipRect(double x, double y, double width, double height)
{
    checkStream();
    checkStatus(StatusDefault);
    PoDoFo::WriteOperator_re(m_stream, x, y, width, height);
    PoDoFo::WriteOperator_W(m_stream);
    PoDoFo::WriteOperator_n(m_stream);
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

    if (m_painterStatus == StatusDefault)
        PoDoFo::WriteOperator_BT(m_stream);
    writeTextState();
    drawText(str, x, y,
        (style & PdfDrawTextStyle::Underline) != PdfDrawTextStyle::Regular,
        (style & PdfDrawTextStyle::StrikeOut) != PdfDrawTextStyle::Regular);
    if (m_painterStatus == StatusDefault)
        PoDoFo::WriteOperator_ET(m_stream);
}

void PdfPainter::drawText(const string_view& str, double x, double y, bool isUnderline, bool isStrikeOut)
{
    PoDoFo::WriteOperator_Td(m_stream, x, y);

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

    PoDoFo::WriteOperator_Tj(m_stream, font.GetEncoding().ConvertToEncoded(str),
        !font.GetEncoding().IsSimpleEncoding());
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
    checkStatus(StatusDefault | StatusTextObject);
    checkFont();

    if (width <= 0.0 || height <= 0.0) // nonsense arguments
        return;

    if (m_painterStatus == StatusDefault)
        PoDoFo::WriteOperator_BT(m_stream);
    writeTextState();
    drawMultiLineText(str, x, y, width, height,
        params.HorizontalAlignment, params.VerticalAlignment,
        params.Clip, params.SkipSpaces, params.Style);
    if (m_painterStatus == StatusDefault)
        PoDoFo::WriteOperator_ET(m_stream);
}

void PdfPainter::DrawTextAligned(const string_view& str, double x, double y, double width,
    PdfHorizontalAlignment hAlignment, PdfDrawTextStyle style)
{
    if (width <= 0.0) // nonsense arguments
        return;

    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    checkFont();

    if (m_painterStatus == StatusDefault)
        PoDoFo::WriteOperator_BT(m_stream);
    writeTextState();
    drawTextAligned(str, x, y, width, hAlignment, style);
    if (m_painterStatus == StatusDefault)
        PoDoFo::WriteOperator_ET(m_stream);
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

    // use OriginalReference() as the XObject might have been written to disk
    // already and is not in memory anymore in this case.
    this->addToPageResources("XObject", obj.GetIdentifier(), obj.GetObject());

    PoDoFo::WriteOperator_q(m_stream);
    PoDoFo::WriteOperator_cm(m_stream, scaleX, 0, 0, scaleY, x, y);
    PoDoFo::WriteOperator_Do(m_stream, obj.GetIdentifier().GetString());
    PoDoFo::WriteOperator_Q(m_stream);
}

void PdfPainter::DrawPath(const PdfPainterPath& path, PdfPathDrawMode drawMode)
{
    checkStream();
    checkStatus(StatusDefault);

    // ISO 32000-2:2020, 8.5.3.1 General "Attempting to execute
    // a painting operator when the current path is undefined
    // (at the beginning of a new page or immediately after a
    // painting operator has been executed) shall generate an error"

    ((OutputStream&)m_stream).Write(path.GetView());
    drawPath(drawMode);
    m_StateStack.Current->CurrentPoint = path.GetCurrentPoint();
}

void PdfPainter::ClipPath(const PdfPainterPath& path, bool useEvenOddRule)
{
    checkStream();
    checkStatus(StatusDefault);

    ((OutputStream&)m_stream).Write(path.GetView());
    if (useEvenOddRule)
        PoDoFo::WriteOperator_WStar(m_stream);
    else
        PoDoFo::WriteOperator_W(m_stream);

    PoDoFo::WriteOperator_n(m_stream);
    m_StateStack.Current->CurrentPoint = path.GetCurrentPoint();
}

void PdfPainter::Save()
{
    checkStream();
    checkStatus(StatusDefault);
    save();
}

void PdfPainter::save()
{
    PoDoFo::WriteOperator_q(m_stream);
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
    PoDoFo::WriteOperator_Q(m_stream);
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
    PoDoFo::WriteOperator_gs(m_stream, inGState.GetIdentifier().GetString());
}

// TODO: Validate when marked content can be put
void PdfPainter::BeginMarkedContent(const string_view& tag)
{
    checkStatus(StatusDefault);
    PoDoFo::WriteOperator_BMC(m_stream, tag);
}

void PdfPainter::EndMarkedContent()
{
    checkStatus(StatusDefault);
    PoDoFo::WriteOperator_EMC(m_stream);
}

void PdfPainter::SetTransformationMatrix(const Matrix& matrix)
{
    checkStream();
    checkStatus(StatusDefault);
    PoDoFo::WriteOperator_cm(m_stream, matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5]);
}

void PdfPainter::SetPrecision(unsigned short precision)
{
    m_stream.SetPrecision(precision);
}

unsigned short PdfPainter::GetPrecision() const
{
    return static_cast<unsigned char>(m_stream.GetPrecision());
}

string_view PdfPainter::GetView() const
{
    return m_stream.GetString();
}

void PdfPainter::closePath()
{
    PoDoFo::WriteOperator_h(m_stream);
    // TODO: should update last point with last point in 'x y m' call
    // or something similar, needs testing
}

void PdfPainter::BeginText()
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_BT(m_stream);
    enterTextObject();
    writeTextState();
}

void PdfPainter::TextMoveTo(double x, double y)
{
    checkStream();
    checkStatus(StatusTextObject);
    PoDoFo::WriteOperator_Td(m_stream, x, y);
}

void PdfPainter::AddText(const string_view& str)
{
    checkStream();
    checkStatus(StatusTextObject);
    checkFont();
    auto expStr = this->expandTabs(str);
    auto& font = *m_StateStack.Current->TextState.Font;
    PoDoFo::WriteOperator_Tj(m_stream, font.GetEncoding().ConvertToEncoded(expStr),
        !font.GetEncoding().IsSimpleEncoding());
}

void PdfPainter::EndText()
{
    checkStream();
    checkStatus(StatusTextObject);
    PoDoFo::WriteOperator_ET(m_stream);
    exitTextObject();
}

void PdfPainter::SetLineWidth(double value)
{
    checkStream();
    setLineWidth(value);
}

void PdfPainter::setLineWidth(double width)
{
    PoDoFo::WriteOperator_w(m_stream, width);
}

void PdfPainter::SetMiterLimit(double miterLimit)
{
    checkStream();
    PoDoFo::WriteOperator_M(m_stream, miterLimit);
}

void PdfPainter::SetLineCapStyle(PdfLineCapStyle style)
{
    checkStream();
    PoDoFo::WriteOperator_J(m_stream, style);
}

void PdfPainter::SetLineJoinStyle(PdfLineJoinStyle style)
{
    checkStream();
    PoDoFo::WriteOperator_j(m_stream, style);
}

void PdfPainter::SetRenderingIntent(const string_view& intent)
{
    checkStream();
    PoDoFo::WriteOperator_ri(m_stream, intent);
}

void PdfPainter::SetFillColor(const PdfColor& color)
{
    checkStream();
    switch (color.GetColorSpace())
    {
        default:
        case PdfColorSpace::DeviceRGB:
        {
            PoDoFo::WriteOperator_rg(m_stream, color.GetRed(), color.GetGreen(), color.GetBlue());
            break;
        }
        case PdfColorSpace::DeviceCMYK:
        {
            PoDoFo::WriteOperator_k(m_stream, color.GetCyan(), color.GetMagenta(), color.GetYellow(), color.GetBlack());
            break;
        }
        case PdfColorSpace::DeviceGray:
        {
            PoDoFo::WriteOperator_g(m_stream, color.GetGrayScale());
            break;
        }
        case PdfColorSpace::Separation:
        {
            m_canvas->GetOrCreateResources().AddColorResource(color);
            vector<double> components = { color.GetDensity() };
            PoDoFo::WriteOperator_cs(m_stream, PdfName(color.GetName()).GetEscapedName());
            PoDoFo::WriteOperator_scn(m_stream, components);
            break;
        }
        case PdfColorSpace::Lab:
        {
            m_canvas->GetOrCreateResources().AddColorResource(color);
            vector<double> components = { color.GetCieL(),color.GetCieA(), color.GetCieB() };
            PoDoFo::WriteOperator_cs(m_stream, "ColorSpaceCieLab");
            PoDoFo::WriteOperator_scn(m_stream, components);
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
            PoDoFo::WriteOperator_RG(m_stream, color.GetRed(), color.GetGreen(), color.GetBlue());
            break;
        }
        case PdfColorSpace::DeviceCMYK:
        {
            PoDoFo::WriteOperator_K(m_stream, color.GetCyan(), color.GetMagenta(), color.GetYellow(), color.GetBlack());
            break;
        }
        case PdfColorSpace::DeviceGray:
        {
            PoDoFo::WriteOperator_G(m_stream, color.GetGrayScale());
            break;
        }
        case PdfColorSpace::Separation:
        {
            m_canvas->GetOrCreateResources().AddColorResource(color);
            vector<double> components = { color.GetDensity() };
            PoDoFo::WriteOperator_CS(m_stream, PdfName(color.GetName()).GetEscapedName());
            PoDoFo::WriteOperator_SCN(m_stream, components);
            break;
        }
        case PdfColorSpace::Lab:
        {
            m_canvas->GetOrCreateResources().AddColorResource(color);
            vector<double> components = { color.GetCieL(),color.GetCieA(), color.GetCieB() };
            PoDoFo::WriteOperator_CS(m_stream, "ColorSpaceCieLab");
            PoDoFo::WriteOperator_SCN(m_stream, components);
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
    if (m_painterStatus == StatusTextObject)
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

    PoDoFo::WriteOperator_Tf(m_stream, font->GetIdentifier().GetString(), fontSize);
    textState.Font = font;
    textState.FontSize = fontSize;
}

void PdfPainter::SetFontScale(double value)
{
    checkStream();
    if (m_painterStatus == StatusTextObject)
        setFontScale(value);
}

void PdfPainter::setFontScale(double value)
{
    auto& textState = m_StateStack.Current->EmittedTextState;
    if (textState.FontScale == value)
        return;

    PoDoFo::WriteOperator_Tz(m_stream, value * 100);
    textState.FontScale = value;
}

void PdfPainter::SetCharSpacing(double value)
{
    checkStream();
    if (m_painterStatus == StatusTextObject)
        setCharSpacing(value);
}

void PdfPainter::setCharSpacing(double value)
{
    auto& textState = m_StateStack.Current->EmittedTextState;
    if (textState.CharSpacing == value)
        return;

    PoDoFo::WriteOperator_Tc(m_stream, value);
    textState.CharSpacing = value;
}

void PdfPainter::SetWordSpacing(double value)
{
    checkStream();
    if (m_painterStatus == StatusTextObject)
        setWordSpacing(value);
}

void PdfPainter::setWordSpacing(double value)
{
    auto& textState = m_StateStack.Current->EmittedTextState;
    if (textState.WordSpacing == value)
        return;

    PoDoFo::WriteOperator_Tw(m_stream, value);
    textState.WordSpacing = value;
}

void PdfPainter::SetTextRenderingMode(PdfTextRenderingMode value)
{
    checkStream();
    if (m_painterStatus == StatusTextObject)
        setTextRenderingMode(value);
}

void PdfPainter::setTextRenderingMode(PdfTextRenderingMode value)
{
    auto& textState = m_StateStack.Current->EmittedTextState;
    if (textState.RenderingMode == value)
        return;

    PoDoFo::WriteOperator_Tr(m_stream, value);
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

void PdfPainter::enterTextObject()
{
    m_textStackCount++;
    m_painterStatus = StatusTextObject;
}

void PdfPainter::exitTextObject()
{
    PODOFO_ASSERT(m_textStackCount != 0);
    m_textStackCount--;
    if (m_textStackCount == 0)
        m_painterStatus = StatusDefault;
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

void PdfPainterTextContext::DrawText(const string_view& str, double x, double y, PdfDrawTextStyle style)
{
    m_painter->DrawText(str, x, y, style);
}

void PdfPainterTextContext::DrawTextMultiLine(const string_view& str, double x, double y, double width, double height, const PdfDrawTextMultiLineParams& params)
{
    m_painter->DrawTextMultiLine(str, x, y, width, height, params);
}

void PdfPainterTextContext::DrawTextMultiLine(const string_view& str, const PdfRect& rect, const PdfDrawTextMultiLineParams& params)
{
    m_painter->DrawTextMultiLine(str, rect.GetLeft(), rect.GetBottom(), rect.GetWidth(), rect.GetHeight(), params);
}

void PdfPainterTextContext::DrawTextAligned(const string_view& str, double x, double y, double width, PdfHorizontalAlignment hAlignment, PdfDrawTextStyle style)
{
    m_painter->DrawTextAligned(str, x, y, width, hAlignment, style);
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

void PdfPainter::addLineTo(double x, double y)
{
    PoDoFo::WriteOperator_l(m_stream, x, y);
    m_StateStack.Current->CurrentPoint = Vector2(x, y);
}

void PdfPainter::addCubicBezierTo(double x1, double y1, double x2, double y2, double x3, double y3)
{
    PoDoFo::WriteOperator_c(m_stream, x1, y1, x2, y2, x3, y3);
    m_StateStack.Current->CurrentPoint = Vector2(x3, y3);
}

void PdfPainter::pathMoveTo(double x, double y)
{
    PoDoFo::WriteOperator_m(m_stream, x, y);
    m_StateStack.Current->CurrentPoint = Vector2(x, y);
}

void PdfPainter::addArcTo(double x1, double y1, double x2, double y2, double r)
{
    double x0 = m_StateStack.Current->CurrentPoint.X;
    double y0 = m_StateStack.Current->CurrentPoint.Y;

    PoDoFo::WriteArcTo(m_stream, x0, y0, x1, y1, x2, y2, r, m_StateStack.Current->CurrentPoint);
}

void PdfPainter::addArc(double x, double y, double radius, double startAngle, double endAngle, double counterclockwise)
{
    PoDoFo::WriteArc(m_stream, x, y, radius, startAngle, endAngle,
        counterclockwise, m_StateStack.Current->CurrentPoint);
}

void PdfPainter::addCircle(double x, double y, double radius)
{
    PoDoFo::WriteCircle(m_stream, x, y, radius, m_StateStack.Current->CurrentPoint);
}

void PdfPainter::addEllipse(double x, double y, double width, double height)
{
    PoDoFo::WriteEllipse(m_stream, x, y, width, height, m_StateStack.Current->CurrentPoint);
}

void PdfPainter::addRectangle(double x, double y, double width, double height, double roundX, double roundY)
{
    PoDoFo::WriteRectangle(m_stream, x, y, width, height, roundX, roundY, m_StateStack.Current->CurrentPoint);
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
    PoDoFo::WriteOperator_S(m_stream);
}

void PdfPainter::fill(bool useEvenOddRule)
{
    if (useEvenOddRule)
        PoDoFo::WriteOperator_fStar(m_stream);
    else
        PoDoFo::WriteOperator_f(m_stream);
}

void PdfPainter::strokeAndFill(bool useEvenOddRule)
{
    if (useEvenOddRule)
        PoDoFo::WriteOperator_BStar(m_stream);
    else
        PoDoFo::WriteOperator_B(m_stream);
}

PdfContentStreamOperators::PdfContentStreamOperators() { }

string expandTabs(const string_view& str, unsigned tabWidth, unsigned tabCount)
{
    auto it = str.begin();
    auto end = str.end();

    string ret;
    ret.reserve(str.length() + tabCount * ((size_t)tabWidth - 1));
    while (it != end)
    {
        char32_t ch = (char32_t)utf8::next(it, end);
        if (ch == U'\t')
            ret.append(tabWidth, ' ');

        utf8::append(ch, ret);
    }

    return ret;
}
