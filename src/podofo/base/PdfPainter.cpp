/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPainter.h"

#include <iomanip>

#include <utfcpp/utf8.h>

#include "PdfColor.h"
#include "PdfDictionary.h"
#include "PdfFilter.h"
#include "PdfName.h"
#include "PdfRect.h"
#include "PdfObjectStream.h"
#include "PdfString.h"
#include "PdfContents.h"
#include "PdfExtGState.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfImage.h"
#include "PdfMemDocument.h"
#include "PdfXObject.h"

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
    m_stream(nullptr),
    m_canvas(nullptr),
    m_PainterGraphicsState(*this, m_GraphicsState),
    m_PainterTextState(*this, m_TextState),
    m_TabWidth(4),
    m_isTextOpen(false),
    m_lpx(0),
    m_lpy(0),
    m_lpx2(0),
    m_lpy2(0),
    m_lpx3(0),
    m_lpy3(0),
    m_lcx(0),
    m_lcy(0),
    m_lrx(0),
    m_lry(0)
{
}

PdfPainter::~PdfPainter() noexcept(false)
{
    // Throwing exceptions in C++ destructors is not allowed.
    // Just log the error.
    // PODOFO_RAISE_LOGIC_IF( m_Canvas, "FinishPage() has to be called after a page is completed!" );
    // Note that we can't do this for the user, since FinishPage() might
    // throw and we can't safely have that in a dtor. That also means
    // we can't throw here, but must abort.
    if (m_stream != nullptr && !std::uncaught_exceptions())
    {
        PoDoFo::LogMessage(PdfLogSeverity::Error,
            "PdfPainter::~PdfPainter(): FinishDrawing() has to be called after drawing is completed!");
    }
}

void PdfPainter::SetCanvas(PdfCanvas& canvas)
{
    // Ignore setting the same canvas twice
    if (m_canvas == &canvas)
        return;

    finishDrawing();

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
        m_stream = nullptr;
        m_canvas = nullptr;
        throw e;
    }

    m_stream = nullptr;
    m_canvas = nullptr;
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

    // Reset temporary stream
    m_tmpStream.Clear();
}

void PdfPainter::SetStrokingShadingPattern(const PdfShadingPattern& pattern)
{
    checkStream();
    this->addToPageResources("Pattern", pattern.GetIdentifier(), pattern.GetObject());
    m_tmpStream << "/Pattern CS /" << pattern.GetIdentifier().GetString() << " SCN" << endl;
}

void PdfPainter::SetShadingPattern(const PdfShadingPattern& pattern)
{
    checkStream();
    this->addToPageResources("Pattern", pattern.GetIdentifier(), pattern.GetObject());
    m_tmpStream << "/Pattern cs /" << pattern.GetIdentifier().GetString() << " scn" << endl;
}

void PdfPainter::SetStrokingTilingPattern(const PdfTilingPattern& pattern)
{
    checkStream();
    this->addToPageResources("Pattern", pattern.GetIdentifier(), pattern.GetObject());
    m_tmpStream << "/Pattern CS /" << pattern.GetIdentifier().GetString() << " SCN" << endl;
}

void PdfPainter::SetStrokingTilingPattern(const string_view& patternName)
{
    checkStream();
    m_tmpStream << "/Pattern CS /" << patternName << " SCN" << endl;
}

void PdfPainter::SetTilingPattern(const PdfTilingPattern& pattern)
{
    checkStream();
    this->addToPageResources("Pattern", pattern.GetIdentifier(), pattern.GetObject());
    m_tmpStream << "/Pattern cs /" << pattern.GetIdentifier().GetString() << " scn" << endl;
}

void PdfPainter::SetTilingPattern(const string_view& patternName)
{
    checkStream();
    m_tmpStream << "/Pattern cs /" << patternName << " scn" << endl;
}

void PdfPainter::SetStrokeStyle(PdfStrokeStyle strokeStyle, const string_view& custom, bool inverted, double scale, bool subtractJoinCap)
{
    bool have = false;
    checkStream();
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
                {
                    m_tmpStream << scale * 2.0 << " " << scale * 2.0;
                }
                else
                {
                    m_tmpStream << scale * 3.0 << " " << scale * 1.0;
                }
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
    m_tmpStream << x << " "
        << y << " "
        << width << " "
        << height
        << " re W n" << endl;
}

void PdfPainter::DrawLine(double startX, double startY, double endX, double endY)
{
    checkStream();

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
    if (static_cast<int>(roundX) || static_cast<int>(roundY))
    {
        double w = width, h = height,
            rx = roundX, ry = roundY;
        double b = 0.4477f;

        MoveTo(x + rx, y);
        LineTo(x + w - rx, y);
        CubicBezierTo(x + w - rx * b, y, x + w, y + ry * b, x + w, y + ry);
        LineTo(x + w, y + h - ry);
        CubicBezierTo(x + w, y + h - ry * b, x + w - rx * b, y + h, x + w - rx, y + h);
        LineTo(x + rx, y + h);
        CubicBezierTo(x + rx * b, y + h, x, y + h - ry * b, x, y + h - ry);
        LineTo(x, y + ry);
        CubicBezierTo(x, y + ry * b, x + rx * b, y, x + rx, y);
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
    double dPointX[BEZIER_POINTS];
    double dPointY[BEZIER_POINTS];

    checkStream();

    convertRectToBezier(x, y, width, height, dPointX, dPointY);

    m_tmpStream << dPointX[0] << " "
        << dPointY[0]
        << " m" << endl;

    for (unsigned i = 1; i < BEZIER_POINTS; i += 3)
    {
        m_tmpStream << dPointX[i] << " "
            << dPointY[i] << " "
            << dPointX[i + 1] << " "
            << dPointY[i + 1] << " "
            << dPointX[i + 2] << " "
            << dPointY[i + 2]
            << " c" << endl;
    }
}

void PdfPainter::Circle(double x, double y, double radius)
{
    checkStream();

    // draw four Bezier curves to approximate a circle
    MoveTo(x + radius, y);
    CubicBezierTo(x + radius, y + radius * ARC_MAGIC,
        x + radius * ARC_MAGIC, y + radius,
        x, y + radius);
    CubicBezierTo(x - radius * ARC_MAGIC, y + radius,
        x - radius, y + radius * ARC_MAGIC,
        x - radius, y);
    CubicBezierTo(x - radius, y - radius * ARC_MAGIC,
        x - radius * ARC_MAGIC, y - radius,
        x, y - radius);
    CubicBezierTo(x + radius * ARC_MAGIC, y - radius,
        x + radius, y - radius * ARC_MAGIC,
        x + radius, y);
    Close();
}

void PdfPainter::DrawText(const string_view& str, double x, double y)
{
    checkStream();
    checkTextModeClosed();
    checkFont();

    m_tmpStream << "BT" << endl;
    writeTextState();
    drawText(str, x, y, false, false);
    m_tmpStream << "ET" << endl;
}

void PdfPainter::drawText(const std::string_view& str, double x, double y, bool isUnderline, bool isStrikeOut)
{
    m_tmpStream << x << " " << y <<  " Td ";

    auto& font = *m_TextState.Font;
    auto expStr = this->expandTabs(str);

    if (isUnderline || isStrikeOut)
    {
        this->Save();

        // Draw underline
        this->setLineWidth(font.GetUnderlineThickness(m_TextState));
        if (isUnderline)
        {
            this->DrawLine(x,
                y + font.GetUnderlinePosition(m_TextState),
                x + font.GetStringLength(expStr, m_TextState),
                y + font.GetUnderlinePosition(m_TextState));
        }

        // Draw strikeout
        this->setLineWidth(font.GetStrikeOutThickness(m_TextState));
        if (isStrikeOut)
        {
            this->DrawLine(x,
                y + font.GetStrikeOutPosition(m_TextState),
                x + font.GetStringLength(expStr, m_TextState),
                y + font.GetStrikeOutPosition(m_TextState));
        }

        this->Restore();
    }

    font.WriteStringToStream(m_tmpStream, expStr);
    m_tmpStream << " Tj" << endl;
}

void PdfPainter::BeginText(double x, double y)
{
    checkStream();
    checkTextModeClosed();

    m_tmpStream << "BT" << endl;
    m_tmpStream << x << " " << y << " Td" << endl;
    m_isTextOpen = true;
}

void PdfPainter::MoveTextPos(double x, double y)
{
    checkStream();
    checkTextModeOpened();

    m_tmpStream << x << " " << y << " Td" << endl;
}

void PdfPainter::AddText(const string_view& str)
{
    checkStream();
    checkFont();
    checkTextModeOpened();
    auto expStr = this->expandTabs(str);

    // TODO: Underline and Strikeout not yet supported
    m_TextState.Font->WriteStringToStream(m_tmpStream, expStr);

    m_tmpStream << " Tj" << endl;
}

void PdfPainter::EndText()
{
    checkStream();
    checkTextModeOpened();

    m_tmpStream << "ET" << endl;
    m_isTextOpen = false;
}

void PdfPainter::DrawMultiLineText(const string_view& str, const PdfRect& rect,
    PdfHorizontalAlignment hAlignment, PdfVerticalAlignment vAlignment, bool clip, bool skipSpaces)
{
    this->DrawMultiLineText(str, rect.GetLeft(), rect.GetBottom(), rect.GetWidth(), rect.GetHeight(),
        hAlignment, vAlignment, clip, skipSpaces);
}

void PdfPainter::DrawMultiLineText(const string_view& str, double x, double y, double width, double height,
    PdfHorizontalAlignment hAlignment, PdfVerticalAlignment vAlignment, bool clip, bool skipSpaces)
{
    checkStream();
    checkFont();
    checkTextModeClosed();

    if (width <= 0.0 || height <= 0.0) // nonsense arguments
        return;

    m_tmpStream << "BT" << endl;
    writeTextState();
    drawMultiLineText(str, x, y, width, height, hAlignment, vAlignment, clip, skipSpaces);
    m_tmpStream << "ET" << endl;
}

void PdfPainter::DrawTextAligned(const string_view& str, double x, double y, double width, PdfHorizontalAlignment hAlignment)
{
    if (width <= 0.0) // nonsense arguments
        return;

    checkStream();
    checkTextModeClosed();
    checkFont();

    m_tmpStream << "BT" << endl;
    writeTextState();
    drawTextAligned(str, x, y, width, hAlignment);
    m_tmpStream << "ET" << endl;
}

void PdfPainter::drawMultiLineText(const std::string_view& str, double x, double y, double width, double height, PdfHorizontalAlignment hAlignment, PdfVerticalAlignment vAlignment, bool clip, bool skipSpaces)
{
    auto& font = *m_TextState.Font;

    this->Save();
    if (clip)
        this->SetClipRect(x, y, width, height);

    auto expanded = this->expandTabs(str);

    vector<string> lines = getMultiLineTextAsLines(expanded, width, skipSpaces);
    double dLineGap = font.GetLineSpacing(m_TextState) - font.GetAscent(m_TextState) + font.GetDescent(m_TextState);
    // Do vertical alignment
    switch (vAlignment)
    {
        default:
        case PdfVerticalAlignment::Top:
            y += height; break;
        case PdfVerticalAlignment::Bottom:
            y += font.GetLineSpacing(m_TextState) * lines.size(); break;
        case PdfVerticalAlignment::Center:
            y += (height -
                ((height - (font.GetLineSpacing(m_TextState) * lines.size())) / 2.0));
            break;
    }

    y -= (font.GetAscent(m_TextState) + dLineGap / (2.0));

    vector<string>::const_iterator it = lines.begin();
    while (it != lines.end())
    {
        if (it->length() != 0)
            this->drawTextAligned(*it, x, y, width, hAlignment);

        y -= font.GetLineSpacing(m_TextState);
        it++;
    }
    this->Restore();
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

    auto& font = *m_TextState.Font;
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
                        m_TextState);
                }
                else
                {
                    curWidthOfLine = 0.0;
                }
            }
            ////else if( ( dCurWidthOfLine + font.GetCharWidth( *currentCharacter, m_textState) ) > width )
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
                ////    dCurWidthOfLine += font.GetCharWidth( *currentCharacter, m_textState);
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

            ////if ((dCurWidthOfLine + font.GetCharWidth(*currentCharacter, m_textState)) > width)
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
                        ////dCurWidthOfLine = font.GetCharWidth(*currentCharacter, m_textState);
                    }
                }
                else
                {
                    // The current word does not fit in the current line.
                    // -> Move it to the next one.
                    lines.push_back(string(lineBegin, (size_t)(startOfCurrentWord - lineBegin)));
                    lineBegin = startOfCurrentWord;
                    curWidthOfLine = font.GetStringLength({ startOfCurrentWord, (size_t)((currentCharacter - startOfCurrentWord) + 1) }, m_TextState);
                }
            }
            ////else
            {
                ////    dCurWidthOfLine += font.GetCharWidth(*currentCharacter, m_textState);
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

void PdfPainter::drawTextAligned(const std::string_view& str, double x, double y, double width, PdfHorizontalAlignment hAlignment)
{
    switch (hAlignment)
    {
        default:
        case PdfHorizontalAlignment::Left:
            break;
        case PdfHorizontalAlignment::Center:
            x += (width - m_TextState.Font->GetStringLength(str, m_TextState)) / 2.0;
            break;
        case PdfHorizontalAlignment::Right:
            x += (width - m_TextState.Font->GetStringLength(str, m_TextState));
            break;
    }

    this->drawText(str, x, y, false, false);
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
    m_tmpStream << "h" << endl;
}

void PdfPainter::LineTo(double x, double y)
{
    checkStream();
    m_tmpStream << x << " "
        << y
        << " l" << endl;
}

void PdfPainter::MoveTo(double x, double y)
{
    checkStream();
    m_tmpStream << x << " "
        << y
        << " m" << endl;
}

void PdfPainter::CubicBezierTo(double x1, double y1, double x2, double y2, double x3, double y3)
{
    checkStream();
    m_tmpStream << x1 << " "
        << y1 << " "
        << x2 << " "
        << y2 << " "
        << x3 << " "
        << y3
        << " c" << endl;
}

void PdfPainter::HorizontalLineTo(double x)
{
    LineTo(x, m_lpy3);
}

void PdfPainter::VerticalLineTo(double y)
{
    LineTo(m_lpx3, y);
}

void PdfPainter::SmoothCurveTo(double x2, double y2, double x3, double y3)
{
    double px, py, px2 = x2;
    double py2 = y2;
    double px3 = x3;
    double py3 = y3;

    // compute the reflective points (thanks Raph!)
    px = 2 * m_lcx - m_lrx;
    py = 2 * m_lcy - m_lry;

    m_lpx = px;
    m_lpy = py;
    m_lpx2 = px2;
    m_lpy2 = py2;
    m_lpx3 = px3;
    m_lpy3 = py3;
    m_lcx = px3;
    m_lcy = py3;
    m_lrx = px2;
    m_lry = py2;

    CubicBezierTo(px, py, px2, py2, px3, py3);
}

void PdfPainter::QuadCurveTo(double x1, double y1, double x3, double y3)
{
    double px = x1, py = y1,
        px2, py2,
        px3 = x3, py3 = y3;

    // raise quadratic bezier to cubic - thanks Raph!
    // http://www.icce.rug.nl/erikjan/bluefuzz/beziers/beziers/beziers.html
    px = (m_lcx + 2 * px) * (1.0 / 3.0);
    py = (m_lcy + 2 * py) * (1.0 / 3.0);
    px2 = (px3 + 2 * px) * (1.0 / 3.0);
    py2 = (py3 + 2 * py) * (1.0 / 3.0);

    m_lpx = px;
    m_lpy = py;
    m_lpx2 = px2;
    m_lpy2 = py2;
    m_lpx3 = px3;
    m_lpy3 = py3;
    m_lcx = px3;
    m_lcy = py3;
    m_lrx = px2;
    m_lry = py2;

    CubicBezierTo(px, py, px2, py2, px3, py3);
}

void PdfPainter::SmoothQuadCurveTo(double x3, double y3)
{
    double px, py, px2, py2,
        px3 = x3, py3 = y3;

    double xc, yc; // quadratic control point
    xc = 2 * m_lcx - m_lrx;
    yc = 2 * m_lcy - m_lry;

    // generate a quadratic bezier with control point = xc, yc
    px = (m_lcx + 2 * xc) * (1.0 / 3.0);
    py = (m_lcy + 2 * yc) * (1.0 / 3.0);
    px2 = (px3 + 2 * xc) * (1.0 / 3.0);
    py2 = (py3 + 2 * yc) * (1.0 / 3.0);

    m_lpx = px; m_lpy = py; m_lpx2 = px2; m_lpy2 = py2; m_lpx3 = px3; m_lpy3 = py3;
    m_lcx = px3;    m_lcy = py3;    m_lrx = xc;    m_lry = yc;    // thanks Raph!

    CubicBezierTo(px, py, px2, py2, px3, py3);
}

void PdfPainter::ArcTo(double x, double y, double radiusX, double radiusY,
    double rotation, bool large, bool sweep)
{
    double px = x, py = y;
    double rx = radiusX, ry = radiusY, rot = rotation;

    double sin_th, cos_th;
    double a00, a01, a10, a11;
    double x0, y0, x1, y1, xc, yc;
    double d, sfactor, sfactor_sq;
    double th0, th1, th_arc;
    int i, n_segs;

    sin_th = sin(rot);
    cos_th = cos(rot);
    a00 = cos_th / rx;
    a01 = sin_th / rx;
    a10 = -sin_th / ry;
    a11 = cos_th / ry;
    x0 = a00 * m_lcx + a01 * m_lcy;
    y0 = a10 * m_lcx + a11 * m_lcy;
    x1 = a00 * px + a01 * py;
    y1 = a10 * px + a11 * py;
    // (x0, y0) is current point in transformed coordinate space.
    // (x1, y1) is new point in transformed coordinate space.

    // The arc fits a unit-radius circle in this space.
    d = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
    sfactor_sq = 1.0 / d - 0.25;
    if (sfactor_sq < 0)
        sfactor_sq = 0;
    sfactor = sqrt(sfactor_sq);
    if (sweep == large)
        sfactor = -sfactor;
    xc = 0.5 * (x0 + x1) - sfactor * (y1 - y0);
    yc = 0.5 * (y0 + y1) + sfactor * (x1 - x0);
    // (xc, yc) is center of the circle

    th0 = atan2(y0 - yc, x0 - xc);
    th1 = atan2(y1 - yc, x1 - xc);

    th_arc = th1 - th0;
    if (th_arc < 0 && sweep)
        th_arc += 2 * numbers::pi;
    else if (th_arc > 0 && !sweep)
        th_arc -= 2 * numbers::pi;

    n_segs = static_cast<int>(ceil(fabs(th_arc / (numbers::pi * 0.5 + 0.001))));

    for (i = 0; i < n_segs; i++)
    {
        double nth0 = th0 + (double)i * th_arc / n_segs,
            nth1 = th0 + ((double)i + 1) * th_arc / n_segs;
        double nsin_th = 0.0;
        double ncos_th = 0.0;
        double na00 = 0.0;
        double na01 = 0.0;
        double na10 = 0.0;
        double na11 = 0.0;
        double nx1 = 0.0;
        double ny1 = 0.0;
        double nx2 = 0.0;
        double ny2 = 0.0;
        double nx3 = 0.0;
        double ny3 = 0.0;
        double t = 0.0;
        double th_half = 0.0;

        nsin_th = sin(rot);
        ncos_th = cos(rot);
        /* inverse transform compared with rsvg_path_arc */
        na00 = ncos_th * rx;
        na01 = -nsin_th * ry;
        na10 = nsin_th * rx;
        na11 = ncos_th * ry;

        th_half = 0.5 * (nth1 - nth0);
        t = (8.0 / 3.0) * sin(th_half * 0.5) * sin(th_half * 0.5) / sin(th_half);
        nx1 = xc + cos(nth0) - t * sin(nth0);
        ny1 = yc + sin(nth0) + t * cos(nth0);
        nx3 = xc + cos(nth1);
        ny3 = yc + sin(nth1);
        nx2 = nx3 + t * sin(nth1);
        ny2 = ny3 - t * cos(nth1);
        nx1 = na00 * nx1 + na01 * ny1;
        ny1 = na10 * nx1 + na11 * ny1;
        nx2 = na00 * nx2 + na01 * ny2;
        ny2 = na10 * nx2 + na11 * ny2;
        nx3 = na00 * nx3 + na01 * ny3;
        ny3 = na10 * nx3 + na11 * ny3;
        CubicBezierTo(nx1, ny1, nx2, ny2, nx3, ny3);
    }

    m_lpx = m_lpx2 = m_lpx3 = px;
    m_lpy = m_lpy2 = m_lpy3 = py;
    m_lcx = px;
    m_lcy = py;
    m_lrx = px;
    m_lry = py;
}

bool PdfPainter::Arc(double x, double y, double radius, double angle1, double angle2)
{
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented,
        "FIX-ME to accept input as radians. Degrees input is no more supported");

    bool cont_flg = false;

    if (angle1 >= angle2 || (angle2 - angle1) >= 360.0f)
        return false;

    while (angle1 < 0.0f || angle2 < 0.0f) {
        angle1 = angle1 + 360.0f;
        angle2 = angle2 + 360.0f;
    }

    while (true)
    {
        if (angle2 - angle1 <= 90.0f)
        {
            internalArc(x, y, radius, angle1, angle2, cont_flg);
            return true;
        }
        else
        {
            double tmp_ang = angle1 + 90.0f;

            internalArc(x, y, radius, angle1, tmp_ang, cont_flg);

            angle1 = tmp_ang;
        }

        if (angle1 >= angle2)
            break;

        cont_flg = true;
    }

    return true;
}

// NOTE: This should have already been ported to use radians
// but the code is still fishy, see delta_angle/new_angle
void PdfPainter::internalArc(double x, double y, double ray, double ang1,
    double ang2, bool contFlg)
{
    double rx0, ry0, rx1, ry1, rx2, ry2, rx3, ry3;
    double x0, y0, x1, y1, x2, y2, x3, y3;
    double delta_angle = numbers::pi / 2 - (ang1 + ang2) / 2.0;
    double new_angle = (ang2 - ang1) / 2.0;

    rx0 = ray * cos(new_angle);
    ry0 = ray * sin(new_angle);
    rx2 = (ray * 4.0 - rx0) / 3.0;
    ry2 = ((ray * 1.0 - rx0) * (rx0 - ray * 3.0)) / (3.0 * ry0);
    rx1 = rx2;
    ry1 = -ry2;
    rx3 = rx0;
    ry3 = -ry0;

    x0 = rx0 * cos(delta_angle) - ry0 * sin(delta_angle) + x;
    y0 = rx0 * sin(delta_angle) + ry0 * cos(delta_angle) + y;
    x1 = rx1 * cos(delta_angle) - ry1 * sin(delta_angle) + x;
    y1 = rx1 * sin(delta_angle) + ry1 * cos(delta_angle) + y;
    x2 = rx2 * cos(delta_angle) - ry2 * sin(delta_angle) + x;
    y2 = rx2 * sin(delta_angle) + ry2 * cos(delta_angle) + y;
    x3 = rx3 * cos(delta_angle) - ry3 * sin(delta_angle) + x;
    y3 = rx3 * sin(delta_angle) + ry3 * cos(delta_angle) + y;

    if (!contFlg)
        MoveTo(x0, y0);

    CubicBezierTo(x1, y1, x2, y2, x3, y3);

    //attr->cur_pos.x = (HPDF_REAL)x3;
    //attr->cur_pos.y = (HPDF_REAL)y3;
    m_lcx = x3;
    m_lcy = y3;

    m_lpx = m_lpx2 = m_lpx3 = x3;
    m_lpy = m_lpy2 = m_lpy3 = y3;
    m_lcx = x3;
    m_lcy = y3;
    m_lrx = x3;
    m_lry = y3;
}

void PdfPainter::Close()
{
    checkStream();
    m_tmpStream << "h" << endl;
}

void PdfPainter::Stroke()
{
    checkStream();
    m_tmpStream << "S" << endl;
}

void PdfPainter::Fill(bool useEvenOddRule)
{
    checkStream();
    if (useEvenOddRule)
        m_tmpStream << "f*" << endl;
    else
        m_tmpStream << "f" << endl;
}

void PdfPainter::FillAndStroke(bool useEvenOddRule)
{
    checkStream();
    if (useEvenOddRule)
        m_tmpStream << "B*" << endl;
    else
        m_tmpStream << "B" << endl;
}

void PdfPainter::Clip(bool useEvenOddRule)
{
    checkStream();
    if (useEvenOddRule)
        m_tmpStream << "W* n" << endl;
    else
        m_tmpStream << "W n" << endl;
}

void PdfPainter::EndPath()
{
    checkStream();
    m_tmpStream << "n" << endl;
}

void PdfPainter::Save()
{
    checkStream();
    m_tmpStream << "q" << endl;
}

void PdfPainter::Restore()
{
    checkStream();
    m_tmpStream << "Q" << endl;
}


void PdfPainter::SetPrecision(unsigned short precision)
{
    m_tmpStream.SetPrecision(precision);
}

unsigned short PdfPainter::GetPrecision() const
{
    return static_cast<unsigned char>(m_tmpStream.GetPrecision());
}

void PdfPainter::SetExtGState(const PdfExtGState& inGState)
{
    checkStream();
    this->addToPageResources("ExtGState", inGState.GetIdentifier(), inGState.GetObject());
    m_tmpStream << "/" << inGState.GetIdentifier().GetString()
        << " gs" << endl;
}

void PdfPainter::Rectangle(const PdfRect& rect, double roundX, double roundY)
{
    this->Rectangle(rect.GetLeft(), rect.GetBottom(),
        rect.GetWidth(), rect.GetHeight(),
        roundX, roundY);
}

void PdfPainter::BeginMarkedContext(const string_view& tag)
{
    m_tmpStream << '/' << tag << " BMC" << endl;
}

void PdfPainter::EndMarkedContext()
{
    m_tmpStream << "EMC" << endl;
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

void PdfPainter::SetTransformationMatrix(const Matrix& matrix)
{
    checkStream();
    m_tmpStream
        << matrix[0] << " "
        << matrix[1] << " "
        << matrix[2] << " "
        << matrix[3] << " "
        << matrix[4] << " "
        << matrix[5] << " cm" << endl;
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

void PdfPainter::writeTextState()
{
    if (m_TextState.Font != nullptr)
        setFont(m_TextState.Font, m_TextState.FontSize);

    if (m_TextState.FontScale != 1)
        setFontScale(m_TextState.FontScale);

    if (m_TextState.CharSpacing != 0)
        setCharSpacing(m_TextState.CharSpacing);

    if (m_TextState.WordSpacing != 0)
        setWordSpacing(m_TextState.WordSpacing);

    if (m_TextState.RenderingMode != PdfTextRenderingMode::Fill)
        setTextRenderingMode(m_TextState.RenderingMode);
}

void PdfPainter::SetFont(const PdfFont* font, double fontSize)
{
    if (font == nullptr)
        return;

    this->addToPageResources("Font", font->GetIdentifier(), font->GetObject());
    if (m_isTextOpen)
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
    if (m_isTextOpen)
        setFontScale(value);
}

void PdfPainter::setFontScale(double value)
{
    checkStream();
    m_tmpStream << value * 100 << " Tz" << endl;
}

void PdfPainter::SetCharSpacing(double value)
{
    if (m_isTextOpen)
        setCharSpacing(value);
}

void PdfPainter::setCharSpacing(double value)
{
    checkStream();
    m_tmpStream << value << " Tc" << endl;
}

void PdfPainter::SetWordSpacing(double value)
{
    if (m_isTextOpen)
        setWordSpacing(value);
}

void PdfPainter::setWordSpacing(double value)
{
    checkStream();
    m_tmpStream << value << " Tw" << endl;
}

void PdfPainter::SetTextRenderingMode(PdfTextRenderingMode value)
{
    if (m_isTextOpen)
        setTextRenderingMode(value);
}

void PdfPainter::setTextRenderingMode(PdfTextRenderingMode value)
{
    checkStream();
    m_tmpStream << (int)value << " Tr" << endl;
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
    if (m_TextState.Font == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Font should be set prior calling the method");
}

void PdfPainter::checkTextModeOpened()
{
    if (!m_isTextOpen)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Text writing is not opened");
}

void PdfPainter::checkTextModeClosed()
{
    if (m_isTextOpen)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Text writing is already opened");
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
