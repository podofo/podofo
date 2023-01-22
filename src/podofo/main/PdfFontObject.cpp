/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfFontObject.h"
#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

// All Standard14 fonts have glyphs that start with a
// white space (code 0x20, or 32)
static constexpr unsigned DEFAULT_STD14_FIRSTCHAR = 32;

PdfFontObject::PdfFontObject(PdfObject& obj, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding) :
    PdfFont(obj, metrics, encoding) { }

unique_ptr<PdfFontObject> PdfFontObject::Create(PdfObject& obj, PdfObject& descendantObj,
    const PdfFontMetricsConstPtr& metrics, const PdfEncoding& encoding)
{
    (void)descendantObj;
    // TODO: MAke a virtual GetDescendantFontObject()
    return unique_ptr<PdfFontObject>(new PdfFontObject(obj, metrics, encoding));
}

unique_ptr<PdfFontObject> PdfFontObject::Create(PdfObject& obj, const PdfFontMetricsConstPtr& metrics, const PdfEncoding& encoding)
{
    return unique_ptr<PdfFontObject>(new PdfFontObject(obj, metrics, encoding));
}

bool PdfFontObject::tryMapCIDToGID(unsigned cid, unsigned& gid) const
{
    if (m_Metrics->IsStandard14FontMetrics() && !m_Encoding->HasParsedLimits())
    {
        gid = cid - DEFAULT_STD14_FIRSTCHAR;
    }
    else
    {
        if (m_Encoding->IsSimpleEncoding())
        {
            // We just convert to a GID using /FirstChar
            gid = cid - m_Encoding->GetFirstChar().Code;
        }
        else
        {
            // Else we assume identity
            gid = cid;
        }
    }

    return true;
}

bool PdfFontObject::IsObjectLoaded() const
{
    return true;
}

PdfFontType PdfFontObject::GetType() const
{
    // TODO Just read the object from the object
    return PdfFontType::Unknown;
}
