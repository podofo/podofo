/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfPainter.h"
#include <podofo/private/PdfDrawingOperations.h>

using namespace std;
using namespace PoDoFo;

void PdfPainter::re_Operator(double x, double y, double width, double height)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    openPath(x, y);
    PoDoFo::WriteOperator_re(m_stream, x, y, width, height);
    m_StateStack.Current->CurrentPoint = Vector2(x, y);
}

void PdfPainter::m_Operator(double x, double y)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    openPath(x, y);
    PoDoFo::WriteOperator_m(m_stream, x, y);
    m_StateStack.Current->CurrentPoint = Vector2(x, y);
}

void PdfPainter::l_Operator(double x, double y)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    checkPathOpened();
    PoDoFo::WriteOperator_l(m_stream, x, y);
    m_StateStack.Current->CurrentPoint = Vector2(x, y);
}

void PdfPainter::c_Operator(double c1x, double c1y, double c2x, double c2y, double x, double y)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    checkPathOpened();
    PoDoFo::WriteOperator_c(m_stream, c1x, c1y, c2x, c2y, x, y);
    m_StateStack.Current->CurrentPoint = Vector2(x, y);
}

void PdfPainter::n_Operator()
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_n(m_stream);
    resetPath();
}

void PdfPainter::h_Operator()
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    checkPathOpened();
    PoDoFo::WriteOperator_h(m_stream);
    PODOFO_ASSERT(m_StateStack.Current->FirstPoint != nullptr);
    m_StateStack.Current->CurrentPoint = *m_StateStack.Current->FirstPoint;
}

void PdfPainter::b_Operator()
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_b(m_stream);
    resetPath();
}

void PdfPainter::B_Operator()
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_B(m_stream);
    resetPath();
}

void PdfPainter::bStar_Operator()
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_bStar(m_stream);
    resetPath();
}

void PdfPainter::BStar_Operator()
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_BStar(m_stream);
    resetPath();
}

void PdfPainter::s_Operator()
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_s(m_stream);
    resetPath();
}

void PdfPainter::S_Operator()
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_S(m_stream);
    resetPath();
}

void PdfPainter::f_Operator()
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_f(m_stream);
    resetPath();
}

void PdfPainter::fStar_Operator()
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_fStar(m_stream);
    resetPath();
}

void PdfPainter::W_Operator()
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_W(m_stream);
}

void PdfPainter::WStar_Operator()
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_WStar(m_stream);
}

void PdfPainter::MP_Operator(const string_view& tag)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_MP(m_stream, tag);
}

void PdfPainter::DP_Operator(const string_view& tag, const PdfDictionary& properties)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_DP(m_stream, tag, properties);
}

void PdfPainter::DP_Operator(const string_view& tag, const std::string_view& propertyDictName)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_DP(m_stream, tag, propertyDictName);
}

void PdfPainter::BMC_Operator(const string_view& tag)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_BMC(m_stream, tag);
}

void PdfPainter::BDC_Operator(const string_view& tag, const PdfDictionary& properties)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_BDC(m_stream, tag, properties);
}

void PdfPainter::BDC_Operator(const string_view& tag, const string_view& propertyDictName)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_BDC(m_stream, tag, propertyDictName);
}

void PdfPainter::EMC_Operator()
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_EMC(m_stream);
}

void PdfPainter::q_Operator()
{
    checkStream();
    checkStatus(StatusDefault);
    PoDoFo::WriteOperator_q(m_stream);
}

void PdfPainter::Q_Operator()
{
    checkStream();
    checkStatus(StatusDefault);
    PoDoFo::WriteOperator_Q(m_stream);
}

void PdfPainter::BT_Operator()
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_BT(m_stream);
    enterTextObject();
}

void PdfPainter::ET_Operator()
{
    checkStream();
    checkStatus(StatusTextObject);
    PoDoFo::WriteOperator_ET(m_stream);
    exitTextObject();
}

void PdfPainter::Td_Operator(double tx, double ty)
{
    checkStream();
    checkStatus(StatusTextObject);
    PoDoFo::WriteOperator_Td(m_stream, tx, ty);
}

void PoDoFo::PdfPainter::TD_Operator(double tx, double ty)
{
    checkStream();
    checkStatus(StatusTextObject);
    PoDoFo::WriteOperator_TD(m_stream, tx, ty);
}

void PdfPainter::Tm_Operator(double a, double b, double c, double d, double e, double f)
{
    checkStream();
    checkStatus(StatusTextObject);
    PoDoFo::WriteOperator_Tm(m_stream, a, b, c, d, e, f);
}

void PdfPainter::Tr_Operator(PdfTextRenderingMode mode)
{
    checkStream();
    checkStatus(StatusTextObject);
    PoDoFo::WriteOperator_Tr(m_stream, mode);
}

void PdfPainter::Ts_Operator(double rise)
{
    checkStream();
    checkStatus(StatusTextObject);
    PoDoFo::WriteOperator_Ts(m_stream, rise);
}

void PdfPainter::Tc_Operator(double charSpace)
{
    checkStream();
    checkStatus( StatusTextObject);
    PoDoFo::WriteOperator_Tc(m_stream, charSpace);
}


void PdfPainter::TL_Operator(double leading)
{
    checkStream();
    checkStatus(StatusTextObject);
    PoDoFo::WriteOperator_TL(m_stream, leading);
}

void PdfPainter::Tf_Operator(const string_view& fontName, double fontSize)
{
    checkStream();
    checkStatus(StatusTextObject);
    PoDoFo::WriteOperator_Tf(m_stream, fontName, fontSize);
}

void PdfPainter::Tw_Operator(double wordSpace)
{
    checkStream();
    checkStatus(StatusTextObject);
    PoDoFo::WriteOperator_Tw(m_stream, wordSpace);
}

void PdfPainter::Tz_Operator(double scale)
{
    checkStream();
    checkStatus(StatusTextObject);
    PoDoFo::WriteOperator_Tz(m_stream, scale);
}

void PdfPainter::Tj_Operator(const string_view& encoded, bool hex)
{
    checkStream();
    checkStatus(StatusTextObject);
    PoDoFo::WriteOperator_Tj(m_stream, encoded, hex);
}

void PdfPainter::TJ_Operator_Begin()
{
    checkStream();
    checkStatus(StatusTextObject);
    PoDoFo::WriteOperator_TJ_Begin(m_stream);
    m_painterStatus = StatusTextArray;
}

void PdfPainter::TJ_Operator_Delta(double delta)
{
    checkStream();
    checkStatus(StatusTextArray);
    PoDoFo::WriteOperator_TJ_Delta(m_stream, delta);
}

void PdfPainter::TJ_Operator_Glyphs(const string_view& encoded, bool hex)
{
    checkStream();
    checkStatus(StatusTextArray);
    PoDoFo::WriteOperator_TJ_String(m_stream, encoded, hex);
}

void PdfPainter::TJ_Operator_End()
{
    checkStream();
    checkStatus(StatusTextArray);
    PoDoFo::WriteOperator_TJ_End(m_stream);
    m_painterStatus = StatusTextObject;
}

void PdfPainter::cm_Operator(double a, double b, double c, double d, double e, double f)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_cm(m_stream, a, b, c, d, e, f);
}

void PdfPainter::w_Operator(double lineWidth)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_w(m_stream, lineWidth);
}

void PdfPainter::J_Operator(PdfLineCapStyle style)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_J(m_stream, style);
}

void PdfPainter::j_Operator(PdfLineJoinStyle style)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_j(m_stream, style);
}

void PdfPainter::M_Operator(double miterLimit)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_M(m_stream, miterLimit);
}

void PdfPainter::d_Operator(const cspan<double>& dashArray, double fase)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_d(m_stream, dashArray, fase);
}

void PdfPainter::ri_Operator(const string_view& intent)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_ri(m_stream, intent);
}

void PdfPainter::i_Operator(double flatness)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_i(m_stream, flatness);
}

void PdfPainter::gs_Operator(const string_view& dictName)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_gs(m_stream, dictName);
}

void PdfPainter::Do_Operator(const string_view& xobjname)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_Do(m_stream, xobjname);
}

void PdfPainter::cs_Operator(PdfColorSpaceType colorSpace)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_cs(m_stream, colorSpace);
}

void PdfPainter::cs_Operator(const string_view& name)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_cs(m_stream, name);
}

void PdfPainter::CS_Operator(PdfColorSpaceType colorSpace)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_CS(m_stream, colorSpace);
}

void PdfPainter::CS_Operator(const string_view& name)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_CS(m_stream, name);
}

void PdfPainter::sc_Operator(const cspan<double>& components)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_sc(m_stream, components);
}

void PdfPainter::SC_Operator(const cspan<double>& components)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_SC(m_stream, components);
}

void PdfPainter::scn_Operator(const cspan<double>& components)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_scn(m_stream, components);
}

void PdfPainter::SCN_Operator(const cspan<double>& components)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_SCN(m_stream, components);
}

void PdfPainter::scn_Operator(const cspan<double>& components, const string_view& patternName)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_scn(m_stream, components, patternName);
}

void PdfPainter::SCN_Operator(const cspan<double>& components, const string_view& patternName)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_SCN(m_stream, components, patternName);
}

void PdfPainter::scn_Operator(const string_view& patternName)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_scn(m_stream, patternName);
}

void PdfPainter::SCN_Operator(const string_view& patternName)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_SCN(m_stream, patternName);
}

void PdfPainter::G_Operator(double gray)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_G(m_stream, gray);
}

void PdfPainter::g_Operator(double gray)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_g(m_stream, gray);
}

void PdfPainter::RG_Operator(double red, double green, double blue)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_RG(m_stream, red, green, blue);
}

void PdfPainter::rg_Operator(double red, double green, double blue)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_rg(m_stream, red, green, blue);
}

void PdfPainter::K_Operator(double cyan, double magenta, double yellow, double black)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_K(m_stream, cyan, magenta, yellow, black);
}

void PdfPainter::k_Operator(double cyan, double magenta, double yellow, double black)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    PoDoFo::WriteOperator_k(m_stream, cyan, magenta, yellow, black);
}

void PdfPainter::sh_Operator(const string_view& shadingDictName)
{
    checkStream();
    checkStatus(StatusDefault);
    PoDoFo::WriteOperator_sh(m_stream, shadingDictName);
}

void PdfPainter::BX_Operator()
{
    checkStream();
    checkStatus(StatusDefault);
    PoDoFo::WriteOperator_BX(m_stream);
    m_painterStatus = StatusExtension;
}

void PdfPainter::EX_Operator()
{
    checkStream();
    checkStatus(StatusExtension);
    PoDoFo::WriteOperator_EX(m_stream);
    m_painterStatus = StatusDefault;
}

void PdfPainter::v_Operator(double cx, double cy, double x, double y)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    checkPathOpened();
    PoDoFo::WriteOperator_v(m_stream, cx, cy, x, y);
    m_StateStack.Current->CurrentPoint = Vector2(x, y);
}

void PdfPainter::y_Operator(double cx, double cy, double x, double y)
{
    checkStream();
    checkStatus(StatusDefault | StatusTextObject);
    checkPathOpened();
    PoDoFo::WriteOperator_y(m_stream, cx, cy, x, y);
    m_StateStack.Current->CurrentPoint = Vector2(x, y);
}

void PdfPainter::Extension_Operator(const string_view& opName, const cspan<PdfVariant>& operands)
{
    checkStream();
    checkStatus(StatusExtension);
    PoDoFo::WriteOperator_Extension(m_stream, opName, operands);
}
