/**
 * SPDX-FileCopyrightText: (C) 2009 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "MoveOperation.h"

#include <sstream>

using namespace std;
using namespace PoDoFo;

MoveOperation::MoveOperation(int nFrom, int nTo)
    : m_fromIndex(nFrom), m_toIndex(nTo)
{
}

void MoveOperation::Perform(PdfDocument& doc)
{
    auto& pages = doc.GetPages();
    auto& page = pages.GetPageAt(m_fromIndex);
    page.MoveTo(m_toIndex);
}

string MoveOperation::ToString() const
{
    ostringstream oss;
    oss << "Moving page " << m_fromIndex << " to " << m_toIndex << "." << std::endl;
    return oss.str();
}

