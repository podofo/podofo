/**
 * SPDX-FileCopyrightText: (C) 2009 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "DeleteOperation.h"

#include <podofo/podofo.h>
#include <sstream>

using namespace std;
using namespace PoDoFo;

DeleteOperation::DeleteOperation(unsigned pageIndex)
    : m_pageIndex(pageIndex)
{
}

void DeleteOperation::Perform(PdfDocument& doc)
{
    doc.GetPages().RemovePageAt(m_pageIndex);
}

string DeleteOperation::ToString() const
{
    ostringstream oss;
    oss << "Deleting page: " << m_pageIndex << "." << std::endl;
    return oss.str();
}
