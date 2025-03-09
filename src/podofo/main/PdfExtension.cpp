/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PdfExtension.h"

using namespace std;
using namespace PoDoFo;

PdfExtension::PdfExtension(const PdfName& ns, int64_t level, PdfVersion baseVersion,
        nullable<const PdfString&> url, nullable<const PdfString&> extensionRevision) :
    m_BaseVersion(baseVersion),
    m_Namespace(ns),
    m_Level(level)
{
    if (url != nullptr)
        m_Url = *url;

    if (extensionRevision != nullptr)
        m_ExtensionRevision = *extensionRevision;
}
