/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PDFINFO_H
#define PDFINFO_H

#include <ostream>
#include <podofo/podofo.h>

class PdfInfoHelper
{
public:
    PdfInfoHelper(const std::string& filepath);
    virtual ~PdfInfoHelper();

    void OutputDocumentInfo(std::ostream& outStream);
    void OutputInfoDict(std::ostream& outStream);
    void OutputPageInfo(std::ostream& outStream);
    void OutputOutlines(std::ostream& outStream, PoDoFo::PdfOutlineItem* first = nullptr, int level = 0);
    void OutputNames(std::ostream& outStream);

private:
    PoDoFo::PdfMemDocument* m_doc;

    void OutputOneName(std::ostream& outStream, PoDoFo::PdfNameTrees& names,
        PoDoFo::PdfKnownNameTree treeName, const std::string_view& title);
    std::string GuessFormat();
};

#endif	// PDFINFO_H
