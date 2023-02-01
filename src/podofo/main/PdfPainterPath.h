/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_PAINTER_PATH_H
#define PDF_PAINTER_PATH_H

#include "PdfStringStream.h"

namespace PoDoFo {

/**
 * An enum describing modes to draw paths and figures
 */
enum class PdfPainterDrawMode
{
    Stroke = 1,
    Fill = 2,               ///< Fill using the the non-zero winding number rule to determine the region to fill
    StrokeFill = 3,         ///< Stroke and fill using the the even-odd rule to determine the region to fill
    FillEvenOdd = 4,        ///< Fill using the the even-odd rule to determine the region to fill
    StrokeFillEvenOdd = 5,  ///< Stroke and fill using the the even-odd rule to determine the region to fill
};

class PdfPainter;

/**
 * This class describe a PDF painting path being writter through PdfPainter
 * 
 */
class PODOFO_API PdfPainterPath final
{
    friend class PdfPainter;

public:
    PdfPainterPath();
    PdfPainterPath(PdfPainterPath&& path) noexcept;

private:
    PdfPainterPath(PdfPainter& painter);

public:
    ~PdfPainterPath();

public:
    /** Append a straight line segment from the current point to the point (x, y) to the path
     *  Matches the PDF 'l' operator.
     *  \param x x position
     *  \param y y position
     */
    void AppendLine(double x, double y);

    /** Add straight line segment from the point (x1, y1) to (x2, y2) the path
     *  Matches the PDF 'l' operator.
     *  \param x1 x coordinate of the first point
     *  \param y1 y coordinate of the first point
     *  \param x2 x coordinate of the second point
     *  \param y2 y coordinate of the second point
     */
    void AddLine(double x1, double y1, double x2, double y2);

    /** Append a cubic bezier curve from from the current point to the current path
     *  Matches the PDF 'c' operator.
     *  \param x1 x coordinate of the first control point
     *  \param y1 y coordinate of the first control point
     *  \param x2 x coordinate of the second control point
     *  \param y2 y coordinate of the second control point
     *  \param x3 x coordinate of the end point, which is the new current point
     *  \param y3 y coordinate of the end point, which is the new current point
     */
    void AppendCubicBezier(double x1, double y1, double x2, double y2, double x3, double y3);

    /** Add a cubic bezier curve starting from the (x1,y1) point to the current path
     *  \param x1 x coordinate of the starting point
     *  \param y1 y coordinate of the starting point
     *  \param x2 x coordinate of the first control point
     *  \param y2 y coordinate of the first control point
     *  \param x3 x coordinate of the second control point
     *  \param y3 y coordinate of the second control point
     *  \param x4 x coordinate of the end point, which is the new current point
     *  \param y4 y coordinate of the end point, which is the new current point
     */
    void AddCubicBezier(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4);

    /** Add a circle into the current path to the given coordinates
     *  \param x x center coordinate of the circle
     *  \param y y coordinate of the circle
     *  \param radius radius of the circle
     */
    void AddCircle(double x, double y, double radius);

    /** Add an ellipse into the current path to the given coordinates
     *  \param x x coordinate of the ellipse (left coordinate)
     *  \param y y coordinate of the ellipse (top coordinate)
     *  \param width width of the ellipse
     *  \param height absolute height of the ellipse
     */
    void AddEllipse(double x, double y, double width, double height);

    /** Add an arc into the current path to the given coordinate, with angles and radius
     *  \param x x coordinate of the center of the arc (left coordinate)
     *  \param y y coordinate of the center of the arc (top coordinate)
     *	\param radius radius
     *	\param angle1 angle1 in radians
     *	\param angle2 angle2 in radians
     */
    void AddArc(double x, double y, double radius, double angle1, double angle2);

    /** Add an arc into the current path to the given coordinates and radius
     *  \param x1 x coordinate the first control point
     *  \param y1 y coordinate the first control point
     *  \param x2 x coordinate the second control point
     *  \param y2 y coordinate the second control point
     *	\param radius radius
     */
    void AddArcTo(double x1, double y1, double x2, double y2, double radius);

    /** Add a rectangle into the current path to the given coordinates
     *  \param x x coordinate of the rectangle (left coordinate)
     *  \param y y coordinate of the rectangle (bottom coordinate)
     *  \param width width of the rectangle
     *  \param height absolute height of the rectangle
     *  \param roundX rounding factor, x direction
     *  \param roundY rounding factor, y direction
     */
    void AddRectangle(double x, double y, double width, double height,
        double roundX = 0.0, double roundY = 0.0);

    /** Add a rectangle into the current path to the given coordinates
     *  \param rect the rectangle area
     *  \param roundX rounding factor, x direction
     *  \param roundY rounding factor, y direction
     */
    void AddRectangle(const PdfRect& rect, double roundX = 0.0, double roundY = 0.0);

    /** Clip the current path. Matches the PDF 'W' operator.
     *  \param useEvenOddRule select even-odd rule instead of nonzero winding number rule
     */
    void Clip(bool useEvenOddRule = false);

    /** End current path without filling or stroking it.
     *  Matches the PDF 'n' operator.
     */
    void Discard();

public:
    PdfPainterDrawMode GetDrawMode() const { return m_DrawMode; }
    void SetDrawMode(PdfPainterDrawMode mode) { m_DrawMode = mode; }

public:
    PdfPainterPath& operator=(PdfPainterPath&& path) noexcept;

private:
    void checkClosed();

private:
    PdfPainterPath(const PdfPainter& painter) = delete;
    PdfPainterPath& operator=(const PdfPainter& painter) = delete;

private:
    PdfPainter* m_painter;
    PdfStringStream* m_stream;
    bool m_closed;
    PdfPainterDrawMode m_DrawMode;
};

}

#endif // PDF_PAINTER_PATH_H
