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
class PODOFO_API PdfFontCID : public PdfFont
{
    friend class PdfFont;
    friend class PdfFontCIDTrueType;
    friend class PdfFontCIDCFF;

private:
    PdfFontCID(PdfDocument& doc, PdfFontType type,
        PdfFontMetricsConstPtr&& metrics, const PdfEncoding& encoding);

public:
    bool SupportsSubsetting() const override;

protected:
    void embedFont() override;
    void embedFontSubset() override;
    PdfObject* getDescendantFontObject() override;
    void createWidths(PdfDictionary& fontDict, const cspan<PdfCharGIDInfo>& infos);

protected:
    virtual void embedFontFileSubset(const std::vector<PdfCharGIDInfo>& subsetInfos,
        const PdfCIDSystemInfo& cidInfo) = 0;
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
