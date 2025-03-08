/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_PAINTER_H
#define PDF_PAINTER_H

#include "PdfCanvas.h"
#include "PdfTextState.h"
#include "PdfGraphicsState.h"
#include "PdfPainterPath.h"
#include "PdfPainterTextObject.h"
#include "PdfColorSpace.h"
#include "PdfPattern.h"
#include "PdfContentStreamOperators.h"

#include <podofo/auxiliary/StateStack.h>

namespace PoDoFo {

class PdfExtGState;
class PdfFont;
class PdfImage;
class PdfObjectStream;
class PdfXObject;
class PdfPainter;

enum class PdfPainterFlags
{
    None = 0,
    Prepend = 1,            ///< Does nothing for now
    NoSaveRestorePrior = 2, ///< Do not perform a Save/Restore or previous content. Implies RawCoordinates
    NoSaveRestore = 4,      ///< Do not perform a Save/Restore of added content in this painting session
    RawCoordinates = 8,     ///< Does nothing for now
};

/**
 * An enum describing modes to draw paths and figures
 */
enum class PdfPathDrawMode : uint8_t
{
    Stroke = 1,
    Fill = 2,               ///< Fill using the the non-zero winding number rule to determine the region to fill
    StrokeFill = 3,         ///< Stroke and fill using the the even-odd rule to determine the region to fill
    FillEvenOdd = 4,        ///< Fill using the the even-odd rule to determine the region to fill
    StrokeFillEvenOdd = 5,  ///< Stroke and fill using the the even-odd rule to determine the region to fill
};

enum class PdfDrawTextStyle : uint8_t
{
    Regular = 0,
    StrikeThrough = 1,
    Underline = 2,
};

struct PODOFO_API PdfDrawTextMultiLineParams final
{
    PdfDrawTextStyle Style = PdfDrawTextStyle::Regular;                         ///< style of the draw text operation
    PdfHorizontalAlignment HorizontalAlignment = PdfHorizontalAlignment::Left;  ///< alignment of the individual text lines in the given bounding box
    PdfVerticalAlignment VerticalAlignment = PdfVerticalAlignment::Top;         ///< vertical alignment of the text in the given bounding box
    bool SkipClip = false;                                                      ///< Where a clipping rectangle, skip clipping text 
    bool PreserveTrailingSpaces = false;                                        ///< Whether to the preserve trailing whitespaces
};

struct PODOFO_API PdfPainterState final
{
    PdfPainterState();

    PdfGraphicsState GraphicsState;
    PdfTextState TextState;         ///< The current semantical text state
    nullable<Vector2> FirstPoint;
    nullable<Vector2> CurrentPoint;
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
    /** Apply the given matrix to the current transformation matrix (CTM)
     * \remarks Corresponds to the PDF 'cm' operator 
     */
    void ConcatenateTransformationMatrix(const Matrix& matrix);
    void SetLineWidth(double lineWidth);
    void SetMiterLevel(double value);
    void SetLineCapStyle(PdfLineCapStyle capStyle);
    void SetLineJoinStyle(PdfLineJoinStyle joinStyle);
    void SetRenderingIntent(const std::string_view& intent);
    /** Set the color for PDF "nonstroking" operations (including "fill")
     */
    void SetNonStrokingColorSpace(PdfColorSpaceInitializer&& colorSpace);
    /** Set the color for PDF "stroking" operations
     */
    void SetStrokingColorSpace(PdfColorSpaceInitializer&& color);
    /** Set the color for PDF "nonstroking" operations (including "fill")
     */
    void SetNonStrokingColor(const PdfColor& color);
    /** Set the color for PDF "nonstroking" operations (including "fill")
     */
    void SetStrokingColor(const PdfColor& color);
    /** Set the color for PDF "nonstroking" operations (including "fill")
     */
    void SetNonStrokingColor(const PdfColorRaw& color);
    /** Set the color for PDF "stroking" operations
     */
    void SetStrokingColor(const PdfColorRaw& color);
    void SetExtGState(const PdfExtGState& extGState);
    /** Set a stroking uncoloured tiling pattern
     */
    void SetStrokingUncolouredTilingPattern(const PdfUncolouredTilingPattern& pattern, const PdfColorRaw& color);

    /** Set a non stroking uncoloured tiling pattern
     */
    void SetNonStrokingUncolouredTilingPattern(const PdfUncolouredTilingPattern& pattern, const PdfColorRaw& color);
    /** Set a stroking coloured tiling pattern
     */
    void SetStrokingPattern(const PdfPattern& pattern);
    /** Set a non stroking coloured tiling pattern
     */
    void SetNonStrokingPattern(const PdfPattern& pattern);
    /** Set a shading dictionary
     */
    void SetShadingDictionary(const PdfShadingDictionary& shading);

public:
    /** Get the current transformation matrix (CTM)
     */
    const Matrix& GetCurrentMatrix() { return m_state->CTM; }
    double GetLineWidth() const { return m_state->LineWidth; }
    double GetMiterLevel() const { return m_state->MiterLimit; }
    PdfLineCapStyle GetLineCapStyle() const { return m_state->LineCapStyle; }
    PdfLineJoinStyle GetLineJoinStyle() const { return m_state->LineJoinStyle; }
    const std::string& GetRenderingIntent() const { return m_state->RenderingIntent; }
    const PdfColorRaw& GetNonStrokingColor() const { return m_state->NonStrokingColor; }
    const PdfColorRaw& GetStrokingColor() const { return m_state->StrokingColor; }
    PdfColorSpaceFilterPtr GetNonStrokingColorSpace() const { return m_state->NonStrokingColorSpaceFilter; }
    PdfColorSpaceFilterPtr GetStrokingColorSpace() const { return m_state->StrokingColorSpaceFilter; }

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
     *  \param wordSpacing word spacing in PDF units
     */
    void SetWordSpacing(double wordSpacing);

    void SetRenderingMode(PdfTextRenderingMode mode);

    /** Set the text spacing (operator Tw)
     *  \param fWordSpace word spacing in PDF units
     */
    void SetMatrix(const Matrix& matrix);

public:
    inline const PdfFont* GetFont() const { return m_State->Font; }

    /** Retrieve the current font size (operator Tf, controlling Tfs)
     *  \returns the current font size
     */
    inline double GetFontSize() const { return m_State->FontSize; }

    /** Retrieve the current horizontal scaling (operator Tz)
     *  \returns the current font scaling in [0,1]
     */
    inline double GetFontScale() const { return m_State->FontScale; }

    /** Retrieve the character spacing (operator Tc)
     *  \returns the current font character spacing
     */
    inline double GetCharSpacing() const { return m_State->CharSpacing; }

    /** Retrieve the current word spacing (operator Tw)
     *  \returns the current font word spacing in PDF units
     */
    inline double GetWordSpacing() const { return m_State->WordSpacing; }

    inline PdfTextRenderingMode GetRenderingMode() const { return m_State->RenderingMode; }

public:
    const PdfTextState& GetState() const { return *m_State; }
    operator const PdfTextState&() const { return *m_State; }

private:
    void SetState(PdfTextState& state) { m_State = &state; }

private:
    PdfPainter* m_painter;
    PdfTextState* m_State;
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
 * \remarks It should not be generally needede but you can cast the instance to
 * PdfContentStreamOperators to access low level PDF operators
 */
class PODOFO_API PdfPainter final : public PdfContentStreamOperators
{
    friend class PdfGraphicsStateWrapper;
    friend class PdfTextStateWrapper;
    friend class PdfPainterPathContext;
    friend class PdfPainterTextObject;

public:
    /** Create a new PdfPainter object.
     *
     *  \param saveRestore do save/restore state before appending
     */
    PdfPainter();

    ~PdfPainter() noexcept(false);

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
    void SetCanvas(PdfCanvas& canvas, PdfPainterFlags flags = PdfPainterFlags::None);

    /** Finish drawing onto a canvas.
     *
     *  This has to be called whenever a page has been drawn complete.
     */
    void FinishDrawing();

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
     *
     */
    void SetStrokeStyle(PdfStrokeStyle strokeStyle, bool inverted = false, double scale = 1.0, bool subtractJoinCap = false);

    void SetStrokeStyle(const cspan<double>& dashArray, double phase);

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
    void SetClipRect(const Rect& rect);

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

    /** Stroke a counterclockwise arc to the given coordinate spanning from given angles and radius
     *  This is the equivalent of the "arc"/"arcn" operators in PostScript
     *  \param x x coordinate of the center of the arc (left coordinate)
     *  \param y y coordinate of the center of the arc (top coordinate)
     *	\param radius radius
     *	\param startAngle startAngle in radians measured counterclockwise from the origin
     *	\param endAngle endAngle in radians measured counterclockwise from the origin
     *	\param clockwise The arc is drawn clockwise instead
     */
    void DrawArc(double x, double y, double radius, double startAngle, double endAngle, bool clockwise = false);

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
    void DrawRectangle(const Rect& rect, PdfPathDrawMode mode = PdfPathDrawMode::Stroke,
        double roundX = 0.0, double roundY = 0.0);

    /** Draw a single-line text string on a page using a given font object.
     *  You have to call SetFont before calling this function.
     *  \param str the text string which should be printed
     *  \param x the x coordinate
     *  \param y the y coordinate
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
    void DrawTextMultiLine(const std::string_view& str, const Rect& rect,
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
     * Draw the current path with the given
     */
    void DrawPath(const PdfPainterPath& path, PdfPathDrawMode drawMode = PdfPathDrawMode::Stroke);

    /** Clip the current path. Matches the PDF 'W' operator.
     *  \param useEvenOddRule select even-odd rule instead of nonzero winding number rule
     */
    void ClipPath(const PdfPainterPath& path, bool useEvenOddRule = false);

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

    /** Set the floating point precision.
     *
     *  \param precision write this many decimal places
     */
    void SetPrecision(unsigned short precision);

    /** Get the currently set floating point precision
     *  \returns how many decimal places will be written out for any floating point value
     */
    unsigned short GetPrecision() const;

    /**
     * Get a string view of the current content stream being built
     */
    std::string_view GetContent() const;

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
    // To be called by PdfPainterTextObject
    void BeginText();
    void EndText();
    void TextMoveTo(double x, double y);
    void AddText(const std::string_view& str);

private:
    // To be called by state wrappers
    void SetLineWidth(double value);
    void SetMiterLimit(double miterLimit);
    void SetLineCapStyle(PdfLineCapStyle style);
    void SetLineJoinStyle(PdfLineJoinStyle style);
    void SetNonStrokingColor(const PdfColor& color);
    void SetStrokingColor(const PdfColor& color);
    void SetNonStrokingColor(const PdfColorRaw& color, const PdfColorSpaceFilter& colorSpace);
    void SetStrokingColor(const PdfColorRaw& color, const PdfColorSpaceFilter& colorSpace);
    void SetNonStrokingColorSpace(const PdfVariant& expVar);
    void SetStrokingColorSpace(const PdfVariant& expVar);
    void SetStrokingPattern(const PdfPattern& pattern, const PdfColorRaw* color, const PdfColorSpaceFilter* colorSpace);
    void SetNonStrokingPattern(const PdfPattern& pattern, const PdfColorRaw* color, const PdfColorSpaceFilter* colorSpace);
    void SetShadingDictionary(const PdfShadingDictionary& shading);
    void SetRenderingIntent(const std::string_view& intent);
    void SetTransformationMatrix(const Matrix& matrix);
    void SetFont(const PdfFont& font, double fontSize);
    void SetFontScale(double value);
    void SetCharSpacing(double value);
    void SetWordSpacing(double value);
    void SetTextRenderingMode(PdfTextRenderingMode value);
    void SetTextMatrix(const Matrix& matrix);
    void SetExtGState(const PdfExtGState& extGState);

private:
    void writeTextState();
    void setFont(const PdfFont& font, double fontSize);
    void setFontScale(double value);
    void setCharSpacing(double value);
    void setWordSpacing(double value);
    void setTextRenderingMode(PdfTextRenderingMode value);
    void setTextMatrix(const Matrix& value);
    void save();
    void restore();
    void reset();
    void drawRectangle(double x, double y, double width, double height, PdfPathDrawMode mode, double roundX, double roundY);
    void drawPath(PdfPathDrawMode mode);
    void stroke();
    void fill(bool useEvenOddRule);
    void strokeAndFill(bool useEvenOddRule);
    PdfName tryAddResource(const PdfObject& obj, PdfResourceType type);
    PdfName tryAddResource(const PdfReference& ref, PdfResourceType type);
    void drawLines(const std::vector<std::array<double, 4>>& lines);

private:
    // PdfContentStreamOperators implementation
    void re_Operator(double x, double y, double width, double height) override;
    void m_Operator(double x, double y) override;
    void l_Operator(double x, double y) override;
    void c_Operator(double c1x, double c1y, double c2x, double c2y, double x, double y) override;
    void v_Operator(double cx, double cy, double x, double y) override;
    void y_Operator(double cx, double cy, double x, double y) override;
    void n_Operator() override;
    void h_Operator() override;
    void b_Operator() override;
    void B_Operator() override;
    void bStar_Operator() override;
    void BStar_Operator() override;
    void s_Operator() override;
    void S_Operator() override;
    void f_Operator() override;
    void fStar_Operator() override;
    void W_Operator() override;
    void WStar_Operator() override;
    void MP_Operator(const std::string_view& tag) override;
    void DP_Operator(const std::string_view& tag, const PdfDictionary& properties) override;
    void DP_Operator(const std::string_view& tag, const std::string_view& propertyDictName) override;
    void BMC_Operator(const std::string_view& tag) override;
    void BDC_Operator(const std::string_view& tag, const PdfDictionary& properties) override;
    void BDC_Operator(const std::string_view& tag, const std::string_view& propertyDictName) override;
    void EMC_Operator() override;
    void q_Operator() override;
    void Q_Operator() override;
    void BT_Operator() override;
    void ET_Operator() override;
    void Td_Operator(double tx, double ty) override;
    void TD_Operator(double tx, double ty) override;
    void Tm_Operator(double a, double b, double c, double d, double e, double f) override;
    void Tr_Operator(PdfTextRenderingMode mode) override;
    void Ts_Operator(double rise) override;
    void Tc_Operator(double charSpace) override;
    void TL_Operator(double leading) override;
    void Tf_Operator(const std::string_view& fontName, double fontSize) override;
    void Tw_Operator(double wordSpace) override;
    void Tz_Operator(double scale) override;
    void Tj_Operator(const std::string_view& encoded, bool hex) override;
    void TJ_Operator_Begin() override;
    void TJ_Operator_Delta(double delta) override;
    void TJ_Operator_Glyphs(const std::string_view& encoded, bool hex) override;
    void TJ_Operator_End() override;
    void cm_Operator(double a, double b, double c, double d, double e, double f) override;
    void w_Operator(double lineWidth) override;
    void J_Operator(PdfLineCapStyle style) override;
    void j_Operator(PdfLineJoinStyle style) override;
    void M_Operator(double miterLimit) override;
    void d_Operator(const cspan<double>& dashArray, double fase) override;
    void ri_Operator(const std::string_view& intent) override;
    void i_Operator(double flatness) override;
    void gs_Operator(const std::string_view& dictName) override;
    void Do_Operator(const std::string_view& xobjname) override;
    void cs_Operator(PdfColorSpaceType colorSpace) override;
    void cs_Operator(const std::string_view& name) override;
    void CS_Operator(PdfColorSpaceType colorSpace) override;
    void CS_Operator(const std::string_view& name) override;
    void sc_Operator(const cspan<double>& components) override;
    void SC_Operator(const cspan<double>& components) override;
    void scn_Operator(const cspan<double>& components) override;
    void SCN_Operator(const cspan<double>& components) override;
    void scn_Operator(const cspan<double>& components, const std::string_view& patternName) override;
    void SCN_Operator(const cspan<double>& components, const std::string_view& patternName) override;
    void scn_Operator(const std::string_view& patternName) override;
    void SCN_Operator(const std::string_view& patternName) override;
    void G_Operator(double gray) override;
    void g_Operator(double gray) override;
    void RG_Operator(double red, double green, double blue) override;
    void rg_Operator(double red, double green, double blue) override;
    void K_Operator(double cyan, double magenta, double yellow, double black) override;
    void k_Operator(double cyan, double magenta, double yellow, double black) override;
    void sh_Operator(const std::string_view& shadingDictName) override;
    void BX_Operator() override;
    void EX_Operator() override;
    void Extension_Operator(const std::string_view& opName, const cspan<PdfVariant>& operands) override;

private:
    enum PainterStatus
    {
        StatusDefault = 1,
        StatusTextObject = 2,       ///< A text (BT ... ET) object is opened
        StatusTextArray = 4,        ///< A text array [ ... ] TJ
        StatusExtension = 8,        ///< Extension operators between BX .. EX
    };

private:
    void drawTextAligned(const std::string_view& str, double x, double y, double width,
        PdfHorizontalAlignment hAlignment, PdfDrawTextStyle style, std::vector<std::array<double, 4>>& linesToDraw);

    void drawText(const std::string_view& str, double x, double y,
        bool isUnderline, bool isStrikeThrough, std::vector<std::array<double, 4>>& linesToDraw);

    void drawMultiLineText(const std::string_view& str, double x, double y, double width, double height,
        PdfHorizontalAlignment hAlignment, PdfVerticalAlignment vAlignment, bool skipClip, bool preserveTrailingSpaces,
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
    void openPath(double x, double y);
    void resetPath();
    void checkPathOpened() const;
    void checkFont() const;
    void finishDrawing();
    void checkStatus(int expectedStatus);
    void enterTextObject();
    void exitTextObject();

private:
    PdfPainter(const PdfPainter&) = delete;
    PdfPainter& operator=(const PdfPainter&) = delete;

private:
    PdfPainterFlags m_flags;
    PainterStatus m_painterStatus;
    PdfPainterStateStack m_StateStack;
    unsigned m_textStackCount;

public:
    PdfGraphicsStateWrapper GraphicsState;
    PdfTextStateWrapper TextState;
    PdfPainterTextObject TextObject;

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

    std::unordered_map<PdfReference, PdfName> m_resNameCache;
};

}

ENABLE_BITMASK_OPERATORS(PoDoFo::PdfPainterFlags);
ENABLE_BITMASK_OPERATORS(PoDoFo::PdfDrawTextStyle);

#endif // PDF_PAINTER_H
