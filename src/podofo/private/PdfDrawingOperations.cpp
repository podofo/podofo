/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PdfDeclarationsPrivate.h"
#include "PdfDrawingOperations.h"

using namespace std;
using namespace PoDoFo;

constexpr unsigned BEZIER_POINTS = 13;

// 4/3 * (1-cos 45<-,A0<-(B)/sin 45<-,A0<-(B = 4/3 * sqrt(2) - 1
constexpr double ARC_MAGIC = 0.552284749;

static void convertRectToBezier(double x, double y, double width, double height, double pointsX[], double pointsY[]);
static void writeColorComponents(PdfStringStream& stream, const cspan<double>& components);
static double normalizeAngle(double alpha);
static void getCirclePoint(double cx, double cy, double radius, double alpha, double& x, double& y);
static void getArcBezierControlPoints(double cx, double cy, double x0, double y0, double x3, double y3,
    double& x1, double& y1, double& x2, double& y2);

// https://hepunx.rl.ac.uk/~adye/psdocs/ref/PSL2a.html#arct
void PoDoFo::WriteArcTo(PdfStringStream& stream, double x0, double y0, double x1, double y1,
    double x2, double y2, double radius, Vector2& currP)
{
    // Reference https://math.stackexchange.com/questions/191942/find-arc-center-from-tangent-lines-and-rounding-value

    double x1_0 = x0 - x1;
    double y1_0 = y0 - y1;
    double x1_2 = x2 - x1;
    double y1_2 = y2 - y1;

    // Compute the tagent points
    double norm1 = std::sqrt(x1_0 * x1_0 + y1_0 * y1_0);
    double norm2 = std::sqrt(x1_2 * x1_2 + y1_2 * y1_2);

    double x1t = x1 + x1_0 / norm1 * radius;
    double y1t = y1 + y1_0 / norm1 * radius;
    double x2t = x1 + x1_2 / norm2 * radius;
    double y2t = y1 + y1_2 / norm2 * radius;

    // Compute a two-point form -(y2–y1)*(x-x1) + (x2-x1)*(y-y1) = 0 and
    // then find the equation of perpendicular on the point (x1,y1) with b*x - a*y + a * y1 − b * x1 = 0

    // Compute the coefficientes of a line passing through (x1t, y1t) and perpendicular to the arc tangent
    double a0t = x1_0;
    double b0t = y1_0;
    double c0t = -x1_0 * x1t - y1_0 * y1t;

    // Compute the coefficientes of a line passing through (x2t, y2t) and perpendicular to the arc tangent
    double a2t = x1_2;
    double b2t = y1_2;
    double c2t = -x1_2 * x2t - y1_2 * y2t;

    // https://www.cuemath.com/geometry/intersection-of-two-lines/
    double xc = (b0t * c2t - b2t * c0t) / (a0t * b2t - a2t * b0t);
    double yc = (c0t * a2t - c2t * a0t) / (a0t * b2t - a2t * b0t);

    double c1x;
    double c1y;
    double c2x;
    double c2y;
    getArcBezierControlPoints(xc, yc, x1t, y1t, x2t, y2t, c1x, c1y, c2x, c2y);

    // Draw the advancement to the first tangent point
    PoDoFo::WriteOperator_l(stream, x1t, y1t);
    // Draw the bezier curve
    PoDoFo::WriteOperator_c(stream, c1x, c1y, c2x, c2y, x2t, y2t);

    // According to testing of HTML5 canvas, it's not
    // needed to continue the line to the point (x2, y2)

    currP.X = x2t;
    currP.Y = y2t;
}

// https://hepunx.rl.ac.uk/~adye/psdocs/ref/PSL2a.html#arc
void PoDoFo::WriteArc(PdfStringStream& stream, double x, double y, double radius,
    double startAngle, double endAngle, bool clockWise, Vector2& currP)
{
    startAngle = normalizeAngle(startAngle);
    endAngle = normalizeAngle(endAngle);
    double angleDiff = endAngle - startAngle;
    if (!clockWise)
        angleDiff = -angleDiff;

    unsigned subArcCount = (unsigned)std::ceil(std::abs(angleDiff) / (numbers::pi / 2));
    double x0;
    double y0;
    getCirclePoint(x, y, radius, startAngle, x0, y0);
    PoDoFo::WriteOperator_l(stream, x0, y0);

    double angleStep = angleDiff / subArcCount;
    double angleOffset = startAngle;
    double x1, x2, x3 = x0;
    double y1, y2, y3 = y0;
    for (unsigned i = 0; i < subArcCount; i++)
    {
        angleOffset += angleStep;
        getCirclePoint(x, y, radius, angleOffset, x3, y3);
        getArcBezierControlPoints(x, y, x0, y0, x3, y3, x1, y1, x2, y2);
        PoDoFo::WriteOperator_c(stream, x1, y1, x2, y2, x3, y3);
        x0 = x3;
        y0 = y3;
    }

    currP.X = x3;
    currP.Y = y3;
}

void PoDoFo::WriteCircle(PdfStringStream& stream, double x, double y, double radius, Vector2& currP)
{
    // draw four Bezier curves to approximate a circle
    PoDoFo::WriteOperator_m(stream, x + radius, y);
    PoDoFo::WriteOperator_c(stream, x + radius, y + radius * ARC_MAGIC,
        x + radius * ARC_MAGIC, y + radius,
        x, y + radius);
    PoDoFo::WriteOperator_c(stream, x - radius * ARC_MAGIC, y + radius,
        x - radius, y + radius * ARC_MAGIC,
        x - radius, y);
    PoDoFo::WriteOperator_c(stream, x - radius, y - radius * ARC_MAGIC,
        x - radius * ARC_MAGIC, y - radius,
        x, y - radius);
    PoDoFo::WriteOperator_c(stream, x + radius * ARC_MAGIC, y - radius,
        x + radius, y - radius * ARC_MAGIC,
        x + radius, y);
    PoDoFo::WriteOperator_h(stream);
    currP = Vector2(x + radius, y);
}

void PoDoFo::WriteEllipse(PdfStringStream& stream, double x, double y,
    double width, double height, Vector2& currP)
{
    double pointsX[BEZIER_POINTS];
    double pointsY[BEZIER_POINTS];
    convertRectToBezier(x, y, width, height, pointsX, pointsY);

    PoDoFo::WriteOperator_m(stream, x, y);
    for (unsigned i = 1; i < BEZIER_POINTS; i += 3)
        PoDoFo::WriteOperator_c(stream, pointsX[i], pointsY[i], pointsX[i + 1], pointsY[i + 1], pointsX[i + 2], pointsY[i + 2]);

    PoDoFo::WriteOperator_h(stream);
    currP = Vector2(pointsX[BEZIER_POINTS - 1], pointsY[BEZIER_POINTS - 1]);
}

void PoDoFo::WriteRectangle(PdfStringStream& stream, double x, double y,
    double width, double height, double roundX, double roundY, Vector2& currP)
{
    if (static_cast<int>(roundX) || static_cast<int>(roundY))
    {
        double w = width;
        double h = height;
        double rx = roundX;
        double ry = roundY;
        double b = 0.4477f;

        PoDoFo::WriteOperator_m(stream, x + rx, y);
        PoDoFo::WriteOperator_l(stream, x + w - rx, y);
        PoDoFo::WriteOperator_c(stream, x + w - rx * b, y, x + w, y + ry * b, x + w, y + ry);
        PoDoFo::WriteOperator_l(stream, x + w, y + h - ry);
        PoDoFo::WriteOperator_c(stream, x + w, y + h - ry * b, x + w - rx * b, y + h, x + w - rx, y + h);
        PoDoFo::WriteOperator_l(stream, x + rx, y + h);
        PoDoFo::WriteOperator_c(stream, x + rx * b, y + h, x, y + h - ry * b, x, y + h - ry);
        PoDoFo::WriteOperator_l(stream, x, y + ry);
        PoDoFo::WriteOperator_c(stream, x, y + ry * b, x + rx * b, y, x + rx, y);
        PoDoFo::WriteOperator_h(stream);
        currP = Vector2(x + rx, y);
    }
    else
    {
        PoDoFo::WriteOperator_re(stream, x, y, width, height);
        currP = Vector2(x, y);
    }
}

void PoDoFo::WriteOperator_re(PdfStringStream& stream, double x, double y, double width, double height)
{
    stream << x << ' ' << y << ' ' << width << ' ' << height << " re\n";
}

void PoDoFo::WriteOperator_m(PdfStringStream& stream, double x, double y)
{
    stream << x << ' ' << y << " m\n";
}

void PoDoFo::WriteOperator_l(PdfStringStream& stream, double x, double y)
{
    stream << x << ' ' << y << " l\n";
}

void PoDoFo::WriteOperator_c(PdfStringStream& stream, double c1x, double c1y, double c2x, double c2y, double x, double y)
{
    stream << c1x << ' ' << c1y << ' ' << c2x << ' ' << c2y << ' ' << x << ' ' << y << " c\n";
}

void PoDoFo::WriteOperator_y(PdfStringStream& stream, double cx, double cy, double x, double y)
{
    stream << cx << ' ' << cy << ' ' << x << ' ' << y << " y\n";
}

void PoDoFo::WriteOperator_v(PdfStringStream& stream, double cx, double cy, double x, double y)
{
    stream << cx << ' ' << cy << ' ' << x << ' ' << y << " v\n";
}

void PoDoFo::WriteOperator_n(PdfStringStream& stream)
{
    stream << "n\n";
}

void PoDoFo::WriteOperator_h(PdfStringStream& stream)
{
    stream << "h\n";
}

void PoDoFo::WriteOperator_b(PdfStringStream& stream)
{
    stream << "b\n";
}

void PoDoFo::WriteOperator_B(PdfStringStream& stream)
{
    stream << "B\n";
}

void PoDoFo::WriteOperator_bStar(PdfStringStream& stream)
{
    stream << "b*\n";
}

void PoDoFo::WriteOperator_BStar(PdfStringStream& stream)
{
    stream << "B*\n";
}

void PoDoFo::WriteOperator_s(PdfStringStream& stream)
{
    stream << "s\n";
}

void PoDoFo::WriteOperator_S(PdfStringStream& stream)
{
    stream << "S\n";
}

void PoDoFo::WriteOperator_f(PdfStringStream& stream)
{
    stream << "f\n";
}

void PoDoFo::WriteOperator_fStar(PdfStringStream& stream)
{
    stream << "f*\n";
}

void PoDoFo::WriteOperator_W(PdfStringStream& stream)
{
    stream << "W\n";
}

void PoDoFo::WriteOperator_WStar(PdfStringStream& stream)
{
    stream << "W*\n";
}

void PoDoFo::WriteOperator_MP(PdfStringStream& stream, const string_view& tag)
{
    stream << '/' << tag << " MP\n";
}

void PoDoFo::WriteOperator_DP(PdfStringStream& stream, const string_view& tag, const PdfDictionary& properties)
{
    charbuff buffer;
    stream << '/' << tag << ' ';
    properties.Write(stream, PdfWriteFlags::None, { }, buffer);
    stream << " DP\n";
}

void PoDoFo::WriteOperator_DP(PdfStringStream& stream, const string_view& tag, const string_view& propertyDictName)
{
    stream << '/' << tag << ' ' << '/' << propertyDictName << " DP\n";
}

void PoDoFo::WriteOperator_BMC(PdfStringStream& stream, const string_view& tag)
{
    stream << '/' << tag << " BMC\n";
}

void PoDoFo::WriteOperator_BDC(PdfStringStream& stream, const string_view& tag, const PdfDictionary& properties)
{
    charbuff buffer;
    stream << '/' << tag << ' ';
    properties.Write(stream, PdfWriteFlags::None, { }, buffer);
    stream << " BDC\n";
}

void PoDoFo::WriteOperator_BDC(PdfStringStream& stream, const string_view& tag, const string_view& propertyDictName)
{
    stream << '/' << tag << ' ' << '/' << propertyDictName << " BDC\n";
}

void PoDoFo::WriteOperator_EMC(PdfStringStream& stream)
{
    stream << "EMC\n";
}

void PoDoFo::WriteOperator_q(PdfStringStream& stream)
{
    stream << "q\n";
}

void PoDoFo::WriteOperator_Q(PdfStringStream& stream)
{
    stream << "Q\n";
}

void PoDoFo::WriteOperator_BT(PdfStringStream& stream)
{
    stream << "BT\n";
}

void PoDoFo::WriteOperator_ET(PdfStringStream& stream)
{
    stream << "ET\n";
}

void PoDoFo::WriteOperator_Td(PdfStringStream& stream, double tx, double ty)
{
    stream << tx << ' ' << ty << " Td\n";
}

void PoDoFo::WriteOperator_TD(PdfStringStream& stream, double tx, double ty)
{
    stream << tx << ' ' << ty << " TD\n";
}

void PoDoFo::WriteOperator_Tm(PdfStringStream& stream, double a, double b, double c, double d, double e, double f)
{
    stream << a << ' ' << b << ' ' << c << ' ' << d << ' ' << e << ' ' << f << " Tm\n";
}

void PoDoFo::WriteOperator_Tr(PdfStringStream& stream, PdfTextRenderingMode mode)
{
    stream << (unsigned)mode << " Tr\n";
}

void PoDoFo::WriteOperator_Ts(PdfStringStream& stream, double rise)
{
    stream << rise << " Ts\n";
}

void PoDoFo::WriteOperator_Tc(PdfStringStream& stream, double charSpace)
{
    stream << charSpace << " Tc\n";
}

void PoDoFo::WriteOperator_TL(PdfStringStream& stream, double leading)
{
    stream << leading << " TL\n";
}

void PoDoFo::WriteOperator_Tf(PdfStringStream& stream, const string_view& fontName, double fontSize)
{
    stream << '/' << fontName << ' ' << fontSize << " Tf\n";
}

void PoDoFo::WriteOperator_Tw(PdfStringStream& stream, double wordSpace)
{
    stream << wordSpace << " Tw\n";
}

void PoDoFo::WriteOperator_Tz(PdfStringStream& stream, double scale)
{
    stream << scale << " Tz\n";
}

void PoDoFo::WriteOperator_Tj(PdfStringStream& stream, const string_view& encoded, bool hex)
{
    utls::SerializeEncodedString(stream, encoded, hex);
    stream << " Tj\n";
}

void PoDoFo::WriteOperator_TJ_Begin(PdfStringStream& stream)
{
    stream << "[ ";
}

void PoDoFo::WriteOperator_TJ_Delta(PdfStringStream& stream, double delta)
{
    stream << delta << ' ';
}

void PoDoFo::WriteOperator_TJ_String(PdfStringStream& stream, const string_view& encoded, bool hex)
{
    utls::SerializeEncodedString(stream, encoded, hex);
    stream << ' ';
}

void PoDoFo::WriteOperator_TJ_End(PdfStringStream& stream)
{
    stream << ']' << " TJ\n" << endl;
}

void PoDoFo::WriteOperator_cm(PdfStringStream& stream, double a, double b, double c, double d, double e, double f)
{
    stream << a << ' ' << b << ' ' << c << ' ' << d << ' ' << e << ' ' << f << " cm\n";
}

void PoDoFo::WriteOperator_w(PdfStringStream& stream, double lineWidth)
{
    stream << lineWidth << " w\n";
}

void PoDoFo::WriteOperator_J(PdfStringStream& stream, PdfLineCapStyle style)
{
    stream << (unsigned)style << " J\n";
}

void PoDoFo::WriteOperator_j(PdfStringStream& stream, PdfLineJoinStyle style)
{
    stream << (unsigned)style << " j\n";
}

void PoDoFo::WriteOperator_M(PdfStringStream& stream, double miterLimit)
{
    stream << miterLimit << " M\n";
}

void PoDoFo::WriteOperator_d(PdfStringStream& stream, const cspan<double>& dashArray, double fase)
{
    stream <<  '[';
    for (unsigned i = 0; i < dashArray.size(); i++)
        stream << dashArray[i] << ' ';
    stream << "] " << fase << " d\n";
}

void PoDoFo::WriteOperator_ri(PdfStringStream& stream, const string_view& intent)
{
    stream << '/' << intent << " ri\n";
}

void PoDoFo::WriteOperator_i(PdfStringStream& stream, double flatness)
{
    stream << flatness << " i\n";
}

void PoDoFo::WriteOperator_gs(PdfStringStream& stream, const string_view& dictName)
{
    stream << '/' << dictName << " gs\n";
}

void PoDoFo::WriteOperator_Do(PdfStringStream& stream, const string_view& xobjname)
{
    stream << '/' << xobjname << " Do\n";
}

void PoDoFo::WriteOperator_cs(PdfStringStream& stream, PdfColorSpaceType colorSpace)
{
    stream << '/' << PoDoFo::ToString(colorSpace) << " cs\n";
}

void PoDoFo::WriteOperator_cs(PdfStringStream& stream, const string_view& name)
{
    stream << '/' << name << " cs\n";
}

void PoDoFo::WriteOperator_CS(PdfStringStream& stream, PdfColorSpaceType colorSpace)
{
    stream << '/' << PoDoFo::ToString(colorSpace) << " CS\n";
}

void PoDoFo::WriteOperator_CS(PdfStringStream& stream, const string_view& name)
{
    stream << '/' << name << " CS\n";
}

void PoDoFo::WriteOperator_sc(PdfStringStream& stream, const cspan<double>& components)
{
    writeColorComponents(stream, components);
    stream << "sc\n";
}

void PoDoFo::WriteOperator_SC(PdfStringStream& stream, const cspan<double>& components)
{
    writeColorComponents(stream, components);
    stream << "SC\n";
}

void PoDoFo::WriteOperator_scn(PdfStringStream& stream, const cspan<double>& components)
{
    writeColorComponents(stream, components);
    stream << "scn\n";
}

void PoDoFo::WriteOperator_SCN(PdfStringStream& stream, const cspan<double>& components)
{
    writeColorComponents(stream, components);
    stream << "SCN\n";
}

void PoDoFo::WriteOperator_scn(PdfStringStream& stream, const cspan<double>& components, const string_view& patternName)
{
    writeColorComponents(stream, components);
    stream << '/' << patternName << " scn\n";
}

void PoDoFo::WriteOperator_SCN(PdfStringStream& stream, const cspan<double>& components, const string_view& patternName)
{
    writeColorComponents(stream, components);
    stream << '/' << patternName << " SCN\n";
}

void PoDoFo::WriteOperator_scn(PdfStringStream& stream, const string_view& patternName)
{
    stream << '/' << patternName << " scn\n";
}

void PoDoFo::WriteOperator_SCN(PdfStringStream& stream, const string_view& patternName)
{
    stream << '/' << patternName << " SCN\n";
}

void PoDoFo::WriteOperator_G(PdfStringStream& stream, double gray)
{
    stream << gray << " G\n";
}

void PoDoFo::WriteOperator_g(PdfStringStream& stream, double gray)
{
    stream << gray << " g\n";
}

void PoDoFo::WriteOperator_RG(PdfStringStream& stream, double red, double green, double blue)
{
    stream << red << ' ' << green << ' ' << blue << " RG\n";
}

void PoDoFo::WriteOperator_rg(PdfStringStream& stream, double red, double green, double blue)
{
    stream << red << ' ' << green << ' ' << blue << " rg\n";
}

void PoDoFo::WriteOperator_K(PdfStringStream& stream, double cyan, double magenta, double yellow, double black)
{
    stream << cyan << ' ' << magenta << ' ' << yellow << ' ' << black << " K\n";
}

void PoDoFo::WriteOperator_k(PdfStringStream& stream, double cyan, double magenta, double yellow, double black)
{
    stream << cyan << ' ' << magenta << ' ' << yellow << ' ' << black << " k\n";
}

void PoDoFo::WriteOperator_sh(PdfStringStream& stream, const string_view& shadingDictName)
{
    stream << '/' << shadingDictName << " sh\n";
}

void PoDoFo::WriteOperator_BX(PdfStringStream& stream)
{
    stream << "BX\n";
}

void PoDoFo::WriteOperator_EX(PdfStringStream& stream)
{
    stream << "EX\n";
}

void PoDoFo::WriteOperator_Extension(PdfStringStream& stream, const string_view& opName, const cspan<PdfVariant>& operands)
{
    charbuff buffer;
    for (unsigned i = 0; i < operands.size(); i++)
    {
        operands[i].Write(stream, PdfWriteFlags::None, { }, buffer);
        stream << ' ';
    }

    stream << opName << '\n';
}

/*
 * Converts a rectangle to an array of points which can be used
 * to draw an ellipse using 4 bezier curves.
 *
 * The arrays plPointX and plPointY need space for at least 12 longs
 * to be stored.
 *
 * \param x x position of the bounding rectangle
 * \param y y position of the bounding rectangle
 * \param width width of the bounding rectangle
 * \param height height of the bounding rectangle
 * \param pointsX pointer to an array were the x coordinates
 *                 of the resulting points will be stored
 * \param pointsY pointer to an array were the y coordinates
 *                 of the resulting points will be stored
 *
 */
void convertRectToBezier(double x, double y, double width, double height, double pointsX[], double pointsY[])
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

void writeColorComponents(PdfStringStream& stream, const cspan<double>& components)
{
    for (unsigned i = 0; i < components.size(); i++)
        stream << components[i] << ' ';
}

double normalizeAngle(double alpha)
{
    return alpha - 2 * numbers::pi * std::floor(alpha / (2 * numbers::pi));
}

void getCirclePoint(double xc, double yc, double radius, double alpha, double& x, double& y)
{
    x = xc + radius * std::cos(alpha);
    y = yc + radius * std::sin(alpha);
}

// Ref. https://stackoverflow.com/a/44829356/213871
void getArcBezierControlPoints(double xc, double yc, double x0, double y0, double x3, double y3, double& x1, double& y1, double& x2, double& y2)
{
    double ax = x0 - xc;
    double ay = y0 - yc;
    double bx = x3 - xc;
    double by = y3 - yc;
    double q1 = ax * ax + ay * ay;
    double q2 = q1 + ax * bx + ay * by;
    double k2 = (4 / 3.) * (sqrt(2 * q1 * q2) - q2) / (ax * by - ay * bx);

    x1 = xc + ax - k2 * ay;
    y1 = yc + ay + k2 * ax;
    x2 = xc + bx + k2 * by;
    y2 = yc + by - k2 * bx;
}

void getControlPoint(double cx, double cy, double x0, double y0, double x2, double y2, double& x1, double& y1)
{
    // Compute the coefficients of the tantent to the point P0
    double a0 = cy - y0;
    double b0 = x0 - cx;
    double a0t = b0;
    double b0t = -a0;
    double c0t = -b0 * x0 + a0 * y0;

    // Compute the coefficients of the tantent to the point P0
    double a2 = cy - y2;
    double b2 = x2 - cx;
    double a2t = b2;
    double b2t = -a2;
    double c2t = -b2 * x2 + a2 * y2;

    x1 = (b0t * c2t - b2t * c0t) / (a0t * b2t - a2t * b0t);
    y1 = (c0t * a2t - c2t * a0t) / (a0t * b2t - a2t * b0t);
}
