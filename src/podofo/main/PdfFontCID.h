/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_CID_H
#define PDF_FONT_CID_H

#include "PdfFont.h"

namespace PoDoFo {

/** A PdfFont that represents a CID-keyed font
 */
class PdfFontCID : public PdfFont
{
    friend class PdfFont;

protected:
    PdfFontCID(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding);

public:
    bool SupportsSubsetting() const override;

protected:
    void embedFont() override;
    PdfObject* getDescendantFontObject() override;
    void createWidths(PdfDictionary& fontDict, const CIDToGIDMap& glyphWidths);
    static CIDToGIDMap getCIDToGIDMapSubset(const UsedGIDsMap& usedGIDs);

private:
    CIDToGIDMap getIdentityCIDToGIDMap();

protected:
    void initImported() override;

protected:
    PdfObject& GetDescendantFont() { return *m_descendantFont; }
    PdfObject& GetDescriptor() { return *m_descriptor; }

private:
    PdfObject* m_descendantFont;
    PdfObject* m_descriptor;
};

};

#endif // PDF_FONT_CID_H
