/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPainter.h"

#include <podofo/private/PdfDrawingOperations.h>

#include <utf8cpp/utf8.h>

#include "PdfExtGState.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfImage.h"
#include "PdfDocument.h"

using namespace std;
using namespace PoDoFo;

static PdfColorSpaceFilterPtr getSimpleColorSpaceFilter(PdfColorSpaceType type);
static string expandTabs(const string_view& str, unsigned tabWidth, unsigned tabCount);

PdfPainter::PdfPainter(PdfPainterFlags flags) :
    m_flags(flags),
    m_painterStatus(StatusDefault),
    m_textStackCount(0),
    GraphicsState(*this, m_StateStack.Current->GraphicsState),
    TextState(*this, m_StateStack.Current->TextState),
    TextObject(*this),
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
        if (std::uncaught_exceptions() == 0)
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
    canvas.EnsureResourcesCreated();
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

void PdfPainter::SetClipRect(const Rect& rect)
{
    this->SetClipRect(rect.X, rect.Y, rect.Width, rect.Height);
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
    PoDoFo::WriteOperator_m(m_stream, x1, y1);
    PoDoFo::WriteOperator_l(m_stream, x2, y2);
    stroke();
    resetPath();
}

void PdfPainter::DrawCubicBezier(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4)
{
    checkStream();
    checkStatus(StatusDefault);
    PoDoFo::WriteOperator_m(m_stream, x1, y1);
    PoDoFo::WriteOperator_c(m_stream, x2, y2, x3, y3, x4, y4);
    stroke();
    resetPath();
}

void PdfPainter::DrawArc(double x, double y, double radius, double startAngle, double endAngle, bool clockWise)
{
    checkStream();
    checkStatus(StatusDefault);
    Vector2 currP;
    PoDoFo::WriteArc(m_stream, x, y, radius, startAngle, endAngle, clockWise, currP);
    stroke();
    resetPath();
}

void PdfPainter::DrawCircle(double x, double y, double radius, PdfPathDrawMode mode)
{
    checkStream();
    checkStatus(StatusDefault);
    Vector2 currP;
    PoDoFo::WriteCircle(m_stream, x, y, radius, currP);
    drawPath(mode);
    resetPath();
}

void PdfPainter::DrawEllipse(double x, double y, double width, double height, PdfPathDrawMode mode)
{
    checkStream();
    checkStatus(StatusDefault);
    Vector2 currP;
    PoDoFo::WriteEllipse(m_stream, x, y, width, height, currP);
    drawPath(mode);
    resetPath();
}

void PdfPainter::DrawRectangle(double x, double y, double width, double height, PdfPathDrawMode mode, double roundX, double roundY)
{
    drawRectangle(x, y, width, height, mode, roundX, roundY);
}

void PdfPainter::DrawRectangle(const Rect& rect, PdfPathDrawMode mode, double roundX, double roundY)
{
    drawRectangle(rect.X, rect.Y, rect.Width, rect.Height, mode, roundX, roundY);
}

void PdfPainter::DrawText(const string_view& str, double x, double y,
    PdfDrawTextStyle style)
{
    checkStream();
    checkStatus(StatusDefault);
    checkFont();

    vector<array<double, 4>> linesToDraw;
    save();
    PoDoFo::WriteOperator_BT(m_stream);
    writeTextState();
    drawText(str, x, y,
        (style & PdfDrawTextStyle::Underline) != PdfDrawTextStyle::Regular,
        (style & PdfDrawTextStyle::StrikeThrough) != PdfDrawTextStyle::Regular, linesToDraw);
    PoDoFo::WriteOperator_ET(m_stream);
    drawLines(linesToDraw);
    restore();
}

void PdfPainter::drawText(const string_view& str, double x, double y,
    bool isUnderline, bool isStrikeThrough, vector<array<double, 4>>& linesToDraw)
{
    auto& textState = m_StateStack.Current->TextState;
    auto& font = *textState.Font;
    auto expStr = this->expandTabs(str);

    if (isUnderline || isStrikeThrough)
    {
        // Draw underline
        this->setLineWidth(font.GetUnderlineThickness(textState));
        if (isUnderline)
        {
            linesToDraw.push_back({ x,
                y + font.GetUnderlinePosition(textState),
                x + font.GetStringLength(expStr, textState),
                y + font.GetUnderlinePosition(textState)
            });
        }

        // Draw strikethrough
        if (isStrikeThrough)
        {
            linesToDraw.push_back({ x,
                y + font.GetStrikeThroughPosition(textState),
                x + font.GetStringLength(expStr, textState),
                y + font.GetStrikeThroughPosition(textState)
            });
        }
    }

    PoDoFo::WriteOperator_Td(m_stream, x, y);

    PoDoFo::WriteOperator_Tj(m_stream, font.GetEncoding().ConvertToEncoded(str),
        !font.GetEncoding().IsSimpleEncoding());
}

void PdfPainter::DrawTextMultiLine(const string_view& str, const Rect& rect,
    const PdfDrawTextMultiLineParams& params)
{
    this->DrawTextMultiLine(str, rect.X, rect.Y, rect.Width, rect.Height, params);
}

void PdfPainter::DrawTextMultiLine(const string_view& str, double x, double y, double width, double height,
    const PdfDrawTextMultiLineParams& params)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    checkFont();

    if (width <= 0 || height <= 0) // nonsense arguments
        return;

    drawMultiLineText(str, x, y, width, height,
        params.HorizontalAlignment, params.VerticalAlignment,
        params.SkipClip, params.PreserveTrailingSpaces, params.Style);
}

void PdfPainter::DrawTextAligned(const string_view& str, double x, double y, double width,
    PdfHorizontalAlignment hAlignment, PdfDrawTextStyle style)
{
    if (width <= 0.0) // nonsense arguments
        return;

    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    checkFont();

    save();
    PoDoFo::WriteOperator_BT(m_stream);
    writeTextState();
    vector<array<double, 4>> linesToDraw;
    drawTextAligned(str, x, y, width, hAlignment, style, linesToDraw);
    PoDoFo::WriteOperator_ET(m_stream);
    drawLines(linesToDraw);
    restore();
}

void PdfPainter::drawMultiLineText(const string_view& str, double x, double y, double width, double height,
    PdfHorizontalAlignment hAlignment, PdfVerticalAlignment vAlignment, bool skipClip, bool preserveTrailingSpaces,
    PdfDrawTextStyle style)
{
    auto& textState = m_StateStack.Current->TextState;
    auto& font = *textState.Font;

    this->save();
    if (!skipClip)
        this->SetClipRect(x, y, width, height);

    auto expanded = this->expandTabs(str);

    PoDoFo::WriteOperator_BT(m_stream);
    writeTextState();
    vector<string> lines = m_StateStack.Current->TextState.SplitTextAsLines(str, width, preserveTrailingSpaces);
    double lineGap = font.GetLineSpacing(textState) - font.GetAscent(textState) + font.GetDescent(textState);
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
            y += height - (height - (font.GetLineSpacing(textState) * lines.size())) / 2;
            break;
    }

    y -= font.GetAscent(textState) + lineGap / 2;
    vector<array<double, 4>> linesToDraw;
    for (auto& line : lines)
    {
        if (line.length() != 0)
            this->drawTextAligned(line, x, y, width, hAlignment, style, linesToDraw);

        x = 0;
        switch (hAlignment)
        {
            default:
            case PdfHorizontalAlignment::Left:
                break;
            case PdfHorizontalAlignment::Center:
                x = -(width - textState.Font->GetStringLength(line, textState)) / 2.0;
                break;
            case PdfHorizontalAlignment::Right:
                x = -(width - textState.Font->GetStringLength(line, textState));
                break;
        }
        y = -font.GetLineSpacing(textState);
    }
    PoDoFo::WriteOperator_ET(m_stream);
    drawLines(linesToDraw);
    this->restore();
}

void PdfPainter::drawTextAligned(const string_view& str, double x, double y, double width,
    PdfHorizontalAlignment hAlignment, PdfDrawTextStyle style, vector<array<double, 4>>& linesToDraw)
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
        (style & PdfDrawTextStyle::StrikeThrough) != PdfDrawTextStyle::Regular,
        linesToDraw);
}

void PdfPainter::DrawImage(const PdfImage& obj, double x, double y, double scaleX, double scaleY)
{
    this->DrawXObject(obj, x, y,
        scaleX * obj.GetRect().Width,
        scaleY * obj.GetRect().Height);
}

void PdfPainter::DrawXObject(const PdfXObject& obj, double x, double y, double scaleX, double scaleY)
{
    checkStream();
    PoDoFo::WriteOperator_q(m_stream);
    PoDoFo::WriteOperator_cm(m_stream, scaleX, 0, 0, scaleY, x, y);
    PoDoFo::WriteOperator_Do(m_stream, tryAddResource(obj.GetObject(), PdfResourceType::XObject));
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

    ((OutputStream&)m_stream).Write(path.GetContent());
    drawPath(drawMode);
    resetPath();
}

// CHECK-ME: Handle of first/current point
void PdfPainter::ClipPath(const PdfPainterPath& path, bool useEvenOddRule)
{
    checkStream();
    checkStatus(StatusDefault);

    ((OutputStream&)m_stream).Write(path.GetContent());
    if (useEvenOddRule)
        PoDoFo::WriteOperator_WStar(m_stream);
    else
        PoDoFo::WriteOperator_W(m_stream);

    PoDoFo::WriteOperator_n(m_stream);
    resetPath();
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

void PdfPainter::SetExtGState(const PdfExtGState& extGState)
{
    checkStream();
    PoDoFo::WriteOperator_gs(m_stream, tryAddResource(extGState.GetObject(), PdfResourceType::ExtGState));
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

string_view PdfPainter::GetContent() const
{
    return m_stream.GetString();
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

void PdfPainter::SetNonStrokingColor(const PdfColor& color)
{
    checkStream();
    switch (color.GetColorSpace())
    {
        case PdfColorSpaceType::DeviceRGB:
        {
            PoDoFo::WriteOperator_rg(m_stream, color.GetRed(), color.GetGreen(), color.GetBlue());
            break;
        }
        case PdfColorSpaceType::DeviceCMYK:
        {
            PoDoFo::WriteOperator_k(m_stream, color.GetCyan(), color.GetMagenta(), color.GetYellow(), color.GetBlack());
            break;
        }
        case PdfColorSpaceType::DeviceGray:
        {
            PoDoFo::WriteOperator_g(m_stream, color.GetGrayScale());
            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::CannotConvertColor, "Unsupported color space");
        }
    }
}

void PdfPainter::SetStrokingColor(const PdfColor& color)
{
    checkStream();
    switch (color.GetColorSpace())
    {
        case PdfColorSpaceType::DeviceRGB:
        {
            PoDoFo::WriteOperator_RG(m_stream, color.GetRed(), color.GetGreen(), color.GetBlue());
            break;
        }
        case PdfColorSpaceType::DeviceCMYK:
        {
            PoDoFo::WriteOperator_K(m_stream, color.GetCyan(), color.GetMagenta(), color.GetYellow(), color.GetBlack());
            break;
        }
        case PdfColorSpaceType::DeviceGray:
        {
            PoDoFo::WriteOperator_G(m_stream, color.GetGrayScale());
            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::CannotConvertColor, "Unsupported color space");
        }
    }
}

void PdfPainter::SetNonStrokingColor(const PdfColorRaw& color, const PdfColorSpaceFilter& colorSpace)
{
    checkStream();
    PoDoFo::WriteOperator_scn(m_stream, cspan<double>(color.data(), colorSpace.GetColorComponentCount()));
}

void PdfPainter::SetStrokingColor(const PdfColorRaw& color, const PdfColorSpaceFilter& colorSpace)
{
    checkStream();
    PoDoFo::WriteOperator_SCN(m_stream, cspan<double>(color.data(), colorSpace.GetColorComponentCount()));
}

void PdfPainter::SetNonStrokingColorSpace(const PdfColorSpaceFilter& filter, const PdfColorSpace* colorSpace)
{
    checkStream();
    if (colorSpace == nullptr)
    {
        if (filter.IsTrivial())
        {
            PoDoFo::WriteOperator_cs(m_stream, PoDoFo::ToString(filter.GetType()));
        }
        else
        {
            auto& objects = m_canvas->GetElement().GetDocument().GetObjects();
            setNonStrokingColorSpace(objects.CreateDictionaryObject() = filter.GetExportObject(objects));
        }
    }
    else
    {
        setNonStrokingColorSpace(colorSpace->GetObject());
    }
}

void PdfPainter::setNonStrokingColorSpace(const PdfObject& csObj)
{
    PoDoFo::WriteOperator_cs(m_stream, tryAddResource(csObj, PdfResourceType::ColorSpace));
}

void PdfPainter::SetStrokingColorSpace(const PdfColorSpaceFilter& filter, const PdfColorSpace* colorSpace)
{
    checkStream();
    if (colorSpace == nullptr)
    {
        if (filter.IsTrivial())
        {
            PoDoFo::WriteOperator_CS(m_stream, PoDoFo::ToString(filter.GetType()));
        }
        else
        {
            auto& objects = m_canvas->GetElement().GetDocument().GetObjects();
            setStrokingColorSpace(objects.CreateDictionaryObject() = filter.GetExportObject(objects));
        }
    }
    else
    {
        setStrokingColorSpace(colorSpace->GetObject());
    }
}

void PdfPainter::setStrokingColorSpace(const PdfObject& csObj)
{
    PoDoFo::WriteOperator_CS(m_stream, tryAddResource(csObj, PdfResourceType::ColorSpace));
}

PdfName PdfPainter::tryAddResource(const PdfObject& obj, PdfResourceType type)
{
    auto found = m_resNameCache.find(obj.GetIndirectReference());
    if (found == m_resNameCache.end())
    {
        auto name = m_canvas->GetOrCreateResources().AddResource(type, obj);
        m_resNameCache[obj.GetIndirectReference()] = name;
        return name;
    }

    return found->second;
}

void PdfPainter::drawLines(const vector<array<double, 4>>& lines)
{
    for (auto& line : lines)
        this->DrawLine(line[0], line[1], line[2], line[3]);
}

void PdfPainter::SetFont(const PdfFont& font, double fontSize)
{
    checkStream();
    if (m_painterStatus == StatusTextObject)
        setFont(font, fontSize);
}

void PdfPainter::setFont(const PdfFont& font, double fontSize)
{
    auto& textState = m_StateStack.Current->EmittedTextState;
    if (textState.Font == &font
        && textState.FontSize == fontSize)
    {
        return;
    }

    PoDoFo::WriteOperator_Tf(m_stream, tryAddResource(font.GetObject(), PdfResourceType::Font), fontSize);
    textState.Font = &font;
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
        setFont(*textState.Font, textState.FontSize);

    if (textState.FontScale != 1)
        setFontScale(textState.FontScale);

    if (textState.CharSpacing != 0)
        setCharSpacing(textState.CharSpacing);

    if (textState.WordSpacing != 0)
        setWordSpacing(textState.WordSpacing);

    if (textState.RenderingMode != PdfTextRenderingMode::Fill)
        setTextRenderingMode(textState.RenderingMode);
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

void PdfPainter::checkPathOpened() const
{
    if (m_StateStack.Current->CurrentPoint == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Path should be opened with m operator");
}

void PdfPainter::checkStream()
{
    if (m_objStream != nullptr)
        return;

    PODOFO_RAISE_LOGIC_IF(m_canvas == nullptr, "Call SetCanvas() first before doing drawing operations");
    m_objStream = &m_canvas->GetOrCreateContentsStream((PdfStreamAppendFlags)(m_flags & (~PdfPainterFlags::NoSaveRestore)));
}

void PdfPainter::openPath(double x, double y)
{
    if (m_StateStack.Current->FirstPoint != nullptr)
        return;

    m_StateStack.Current->FirstPoint = Vector2(x, y);
}

// Reset must be done after drawing operators (s, S, b, b*, B, B*, f, f*)
// and n operator (discard)
void PdfPainter::resetPath()
{
    m_StateStack.Current->FirstPoint = nullptr;
    m_StateStack.Current->CurrentPoint = nullptr;
}

void PdfPainter::checkFont() const
{
    auto& textState = m_StateStack.Current->TextState;
    if (textState.Font == nullptr)
        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Font should be set prior calling the method");
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

PdfPainterTextObject::PdfPainterTextObject(PdfPainter& painter)
    : m_painter(&painter)
{
}

void PdfPainterTextObject::Begin()
{
    m_painter->BeginText();
}

void PdfPainterTextObject::MoveTo(double x, double y)
{
    m_painter->TextMoveTo(x, y);
}

void PdfPainterTextObject::AddText(const string_view& str)
{
    m_painter->AddText(str);
}

void PdfPainterTextObject::End()
{
    m_painter->EndText();
}

PdfGraphicsStateWrapper::PdfGraphicsStateWrapper(PdfPainter& painter, PdfGraphicsState& state)
    : m_painter(&painter), m_state(&state) { }

void PdfGraphicsStateWrapper::ConcatenateTransformationMatrix(const Matrix& matrix)
{
    m_state->CTM = matrix * m_state->CTM;
    m_painter->SetTransformationMatrix(matrix);
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

void PdfGraphicsStateWrapper::SetNonStrokingColorSpace(PdfColorSpaceInitializer&& colorSpace)
{
    if (m_state->NonStrokingColorSpaceFilter.get() == &colorSpace.GetFilter())
        return;

    const PdfColorSpace* element;
    m_state->NonStrokingColorSpaceFilter = colorSpace.Take(element);
    m_painter->SetNonStrokingColorSpace(*m_state->NonStrokingColorSpaceFilter, element);
}

void PdfGraphicsStateWrapper::SetStrokingColorSpace(PdfColorSpaceInitializer&& colorSpace)
{
    if (m_state->StrokingColorSpaceFilter.get() == &colorSpace.GetFilter())
        return;

    const PdfColorSpace* element;
    m_state->StrokingColorSpaceFilter = colorSpace.Take(element);
    m_painter->SetStrokingColorSpace(*m_state->StrokingColorSpaceFilter, element);
}

void PdfGraphicsStateWrapper::SetNonStrokingColor(const PdfColor& color)
{
    if (m_state->NonStrokingColorSpaceFilter->GetType() == color.GetColorSpace()
        && m_state->NonStrokingColor == color.GetRawColor())
    {
        return;
    }

    m_state->NonStrokingColorSpaceFilter = getSimpleColorSpaceFilter(color.GetColorSpace());
    m_state->NonStrokingColor = color.GetRawColor();
    m_painter->SetNonStrokingColor(color);
}

void PdfGraphicsStateWrapper::SetStrokingColor(const PdfColor& color)
{
    if (m_state->StrokingColorSpaceFilter->GetType() == color.GetColorSpace()
        && m_state->StrokingColor == color.GetRawColor())
    {
        return;
    }

    m_state->StrokingColorSpaceFilter = getSimpleColorSpaceFilter(color.GetColorSpace());
    m_state->StrokingColor = color.GetRawColor();
    m_painter->SetStrokingColor(color);
}

void PdfGraphicsStateWrapper::SetNonStrokingColor(const PdfColorRaw& color)
{
    if (m_state->NonStrokingColor == color)
        return;

    m_state->NonStrokingColor = color;
    m_painter->SetNonStrokingColor(color, *m_state->NonStrokingColorSpaceFilter);
}

void PdfGraphicsStateWrapper::SetStrokingColor(const PdfColorRaw& color)
{
    if (m_state->StrokingColor == color)
        return;

    m_state->StrokingColor = color;
    m_painter->SetStrokingColor(color, *m_state->StrokingColorSpaceFilter);
}

void PdfGraphicsStateWrapper::SetExtGState(const PdfExtGState& extGState)
{
    if (m_state->ExtGState != nullptr
        && m_state->ExtGState->GetObject().GetIndirectReference() == extGState.GetObject().GetIndirectReference())
    {
        return;
    }

    m_state->ExtGState.reset(new PdfExtGState(extGState));
    m_painter->SetExtGState(extGState);
}

PdfTextStateWrapper::PdfTextStateWrapper(PdfPainter& painter, PdfTextState& state)
    : m_painter(&painter), m_State(&state) { }

void PdfTextStateWrapper::SetFont(const PdfFont& font, double fontSize)
{
    if (m_State->Font == &font && m_State->FontSize == fontSize)
        return;

    m_State->Font = &font;
    m_State->FontSize = fontSize;
    m_painter->SetFont(*m_State->Font, m_State->FontSize);
}

void PdfTextStateWrapper::SetFontScale(double scale)
{
    if (m_State->FontScale == scale)
        return;

    m_State->FontScale = scale;
    m_painter->SetFontScale(m_State->FontScale);
}

void PdfTextStateWrapper::SetCharSpacing(double charSpacing)
{
    if (m_State->CharSpacing == charSpacing)
        return;

    m_State->CharSpacing = charSpacing;
    m_painter->SetCharSpacing(m_State->CharSpacing);
}

void PdfTextStateWrapper::SetWordSpacing(double wordSpacing)
{
    if (m_State->WordSpacing == wordSpacing)
        return;

    m_State->WordSpacing = wordSpacing;
    m_painter->SetWordSpacing(m_State->WordSpacing);
}

void PdfTextStateWrapper::SetRenderingMode(PdfTextRenderingMode mode)
{
    if (m_State->RenderingMode == mode)
        return;

    m_State->RenderingMode = mode;
    m_painter->SetTextRenderingMode(m_State->RenderingMode);
}

void PdfPainter::drawRectangle(double x, double y, double width, double height, PdfPathDrawMode mode, double roundX, double roundY)
{
    checkStream();
    checkStatus(StatusDefault);
    Vector2 currP;
    PoDoFo::WriteRectangle(m_stream, x, y, width, height, roundX, roundY, currP);
    drawPath(mode);
    resetPath();
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

PdfColorSpaceFilterPtr getSimpleColorSpaceFilter(PdfColorSpaceType type)
{
    switch (type)
    {
        case PdfColorSpaceType::DeviceGray:
            return PdfColorSpaceFilterFactory::GetDeviceGrayInstace();
        case PdfColorSpaceType::DeviceRGB:
            return PdfColorSpaceFilterFactory::GetDeviceRGBInstace();
        case PdfColorSpaceType::DeviceCMYK:
            return PdfColorSpaceFilterFactory::GetDeviceCMYKInstace();
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::CannotConvertColor, "Unsupported color space");
    }
}

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
