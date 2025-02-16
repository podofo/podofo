/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_CONTENT_STREAM_OPERATORS_H
#define PDF_CONTENT_STREAM_OPERATORS_H

#include <podofo/main/PdfDictionary.h>
#include <podofo/main/PdfStringStream.h>
#include <podofo/main/PdfMath.h>

namespace PoDoFo
{
/**
 * Pdf content stream callble operator interface
 * ISO 32000 - 1:2008 "A.2 PDF Content Stream Operators"
 */
// TODO: Add missing operators
class PODOFO_API PdfContentStreamOperators
{
protected:
    PdfContentStreamOperators();
public:
    virtual void re_Operator(double x, double y, double width, double height) = 0;
    virtual void m_Operator(double x, double y) = 0;
    virtual void l_Operator(double x, double y) = 0;
    virtual void c_Operator(double c1x, double c1y, double c2x, double c2y, double x, double y) = 0;
    virtual void v_Operator(double cx, double cy, double x, double y) = 0;
    virtual void y_Operator(double cx, double cy, double x, double y) = 0;
    virtual void n_Operator() = 0;
    virtual void h_Operator() = 0;
    virtual void b_Operator() = 0;
    virtual void B_Operator() = 0;
    virtual void bStar_Operator() = 0;
    virtual void BStar_Operator() = 0;
    virtual void s_Operator() = 0;
    virtual void S_Operator() = 0;
    virtual void f_Operator() = 0;
    virtual void fStar_Operator() = 0;
    virtual void W_Operator() = 0;
    virtual void WStar_Operator() = 0;
    virtual void MP_Operator(const std::string_view& tag) = 0;
    virtual void DP_Operator(const std::string_view& tag, const PdfDictionary& properties) = 0;
    virtual void DP_Operator(const std::string_view& tag, const std::string_view& propertyDictName) = 0;
    virtual void BMC_Operator(const std::string_view& tag) = 0;
    virtual void BDC_Operator(const std::string_view& tag, const PdfDictionary& properties) = 0;
    virtual void BDC_Operator(const std::string_view& tag, const std::string_view& propertyDictName) = 0;
    virtual void EMC_Operator() = 0;
    virtual void q_Operator() = 0;
    virtual void Q_Operator() = 0;
    virtual void BT_Operator() = 0;
    virtual void ET_Operator() = 0;
    virtual void Td_Operator(double tx, double ty) = 0;
    virtual void TD_Operator(double tx, double ty) = 0;
    virtual void Tm_Operator(double a, double b, double c, double d, double e, double f) = 0;
    virtual void Tr_Operator(PdfTextRenderingMode mode) = 0;
    virtual void Ts_Operator(double rise) = 0;
    virtual void Tc_Operator(double charSpace) = 0;
    virtual void TL_Operator(double leading) = 0;
    virtual void Tf_Operator(const std::string_view& fontName, double fontSize) = 0;
    virtual void Tw_Operator(double wordSpace) = 0;
    virtual void Tz_Operator(double scale) = 0;
    virtual void Tj_Operator(const std::string_view& encoded, bool hex) = 0;
    virtual void TJ_Operator_Begin() = 0;
    virtual void TJ_Operator_Delta(double delta) = 0;
    virtual void TJ_Operator_Glyphs(const std::string_view& encoded, bool hex) = 0;
    virtual void TJ_Operator_End() = 0;
    virtual void cm_Operator(double a, double b, double c, double d, double e, double f) = 0;
    virtual void w_Operator(double lineWidth) = 0;
    virtual void J_Operator(PdfLineCapStyle style) = 0;
    virtual void j_Operator(PdfLineJoinStyle style) = 0;
    virtual void M_Operator(double miterLimit) = 0;
    virtual void d_Operator(const cspan<double>& dashArray, double fase) = 0;
    virtual void ri_Operator(const std::string_view& intent) = 0;
    virtual void i_Operator(double flatness) = 0;
    virtual void gs_Operator(const std::string_view& dictName) = 0;
    virtual void Do_Operator(const std::string_view& xobjname) = 0;
    virtual void cs_Operator(PdfColorSpaceType colorSpace) = 0;
    virtual void cs_Operator(const std::string_view& name) = 0;
    virtual void CS_Operator(PdfColorSpaceType colorSpace) = 0;
    virtual void CS_Operator(const std::string_view& name) = 0;
    virtual void sc_Operator(const cspan<double>& components) = 0;
    virtual void SC_Operator(const cspan<double>& components) = 0;
    virtual void scn_Operator(const cspan<double>& components) = 0;
    virtual void SCN_Operator(const cspan<double>& components) = 0;
    virtual void scn_Operator(const cspan<double>& components, const std::string_view& patternName) = 0;
    virtual void SCN_Operator(const cspan<double>& components, const std::string_view& patternName) = 0;
    virtual void scn_Operator(const std::string_view& patternName) = 0;
    virtual void SCN_Operator(const std::string_view& patternName) = 0;
    virtual void G_Operator(double gray) = 0;
    virtual void g_Operator(double gray) = 0;
    virtual void RG_Operator(double red, double green, double blue) = 0;
    virtual void rg_Operator(double red, double green, double blue) = 0;
    virtual void K_Operator(double cyan, double magenta, double yellow, double black) = 0;
    virtual void k_Operator(double cyan, double magenta, double yellow, double black) = 0;
    virtual void sh_Operator(const std::string_view& patternName) = 0;
    virtual void BX_Operator() = 0;
    virtual void EX_Operator() = 0;
    virtual void Extension_Operator(const std::string_view& opName, const cspan<PdfVariant>& operands) = 0;
protected:
    PdfContentStreamOperators(const PdfContentStreamOperators&) = default;
    PdfContentStreamOperators& operator=(const PdfContentStreamOperators&) = default;
};

}

#endif // PDF_CONTENT_STREAM_OPERATORS_H
