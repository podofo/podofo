/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PdfExtension.h"

using namespace std;
using namespace PoDoFo;

PdfExtension::PdfExtension(const std::string_view& ns, PdfVersion baseVersion, int64_t level) :
    m_Ns(ns), m_BaseVersion(baseVersion), m_Level(level) { }
