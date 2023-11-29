/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_GRAPHICS_PATH_H
#define PDF_GRAPHICS_PATH_H

#include "PdfStringStream.h"
#include <podofo/auxiliary/Rect.h>

namespace PoDoFo {

/**
 * This class describes PDF paths being written to a PdfPainter
 */
class PODOFO_API PdfPainterPath final
{
public:
    PdfPainterPath();

public:
    /** Begin a new path. Matches the PDF 'm' operator.
     * This function is useful to construct an own path
     * for drawing or clipping
     */
    void MoveTo(double x, double y);

    /** Append a straight line segment from the current point to the point (x, y) to the path
     * Matches the PDF 'l' operator.
     * \param x x position
     * \param y y position
     */
    void AddLineTo(double x, double y);

    /** Add straight line segment from the point (x1, y1) to (x2, y2) the path
     * Matches the PDF 'l' operator.
     * \param x1 x coordinate of the first point
     * \param y1 y coordinate of the first point
     * \param x2 x coordinate of the second point
     * \param y2 y coordinate of the second point
     */
    void AddLine(double x1, double y1, double x2, double y2);

    /** Append a cubic bezier curve from from the current point to the current path
     * Matches the PDF 'c' operator.
     * \param x1 x coordinate of the first control point
     * \param y1 y coordinate of the first control point
     * \param x2 x coordinate of the second control point
     * \param y2 y coordinate of the second control point
     * \param x3 x coordinate of the end point, which is the new current point
     * \param y3 y coordinate of the end point, which is the new current point
     */
    void AddCubicBezierTo(double x1, double y1, double x2, double y2, double x3, double y3);

    /** Add a cubic bezier curve starting from the (x1,y1) point to the current path
     * \param x1 x coordinate of the starting point
     * \param y1 y coordinate of the starting point
     * \param x2 x coordinate of the first control point
     * \param y2 y coordinate of the first control point
     * \param x3 x coordinate of the second control point
     * \param y3 y coordinate of the second control point
     * \param x4 x coordinate of the end point, which is the new current point
     * \param y4 y coordinate of the end point, which is the new current point
     */
    void AddCubicBezier(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4);

    /** Add a circle into the current path to the given coordinates
     * \param x x center coordinate of the circle
     * \param y y coordinate of the circle
     * \param radius radius of the circle
     */
    void AddCircle(double x, double y, double radius);

    /** Add an ellipse into the current path to the given coordinates
     * \param x x coordinate of the ellipse (left coordinate)
     * \param y y coordinate of the ellipse (top coordinate)
     * \param width width of the ellipse
     * \param height absolute height of the ellipse
     */
    void AddEllipse(double x, double y, double width, double height);

    /** Add a counter clockwise arc into the current path spanning to the given angles and radius
     * This is the equivalent of the "arc"/"arcn" operators in PostScript
     * \param x x coordinate of the center of the arc (left coordinate)
     * \param y y coordinate of the center of the arc (top coordinate)
     * \param radius radius
     * \param angle1 angle1 in radians measured counterclockwise from the origin
     * \param angle2 angle2 in radians measured counterclockwise from the origin
     * \param clockwise The arc is drawn clockwise instead
     */
    void AddArc(double x, double y, double radius, double angle1, double angle2, bool clockwise = false);

    /** Append an arc from the current point to the current path
     * This is the equivalent of the "arct" operator in PostScript
     * \param x1 x coordinate the first control point
     * \param y1 y coordinate the first control point
     * \param x2 x coordinate of the end point, which is the new current point
     * \param y2 y coordinate of the end point, which is the new current point
     * \param radius radius
     */
    void AddArcTo(double x1, double y1, double x2, double y2, double radius);

    /** Add a rectangle into the current path to the given coordinates
     * \param x x coordinate of the rectangle (left coordinate)
     * \param y y coordinate of the rectangle (bottom coordinate)
     * \param width width of the rectangle
     * \param height absolute height of the rectangle
     * \param roundX rounding factor, x direction
     * \param roundY rounding factor, y direction
     */
    void AddRectangle(double x, double y, double width, double height,
        double roundX = 0.0, double roundY = 0.0);

    /** Add a rectangle into the current path to the given coordinates
     * \param rect the rectangle area
     * \param roundX rounding factor, x direction
     * \param roundY rounding factor, y direction
     */
    void AddRectangle(const Rect& rect, double roundX = 0.0, double roundY = 0.0);

    /**
     * Add a path to the current path
     * \param path the path to add
     * \param connect true if connect the first figure of the given the path to the last figure of this path
     */
    void AddPath(const PdfPainterPath& path, bool connect);

    /** Closes the current path by drawing a line from the current point
     * to the starting point of the path. Matches the PDF 'h' operator.
     * This function is useful to construct a closed path or clipping.
     */
    void Close();

    /**
     * Clear the path currently building and reset the internal state
     */
    void Reset();

public:
    /**
     * Get a string view of the current content stream being built
     */
    std::string_view GetContent() const;

    /**
     * Get the coordinates of the first point
     */
    const Vector2& GetFirstPoint() const;

    /**
     * Get the coordinates of the current point
     */
    const Vector2& GetCurrentPoint() const;

private:
    void checkOpened() const;
    void open(double x, double y);

private:
    PdfPainterPath(const PdfPainterPath& painter) = delete;
    PdfPainterPath& operator=(const PdfPainterPath& painter) = delete;

private:
    PdfStringStream m_stream;
    nullable<Vector2> m_FirstPoint;
    Vector2 m_CurrentPoint;
};

}

#endif // PDF_GRAPHICS_PATH_H
