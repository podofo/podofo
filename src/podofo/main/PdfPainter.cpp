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

string expandTabs(const string_view& str, unsigned tabWidth, unsigned nTabCnt);

inline bool IsNewLineChar(char32_t ch)
{
    return ch == U'\n' || ch == U'\r';
}

inline bool IsSpaceChar(char32_t ch)
{
    if (ch > 255)
        return false;

    return std::isspace((int)ch) != 0;
}

PdfPainter::PdfPainter(PdfPainterFlags flags) :
    m_flags(flags),
    m_painterStatus(PainterStatus::Default),
    m_stream(nullptr),
    m_canvas(nullptr),
    m_GraphicsState(*this, m_StateStack.Current->GraphicsState),
    m_TextState(*this, m_StateStack.Current->TextState),
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
    m_stream = nullptr;
}

void PdfPainter::FinishDrawing()
{
    try
    {
        finishDrawing();
    }
    catch (PdfError& e)
    {
        // clean up, even in case of error
        reset();
        throw e;
    }

    reset();
}

void PdfPainter::finishDrawing()
{
    if (m_stream != nullptr)
    {
        PdfObjectOutputStream output;
        if ((m_flags & PdfPainterFlags::NoSaveRestorePrior) == PdfPainterFlags::NoSaveRestorePrior)
        {
            // GetLength() must be called before BeginAppend()
            if (m_stream->GetLength() == 0)
            {
                output = m_stream->GetOutputStream();
            }
            else
            {
                output = m_stream->GetOutputStream();
                // there is already content here - so let's assume we are appending
                // as such, we MUST put in a "space" to separate whatever we do.
                output.Write("\n");
            }
        }
        else
        {
            charbuff buffer;
            if (m_stream->GetLength() != 0)
                m_stream->CopyTo(buffer);

            if (buffer.size() == 0)
            {
                output = m_stream->GetOutputStream();
            }
            else
            {
                output = m_stream->GetOutputStream(true);
                output.Write("q\n");
                output.Write(buffer);
                output.Write("Q\n");
            }
        }

        if ((m_flags & PdfPainterFlags::NoSaveRestore) == PdfPainterFlags::NoSaveRestore)
        {
            output.Write(m_tmpStream.GetString());
        }
        else
        {
            output.Write("q\n");
            output.Write(m_tmpStream.GetString());
            output.Write("Q\n");
        }
    }
}


void PdfPainter::reset()
{
    m_StateStack.Clear();
    m_tmpStream.Clear();
    m_painterStatus = PainterStatus::Default;
    m_stream = nullptr;
    m_canvas = nullptr;
}

void PdfPainter::SetStrokingShadingPattern(const PdfShadingPattern& pattern)
{
    checkStream();
    checkStatus(PainterStatus::Default);
    this->addToPageResources("Pattern", pattern.GetIdentifier(), pattern.GetObject());
    m_tmpStream << "/Pattern CS /" << pattern.GetIdentifier().GetString() << " SCN" << endl;
}

void PdfPainter::SetShadingPattern(const PdfShadingPattern& pattern)
{
    checkStream();
    checkStatus(PainterStatus::Default);
    this->addToPageResources("Pattern", pattern.GetIdentifier(), pattern.GetObject());
    m_tmpStream << "/Pattern cs /" << pattern.GetIdentifier().GetString() << " scn" << endl;
}

void PdfPainter::SetStrokingTilingPattern(const PdfTilingPattern& pattern)
{
    checkStream();
    checkStatus(PainterStatus::Default);
    this->addToPageResources("Pattern", pattern.GetIdentifier(), pattern.GetObject());
    m_tmpStream << "/Pattern CS /" << pattern.GetIdentifier().GetString() << " SCN" << endl;
}

void PdfPainter::SetStrokingTilingPattern(const string_view& patternName)
{
    checkStream();
    checkStatus(PainterStatus::Default);
    m_tmpStream << "/Pattern CS /" << patternName << " SCN" << endl;
}

void PdfPainter::SetTilingPattern(const PdfTilingPattern& pattern)
{
    checkStream();
    checkStatus(PainterStatus::Default);
    this->addToPageResources("Pattern", pattern.GetIdentifier(), pattern.GetObject());
    m_tmpStream << "/Pattern cs /" << pattern.GetIdentifier().GetString() << " scn" << endl;
}

void PdfPainter::SetTilingPattern(const string_view& patternName)
{
    checkStream();
    checkStatus(PainterStatus::Default);
    m_tmpStream << "/Pattern cs /" << patternName << " scn" << endl;
}

void PdfPainter::SetStrokeStyle(PdfStrokeStyle strokeStyle, const string_view& custom, bool inverted, double scale, bool subtractJoinCap)
{
    bool have = false;
    checkStream();
    checkStatus(PainterStatus::Default);

    if (strokeStyle != PdfStrokeStyle::Custom)
        m_tmpStream << "[";

    if (inverted && strokeStyle != PdfStrokeStyle::Solid && strokeStyle != PdfStrokeStyle::Custom)
        m_tmpStream << "0 ";

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
                m_tmpStream << "6 2";
            }
            else
            {
                if (subtractJoinCap)
                    m_tmpStream << scale * 2.0 << " " << scale * 2.0;
                else
                    m_tmpStream << scale * 3.0 << " " << scale * 1.0;
            }
            break;
        }
        case PdfStrokeStyle::Dot:
        {
            have = true;
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5)
            {
                m_tmpStream << "2 2";
            }
            else
            {
                if (subtractJoinCap)
                {
                    // zero length segments are drawn anyway here
                    m_tmpStream << 0.001 << " " << 2.0 * scale << " " << 0 << " " << 2.0 * scale;
                }
                else
                {
                    m_tmpStream << scale * 1.0 << " " << scale * 1.0;
                }
            }
            break;
        }
        case PdfStrokeStyle::DashDot:
        {
            have = true;
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5)
            {
                m_tmpStream << "3 2 1 2";
            }
            else
            {
                if (subtractJoinCap)
                {
                    // zero length segments are drawn anyway here
                    m_tmpStream << scale * 2.0 << " " << scale * 2.0 << " " << 0 << " " << scale * 2.0;
                }
                else
                {
                    m_tmpStream << scale * 3.0 << " " << scale * 1.0 << " " << scale * 1.0 << " " << scale * 1.0;
                }
            }
            break;
        }
        case PdfStrokeStyle::DashDotDot:
        {
            have = true;
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5)
            {
                m_tmpStream << "3 1 1 1 1 1";
            }
            else
            {
                if (subtractJoinCap)
                {
                    // zero length segments are drawn anyway here
                    m_tmpStream << scale * 2.0 << " " << scale * 2.0 << " " << 0 << " " << scale * 2.0 << " " << 0 << " " << scale * 2.0;
                }
                else {
                    m_tmpStream << scale * 3.0 << " " << scale * 1.0 << " " << scale * 1.0 << " " << scale * 1.0 << " " << scale * 1.0 << " " << scale * 1.0;
                }
            }
            break;
        }
        case PdfStrokeStyle::Custom:
        {
            have = !custom.empty();
            if (have)
                m_tmpStream << custom;
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
        m_tmpStream << " 0";

    if (strokeStyle != PdfStrokeStyle::Custom)
        m_tmpStream << "] 0";

    m_tmpStream << " d" << endl;
}

void PdfPainter::SetClipRect(const PdfRect& rect)
{
    this->SetClipRect(rect.GetLeft(), rect.GetBottom(), rect.GetWidth(), rect.GetHeight());
}

void PdfPainter::SetClipRect(double x, double y, double width, double height)
{
    checkStream();
    checkStatus(PainterStatus::Default);

    m_tmpStream << x << " "
        << y << " "
        << width << " "
        << height
        << " re W n" << endl;
}

void PdfPainter::DrawLine(double startX, double startY, double endX, double endY)
{
    checkStream();
    checkStatus(PainterStatus::Default);

    m_tmpStream << startX << " "
        << startY
        << " m "
        << endX << " "
        << endY
        << " l S" << endl;
}

void PdfPainter::Rectangle(double x, double y, double width, double height,
    double roundX, double roundY)
{
    checkStream();
    checkStatus(PainterStatus::Default);

    if (static_cast<int>(roundX) || static_cast<int>(roundY))
    {
        double w = width;
        double h = height;
        double rx = roundX;
        double ry = roundY;
        double b = 0.4477f;

        moveTo(x + rx, y);
        lineTo(x + w - rx, y);
        cubicBezierTo(x + w - rx * b, y, x + w, y + ry * b, x + w, y + ry);
        lineTo(x + w, y + h - ry);
        cubicBezierTo(x + w, y + h - ry * b, x + w - rx * b, y + h, x + w - rx, y + h);
        lineTo(x + rx, y + h);
        cubicBezierTo(x + rx * b, y + h, x, y + h - ry * b, x, y + h - ry);
        lineTo(x, y + ry);
        cubicBezierTo(x, y + ry * b, x + rx * b, y, x + rx, y);
    }
    else
    {
        m_tmpStream << x << " "
            << y << " "
            << width << " "
            << height
            << " re" << endl;
    }
}

void PdfPainter::Ellipse(double x, double y, double width, double height)
{
    checkStream();
    checkStatus(PainterStatus::Default);

    double pointsX[BEZIER_POINTS];
    double pointsY[BEZIER_POINTS];
    convertRectToBezier(x, y, width, height, pointsX, pointsY);

    m_tmpStream << pointsX[0] << " "
        << pointsY[0]
        << " m" << endl;

    for (unsigned i = 1; i < BEZIER_POINTS; i += 3)
    {
        m_tmpStream << pointsX[i] << " "
            << pointsY[i] << " "
            << pointsX[i + 1] << " "
            << pointsY[i + 1] << " "
            << pointsX[i + 2] << " "
            << pointsY[i + 2]
            << " c" << endl;
    }
}

void PdfPainter::Arc(double x, double y, double radius, double angle1, double angle2)
{
    (void)x;
    (void)y;
    (void)radius;
    (void)angle1;
    (void)angle2;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
    // https://github.com/podofo/podofo/blob/5723d09bbab68340a3a32923d90910c3d6912cdd/src/podofo/doc/PdfPainter.cpp#L1591
}

void PdfPainter::Circle(double x, double y, double radius)
{
    checkStream();
    checkStatus(PainterStatus::Default);

    // draw four Bezier curves to approximate a circle
    moveTo(x + radius, y);
    cubicBezierTo(x + radius, y + radius * ARC_MAGIC,
        x + radius * ARC_MAGIC, y + radius,
        x, y + radius);
    cubicBezierTo(x - radius * ARC_MAGIC, y + radius,
        x - radius, y + radius * ARC_MAGIC,
        x - radius, y);
    cubicBezierTo(x - radius, y - radius * ARC_MAGIC,
        x - radius * ARC_MAGIC, y - radius,
        x, y - radius);
    cubicBezierTo(x + radius * ARC_MAGIC, y - radius,
        x + radius, y - radius * ARC_MAGIC,
        x + radius, y);
    close();
}

void PdfPainter::DrawText(const string_view& str, double x, double y,
    PdfDrawTextStyle style)
{
    checkStream();
    checkStatus(PainterStatus::Default);
    checkFont();

    m_tmpStream << "BT" << endl;
    writeTextState();
    drawText(str, x, y,
        (style & PdfDrawTextStyle::Underline) != PdfDrawTextStyle::Regular,
        (style & PdfDrawTextStyle::StrikeOut) != PdfDrawTextStyle::Regular);
    m_tmpStream << "ET" << endl;
}

void PdfPainter::drawText(const string_view& str, double x, double y, bool isUnderline, bool isStrikeOut)
{
    m_tmpStream << x << " " << y <<  " Td ";

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

    font.WriteStringToStream(m_tmpStream, expStr);
    m_tmpStream << " Tj" << endl;
}

void PdfPainter::BeginText(double x, double y)
{
    checkStream();
    checkStatus(PainterStatus::Default | PainterStatus::TextObject);

    m_tmpStream << "BT" << endl;
    m_tmpStream << x << " " << y << " Td" << endl;
    openTextObject();
}

void PdfPainter::MoveTextPos(double x, double y)
{
    checkStream();
    checkStatus(PainterStatus::TextObject);

    m_tmpStream << x << " " << y << " Td" << endl;
}

void PdfPainter::AddText(const string_view& str)
{
    checkStream();
    checkStatus(PainterStatus::TextObject);
    checkFont();

    auto expStr = this->expandTabs(str);

    // TODO: Underline and Strikeout not yet supported
    m_StateStack.Current->TextState.Font->WriteStringToStream(m_tmpStream, expStr);

    m_tmpStream << " Tj" << endl;
}

void PdfPainter::EndText()
{
    checkStream();
    checkStatus(PainterStatus::TextObject);

    m_tmpStream << "ET" << endl;
    closeTextObject();
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
    checkStatus(PainterStatus::Default | PainterStatus::TextObject);
    checkFont();

    if (width <= 0.0 || height <= 0.0) // nonsense arguments
        return;

    m_tmpStream << "BT" << endl;
    writeTextState();
    drawMultiLineText(str, x, y, width, height,
        params.HorizontalAlignment, params.VerticalAlignment,
        params.Clip, params.SkipSpaces, params.Style);
    m_tmpStream << "ET" << endl;
}

void PdfPainter::DrawTextAligned(const string_view& str, double x, double y, double width,
    PdfHorizontalAlignment hAlignment, PdfDrawTextStyle style)
{
    if (width <= 0.0) // nonsense arguments
        return;

    checkStream();
    checkStatus(PainterStatus::Default | PainterStatus::TextObject);
    checkFont();

    m_tmpStream << "BT" << endl;
    writeTextState();
    drawTextAligned(str, x, y, width, hAlignment, style);
    m_tmpStream << "ET" << endl;
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
    // FIX-ME: This method is currently broken, just rewrite it later
    vector<string> ret = { (string)str };
    return ret;

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
    checkStatus(PainterStatus::Default);

    // use OriginalReference() as the XObject might have been written to disk
    // already and is not in memory anymore in this case.
    this->addToPageResources("XObject", obj.GetIdentifier(), obj.GetObject());

    m_tmpStream << "q" << endl
        << scaleX << " 0 0 "
        << scaleY << " "
        << x << " "
        << y << " cm" << endl
        << "/" << obj.GetIdentifier().GetString() << " Do" << endl << "Q" << endl;
}

void PdfPainter::ClosePath()
{
    checkStream();
    checkStatus(PainterStatus::Default);
    m_tmpStream << "h" << endl;
}

void PdfPainter::LineTo(double x, double y)
{
    checkStream();
    checkStatus(PainterStatus::Default);
    lineTo(x, y);
}

void PdfPainter::lineTo(double x, double y)
{
    m_tmpStream << x << " " << y << " l" << endl;
}

void PdfPainter::MoveTo(double x, double y)
{
    checkStream();
    checkStatus(PainterStatus::Default);
    moveTo(x, y);
}

void PdfPainter::moveTo(double x, double y)
{
    m_tmpStream << x << " " << y << " m" << endl;
}

void PdfPainter::CubicBezierTo(double x1, double y1, double x2, double y2, double x3, double y3)
{
    checkStream();
    checkStatus(PainterStatus::Default);
    cubicBezierTo(x1, y1, x2, y2, x3, y3);
}

void PdfPainter::cubicBezierTo(double x1, double y1, double x2, double y2, double x3, double y3)
{
    m_tmpStream << x1 << " "
        << y1 << " "
        << x2 << " "
        << y2 << " "
        << x3 << " "
        << y3
        << " c" << endl;
}

void PdfPainter::Close()
{
    checkStream();
    checkStatus(PainterStatus::Default);
    close();
}

void PdfPainter::close()
{
    m_tmpStream << "h" << endl;
}

void PdfPainter::Stroke()
{
    checkStream();
    checkStatus(PainterStatus::Default);
    m_tmpStream << "S" << endl;
}

void PdfPainter::Fill(bool useEvenOddRule)
{
    checkStream();
    checkStatus(PainterStatus::Default);
    if (useEvenOddRule)
        m_tmpStream << "f*" << endl;
    else
        m_tmpStream << "f" << endl;
}

void PdfPainter::FillAndStroke(bool useEvenOddRule)
{
    checkStream();
    checkStatus(PainterStatus::Default);
    if (useEvenOddRule)
        m_tmpStream << "B*" << endl;
    else
        m_tmpStream << "B" << endl;
}

void PdfPainter::Clip(bool useEvenOddRule)
{
    checkStream();
    checkStatus(PainterStatus::Default);
    if (useEvenOddRule)
        m_tmpStream << "W* n" << endl;
    else
        m_tmpStream << "W n" << endl;
}

void PdfPainter::EndPath()
{
    checkStream();
    checkStatus(PainterStatus::Default);
    m_tmpStream << "n" << endl;
}

void PdfPainter::Save()
{
    checkStream();
    checkStatus(PainterStatus::Default);
    save();
}

void PdfPainter::save()
{
    m_tmpStream << "q" << endl;
    m_StateStack.Push();
    auto& current = *m_StateStack.Current;
    m_GraphicsState.SetState(current.GraphicsState);
    m_TextState.SetState(current.TextState);
}

void PdfPainter::Restore()
{
    checkStream();
    checkStatus(PainterStatus::Default);

    if (m_StateStack.GetSize() == 1)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Can't restore the state when only default state is opened");

    restore();
}

void PdfPainter::restore()
{
    m_tmpStream << "Q" << endl;
    m_StateStack.Pop();
    auto& current = *m_StateStack.Current;
    m_GraphicsState.SetState(current.GraphicsState);
    m_TextState.SetState(current.TextState);
}

void PdfPainter::SetExtGState(const PdfExtGState& inGState)
{
    checkStream();
    checkStatus(PainterStatus::Default);
    this->addToPageResources("ExtGState", inGState.GetIdentifier(), inGState.GetObject());
    m_tmpStream << "/" << inGState.GetIdentifier().GetString() << " gs" << endl;
}

void PdfPainter::Rectangle(const PdfRect& rect, double roundX, double roundY)
{
    this->Rectangle(rect.GetLeft(), rect.GetBottom(),
        rect.GetWidth(), rect.GetHeight(),
        roundX, roundY);
}

// TODO: Validate when marked context can be put
void PdfPainter::BeginMarkedContext(const string_view& tag)
{
    checkStatus(PainterStatus::Default);
    m_tmpStream << '/' << tag << " BMC" << endl;
}

void PdfPainter::EndMarkedContext()
{
    checkStatus(PainterStatus::Default);
    m_tmpStream << "EMC" << endl;
}

void PdfPainter::SetTransformationMatrix(const Matrix& matrix)
{
    checkStream();
    checkStatus(PainterStatus::Default);
    m_tmpStream
        << matrix[0] << " "
        << matrix[1] << " "
        << matrix[2] << " "
        << matrix[3] << " "
        << matrix[4] << " "
        << matrix[5] << " cm" << endl;
    m_StateStack.Current->CTM = matrix;
}

void PdfPainter::SetPrecision(unsigned short precision)
{
    m_tmpStream.SetPrecision(precision);
}

unsigned short PdfPainter::GetPrecision() const
{
    return static_cast<unsigned char>(m_tmpStream.GetPrecision());
}

void PdfPainter::SetLineWidth(double value)
{
    checkStream();
    setLineWidth(value);
}

void PdfPainter::setLineWidth(double width)
{
    m_tmpStream << width << " w" << endl;
}

void PdfPainter::SetMiterLimit(double value)
{
    checkStream();
    m_tmpStream << value << " M" << endl;
}

void PdfPainter::SetLineCapStyle(PdfLineCapStyle style)
{
    checkStream();
    m_tmpStream << static_cast<int>(style) << " J" << endl;
}

void PdfPainter::SetLineJoinStyle(PdfLineJoinStyle style)
{
    checkStream();
    m_tmpStream << static_cast<int>(style) << " j" << endl;
}

void PdfPainter::SetRenderingIntent(const string_view& intent)
{
    checkStream();
    m_tmpStream << "/" << intent << " ri" << endl;
}

void PdfPainter::SetFillColor(const PdfColor& color)
{
    checkStream();
    switch (color.GetColorSpace())
    {
        default:
        case PdfColorSpace::DeviceRGB:
        {
            m_tmpStream << color.GetRed() << " "
                << color.GetGreen() << " "
                << color.GetBlue()
                << " rg" << endl;
            break;
        }
        case PdfColorSpace::DeviceCMYK:
        {
            m_tmpStream << color.GetCyan() << " "
                << color.GetMagenta() << " "
                << color.GetYellow() << " "
                << color.GetBlack()
                << " k" << endl;
            break;
        }
        case PdfColorSpace::DeviceGray:
        {
            m_tmpStream << color.GetGrayScale() << " g" << endl;
            break;
        }
        case PdfColorSpace::Separation:
        {
            m_canvas->GetOrCreateResources().AddColorResource(color);
            m_tmpStream << "/ColorSpace" << PdfName(color.GetName()).GetEscapedName() << " cs " << color.GetDensity() << " scn" << endl;
            break;
        }
        case PdfColorSpace::Lab:
        {
            m_canvas->GetOrCreateResources().AddColorResource(color);
            m_tmpStream << "/ColorSpaceCieLab" << " cs "
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
            m_tmpStream << color.GetRed() << " "
                << color.GetGreen() << " "
                << color.GetBlue()
                << " RG" << endl;
            break;
        }
        case PdfColorSpace::DeviceCMYK:
        {
            m_tmpStream << color.GetCyan() << " "
                << color.GetMagenta() << " "
                << color.GetYellow() << " "
                << color.GetBlack()
                << " K" << endl;
            break;
        }
        case PdfColorSpace::DeviceGray:
        {
            m_tmpStream << color.GetGrayScale() << " G" << endl;
            break;
        }
        case PdfColorSpace::Separation:
        {
            m_canvas->GetOrCreateResources().AddColorResource(color);
            m_tmpStream << "/ColorSpace" << PdfName(color.GetName()).GetEscapedName() << " CS " << color.GetDensity() << " SCN" << endl;
            break;
        }
        case PdfColorSpace::Lab:
        {
            m_canvas->GetOrCreateResources().AddColorResource(color);
            m_tmpStream << "/ColorSpaceCieLab" << " CS "
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

    this->addToPageResources("Font", font->GetIdentifier(), font->GetObject());
    if (m_painterStatus == PainterStatus::TextObject)
        setFont(font, fontSize);
}

void PdfPainter::setFont(const PdfFont* font, double fontSize)
{
    checkStream();
    m_tmpStream << "/" << font->GetIdentifier().GetString()
        << " " << fontSize
        << " Tf" << endl;
}

void PdfPainter::SetFontScale(double value)
{
    if (m_painterStatus == PainterStatus::TextObject)
        setFontScale(value);
}

void PdfPainter::setFontScale(double value)
{
    checkStream();
    m_tmpStream << value * 100 << " Tz" << endl;
}

void PdfPainter::SetCharSpacing(double value)
{
    if (m_painterStatus == PainterStatus::TextObject)
        setCharSpacing(value);
}

void PdfPainter::setCharSpacing(double value)
{
    checkStream();
    m_tmpStream << value << " Tc" << endl;
}

void PdfPainter::SetWordSpacing(double value)
{
    if (m_painterStatus == PainterStatus::TextObject)
        setWordSpacing(value);
}

void PdfPainter::setWordSpacing(double value)
{
    checkStream();
    m_tmpStream << value << " Tw" << endl;
}

void PdfPainter::SetTextRenderingMode(PdfTextRenderingMode value)
{
    if (m_painterStatus == PainterStatus::TextObject)
        setTextRenderingMode(value);
}

void PdfPainter::setTextRenderingMode(PdfTextRenderingMode value)
{
    checkStream();
    m_tmpStream << (int)value << " Tr" << endl;
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

void PdfPainter::convertRectToBezier(double x, double y, double width, double height, double pointsX[], double pointsY[])
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
    if (m_stream != nullptr)
        return;

    PODOFO_RAISE_LOGIC_IF(m_canvas == nullptr, "Call SetCanvas() first before doing drawing operations");
    m_stream = &m_canvas->GetStreamForAppending((PdfStreamAppendFlags)(m_flags & (~PdfPainterFlags::NoSaveRestore)));
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

void PdfPainter::openTextObject()
{
    m_StateStack.Current->TextObjectCount++;
    m_painterStatus = PainterStatus::TextObject;
}

void PdfPainter::closeTextObject()
{
    auto& current = *m_StateStack.Current;
    PODOFO_ASSERT(current.TextObjectCount != 0);
    current.TextObjectCount--;
    if (current.TextObjectCount == 0)
        m_painterStatus = PainterStatus::Default;
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
