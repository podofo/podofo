/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_PAINTER_H
#define PDF_PAINTER_H

#include "PdfRect.h"
#include "PdfCanvas.h"
#include "PdfTextState.h"
#include "PdfGraphicsState.h"
#include "PdfPainterContexts.h"
#include <podofo/common/StateStack.h>

#include <podofo/staging/PdfShadingPattern.h>
#include <podofo/staging/PdfTilingPattern.h>

namespace PoDoFo {

class PdfExtGState;
class PdfFont;
class PdfImage;
class PdfObjectStream;
class PdfXObject;

enum class PdfDrawTextStyle
{
    Regular = 0,
    StrikeOut = 1,
    Underline = 2,
};

struct PODOFO_API PdfDrawTextMultiLineParams final
{
    PdfDrawTextStyle Style = PdfDrawTextStyle::Regular;                         ///< style of the draw text operation
    PdfHorizontalAlignment HorizontalAlignment = PdfHorizontalAlignment::Left;  ///< alignment of the individual text lines in the given bounding box
    PdfVerticalAlignment VerticalAlignment = PdfVerticalAlignment::Top;         ///< vertical alignment of the text in the given bounding box
    bool Clip = true;                                                           ///< set the clipping rectangle to the given rect, otherwise no clipping is performed
    bool SkipSpaces = true;                                                     ///< whether the trailing whitespaces should be skipped, so that next line doesn't start with whitespace
};

enum class PdfPainterFlags
{
    None = 0,
    Prepend = 1,            ///< Does nothing for now
    NoSaveRestorePrior = 2, ///< Do not perform a Save/Restore or previous content. Implies RawCoordinates
    NoSaveRestore = 4,      ///< Do not perform a Save/Restore of added content in this painting session
    RawCoordinates = 8,     ///< Does nothing for now
};

class PdfPainter;

struct PODOFO_API PdfPainterState final
{
    Vector2 CurrentPoint;
    PdfGraphicsState GraphicsState;
    PdfTextState TextState;         ///< The current sematical text state
private:
    friend class PdfPainter;
private:
    PdfTextState EmittedTextState;  ///< The actually emitted text state
};

using PdfPainterStateStack = StateStack<PdfPainterState>;

class PODOFO_API PdfGraphicsStateWrapper final
{
    friend class PdfPainter;

private:
    PdfGraphicsStateWrapper(PdfPainter& painter, PdfGraphicsState& state);

public:
    void SetCurrentMatrix(const Matrix& matrix);
    void SetLineWidth(double lineWidth);
    void SetMiterLevel(double value);
    void SetLineCapStyle(PdfLineCapStyle capStyle);
    void SetLineJoinStyle(PdfLineJoinStyle joinStyle);
    void SetRenderingIntent(const std::string_view& intent);
    void SetFillColor(const PdfColor& color);
    void SetStrokeColor(const PdfColor& color);

public:
    const Matrix& GetCurrentMatrix() { return m_state->CTM; }
    double GetLineWidth() const { return m_state->LineWidth; }
    double GetMiterLevel() const { return m_state->MiterLimit; }
    PdfLineCapStyle GetLineCapStyle() const { return m_state->LineCapStyle; }
    PdfLineJoinStyle GetLineJoinStyle() const { return m_state->LineJoinStyle; }
    const std::string& GetRenderingIntent() const { return m_state->RenderingIntent; }
    const PdfColor& GetFillColor() const { return m_state->FillColor; }
    const PdfColor& GetStrokeColor() const { return m_state->StrokeColor; }

public:
    operator const PdfGraphicsState&() const { return *m_state; }

private:
    void SetState(PdfGraphicsState& state) { m_state = &state; }

private:
    PdfPainter* m_painter;
    PdfGraphicsState* m_state;
};

class PODOFO_API PdfTextStateWrapper final
{
    friend class PdfPainter;

private:
    PdfTextStateWrapper(PdfPainter& painter, PdfTextState& state);

public:
    void SetFont(const PdfFont& font, double fontSize);

    /** Set the current horizontal scaling (operator Tz)
     *
     *  \param scale scaling in [0,1]
     */
    void SetFontScale(double scale);

    /** Set the character spacing (operator Tc)
     *  \param charSpace character spacing in percent
     */
    void SetCharSpacing(double charSpacing);

    /** Set the word spacing (operator Tw)
     *  \param fWordSpace word spacing in PDF units
     */
    void SetWordSpacing(double wordSpacing);

    void SetRenderingMode(PdfTextRenderingMode mode);

public:
    inline const PdfFont* GetFont() const { return m_state->Font; }

    /** Retrieve the current font size (operator Tf, controlling Tfs)
     *  \returns the current font size
     */
    inline double GetFontSize() const { return m_state->FontSize; }

    /** Retrieve the current horizontal scaling (operator Tz)
     *  \returns the current font scaling in [0,1]
     */
    inline double GetFontScale() const { return m_state->FontScale; }

    /** Retrieve the character spacing (operator Tc)
     *  \returns the current font character spacing
     */
    inline double GetCharSpacing() const { return m_state->CharSpacing; }

    /** Retrieve the current word spacing (operator Tw)
     *  \returns the current font word spacing in PDF units
     */
    inline double GetWordSpacing() const { return m_state->WordSpacing; }

    inline PdfTextRenderingMode GetRenderingMode() const { return m_state->RenderingMode; }

public:
    operator const PdfTextState&() const { return *m_state; }

private:
    void SetState(PdfTextState& state) { m_state = &state; }

private:
    PdfPainter* m_painter;
    PdfTextState* m_state;
};

/**
 * This class provides an easy to use painter object which allows you to draw on a PDF page
 * object.
 *
 * During all drawing operations, you are still able to access the stream of the object you are
 * drawing on directly.
 *
 * All functions that take coordinates expect these to be in PDF User Units. Keep in mind that PDF has
 * its coordinate system origin at the bottom left corner.
 */
class PODOFO_API PdfPainter final
{
    friend class PdfGraphicsStateWrapper;
    friend class PdfTextStateWrapper;
    friend class PdfPainterPathContext;
    friend class PdfPainterTextContext;

public:
    /** Create a new PdfPainter object.
     *
     *  \param saveRestore do save/restore state before appending
     */
    PdfPainter(PdfPainterFlags flags = PdfPainterFlags::None);

    virtual ~PdfPainter() noexcept(false);

    /** Set the page on which the painter should draw.
     *  The painter will draw of course on the pages
     *  contents object.
     *
     *  Calls FinishPage() on the last page if it was not yet called.
     *
     *  \param page a PdfCanvas object (most likely a PdfPage or PdfXObject).
     *
     *  \see PdfPage \see PdfXObject
     *  \see FinishPage()
     */
    void SetCanvas(PdfCanvas& page);

    /** Finish drawing onto a canvas.
     *
     *  This has to be called whenever a page has been drawn complete.
     */
    void FinishDrawing();

    /** Set the shading pattern for all following stroking operations.
     *  This operation uses the 'SCN' PDF operator.
     *
     *  \param pattern a shading pattern
     */
    void SetStrokingShadingPattern(const PdfShadingPattern& pattern);

    /** Set the shading pattern for all following non-stroking operations.
     *  This operation uses the 'scn' PDF operator.
     *
     *  \param pattern a shading pattern
     */
    void SetShadingPattern(const PdfShadingPattern& pattern);

    /** Set the tiling pattern for all following stroking operations.
     *  This operation uses the 'SCN' PDF operator.
     *
     *  \param pattern a tiling pattern
     */
    void SetStrokingTilingPattern(const PdfTilingPattern& pattern);

    /** Set the tiling pattern for all following stroking operations by pattern name,
     *  Use when it's already in resources.
     *  This operation uses the 'SCN' PDF operator.
     *
     *  \param patternName a tiling pattern name
     */
    void SetStrokingTilingPattern(const std::string_view& patternName);

    /** Set the tiling pattern for all following non-stroking operations.
     *  This operation uses the 'scn' PDF operator.
     *
     *  \param pattern a tiling pattern
     */
    void SetTilingPattern(const PdfTilingPattern& pattern);

    /** Set the tiling pattern for all following non-stroking operations by pattern name.
      *  Use when it's already in resources.
     *  This operation uses the 'scn' PDF operator.
     *
     *  \param pattern a tiling pattern
     */
    void SetTilingPattern(const std::string_view& patternName);

    /** Set the stoke style for all stroking operations.
     *  \param strokeStyle style of the stroking operations
     *  \param custom a custom stroking style which is used when
     *                   strokeStyle == PdfStrokeStyle::Custom.
      *  \param inverted inverted dash style (gaps for drawn spaces),
      *                  it is ignored for None, Solid and Custom styles
      *  \param scale scale factor of the stroke style
      *                  it is ignored for None, Solid and Custom styles
      *  \param subtractJoinCap if true, subtracts scaled width on filled parts,
      *                       thus the line capability still draws into the cell;
      *                        is used only if scale is not 1.0
     *
     *  Possible values:
     *    PdfStrokeStyle::None
     *    PdfStrokeStyle::Solid
     *    PdfStrokeStyle::Dash
     *    PdfStrokeStyle::Dot
     *    PdfStrokeStyle::DashDot
     *    PdfStrokeStyle::DashDotDot
     *    PdfStrokeStyle::Custom
     *
     */
    void SetStrokeStyle(PdfStrokeStyle strokeStyle, const std::string_view& custom = { }, bool inverted = false, double scale = 1.0, bool subtractJoinCap = false);

    /** Set a clipping rectangle
     *
     *  \param x x coordinate of the rectangle (left coordinate)
     *  \param y y coordinate of the rectangle (bottom coordinate)
     *  \param width width of the rectangle
     *  \param height absolute height of the rectangle
     */
    void SetClipRect(double x, double y, double width, double height);

    /** Set a clipping rectangle
     *
     *  \param rect rectangle
     */
    void SetClipRect(const PdfRect& rect);

    /** Stroke a line with current color and line settings.
     *  \param x1 x coordinate of the starting point
     *  \param y1 y coordinate of the starting point
     *  \param x2 x coordinate of the ending point
     *  \param y2 y coordinate of the ending point
     */
    void DrawLine(double x1, double y1, double x2, double y2);

    /** Stroke a cubic bezier with current color and line settings.
     *  \param x1 x coordinate of the starting point
     *  \param y1 y coordinate of the starting point
     *  \param x2 x coordinate of the first control point
     *  \param y2 y coordinate of the first control point
     *  \param x3 x coordinate of the second control point
     *  \param y3 y coordinate of the second control point
     *  \param x4 x coordinate of the end point, which is the new current point
     *  \param y5 y coordinate of the end point, which is the new current point
     */
    void DrawCubicBezier(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4);

    /** Stroke an arc to the given coordinate, with angles and radius
     *  \param x x coordinate of the center of the arc (left coordinate)
     *  \param y y coordinate of the center of the arc (top coordinate)
     *	\param radius radius
     *	\param angle1 angle1 in radians
     *	\param angle2 angle2 in radians
     */
    void DrawArc(double x, double y, double radius, double angle1, double angle2, bool counterclockwise = false);

    /** Draw a circle
     *  \param x x center coordinate of the circle
     *  \param y y coordinate of the circle
     *  \param radius radius of the circle
     */
    void DrawCircle(double x, double y, double radius, PdfPathDrawMode mode = PdfPathDrawMode::Stroke);

    /** Draw an ellipse to the given coordinates
     *  \param x x coordinate of the ellipse (left coordinate)
     *  \param y y coordinate of the ellipse (top coordinate)
     *  \param width width of the ellipse
     *  \param height absolute height of the ellipse
     */
    void DrawEllipse(double x, double y, double width, double height,
        PdfPathDrawMode mode = PdfPathDrawMode::Stroke);

    /** Draw a rectangle to the given coordinates
     *  \param x x coordinate of the rectangle (left coordinate)
     *  \param y y coordinate of the rectangle (bottom coordinate)
     *  \param width width of the rectangle
     *  \param height absolute height of the rectangle
     *  \param roundX rounding factor, x direction
     *  \param roundY rounding factor, y direction
     */
    void DrawRectangle(double x, double y, double width, double height,
        PdfPathDrawMode mode = PdfPathDrawMode::Stroke, double roundX = 0.0, double roundY = 0.0);

    /** Draw a rectangle into the current path to the given coordinates
     *  \param rect the rectangle area
     *  \param roundX rounding factor, x direction
     *  \param roundY rounding factor, y direction
     */
    void DrawRectangle(const PdfRect& rect, PdfPathDrawMode mode = PdfPathDrawMode::Stroke,
        double roundX = 0.0, double roundY = 0.0);

    /** Draw a single-line text string on a page using a given font object.
     *  You have to call SetFont before calling this function.
     *  \param str the text string which should be printed
     *  \param x the x coordinate
     *  \param y the y coordinate
     *
     *  \see SetFont()
     */
    void DrawText(const std::string_view& str, double x, double y,
        PdfDrawTextStyle style = PdfDrawTextStyle::Regular);

    /** Draw multiline text into a rectangle doing automatic wordwrapping.
     *  The current font is used and SetFont has to be called at least once
     *  before using this function
     *
     *  \param str the text which should be drawn
     *  \param x the x coordinate of the text area (left)
     *  \param y the y coordinate of the text area (bottom)
     *  \param width width of the text area
     *  \param height height of the text area
     *  \param params parameters of the draw operation
     */
    void DrawTextMultiLine(const std::string_view& str, double x, double y, double width, double height,
        const PdfDrawTextMultiLineParams& params = { });

    /** Draw multiline text into a rectangle doing automatic wordwrapping.
     *  The current font is used and SetFont has to be called at least once
     *  before using this function
     *
     *  \param str the text which should be drawn
     *  \param rect bounding rectangle of the text
     *  \param params parameters of the draw operation
     */
    void DrawTextMultiLine(const std::string_view& str, const PdfRect& rect,
        const PdfDrawTextMultiLineParams& params = { });

    /** Draw a single line of text horizontally aligned.
     *  \param str the text to draw
     *  \param x the x coordinate of the text line
     *  \param y the y coordinate of the text line
     *  \param width the width of the text line
     *  \param hAlignment alignment of the text line
     *  \param style style of the draw text operation
     */
    void DrawTextAligned(const std::string_view& str, double x, double y,
        double width, PdfHorizontalAlignment hAlignment,
        PdfDrawTextStyle style = PdfDrawTextStyle::Regular);

    /** Draw an image on the current page.
     *  \param x the x coordinate (left position of the image)
     *  \param y the y coordinate (bottom position of the image)
     *  \param obj an PdfXObject
     *  \param scaleX option scaling factor in x direction
     *  \param scaleY option scaling factor in y direction
     */
    void DrawImage(const PdfImage& obj, double x, double y, double scaleX = 1.0, double scaleY = 1.0);

    /** Draw an XObject on the current page. For PdfImage use DrawImage.
     *
     *  \param x the x coordinate (left position of the XObject)
     *  \param y the y coordinate (bottom position of the XObject)
     *  \param obj an PdfXObject
     *  \param scaleX option scaling factor in x direction
     *  \param scaleY option scaling factor in y direction
     *
     *  \see DrawImage
     */
    void DrawXObject(const PdfXObject& obj, double x, double y, double scaleX = 1.0, double scaleY = 1.0);

    /**
     * Begin a marked-content sequence (operator BMC) 
     */
    void BeginMarkedContent(const std::string_view& tag);

    /**
     * End a marked-content sequence begun by a BMC or BDC operator
     */ 
    void EndMarkedContent();

    /** Save the current graphics settings onto the graphics
     *  stack. Operator 'q' in PDF.
     *  This call has to be balanced with a corresponding call
     *  to Restore()!
     *
     *  \see Restore
     */
    void Save();

    /** Restore the current graphics settings from the graphics
     *  stack. Operator 'Q' in PDF.
     *  This call has to be balanced with a corresponding call
     *  to Save()!
     *
     *  \see Save
     */
    void Restore();

    /** Sets a specific PdfExtGState as being active
     *	\param inGState the specific ExtGState to set
     */
    void SetExtGState(const PdfExtGState& inGState);

    /** Set the floating point precision.
     *
     *  \param precision write this many decimal places
     */
    void SetPrecision(unsigned short precision);

    /** Get the currently set floating point precision
     *  \returns how many decimal places will be written out for any floating point value
     */
    unsigned short GetPrecision() const;

public:
    inline const PdfPainterStateStack& GetStateStack() const { return m_StateStack; }

    /** Set the tab width for the DrawText operation.
     *  Every tab '\\t' is replaced with tabWidth
     *  spaces before drawing text. Default is a value of 4
     *
     *  \param tabWidth replace every tabulator by this much spaces
     *
     *  \see DrawText
     *  \see TabWidth
     */
    inline void SetTabWidth(unsigned short tabWidth) { m_TabWidth = tabWidth; }

    /** Get the currently set tab width
     *  \returns by how many spaces a tabulator will be replaced
     *
     *  \see DrawText
     *  \see TabWidth
     */
    inline unsigned short GetTabWidth() const { return m_TabWidth; }

    /** Return the current page that is that on the painter.
     *
     *  \returns the current page of the painter or nullptr if none is set
     */
    inline PdfCanvas* GetCanvas() const { return m_canvas; }

    /** Return the current canvas stream that is set on the painter.
     *
     *  \returns the current page canvas stream of the painter or nullptr if none is set
     */
    inline PdfObjectStream* GetStream() const { return m_objStream; }

private:
    // To be called by PdfPainterTextContext
    void BeginText();
    void TextMoveTo(double x, double y);
    void AddText(const std::string_view& str);
    void EndText();

    // To be called by PdfPainterPathContext
    void BeginPath(double x, double y);
    void EndPath();
    void DrawPath(PdfPathDrawMode mode);
    void PathMoveTo(double x, double y);
    void AddLineTo(double x, double y);
    void AddCubicBezierTo(double x1, double y1, double x2, double y2, double x3, double y3);
    void AddArcTo(double x1, double y1, double x2, double y2, double radius);
    void AddArc(double x, double y, double radius, double startAngle, double endAngle, double counterclockwise);
    void AddCircle(double x, double y, double radius);
    void AddEllipse(double x, double y, double width, double height);
    void AddRectangle(double x, double y, double width, double height, double roundX, double roundY);
    void Clip(bool useEvenOddRule);
    void ClosePath();

private:
    // To be called by state wrappers
    void SetLineWidth(double value);
    void SetMiterLimit(double value);
    void SetLineCapStyle(PdfLineCapStyle style);
    void SetLineJoinStyle(PdfLineJoinStyle style);
    void SetFillColor(const PdfColor& color);
    void SetStrokeColor(const PdfColor& color);
    void SetRenderingIntent(const std::string_view& intent);
    void SetTransformationMatrix(const Matrix& matrix);
    void SetFont(const PdfFont* font, double fontSize);
    void SetFontScale(double value);
    void SetCharSpacing(double value);
    void SetWordSpacing(double value);
    void SetTextRenderingMode(PdfTextRenderingMode value);

private:
    void writeTextState();
    void setFont(const PdfFont* font, double fontSize);
    void setFontScale(double value);
    void setCharSpacing(double value);
    void setWordSpacing(double value);
    void setTextRenderingMode(PdfTextRenderingMode value);
    void save();
    void restore();
    void reset();
    void pathMoveTo(double x, double y);
    void endPath();
    void addLineTo(double x, double y);
    void addCubicBezierTo(double x1, double y1, double x2, double y2, double x3, double y3);
    void addArcTo(double x1, double y1, double x2, double y2, double radius);
    void addArc(double x, double y, double radius, double startAngle, double endAngle, double counterclockwise);
    void addCircle(double x, double y, double radius);
    void addEllipse(double x, double y, double width, double height);
    void addRectangle(double x, double y, double width, double height, double roundX, double roundY);
    void closePath();
    void drawPath(PdfPathDrawMode mode);
    void stroke();
    void fill(bool useEvenOddRule);
    void strokeAndFill(bool useEvenOddRule);

private:
    enum PainterStatus
    {
        StatusDefault = 1,
        StatusText = 2,
        StatusPath = 4,
    };

private:
    /** Gets the text divided into individual lines, using the current font and clipping rectangle.
     *
     *  \param str the text which should be drawn
     *  \param width width of the text area
     *  \param skipSpaces whether the trailing whitespaces should be skipped, so that next line doesn't start with whitespace
     */
    std::vector<std::string> getMultiLineTextAsLines(const std::string_view& str, double width, bool skipSpaces);

    /** Register an object in the resource dictionary of this page
     *  so that it can be used for any following drawing operations.
     *
     *  \param type register under this key in the resource dictionary
     *  \param name identifier of this object, e.g. /Ft0
     *  \param obj the object you want to register
     */
    void addToPageResources(const PdfName& type, const PdfName& identifier, const PdfObject& obj);

    void drawTextAligned(const std::string_view& str, double x, double y, double width,
        PdfHorizontalAlignment hAlignment, PdfDrawTextStyle style);

    void drawText(const std::string_view& str, double x, double y, bool isUnderline, bool isStrikeOut);

    void drawMultiLineText(const std::string_view& str, double x, double y, double width, double height,
        PdfHorizontalAlignment hAlignment, PdfVerticalAlignment vAlignment, bool clip, bool skipSpaces,
        PdfDrawTextStyle style);

    void setLineWidth(double width);

    /** Expand all tab characters in a string
     *  using spaces.
     *
     *  \param str expand all tabs in this string using spaces
     *  \param len use only len characters of the string. If negative use all
     *      string length
     *  \returns an expanded copy of the passed string
     *  \see SetTabWidth
     */
    std::string expandTabs(const std::string_view& str) const;
    void checkStream();
    void checkFont();
    void finishDrawing();
    void checkStatus(int expectedStatus);

private:
    PdfPainterFlags m_flags;
    PainterStatus m_painterStatus;
    PdfPainterStateStack m_StateStack;
    unsigned m_textStackCount;
    std::vector<Vector2> m_subPaths;

public:
    PdfGraphicsStateWrapper GraphicsState;
    PdfTextStateWrapper TextState;
    PdfPainterPathContext Path;
    PdfPainterTextContext Text;

private:
    /** All drawing operations work on this stream.
     *  This object may not be nullptr. If it is nullptr any function accessing it should
     *  return ERROR_PDF_INVALID_HANDLE
     */
    PdfObjectStream* m_objStream;

    /** The page object is needed so that fonts etc. can be added
     *  to the page resource dictionary as appropriate.
     */
    PdfCanvas* m_canvas;

    /** Every tab '\\t' is replaced with m_TabWidth
     *  spaces before drawing text. Default is a value of 4
     */
    unsigned short m_TabWidth;

    /** temporary stream buffer
     */
    PdfStringStream m_stream;
};

}

ENABLE_BITMASK_OPERATORS(PoDoFo::PdfPainterFlags);
ENABLE_BITMASK_OPERATORS(PoDoFo::PdfDrawTextStyle);

#endif // PDF_PAINTER_H
