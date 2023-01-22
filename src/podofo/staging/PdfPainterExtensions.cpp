/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPainterExtensions.h"
#include <podofo/private/numbers_compat.h>
#include <podofo/main/PdfPainter.h>

using namespace std;
using namespace PoDoFo;

PdfPainterExtensions::PdfPainterExtensions(PdfPainter& painter) :
    m_painter(&painter),
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

void PdfPainterExtensions::HorizontalLineTo(double x)
{
    m_painter->LineTo(x, m_lpy3);
}

void PdfPainterExtensions::VerticalLineTo(double y)
{
    m_painter->LineTo(m_lpx3, y);
}

void PdfPainterExtensions::SmoothCurveTo(double x2, double y2, double x3, double y3)
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

    m_painter->CubicBezierTo(px, py, px2, py2, px3, py3);
}

void PdfPainterExtensions::QuadCurveTo(double x1, double y1, double x3, double y3)
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

    m_painter->CubicBezierTo(px, py, px2, py2, px3, py3);
}

void PdfPainterExtensions::SmoothQuadCurveTo(double x3, double y3)
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

    m_painter->CubicBezierTo(px, py, px2, py2, px3, py3);
}

void PdfPainterExtensions::ArcTo(double x, double y, double radiusX, double radiusY,
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
        m_painter->CubicBezierTo(nx1, ny1, nx2, ny2, nx3, ny3);
    }

    m_lpx = m_lpx2 = m_lpx3 = px;
    m_lpy = m_lpy2 = m_lpy3 = py;
    m_lcx = px;
    m_lcy = py;
    m_lrx = px;
    m_lry = py;
}
