/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_DRAWING_OPERATORS_H
#define PDF_DRAWING_OPERATORS_H

#include <podofo/main/PdfDictionary.h>
#include <podofo/main/PdfStringStream.h>
#include <podofo/main/PdfMath.h>

namespace PoDoFo {

void WriteArcTo(PdfStringStream& stream, double x0, double y0, double x1, double y1,
    double x2, double y2, double radius, Vector2& currP);
void WriteArc(PdfStringStream& stream, double x, double y, double radius,
    double startAngle, double endAngle, bool clockWise, Vector2& currP);
void WriteCircle(PdfStringStream& stream, double x, double y, double radius, Vector2& currP);
void WriteEllipse(PdfStringStream& stream, double x, double y,
    double width, double height, Vector2& currP);
void WriteRectangle(PdfStringStream& stream, double x, double y,
    double width, double height, double roundX, double roundY, Vector2& currP);

// Raw operators
void WriteOperator_re(PdfStringStream& stream, double x, double y, double width, double height);
void WriteOperator_m(PdfStringStream& stream, double x, double y);
void WriteOperator_l(PdfStringStream& stream, double x, double y);
void WriteOperator_c(PdfStringStream& stream, double c1x, double c1y, double c2x, double c2y, double x, double y);
void WriteOperator_y(PdfStringStream& stream, double cx, double cy, double x, double y);
void WriteOperator_v(PdfStringStream& stream, double cx, double cy, double x, double y);
void WriteOperator_n(PdfStringStream& stream);
void WriteOperator_h(PdfStringStream& stream);
void WriteOperator_b(PdfStringStream& stream);
void WriteOperator_B(PdfStringStream& stream);
void WriteOperator_bStar(PdfStringStream& stream);
void WriteOperator_BStar(PdfStringStream& stream);
void WriteOperator_s(PdfStringStream& stream);
void WriteOperator_S(PdfStringStream& stream);
void WriteOperator_f(PdfStringStream& stream);
void WriteOperator_fStar(PdfStringStream& stream);
void WriteOperator_W(PdfStringStream& stream);
void WriteOperator_WStar(PdfStringStream& stream);
void WriteOperator_MP(PdfStringStream& stream, const std::string_view& tag);
void WriteOperator_DP(PdfStringStream& stream, const std::string_view& tag, const PdfDictionary& properties);
void WriteOperator_DP(PdfStringStream& stream, const std::string_view& tag, const std::string_view& propertyDictName);
void WriteOperator_BMC(PdfStringStream& stream, const std::string_view& tag);
void WriteOperator_BDC(PdfStringStream& stream, const std::string_view& tag, const PdfDictionary& properties);
void WriteOperator_BDC(PdfStringStream& stream, const std::string_view& tag, const std::string_view& propertyDictName);
void WriteOperator_EMC(PdfStringStream& stream);
void WriteOperator_q(PdfStringStream& stream);
void WriteOperator_Q(PdfStringStream& stream);
void WriteOperator_BT(PdfStringStream& stream);
void WriteOperator_ET(PdfStringStream& stream);
void WriteOperator_Td(PdfStringStream& stream, double tx, double ty);
void WriteOperator_TD(PdfStringStream& stream, double tx, double ty);
void WriteOperator_Tm(PdfStringStream& stream, double a, double b, double c, double d, double e, double f);
void WriteOperator_Tr(PdfStringStream& stream, PdfTextRenderingMode mode);
void WriteOperator_Ts(PdfStringStream& stream, double rise);
void WriteOperator_Tc(PdfStringStream& stream, double charSpace);
void WriteOperator_TL(PdfStringStream& stream, double leading);
void WriteOperator_Tf(PdfStringStream& stream, const std::string_view& fontName, double fontSize);
void WriteOperator_Tw(PdfStringStream& stream, double wordSpace);
void WriteOperator_Tz(PdfStringStream& stream, double scale);
void WriteOperator_Tj(PdfStringStream& stream, const std::string_view& encoded, bool hex);
void WriteOperator_TJ_Begin(PdfStringStream& stream);
void WriteOperator_TJ_Delta(PdfStringStream& stream, double delta);
void WriteOperator_TJ_String(PdfStringStream& stream, const std::string_view& encoded, bool hex);
void WriteOperator_TJ_End(PdfStringStream& stream);
void WriteOperator_cm(PdfStringStream& stream, double a, double b, double c, double d, double e, double f);
void WriteOperator_w(PdfStringStream& stream, double lineWidth);
void WriteOperator_J(PdfStringStream& stream, PdfLineCapStyle style);
void WriteOperator_j(PdfStringStream& stream, PdfLineJoinStyle style);
void WriteOperator_M(PdfStringStream& stream, double miterLimit);
void WriteOperator_d(PdfStringStream& stream, const cspan<double>& dashArray, double fase);
void WriteOperator_ri(PdfStringStream& stream, const std::string_view& intent);
void WriteOperator_i(PdfStringStream& stream, double flatness);
void WriteOperator_gs(PdfStringStream& stream, const std::string_view& dictName);
void WriteOperator_Do(PdfStringStream& stream, const std::string_view& xobjname);
void WriteOperator_cs(PdfStringStream& stream, PdfColorSpaceType colorSpace);
void WriteOperator_cs(PdfStringStream& stream, const std::string_view& name);
void WriteOperator_CS(PdfStringStream& stream, PdfColorSpaceType colorSpace);
void WriteOperator_CS(PdfStringStream& stream, const std::string_view& name);
void WriteOperator_sc(PdfStringStream& stream, const cspan<double>& components);
void WriteOperator_SC(PdfStringStream& stream, const cspan<double>& components);
void WriteOperator_scn(PdfStringStream& stream, const cspan<double>& components);
void WriteOperator_SCN(PdfStringStream& stream, const cspan<double>& components);
void WriteOperator_scn(PdfStringStream& stream, const cspan<double>& components, const std::string_view& patternName);
void WriteOperator_SCN(PdfStringStream& stream, const cspan<double>& components, const std::string_view& patternName);
void WriteOperator_scn(PdfStringStream& stream, const std::string_view& patternName);
void WriteOperator_SCN(PdfStringStream& stream, const std::string_view& patternName);
void WriteOperator_G(PdfStringStream& stream, double gray);
void WriteOperator_g(PdfStringStream& stream, double gray);
void WriteOperator_RG(PdfStringStream& stream, double red, double green, double blue);
void WriteOperator_rg(PdfStringStream& stream, double red, double green, double blue);
void WriteOperator_K(PdfStringStream& stream, double cyan, double magenta, double yellow, double black);
void WriteOperator_k(PdfStringStream& stream, double cyan, double magenta, double yellow, double black);
void WriteOperator_sh(PdfStringStream& stream, const std::string_view& shadingDictName);
void WriteOperator_BX(PdfStringStream& stream);
void WriteOperator_EX(PdfStringStream& stream);
void WriteOperator_Extension(PdfStringStream& stream, const std::string_view& opName, const cspan<PdfVariant>& operands);

}

#endif // PDF_DRAWING_OPERATORS_H
