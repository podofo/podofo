/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_PAINTER_EXTENSIONS_H
#define PDF_PAINTER_EXTENSIONS_H

#include <podofo/main/PdfDeclarations.h>

namespace PoDoFo {

    class PdfPainterPathContext;

    /**
     * This class cointains some extensions methods to PdfPainterPath
     * The class mostly implements some SVG commands by using
     * regular PDF operators by maintaining an internal state
     * \remarks This class is not maintained and its use
     * is not recommended until problems found are solved
     * https://www.mail-archive.com/podofo-users@lists.sourceforge.net/msg04879.html
     */
    class PODOFO_API PdfPainterPathExtensions final
    {
        friend class PdfPainterPathContext;

    public:
        PdfPainterPathExtensions(PdfPainterPathContext& path);

    public:
        /** Append a horizontal line to the current path
         *  Matches the SVG 'H' operator
         *
         *  \param x x coordinate to draw the line to
         */
        void AddHorizontalLine(double x);

        /** Append a vertical line to the current path
         *  Matches the SVG 'V' operator
         *
         *  \param y y coordinate to draw the line to
         */
        void AddVerticalLine(double y);

        /** Append a smooth bezier curve to the current path
         *  Matches the SVG 'S' operator.
         *
         *  \param x2 x coordinate of the second control point
         *  \param y2 y coordinate of the second control point
         *  \param x3 x coordinate of the end point, which is the new current point
         *  \param y3 y coordinate of the end point, which is the new current point
         */
        void AddSmoothCurve(double x2, double y2, double x3, double y3);

        /** Append a quadratic bezier curve to the current path
         *  Matches the SVG 'Q' operator.
         *
         *  \param x1 x coordinate of the first control point
         *  \param y1 y coordinate of the first control point
         *  \param x3 x coordinate of the end point, which is the new current point
         *  \param y3 y coordinate of the end point, which is the new current point
         */
        void AddQuadCurve(double x1, double y1, double x3, double y3);

        /** Append a smooth quadratic bezier curve to the current path
         *  Matches the SVG 'T' operator.
         *
         *  \param x3 x coordinate of the end point, which is the new current point
         *  \param y3 y coordinate of the end point, which is the new current point
         */
        void AddSmoothQuadCurve(double x3, double y3);

        /** Append a Arc to the current path
         *  Matches the SVG 'A' operator.
         *
         *  \param x x coordinate of the start point
         *  \param y y coordinate of the start point
         *  \param radiusX x coordinate of the end point, which is the new current point
         *  \param radiusY y coordinate of the end point, which is the new current point
         *	\param rotation degree of rotation in radians
         *	\param large large or small portion of the arc
         *	\param sweep sweep?
         */
        void AddArcTo(double x, double y, double radiusX, double radiusY,
            double rotation, bool large, bool sweep);

    private:
        PdfPainterPathExtensions(const PdfPainterPathExtensions&) = delete;
        PdfPainterPathExtensions& operator=(const PdfPainterPathExtensions&) = delete;

    private:
        PdfPainterPathContext* m_path;

        // TODO: Next comment was found like this and it's is really bad.
        // Document the next fields accurately, possibly moving them
        // to a structure

        // points for this operation
        // last "current" point
        // "reflect points"
        double m_lpx;
        double m_lpy;
        double m_lpx2;
        double m_lpy2;
        double m_lpx3;
        double m_lpy3;
        double m_lcx;
        double m_lcy;
        double m_lrx;
        double m_lry;
    };
}

#endif // PDF_PAINTER_EXTENSIONS_H
